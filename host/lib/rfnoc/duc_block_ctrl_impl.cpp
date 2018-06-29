//
// Copyright 2016-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/duc_block_ctrl.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/convert.hpp>
#include <uhd/types/ranges.hpp>
#include <uhdlib/utils/compat_check.hpp>
#include <uhdlib/utils/math.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <uhdlib/usrp/cores/dsp_core_utils.hpp>
#include <boost/math/special_functions/round.hpp>
#include <cmath>

using namespace uhd::rfnoc;

class duc_block_ctrl_impl : public duc_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(duc_block_ctrl)
        , _fpga_compat(user_reg_read64(RB_REG_COMPAT_NUM))
        , _num_halfbands(uhd::narrow_cast<size_t>(
                    user_reg_read64(RB_REG_NUM_HALFBANDS)))
        , _cic_max_interp(uhd::narrow_cast<size_t>(
                    user_reg_read64(RB_REG_CIC_MAX_INTERP)))
    {
        UHD_LOG_DEBUG(unique_id(),
            "Loading DUC with " << get_num_halfbands() << " halfbands and "
            "max CIC interpolation " << get_cic_max_interp()
        );
        uhd::assert_fpga_compat(
            MAJOR_COMP, MINOR_COMP,
            _fpga_compat,
            "DUC", "DUC",
            false /* Let it slide if minors mismatch */
        );

        // Argument/prop tree hooks
        for (size_t chan = 0; chan < get_input_ports().size(); chan++) {
            const double default_freq = get_arg<double>("freq", chan);
            _tree->access<double>(get_arg_path("freq/value", chan))
                .set_coercer([this, chan](const double value){
                    return this->set_freq(value, chan);
                })
                .set(default_freq);
            ;

            const double default_input_rate =
                get_arg<double>("input_rate", chan);
            _tree->access<double>(get_arg_path("input_rate/value", chan))
                .set_coercer([this, chan](const double value){
                    return this->set_input_rate(value, chan);
                })
                .set(default_input_rate)
            ;
            _tree->access<double>(get_arg_path("output_rate/value", chan))
                .add_coerced_subscriber([this, chan](const double rate){
                    this->set_output_rate(rate, chan);
                })
            ;

            // Legacy properties (for backward compat w/ multi_usrp)
            const uhd::fs_path dsp_base_path = _root_path / "legacy_api" / chan;
            // Legacy properties
            _tree->create<double>(dsp_base_path / "rate/value")
                .set_coercer([this, chan](const double value){
                    return this->_tree->access<double>(
                        this->get_arg_path("input_rate/value", chan)
                    ).set(value).get();
                })
                .set_publisher([this, chan](){
                    return this->_tree->access<double>(
                        this->get_arg_path("input_rate/value", chan)
                    ).get();
                })
            ;
            _tree->create<uhd::meta_range_t>(dsp_base_path / "rate/range")
                .set_publisher([this](){
                    return get_input_rates();
                })
            ;
            _tree->create<double>(dsp_base_path / "freq/value")
                .set_coercer([this, chan](const double value){
                    return this->_tree->access<double>(
                        this->get_arg_path("freq/value", chan)
                    ).set(value).get();
                })
                .set_publisher([this, chan](){
                    return this->_tree->access<double>(
                        this->get_arg_path("freq/value", chan)
                    ).get();
                })
            ;
            _tree->create<uhd::meta_range_t>(dsp_base_path / "freq/range")
                .set_publisher([this](){
                    return get_freq_range();
                })
            ;
            _tree->access<uhd::time_spec_t>("time/cmd")
                .add_coerced_subscriber([this, chan](const uhd::time_spec_t time_spec){
                    this->set_command_time(time_spec, chan);
                })
            ;
            if (_tree->exists("tick_rate")) {
                const double tick_rate =
                    _tree->access<double>("tick_rate").get();
                set_command_tick_rate(tick_rate, chan);
                _tree->access<double>("tick_rate")
                    .add_coerced_subscriber([this, chan](const double rate){
                        this->set_command_tick_rate(rate, chan);
                    })
                ;
            }

            // Rate 1:1 by default
            sr_write("N", 1, chan);
            sr_write("M", 1, chan);
            sr_write("CONFIG", 1, chan); // Enable clear EOB
        }
    } // end ctor

    virtual ~duc_block_ctrl_impl() {}

    double get_input_scale_factor(size_t port=ANY_PORT)
    {
        port = (port == ANY_PORT) ? 0 : port;
        if (not (_tx_streamer_active.count(port) and _tx_streamer_active.at(port))) {
            return SCALE_UNDEFINED;
        }
        return get_arg<double>("scalar_correction", port);
    }

    double get_input_samp_rate(size_t port=ANY_PORT)
    {
        port = (port == ANY_PORT) ? 0 : port;

        // Wait, what? If this seems out of place to you, you're right. However,
        // we need a function call that is called when the graph is complete,
        // but streaming is not yet set up.
        if (_tree->exists("tick_rate")) {
            const double tick_rate = _tree->access<double>("tick_rate").get();
            set_command_tick_rate(tick_rate, port);
        }

        if (not (_tx_streamer_active.count(port) and _tx_streamer_active.at(port))) {
            return RATE_UNDEFINED;
        }
        return get_arg<double>("input_rate", port);
    }

    double get_output_samp_rate(size_t port=ANY_PORT)
    {
        port = (port == ANY_PORT) ? 0 : port;
        if (not (_tx_streamer_active.count(port) and _tx_streamer_active.at(port))) {
            return RATE_UNDEFINED;
        }
        return get_arg<double>("output_rate", port == ANY_PORT ? 0 : port);
    }

    void issue_stream_cmd(
            const uhd::stream_cmd_t &stream_cmd_,
            const size_t chan
    ) {
        UHD_RFNOC_BLOCK_TRACE() << "duc_block_ctrl_base::issue_stream_cmd()" ;

        uhd::stream_cmd_t stream_cmd = stream_cmd_;
        if (stream_cmd.stream_mode == uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE or
            stream_cmd.stream_mode == uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE) {
            size_t interpolation = get_arg<double>("output_rate", chan) / get_arg<double>("input_rate", chan);
            stream_cmd.num_samps *= interpolation;
        }

        for(const node_ctrl_base::node_map_pair_t upstream_node:  list_upstream_nodes()) {
            source_node_ctrl::sptr this_upstream_block_ctrl =
                boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node.second.lock());
            this_upstream_block_ctrl->issue_stream_cmd(stream_cmd, chan);
        }
    }

private:

    static constexpr size_t MAJOR_COMP = 2;
    static constexpr size_t MINOR_COMP = 0;
    static constexpr size_t RB_REG_COMPAT_NUM = 0;
    static constexpr size_t RB_REG_NUM_HALFBANDS = 1;
    static constexpr size_t RB_REG_CIC_MAX_INTERP = 2;

    const uint64_t _fpga_compat;
    const size_t _num_halfbands;
    const size_t _cic_max_interp;

    //! Set the DDS frequency shift the signal to \p requested_freq
    double set_freq(const double requested_freq, const size_t chan)
    {
        const double output_rate = get_arg<double>("output_rate");
        double actual_freq;
        int32_t freq_word;
        get_freq_and_freq_word(requested_freq, output_rate, actual_freq, freq_word);
        sr_write("DDS_FREQ", uint32_t(freq_word), chan);
        return actual_freq;
    }

    //! Return a range of valid frequencies the DDS can tune to
    uhd::meta_range_t get_freq_range(void)
    {
        const double output_rate = get_arg<double>("output_rate");
        return uhd::meta_range_t(
                -output_rate/2,
                +output_rate/2,
                output_rate/std::pow(2.0, 32)
        );
    }

    uhd::meta_range_t get_input_rates(void)
    {
        uhd::meta_range_t range;
        const double output_rate = get_arg<double>("output_rate");
        for (int hb = _num_halfbands; hb >= 0; hb--) {
            const size_t interp_offset = _cic_max_interp<<(hb-1);
            for(size_t interp = _cic_max_interp; interp > 0; interp--) {
                const size_t hb_cic_interp =  interp*(1<<hb);
                if(hb == 0 || hb_cic_interp > interp_offset) {
                    range.push_back(uhd::range_t(output_rate/hb_cic_interp));
                }
            }
        }
        return range;
    }

    double set_input_rate(const int requested_rate, const size_t chan)
    {
        const double output_rate = get_arg<double>("output_rate", chan);
        const size_t interp_rate = boost::math::iround(output_rate/get_input_rates().clip(requested_rate, true));
        size_t interp = interp_rate;

        uint32_t hb_enable = 0;
        while ((interp % 2 == 0) and hb_enable < _num_halfbands) {
            hb_enable++;
            interp /= 2;
        }
        UHD_ASSERT_THROW(hb_enable <= _num_halfbands);
        UHD_ASSERT_THROW(interp > 0 and interp <= _cic_max_interp);
        // What we can't cover with halfbands, we do with the CIC
        sr_write("INTERP_WORD", (hb_enable << 8) | (interp & 0xff), chan);

        // Rate change = M/N
        sr_write("N", 1, chan);
        sr_write("M", std::pow(2.0, double(hb_enable)) * (interp & 0xff), chan);

        if (interp > 1 and hb_enable == 0) {
            UHD_LOGGER_WARNING("RFNOC") << boost::format(
                "The requested interpolation is odd; the user should expect passband CIC rolloff.\n"
                "Select an even interpolation to ensure that a halfband filter is enabled.\n"
                "interpolation = dsp_rate/samp_rate -> %d = (%f MHz)/(%f MHz)\n"
            ) % interp_rate % (output_rate/1e6) % (requested_rate/1e6);
        }

        // Calculate algorithmic gain of CIC for a given interpolation
        // For Ettus CIC R=interp, M=1, N=4. Gain = (R * M) ^ (N - 1)
        const int CIC_N = 4;
        const double rate_pow = std::pow(double(interp & 0xff), CIC_N - 1);
        const double CONSTANT_GAIN = 1.0;

        const double scaling_adjustment =
            std::pow(2, uhd::math::ceil_log2(rate_pow))/(CONSTANT_GAIN*rate_pow);
        update_scalar(scaling_adjustment, chan);
        return output_rate/interp_rate;
    }

    //! Set frequency and interpolation again
    void set_output_rate(const double /* rate */, const size_t chan)
    {
        const double desired_freq = _tree->access<double>(get_arg_path("freq", chan) / "value").get_desired();
        set_arg<double>("freq", desired_freq, chan);
        const double desired_input_rate = _tree->access<double>(get_arg_path("input_rate", chan) / "value").get_desired();
        set_arg<double>("input_rate", desired_input_rate, chan);
    }

    // Calculate compensation gain values for algorithmic gain of DDS and CIC taking into account
    // gain compensation blocks already hardcoded in place in DUC (that provide simple 1/2^n gain compensation).
    // Further more factor in OTW format which adds further gain factor to weight output samples correctly.
    void update_scalar(const double scalar, const size_t chan)
    {
        const double target_scalar = (1 << 15) * scalar;
        const int32_t actual_scalar = boost::math::iround(target_scalar);
        // Calculate the error introduced by using integer representation for the scalar
        const double scalar_correction =
            actual_scalar / target_scalar * (double(1 << 15) - 1.0) // Rounding error, normalized to 1.0
            * get_arg<double>("fullscale"); // Scaling requested by host
        set_arg<double>("scalar_correction", scalar_correction, chan);
        // Write DUC with scaling correction for CIC and CORDIC that maximizes dynamic range in 32/16/12/8bits.
        sr_write("SCALE_IQ", actual_scalar, chan);
    }

    //! Get cached value of FPGA compat number
    uint64_t get_fpga_compat() const
    {
        return _fpga_compat;
    }

    //Get cached value of _num_halfbands
    size_t get_num_halfbands() const
    {
        return _num_halfbands;
    }

    //Get cached value of _cic_max_decim readback
    size_t get_cic_max_interp() const
    {
        return _cic_max_interp;
    }
};

UHD_RFNOC_BLOCK_REGISTER(duc_block_ctrl, "DUC");
