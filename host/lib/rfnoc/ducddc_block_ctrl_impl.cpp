//
// Copyright 2016-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/ducddc_block_ctrl.hpp>
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

class ducddc_block_ctrl_impl : public ducddc_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(ducddc_block_ctrl)
        , _fpga_compat(user_reg_read64(RB_REG_COMPAT_NUM))
        , _num_duc_halfbands(uhd::narrow_cast<size_t>(
                    user_reg_read64(RB_REG_DUC_NUM_HALFBANDS)))
        , _num_ddc_halfbands(uhd::narrow_cast<size_t>(
                    user_reg_read64(RB_REG_DDC_NUM_HALFBANDS)))
        , _cic_max_interp(uhd::narrow_cast<size_t>(
                    user_reg_read64(RB_REG_CIC_MAX_INTERP)))
        , _cic_max_decim(uhd::narrow_cast<size_t>(
                    user_reg_read64(RB_REG_CIC_MAX_DECIM)))
    {
        UHD_LOG_DEBUG(unique_id(),
            "Loading DUCDDC with " << get_num_duc_halfbands() <<
            " DUC halfbands, " << get_num_ddc_halfbands() <<
            "DDC halfbands, max CIC interpolation " << get_cic_max_interp() <<
            ", and max CIC decimation" << get_cic_max_decim()
        );
        uhd::assert_fpga_compat(
            MAJOR_COMP, MINOR_COMP,
            _fpga_compat,
            "DUCDDC", "DUCDDC",
            false /* Let it slide if minors mismatch */
        );

        // Argument/prop tree hooks
        for (size_t chan = 0; chan < get_input_ports().size(); chan++) {
            const double default_interp_rate =
                get_arg<double>("interp", chan);
            _tree->access<double>(get_arg_path("interp/value", chan))
                .set_coercer([this, chan](const double value){
                    return this->set_interp(value, chan);
                })
                .set(default_interp_rate)
            ;
            const double default_decim_rate =
                get_arg<double>("decim", chan);
            _tree->access<double>(get_arg_path("decim/value", chan))
                .set_coercer([this, chan](const double value){
                    return this->set_decim(value, chan);
                })
                .set(default_decim_rate)
            ;

            // Rate 1:1 by default
            sr_write("N", 1, chan);
            sr_write("M", 1, chan);
            sr_write("CONFIG", 1, chan); // Enable clear EOB
            sr_write("DUC_SCALE_IQ", (1 << 14), chan);
            sr_write("DDC_SCALE_IQ", (1 << 14), chan);
        }
    } // end ctor

    virtual ~ducddc_block_ctrl_impl() {}

    double get_input_samp_rate(size_t port=ANY_PORT)
    {
        if (port == ANY_PORT) {
            port = 0;
            for (size_t i = 0; i < get_input_ports().size(); i++) {
                if (_tx_streamer_active.count(i) and _tx_streamer_active.at(i)) {
                    port = i;
                    break;
                }
            }
        }

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
        if (port == ANY_PORT) {
            port = 0;
            for (size_t i = 0; i < get_input_ports().size(); i++) {
                if (_rx_streamer_active.count(i) and _rx_streamer_active.at(i)) {
                    port = i;
                    break;
                }
            }
        }

        // Wait, what? If this seems out of place to you, you're right. However,
        // we need a function call that is called when the graph is complete,
        // but streaming is not yet set up.
        if (_tree->exists("tick_rate")) {
            const double tick_rate = _tree->access<double>("tick_rate").get();
            set_command_tick_rate(tick_rate, port);
        }

        if (not (_rx_streamer_active.count(port) and _rx_streamer_active.at(port))) {
            return RATE_UNDEFINED;
        }
        return get_arg<double>("output_rate", port == ANY_PORT ? 0 : port);
    }

    void issue_stream_cmd(
            const uhd::stream_cmd_t &stream_cmd_,
            const size_t chan
    ) {
        UHD_RFNOC_BLOCK_TRACE() << "ducddc_block_ctrl_base::issue_stream_cmd()" ;
        check_rate_conversion(chan);

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
    static constexpr size_t RB_REG_DUC_NUM_HALFBANDS = 1;
    static constexpr size_t RB_REG_CIC_MAX_INTERP = 2;
    static constexpr size_t RB_REG_DDC_NUM_HALFBANDS = 11;
    static constexpr size_t RB_REG_CIC_MAX_DECIM = 12;

    const uint64_t _fpga_compat;
    const size_t _num_duc_halfbands;
    const size_t _num_ddc_halfbands;
    const size_t _cic_max_interp;
    const size_t _cic_max_decim;

    size_t write_interp_word(const size_t interp_rate, const size_t chan)
    {
        size_t interp = interp_rate;
        uint32_t hb_enable = 0;
        while ((interp % 2 == 0) and hb_enable < _num_duc_halfbands) {
            hb_enable++;
            interp /= 2;
        }
        UHD_LOGGER_DEBUG(unique_id()) << "Setting interp rate: " << interp_rate <<
            " (CIC value: " << interp << ", Num Halfbands: " << hb_enable << ")";
        UHD_ASSERT_THROW(hb_enable <= _num_duc_halfbands);
        UHD_ASSERT_THROW(interp > 0 and interp <= _cic_max_interp);
        // What we can't cover with halfbands, we do with the CIC
        sr_write("INTERP_WORD", (hb_enable << 8) | (interp & 0xff), chan);

        // Rate change = M/N
        size_t interp_result = std::pow(2.0, double(hb_enable)) * (interp & 0xff);
        sr_write("M", interp_result, chan);

        if (interp > 1 and hb_enable == 0) {
            UHD_LOGGER_WARNING(unique_id()) << boost::format(
                "The requested interpolation is odd; the user should expect passband CIC rolloff.\n"
                "Select an even interpolation to ensure that a halfband filter is enabled.\n"
                "interpolation -> %d"
            ) % interp_rate;
        }


        // Calculate algorithmic gain of CIC for a given interpolation
        // For Ettus CIC R=interp, M=1, N=4. Gain = (R * M) ^ (N - 1)
        // See ducc_block_ctrl_impl for info
        const int CIC_N = 4;
        const double rate_pow = std::pow(double(interp & 0xff), CIC_N - 1);
        const double CONSTANT_GAIN = 1.0;
        const double scaling_adjustment =
            std::pow(2, uhd::math::ceil_log2(rate_pow))/(CONSTANT_GAIN*rate_pow);
        UHD_LOGGER_DEBUG(unique_id()) << "DUC Amplitude Scale: " << scaling_adjustment;
        set_arg<double>("duc_scale", scaling_adjustment, chan);

        return interp_result;
    }

    size_t write_decim_word(const size_t decim_rate, const size_t chan)
    {
        size_t decim = decim_rate;
        // The FPGA knows which halfbands to enable for any given value of hb_enable.
        uint32_t hb_enable = 0;
        while ((decim % 2 == 0) and hb_enable < _num_ddc_halfbands) {
            hb_enable++;
            decim /= 2;
        }
        UHD_LOGGER_DEBUG(unique_id()) << "Setting decim rate: " << decim_rate <<
            " (CIC value: " << decim << ", Num Halfbands: " << hb_enable << ")";
        UHD_ASSERT_THROW(hb_enable <= _num_ddc_halfbands);
        UHD_ASSERT_THROW(decim > 0 and decim <= _cic_max_decim);
        // What we can't cover with halfbands, we do with the CIC
        sr_write("DECIM_WORD", (hb_enable << 8) | (decim & 0xff), chan);

        // Rate change = M/N
        size_t decim_result = std::pow(2.0, double(hb_enable)) * (decim & 0xff);
        sr_write("N", decim_result, chan);

        if (decim > 1 and hb_enable == 0) {
            UHD_LOGGER_WARNING(unique_id()) << boost::format(
                "The requested decimation is odd; the user should expect passband CIC rolloff.\n"
                "Select an even decimation to ensure that a halfband filter is enabled.\n"
                "Decimations factorable by 4 will enable 2 halfbands, those factorable by 8 will enable 3 halfbands.\n"
                "decimation -> %d"
            ) % decim_rate;
        }

        // Calculate algorithmic gain of CIC for a given decimation.
        // For Ettus CIC R=decim, M=1, N=4. Gain = (R * M) ^ N
        // See ddc_block_ctrl_impl for info
        const double rate_pow = std::pow(double(decim & 0xff), 4);
        static const double DDS_GAIN = 2.0;
        const double scaling_adjustment =
            std::pow(2, uhd::math::ceil_log2(rate_pow))/(DDS_GAIN*rate_pow);
        UHD_LOGGER_DEBUG(unique_id()) << "DDC Amplitude Scale: " << scaling_adjustment;
        set_arg<double>("ddc_scale", scaling_adjustment, chan);

        return decim_result;
    }

    double calc_output_rate(const size_t chan)
    {
        const double input_rate = get_arg<double>("input_rate", chan);
        const double interp = get_arg<double>("interp", chan);
        const double decim = get_arg<double>("decim", chan);
        double rate_scale = (double) interp / (double) decim;
        double derived_rate = rate_scale * input_rate;
        return derived_rate;
    }

    double set_interp(const size_t interp_rate, const size_t chan)
    {
        // Write N:M registers
        size_t decim_result = write_decim_word(get_arg<double>("decim", chan), chan);
        size_t interp_result = write_interp_word(interp_rate, chan);

        // Write the scale factor
        double total_scale = get_arg<double>("duc_scale",chan) * get_arg<double>("ddc_scale",chan);
        update_scalar(total_scale, chan);

        // Calculate new output rate
        double rate_scale = (double) interp_result / (double) decim_result;
        double derived_rate = rate_scale * get_arg<double>("input_rate", chan);
        set_arg<double>("output_rate", derived_rate, chan);
        return interp_result;
    }

    double set_decim(const size_t decim_rate, const size_t chan)
    {
        // Write N:M registers
        size_t decim_result = write_decim_word(decim_rate, chan);
        size_t interp_result = write_interp_word(get_arg<double>("interp", chan), chan);

        // Write the scale factor
        double total_scale = get_arg<double>("duc_scale",chan) * get_arg<double>("ddc_scale",chan);
        update_scalar(total_scale, chan);

        // Calculate new output rate
        double rate_scale = (double) interp_result / (double) decim_result;
        double derived_rate = rate_scale * get_arg<double>("input_rate", chan);
        set_arg<double>("output_rate", derived_rate, chan);
        return decim_result;
    }

    void check_rate_conversion(const size_t chan)
    {
        double input_rate = get_arg<double>("input_rate", chan);
        double output_rate = get_arg<double>("output_rate", chan);
        double derived_rate = calc_output_rate(chan);
        if (output_rate != derived_rate){
            UHD_LOGGER_ERROR(unique_id()) <<
                "[DUCDDC] Cannot achieve desired rate conversion." << std::endl <<
                "  M (interp)  = " << get_arg<double>("interp", chan) << std::endl <<
                "  N (decim)   = " << get_arg<double>("decim", chan) << std::endl <<
                "  Input Rate  = " << input_rate/1e6 << " MHz" << std::endl <<
                "  Output Rate = " << output_rate/1e6 << " MHz" << std::endl <<
                "  Derived Output Rate = " << derived_rate;
        }
    }

    void update_scalar(const double scalar, const size_t chan)
    {
        const double target_scalar = (1 << 15) * scalar;
        const int32_t actual_scalar = boost::math::iround(target_scalar);
        // TODO: Calculate the error introduced by using integer representation for the scalar, can be corrected in host later.
        // Write DDC with scaling correction for CIC and DDS that maximizes dynamic range in 32/16/12/8bits.
        UHD_LOGGER_DEBUG(unique_id()) << "Setting DUC*DDC combined amplitude scale adjustment " << actual_scalar;
        sr_write("DDC_SCALE_IQ", actual_scalar, chan);
    }

    //! Get cached value of FPGA compat number
    uint64_t get_fpga_compat() const
    {
        return _fpga_compat;
    }

    //Get cached value of _num_duc_halfbands
    size_t get_num_duc_halfbands() const
    {
        return _num_duc_halfbands;
    }

    //Get cached value of _num_ddc_halfbands
    size_t get_num_ddc_halfbands() const
    {
        return _num_ddc_halfbands;
    }

    //Get cached value of _cic_max_interp readback
    size_t get_cic_max_interp() const
    {
        return _cic_max_interp;
    }

    //Get cached value of _cic_max_decim readback
    size_t get_cic_max_decim() const
    {
        return _cic_max_decim;
    }
};

UHD_RFNOC_BLOCK_REGISTER(ducddc_block_ctrl, "DUCDDC");
