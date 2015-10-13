//
// Copyright 2015 Ettus Research
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "ad936x_manager.hpp"
#include <uhd/utils/msg.hpp>
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include <boost/thread/thread.hpp>

using namespace uhd;
using namespace uhd::usrp;

/****************************************************************************
 * Default values
 ***************************************************************************/
const double   ad936x_manager::DEFAULT_GAIN = 0;
const double   ad936x_manager::DEFAULT_BANDWIDTH = 56e6;
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
            _rx_frontends.push_back(str(boost::format("RX%d") % i));
            _tx_frontends.push_back(str(boost::format("TX%d") % i));
        }
    }

    /************************************************************************
     * API Calls
     ***********************************************************************/
    void init_codec()
    {
        BOOST_FOREACH(const std::string &rx_fe, _rx_frontends) {
            _codec_ctrl->set_gain(rx_fe, DEFAULT_GAIN);
            _codec_ctrl->set_bw_filter(rx_fe, DEFAULT_BANDWIDTH);
            _codec_ctrl->tune(rx_fe, DEFAULT_FREQ);
            _codec_ctrl->set_dc_offset_auto(rx_fe, DEFAULT_AUTO_DC_OFFSET);
            _codec_ctrl->set_iq_balance_auto(rx_fe, DEFAULT_AUTO_IQ_BALANCE);
            _codec_ctrl->set_agc(rx_fe, DEFAULT_AGC_ENABLE);
        }
        BOOST_FOREACH(const std::string &tx_fe, _tx_frontends) {
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
            wb_iface::sptr iface,
            wb_iface::wb_addr_type codec_idle_addr,
            wb_iface::wb_addr_type codec_readback_addr
    ) {
        // Put AD936x in loopback mode
        _codec_ctrl->data_port_loopback(true);
        UHD_MSG(status) << "Performing CODEC loopback test... " << std::flush;
        UHD_ASSERT_THROW(bool(iface));
        size_t hash = size_t(time(NULL));

        // Allow some time for AD936x to enter loopback mode.
        // There is no clear statement in the documentation of how long it takes,
        // but UG-570 does say to "allow six ADC_CLK/64 clock cycles of flush time"
        // when leaving the TX or RX states.  That works out to ~75us at the
        // minimum clock rate of 5 MHz, which lines up with test results.
        // Sleeping 1ms is far more than enough.
        boost::this_thread::sleep(boost::posix_time::milliseconds(1));

        for (size_t i = 0; i < 100; i++)
        {
            // Create test word
            boost::hash_combine(hash, i);
            const boost::uint32_t word32 = boost::uint32_t(hash) & 0xfff0fff0;

            // Write test word to codec_idle idle register (on TX side)
            iface->poke32(codec_idle_addr, word32);

            // Read back values - TX is lower 32-bits and RX is upper 32-bits
            const boost::uint64_t rb_word64 = iface->peek64(codec_readback_addr);
            const boost::uint32_t rb_tx = boost::uint32_t(rb_word64 >> 32);
            const boost::uint32_t rb_rx = boost::uint32_t(rb_word64 & 0xffffffff);

            // Compare TX and RX values to test word
            bool test_fail = word32 != rb_tx or word32 != rb_rx;
            if(test_fail)
            {
                UHD_MSG(status) << "fail" << std::endl;
                throw uhd::runtime_error("CODEC loopback test failed.");
            }
        }
        UHD_MSG(status) << "pass" << std::endl;

        // Zero out the idle data.
        iface->poke32(codec_idle_addr, 0);

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
        if (rate > _codec_ctrl->get_bw_filter_range(dir).stop()) {
            UHD_MSG(warning)
                << "Selected " << dir << " bandwidth (" << (rate/1e6) << " MHz) exceeds\n"
                << "analog frontend filter bandwidth (" << (_codec_ctrl->get_bw_filter_range(dir).stop()/1e6) << " MHz)."
                << std::endl;
            return false;
        }
        return true;
    }

    void populate_frontend_subtree(uhd::property_tree::sptr subtree, const std::string &key, uhd::direction_t dir)
    {
        subtree->create<std::string>("name").set("FE-"+key);

        // Sensors
        subtree->create<sensor_value_t>("sensors/temp")
            .publish(boost::bind(&ad9361_ctrl::get_temperature, _codec_ctrl))
        ;
        if (dir == RX_DIRECTION) {
            subtree->create<sensor_value_t>("sensors/rssi")
                .publish(boost::bind(&ad9361_ctrl::get_rssi, _codec_ctrl, key))
            ;
        }

        // Gains
        BOOST_FOREACH(const std::string &name, ad9361_ctrl::get_gain_names(key))
        {
            subtree->create<meta_range_t>(uhd::fs_path("gains") / name / "range")
                .set(ad9361_ctrl::get_gain_range(key));
            subtree->create<double>(uhd::fs_path("gains") / name / "value")
                .set(ad936x_manager::DEFAULT_GAIN)
                .coerce(boost::bind(&ad9361_ctrl::set_gain, _codec_ctrl, key, _1))
            ;
        }

        // FE Settings
        subtree->create<std::string>("connection").set("IQ");
        subtree->create<bool>("enabled").set(true);
        subtree->create<bool>("use_lo_offset").set(false);

        // Analog Bandwidths
        subtree->create<double>("bandwidth/value")
            .set(ad936x_manager::DEFAULT_BANDWIDTH)
            .coerce(boost::bind(&ad9361_ctrl::set_bw_filter, _codec_ctrl, key, _1))
        ;
        subtree->create<meta_range_t>("bandwidth/range")
            .publish(boost::bind(&ad9361_ctrl::get_bw_filter_range, key))
        ;

        // LO Tuning
        subtree->create<meta_range_t>("freq/range")
            .publish(boost::bind(&ad9361_ctrl::get_rf_freq_range))
        ;
        subtree->create<double>("freq/value")
            .publish(boost::bind(&ad9361_ctrl::get_freq, _codec_ctrl, key))
            .coerce(boost::bind(&ad9361_ctrl::tune, _codec_ctrl, key, _1))
        ;

        // Frontend corrections
        if(dir == RX_DIRECTION)
        {
            subtree->create<bool>("dc_offset/enable" )
                .set(ad936x_manager::DEFAULT_AUTO_DC_OFFSET)
                .subscribe(boost::bind(&ad9361_ctrl::set_dc_offset_auto, _codec_ctrl, key, _1))
            ;
            subtree->create<bool>("iq_balance/enable" )
                .set(ad936x_manager::DEFAULT_AUTO_IQ_BALANCE)
                .subscribe(boost::bind(&ad9361_ctrl::set_iq_balance_auto, _codec_ctrl, key, _1))
            ;

            // AGC setup
            const std::list<std::string> mode_strings = boost::assign::list_of("slow")("fast");
            subtree->create<bool>("gain/agc/enable")
                .set(DEFAULT_AGC_ENABLE)
                .subscribe(boost::bind((&ad9361_ctrl::set_agc), _codec_ctrl, key, _1))
            ;
            subtree->create<std::string>("gain/agc/mode/value")
                .subscribe(boost::bind((&ad9361_ctrl::set_agc_mode), _codec_ctrl, key, _1)).set(mode_strings.front())
            ;
            subtree->create< std::list<std::string> >("gain/agc/mode/options")
                .set(mode_strings)
            ;
        }

        // Frontend filters
        BOOST_FOREACH(const std::string &filter_name, _codec_ctrl->get_filter_names(key)) {
            subtree->create<filter_info_base::sptr>(uhd::fs_path("filters") / filter_name / "value" )
                .publish(boost::bind(&ad9361_ctrl::get_filter, _codec_ctrl, key, filter_name))
                .subscribe(boost::bind(&ad9361_ctrl::set_filter, _codec_ctrl, key, filter_name, _1));
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
}; /* class ad936x_manager_impl */

ad936x_manager::sptr ad936x_manager::make(
        const ad9361_ctrl::sptr &codec_ctrl,
        const size_t n_frontends
) {
    return sptr(
        new ad936x_manager_impl(codec_ctrl, n_frontends)
    );
}

