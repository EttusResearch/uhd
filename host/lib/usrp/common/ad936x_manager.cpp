//
// Copyright 2015 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/ad936x_manager.hpp>
#include <uhd/utils/log.hpp>
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <chrono>
#include <thread>

using namespace uhd;
using namespace uhd::usrp;

/****************************************************************************
 * Default values
 ***************************************************************************/
const double   ad936x_manager::DEFAULT_GAIN = 0;
const double   ad936x_manager::DEFAULT_BANDWIDTH = ad9361_device_t::AD9361_MAX_BW;
const double   ad936x_manager::DEFAULT_TICK_RATE = 16e6;
const double   ad936x_manager::DEFAULT_FREQ = 100e6; // Hz
const uint32_t ad936x_manager::DEFAULT_DECIM  = 128;
const uint32_t ad936x_manager::DEFAULT_INTERP = 128;
const bool     ad936x_manager::DEFAULT_AUTO_DC_OFFSET = true;
const bool     ad936x_manager::DEFAULT_AUTO_IQ_BALANCE = true;
const bool     ad936x_manager::DEFAULT_AGC_ENABLE = false;

class ad936x_manager_impl : public ad936x_manager
{
public:
    /************************************************************************
     * Structor
     ***********************************************************************/
    ad936x_manager_impl(
            const ad9361_ctrl::sptr &codec_ctrl,
            const size_t n_frontends
    ) : _codec_ctrl(codec_ctrl),
        _n_frontends(n_frontends)
    {
        if (_n_frontends < 1 or _n_frontends > 2) {
            throw uhd::runtime_error(str(
                boost::format("AD936x device can only have either 1 or 2 frontends, not %d.")
                % _n_frontends
            ));
        }
        for (size_t i = 1; i <= _n_frontends; i++) {
            const std::string rx_fe_str = str(boost::format("RX%d") % i);
            const std::string tx_fe_str = str(boost::format("TX%d") % i);
            _rx_frontends.push_back(rx_fe_str);
            _tx_frontends.push_back(tx_fe_str);
            _bw[rx_fe_str] = 0.0;
            _bw[tx_fe_str] = 0.0;
        }
    }

    /************************************************************************
     * API Calls
     ***********************************************************************/
    void init_codec()
    {
        for (const std::string &rx_fe : _rx_frontends) {
            _codec_ctrl->set_gain(rx_fe, DEFAULT_GAIN);
            _codec_ctrl->set_bw_filter(rx_fe, DEFAULT_BANDWIDTH);
            _codec_ctrl->tune(rx_fe, DEFAULT_FREQ);
            _codec_ctrl->set_dc_offset_auto(rx_fe, DEFAULT_AUTO_DC_OFFSET);
            _codec_ctrl->set_iq_balance_auto(rx_fe, DEFAULT_AUTO_IQ_BALANCE);
            _codec_ctrl->set_agc(rx_fe, DEFAULT_AGC_ENABLE);
        }
        for (const std::string &tx_fe : _tx_frontends) {
            _codec_ctrl->set_gain(tx_fe, DEFAULT_GAIN);
            _codec_ctrl->set_bw_filter(tx_fe, DEFAULT_BANDWIDTH);
            _codec_ctrl->tune(tx_fe, DEFAULT_FREQ);
        }
    }


    //
    // loopback_self_test checks the integrity of the FPGA->AD936x->FPGA sample interface.
    // The AD936x is put in loopback mode that sends the TX data unchanged to the RX side.
    // A test value is written to the codec_idle register in the TX side of the radio.
    // The readback register is then used to capture the values on the TX and RX sides
    // simultaneously for comparison. It is a reasonably effective test for AC timing
    // since I/Q Ch0/Ch1 alternate over the same wires. Note, however, that it uses
    // whatever timing is configured at the time the test is called rather than select
    // worst case conditions to stress the interface.
    //
    void loopback_self_test(
          std::function<void(uint32_t)> poker_functor,
          std::function<uint64_t()> peeker_functor
    ) {
        // Put AD936x in loopback mode
        _codec_ctrl->data_port_loopback(true);
        UHD_LOGGER_DEBUG("AD936X") << "Performing CODEC loopback test... ";
        size_t hash = size_t(time(NULL));

        // Allow some time for AD936x to enter loopback mode.
        // There is no clear statement in the documentation of how long it takes,
        // but UG-570 does say to "allow six ADC_CLK/64 clock cycles of flush time"
        // when leaving the TX or RX states.  That works out to ~75us at the
        // minimum clock rate of 5 MHz, which lines up with test results.
        // Sleeping 1ms is far more than enough.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        constexpr size_t NUM_LOOPBACK_ITERS = 100;
        for (size_t i = 0; i < NUM_LOOPBACK_ITERS; i++) {
            // Create test word
            boost::hash_combine(hash, i);
            const uint32_t word32 = uint32_t(hash) & 0xfff0fff0;

            // Write test word to codec_idle idle register (on TX side)
            poker_functor(word32);

            // Read back values
            const uint64_t rb_word64 = peeker_functor();
            const uint32_t rb_tx = uint32_t(rb_word64 >> 32);
            const uint32_t rb_rx = uint32_t(rb_word64 & 0xffffffff);

            // Compare TX and RX values to test word
            const bool test_fail = word32 != rb_tx or word32 != rb_rx;
            if (test_fail) {
                UHD_LOGGER_ERROR("AD936X")
                  << "CODEC loopback test failed! "
                  << boost::format("Expected: 0x%08X Received (TX/RX): 0x%08X/0x%08X")
                      % word32 % rb_tx % rb_rx;
                throw uhd::runtime_error("CODEC loopback test failed.");
            }
        }
        UHD_LOGGER_DEBUG("AD936X") << "CODEC loopback test passed.";

        // Zero out the idle data.
        poker_functor(0);

        // Take AD936x out of loopback mode
        _codec_ctrl->data_port_loopback(false);
    }


    double get_auto_tick_rate(
            const double lcm_rate,
            size_t num_chans
    ) {
        UHD_ASSERT_THROW(num_chans >= 1 and num_chans <= _n_frontends);
        const uhd::meta_range_t rate_range = _codec_ctrl->get_clock_rate_range();
        const double min_tick_rate = rate_range.start();
        const double max_tick_rate = rate_range.stop() / num_chans;

        // Check if the requested rate is within available limits:
        if (uhd::math::fp_compare::fp_compare_delta<double>(lcm_rate, uhd::math::FREQ_COMPARISON_DELTA_HZ) >
            uhd::math::fp_compare::fp_compare_delta<double>(max_tick_rate, uhd::math::FREQ_COMPARISON_DELTA_HZ)) {
            throw uhd::value_error(str(
                    boost::format("[ad936x_manager] Cannot get determine a tick rate if sampling rate exceeds maximum tick rate (%f > %f)")
                    % lcm_rate % max_tick_rate
            ));
        }

        // **** Choose the new rate ****
        // Rules for choosing the tick rate:
        // Choose a rate that is a power of 2 larger than the sampling rate,
        // but at least 4. Cannot exceed the max tick rate, of course, but must
        // be larger than the minimum tick rate.
        // An equation that does all that is:
        //
        // f_auto = r * 2^floor(log2(f_max/r))
        //        = lcm_rate * multiplier
        //
        // where r is the base rate and f_max is the maximum tick rate. The case
        // where floor() yields 1 must be caught.
        // We use shifts here instead of 2^x because exp2() is not available in all compilers,
        // also this guarantees no rounding issues. The type cast to int32_t serves as floor():
        int32_t multiplier = (1 << int32_t(uhd::math::log2(max_tick_rate / lcm_rate)));
        if (multiplier == 2 and lcm_rate >= min_tick_rate) {
            // Don't bother (see above)
            multiplier = 1;
        }
        const double new_rate = lcm_rate * multiplier;
        UHD_ASSERT_THROW(
            uhd::math::fp_compare::fp_compare_delta<double>(new_rate, uhd::math::FREQ_COMPARISON_DELTA_HZ) >=
            uhd::math::fp_compare::fp_compare_delta<double>(min_tick_rate, uhd::math::FREQ_COMPARISON_DELTA_HZ)
        );
        UHD_ASSERT_THROW(
            uhd::math::fp_compare::fp_compare_delta<double>(new_rate, uhd::math::FREQ_COMPARISON_DELTA_HZ) <=
            uhd::math::fp_compare::fp_compare_delta<double>(max_tick_rate, uhd::math::FREQ_COMPARISON_DELTA_HZ)
        );

        return new_rate;
    }

    bool check_bandwidth(double rate, const std::string dir)
    {
        double bw = _bw[dir == "Rx" ? "RX1" : "TX1"];
        if (bw == 0.) //0 indicates bandwidth is default value.
        {
            double max_bw = ad9361_device_t::AD9361_MAX_BW;
            double min_bw = ad9361_device_t::AD9361_MIN_BW;
            if (rate > max_bw)
            {
            UHD_LOGGER_WARNING("AD936X")
                << "Selected " << dir << " sample rate (" << (rate/1e6) << " MHz) is greater than\n"
                << "analog frontend filter bandwidth (" << (max_bw/1e6) << " MHz)."
                ;
            }
            else if (rate < min_bw)
            {
            UHD_LOGGER_WARNING("AD936X")
                << "Selected " << dir << " sample rate (" << (rate/1e6) << " MHz) is less than\n"
                << "analog frontend filter bandwidth (" << (min_bw/1e6) << " MHz)."
                ;
            }
        }
        return (rate <= bw);
    }

    void populate_frontend_subtree(
        uhd::property_tree::sptr subtree,
        const std::string &key,
        uhd::direction_t dir
    ) {
        subtree->create<std::string>("name").set("FE-"+key);

        // Sensors
        subtree->create<sensor_value_t>("sensors/temp")
            .set_publisher([this](){
                return this->_codec_ctrl->get_temperature();
            })
        ;
        if (dir == RX_DIRECTION) {
            subtree->create<sensor_value_t>("sensors/rssi")
                .set_publisher([this, key](){
                    return this->_codec_ctrl->get_rssi(key);
                })
            ;
        }

        // Gains
        for (const std::string &name : ad9361_ctrl::get_gain_names(key)) {
            subtree->create<meta_range_t>(uhd::fs_path("gains") / name / "range")
                .set(ad9361_ctrl::get_gain_range(key));
            subtree->create<double>(uhd::fs_path("gains") / name / "value")
                .set(ad936x_manager::DEFAULT_GAIN)
                .set_coercer([this, key](const double gain){
                    return this->_codec_ctrl->set_gain(key, gain);
                })
            ;
        }

        // FE Settings
        subtree->create<std::string>("connection").set("IQ");
        subtree->create<bool>("enabled").set(true);
        subtree->create<bool>("use_lo_offset").set(false);

        // Analog Bandwidths
        subtree->create<double>("bandwidth/value")
            .set(DEFAULT_BANDWIDTH)
            .set_coercer([this,key](double bw) {
                    return set_bw_filter(key, bw);
                }
            )
        ;
        subtree->create<meta_range_t>("bandwidth/range")
            .set_publisher([key]() {
                    return ad9361_ctrl::get_bw_filter_range();
                }
            )
        ;

        // LO Tuning
        subtree->create<meta_range_t>("freq/range")
            .set_publisher([](){
                return ad9361_ctrl::get_rf_freq_range();
            })
        ;
        subtree->create<double>("freq/value")
            .set_publisher([this, key](){
                return this->_codec_ctrl->get_freq(key);
            })
            .set_coercer([this, key](const double freq){
                return this->_codec_ctrl->tune(key, freq);
            })
        ;

        // Frontend corrections
        if(dir == RX_DIRECTION)
        {
            subtree->create<bool>("dc_offset/enable" )
                .set(ad936x_manager::DEFAULT_AUTO_DC_OFFSET)
                .add_coerced_subscriber([this, key](const bool enable){
                    this->_codec_ctrl->set_dc_offset_auto(key, enable);
                })
            ;
            subtree->create<bool>("iq_balance/enable" )
                .set(ad936x_manager::DEFAULT_AUTO_IQ_BALANCE)
                .add_coerced_subscriber([this, key](const bool enable){
                   this->_codec_ctrl->set_iq_balance_auto(key, enable);
                })
            ;

            // AGC setup
            const std::list<std::string> mode_strings{"slow", "fast"};
            subtree->create<bool>("gain/agc/enable")
                .set(DEFAULT_AGC_ENABLE)
                .add_coerced_subscriber([this, key](const bool enable){
                    this->_codec_ctrl->set_agc(key, enable);
                })
            ;
            subtree->create<std::string>("gain/agc/mode/value")
                .add_coerced_subscriber([this, key](const std::string& value){
                    this->_codec_ctrl->set_agc_mode(key, value);
                })
                .set(mode_strings.front())
            ;
            subtree->create<std::list<std::string>>("gain/agc/mode/options")
                .set(mode_strings)
            ;
        }

        // Frontend filters
        for (const auto &filter_name : _codec_ctrl->get_filter_names(key)) {
            subtree->create<filter_info_base::sptr>(uhd::fs_path("filters") / filter_name / "value")
                .set_publisher([this, key, filter_name](){
                    return this->_codec_ctrl->get_filter(key, filter_name);
                })
                .add_coerced_subscriber([this, key, filter_name](filter_info_base::sptr filter_info){
                    this->_codec_ctrl->set_filter(key, filter_name, filter_info);
                })
            ;
        }
    }

private:
    //! Store a pointer to an actual AD936x control object
    ad9361_ctrl::sptr _codec_ctrl;

    //! Do we have 1 or 2 frontends?
    const size_t _n_frontends;

    //! List of valid RX frontend names (RX1, RX2)
    std::vector<std::string> _rx_frontends;
    //! List of valid TX frontend names (TX1, TX2)
    std::vector<std::string> _tx_frontends;

    //! Current bandwidths
    std::map<std::string,double> _bw;

    //! Function to set bandwidth so it is tracked here
    double set_bw_filter(const std::string& which, const double bw)
    {
        double actual_bw = _codec_ctrl->set_bw_filter(which, bw);
        _bw[which] = actual_bw;
        return actual_bw;
    }

}; /* class ad936x_manager_impl */

ad936x_manager::sptr ad936x_manager::make(
        const ad9361_ctrl::sptr &codec_ctrl,
        const size_t n_frontends
) {
    return boost::make_shared<ad936x_manager_impl>(codec_ctrl, n_frontends);
}

