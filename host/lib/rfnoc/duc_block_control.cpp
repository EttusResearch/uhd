//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/compat_check.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/cores/dsp_core_utils.hpp>
#include <uhdlib/utils/math.hpp>
#include <cmath>
#include <set>
#include <string>

namespace {

constexpr double DEFAULT_SCALING         = 1.0;
constexpr int DEFAULT_INTERP             = 1;
constexpr double DEFAULT_FREQ            = 0.0;
const uhd::rfnoc::io_type_t DEFAULT_TYPE = uhd::rfnoc::IO_TYPE_SC16;
constexpr double TX_SIGN                 = -1.0;

//! Space (in bytes) between register banks per channel
constexpr uint32_t REG_CHAN_OFFSET = 2048;

} // namespace

using namespace uhd::rfnoc;

const uint16_t duc_block_control::MINOR_COMPAT = 1;
const uint16_t duc_block_control::MAJOR_COMPAT = 0;

const uint32_t duc_block_control::RB_COMPAT_NUM     = 0; // read this first
const uint32_t duc_block_control::RB_NUM_HB         = 8;
const uint32_t duc_block_control::RB_CIC_MAX_INTERP = 16;

const uint32_t duc_block_control::SR_N_ADDR         = 128 * 8;
const uint32_t duc_block_control::SR_M_ADDR         = 129 * 8;
const uint32_t duc_block_control::SR_CONFIG_ADDR    = 130 * 8;
const uint32_t duc_block_control::SR_INTERP_ADDR    = 131 * 8;
const uint32_t duc_block_control::SR_FREQ_ADDR      = 132 * 8;
const uint32_t duc_block_control::SR_SCALE_IQ_ADDR  = 133 * 8;
const uint32_t duc_block_control::SR_TIME_INCR_ADDR = 137 * 8;

class duc_block_control_impl : public duc_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(duc_block_control)
    , _duc_reg_iface(*this, 0, REG_CHAN_OFFSET),
        _fpga_compat(regs().peek32(RB_COMPAT_NUM)),
        _num_halfbands(regs().peek32(RB_NUM_HB)),
        _cic_max_interp(regs().peek32(RB_CIC_MAX_INTERP)),
        _residual_scaling(get_num_input_ports(), DEFAULT_SCALING)
    {
        UHD_ASSERT_THROW(get_num_input_ports() == get_num_output_ports());
        UHD_ASSERT_THROW(_cic_max_interp > 0 && _cic_max_interp <= 0xFF);
        uhd::assert_fpga_compat(MAJOR_COMPAT,
            MINOR_COMPAT,
            _fpga_compat,
            get_unique_id(),
            get_unique_id(),
            false /* Let it slide if minors mismatch */
        );
        RFNOC_LOG_DEBUG("Loading DUC with " << _num_halfbands
                                            << " halfbands and "
                                               "max CIC interpolation "
                                            << _cic_max_interp);
        // This line is not strictly necessary, as ONE_TO_ONE is the default.
        // We set it make it explicit how this block works. Output packets have
        // the same size as the input packet.
        set_mtu_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        // Load list of valid interpolation values
        std::set<size_t> interps{1}; // 1 is always a valid interpolation
        for (size_t hb = 0; hb < _num_halfbands; hb++) {
            for (size_t cic_interp = 1; cic_interp <= _cic_max_interp; cic_interp++) {
                interps.insert((1 << hb) * cic_interp);
            }
        }
        for (size_t interp : interps) {
            _valid_interps.push_back(uhd::range_t(double(interp)));
        }

        // Initialize properties. It is very important to first reserve the
        // space, because we use push_back() further down, and properties must
        // not change their base address after registration and resolver
        // creation.
        _samp_rate_in.reserve(get_num_ports());
        _samp_rate_out.reserve(get_num_ports());
        _scaling_in.reserve(get_num_ports());
        _scaling_out.reserve(get_num_ports());
        _interp.reserve(get_num_ports());
        _freq.reserve(get_num_ports());
        _type_in.reserve(get_num_ports());
        _type_out.reserve(get_num_ports());
        for (size_t chan = 0; chan < get_num_ports(); chan++) {
            _register_props(chan);
        }
        register_issue_stream_cmd();
        register_issue_tune_request();
    }

    double set_freq(const double freq,
        const size_t chan,
        const boost::optional<uhd::time_spec_t> time) override
    {
        // Store the current command time so we can restore it later
        auto prev_cmd_time = get_command_time(chan);
        if (time) {
            set_command_time(time.get(), chan);
        }
        // This will trigger property propagation:
        set_property<double>("freq", freq, chan);
        set_command_time(prev_cmd_time, chan);
        return get_freq(chan);
    }

    double get_freq(const size_t chan) const override
    {
        return _freq.at(chan).get();
    }

    uhd::freq_range_t get_frequency_range(const size_t chan) const override
    {
        const double output_rate = get_output_rate(chan);
        // TODO add steps
        return uhd::freq_range_t(-output_rate / 2, output_rate / 2);
    }

    double get_input_rate(const size_t chan) const override
    {
        return _samp_rate_in.at(chan).is_valid() ? _samp_rate_in.at(chan).get() : 1.0;
    }

    double get_output_rate(const size_t chan) const override
    {
        return _samp_rate_out.at(chan).is_valid() ? _samp_rate_out.at(chan).get() : 1.0;
    }

    void set_output_rate(const double rate, const size_t chan) override
    {
        set_property<double>("samp_rate", rate, {res_source_info::OUTPUT_EDGE, chan});
    }

    uhd::meta_range_t get_input_rates(const size_t chan) const override
    {
        uhd::meta_range_t result;
        if (!_samp_rate_out.at(chan).is_valid()) {
            result.push_back(uhd::range_t(1.0));
            return result;
        }
        const double output_rate = _samp_rate_out.at(chan).get();
        // The interpolations are stored in order (from smallest to biggest), so
        // iterate in reverse order so we can add rates from smallest to biggest
        for (auto it = _valid_interps.rbegin(); it != _valid_interps.rend(); ++it) {
            result.push_back(uhd::range_t(output_rate / it->start()));
        }
        return result;
    }

    double set_input_rate(const double rate, const size_t chan) override
    {
        if (_samp_rate_out.at(chan).is_valid()) {
            const int coerced_interp = coerce_interp(get_output_rate(chan) / rate);
            set_property<int>("interp", coerced_interp, chan);
        } else {
            RFNOC_LOG_DEBUG(
                "Property samp_rate@"
                << chan
                << " is not valid, attempting to set input rate via the edge property.");
            set_property<double>("samp_rate", rate, {res_source_info::INPUT_EDGE, chan});
        }
        return _samp_rate_in.at(chan).get();
    }

protected:
    //! Block-specific register interface
    multichan_register_iface _duc_reg_iface;

private:
    //! Shorthand for num ports, since num input ports always equals num output ports
    inline size_t get_num_ports()
    {
        return get_num_input_ports();
    }

    /**************************************************************************
     * Initialization
     *************************************************************************/
    void _register_props(const size_t chan)
    {
        // Create actual properties and store them
        _samp_rate_in.push_back(
            property_t<double>(PROP_KEY_SAMP_RATE, {res_source_info::INPUT_EDGE, chan}));
        _samp_rate_out.push_back(
            property_t<double>(PROP_KEY_SAMP_RATE, {res_source_info::OUTPUT_EDGE, chan}));
        _scaling_in.push_back(
            property_t<double>(PROP_KEY_SCALING, {res_source_info::INPUT_EDGE, chan}));
        _scaling_out.push_back(
            property_t<double>(PROP_KEY_SCALING, {res_source_info::OUTPUT_EDGE, chan}));
        _interp.push_back(property_t<int>(
            PROP_KEY_INTERP, DEFAULT_INTERP, {res_source_info::USER, chan}));
        _freq.push_back(property_t<double>(
            PROP_KEY_FREQ, DEFAULT_FREQ, {res_source_info::USER, chan}));
        _type_in.emplace_back(property_t<std::string>(
            PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE, chan}));
        _type_out.emplace_back(property_t<std::string>(
            PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE, chan}));
        UHD_ASSERT_THROW(_samp_rate_in.size() == chan + 1);
        UHD_ASSERT_THROW(_samp_rate_out.size() == chan + 1);
        UHD_ASSERT_THROW(_scaling_in.size() == chan + 1);
        UHD_ASSERT_THROW(_scaling_out.size() == chan + 1);
        UHD_ASSERT_THROW(_interp.size() == chan + 1);
        UHD_ASSERT_THROW(_freq.size() == chan + 1);
        UHD_ASSERT_THROW(_type_in.size() == chan + 1);
        UHD_ASSERT_THROW(_type_out.size() == chan + 1);

        // give us some shorthands for the rest of this function
        property_t<double>* samp_rate_in  = &_samp_rate_in.back();
        property_t<double>* samp_rate_out = &_samp_rate_out.back();
        property_t<double>* scaling_in    = &_scaling_in.back();
        property_t<double>* scaling_out   = &_scaling_out.back();
        property_t<int>* interp           = &_interp.back();
        property_t<double>* freq          = &_freq.back();
        property_t<std::string>* type_in  = &_type_in.back();
        property_t<std::string>* type_out = &_type_out.back();

        // register them
        register_property(samp_rate_in);
        register_property(samp_rate_out);
        register_property(scaling_in);
        register_property(scaling_out);
        register_property(interp);
        register_property(freq);
        register_property(type_in);
        register_property(type_out);

        /**********************************************************************
         * Add resolvers
         *********************************************************************/
        // Resolver for _interp: this gets executed when the user directly
        // modifies 'interp'. The desired behaviour is to coerce it first, then
        // keep the output rate constant, and re-calculate the input rate.
        add_property_resolver({interp, scaling_in},
            {interp, samp_rate_out, samp_rate_in, scaling_in},
            [this,
                chan,
                &interp        = *interp,
                &samp_rate_out = *samp_rate_out,
                &samp_rate_in  = *samp_rate_in,
                &scaling_in    = *scaling_in,
                &scaling_out   = *scaling_out]() {
                RFNOC_LOG_TRACE("Calling resolver for `interp'@" << chan);
                interp = coerce_interp(double(interp.get()));
                // The following function will also update _residual_scaling
                if (interp.is_dirty()) {
                    set_interp(interp.get(), chan);
                }
                if (samp_rate_out.is_valid()) {
                    const double new_samp_rate_in = samp_rate_out.get() / interp.get();
                    if (samp_rate_in.is_valid()) {
                        // Only update the samp_rate_in if the new value is not the same
                        // frequency. However, we still want to call the operator= to make
                        // sure metadata gets handled
                        samp_rate_in = uhd::math::frequencies_are_equal(
                                           new_samp_rate_in, samp_rate_in.get())
                                           ? samp_rate_in.get()
                                           : new_samp_rate_in;
                    } else {
                        samp_rate_in = new_samp_rate_in;
                    }
                } else if (samp_rate_in.is_valid()) {
                    const double new_samp_rate_out = samp_rate_in.get() * interp.get();
                    if (samp_rate_out.is_valid()) {
                        // Only update if the new value is not the same frequency
                        samp_rate_out = uhd::math::frequencies_are_equal(
                                            new_samp_rate_out, samp_rate_out.get())
                                            ? samp_rate_out.get()
                                            : new_samp_rate_out;
                    } else {
                        samp_rate_out = new_samp_rate_out;
                    }
                }
                // The scaling is independent of the actual rates
                if (scaling_out.is_valid()) {
                    scaling_in = scaling_out.get() * _residual_scaling.at(chan);
                }
            });
        // Resolver for _freq: this gets executed when the user directly
        // modifies _freq.
        add_property_resolver({freq},
            {freq},
            [this, chan, &samp_rate_out = *samp_rate_out, &freq = *freq]() {
                RFNOC_LOG_TRACE("Calling resolver for `freq'@" << chan);
                if (samp_rate_out.is_valid()) {
                    const double new_freq =
                        _set_freq(freq.get(), samp_rate_out.get(), chan);
                    // If the frequency we just set is sufficiently close to the old
                    // frequency, don't bother updating the property in software
                    if (!uhd::math::frequencies_are_equal(new_freq, freq.get())) {
                        freq = new_freq;
                    }
                } else {
                    RFNOC_LOG_DEBUG("Not setting frequency until sampling rate is set.");
                }
            });
        // Resolver for the input rate: we try and match interp so that the
        // output rate is not modified. if interp needs to be coerced, only then
        // the output rate is modified.
        // Note this might also affect the frequency (if the output rate is
        // modified).
        add_property_resolver({samp_rate_in},
            {interp, samp_rate_out},
            [this,
                chan,
                &interp        = *interp,
                &samp_rate_out = *samp_rate_out,
                &samp_rate_in  = *samp_rate_in]() {
                const auto UHD_UNUSED(log_chan) = chan;
                RFNOC_LOG_TRACE("Calling resolver for `samp_rate_in'@" << chan);
                if (samp_rate_in.is_valid()) {
                    RFNOC_LOG_TRACE("New samp_rate_in is " << samp_rate_in.get());
                    // If interp is changed, that will take care of scaling
                    if (samp_rate_out.is_valid()) {
                        interp = coerce_interp(samp_rate_out.get() / samp_rate_in.get());
                    }
                    const double new_samp_rate_out = samp_rate_in.get() * interp.get();
                    if (samp_rate_out.is_valid()) {
                        // Only update if the new value is not the same frequency
                        samp_rate_out = uhd::math::frequencies_are_equal(
                                            new_samp_rate_out, samp_rate_out.get())
                                            ? samp_rate_out.get()
                                            : new_samp_rate_out;
                    } else {
                        samp_rate_out = new_samp_rate_out;
                    }
                    RFNOC_LOG_TRACE("New samp_rate_out is " << samp_rate_out.get());
                }
            });
        // Resolver for the output rate: like the previous one, but flipped.
        add_property_resolver({samp_rate_out},
            {interp, samp_rate_in, freq},
            [this,
                chan,
                &interp        = *interp,
                &freq          = *freq,
                &samp_rate_out = *samp_rate_out,
                &samp_rate_in  = *samp_rate_in]() {
                const auto UHD_UNUSED(log_chan) = chan;
                RFNOC_LOG_TRACE("Calling resolver for `samp_rate_out'@" << chan);
                if (samp_rate_out.is_valid()) {
                    // If interp is changed, that will take care of scaling
                    if (samp_rate_in.is_valid()) {
                        interp =
                            coerce_interp(int(samp_rate_out.get() / samp_rate_in.get()));
                    }
                    const double new_samp_rate_in = samp_rate_out.get() / interp.get();
                    if (samp_rate_in.is_valid()) {
                        // Only update if the new value is not the same frequency
                        samp_rate_in = uhd::math::frequencies_are_equal(
                                           new_samp_rate_in, samp_rate_in.get())
                                           ? samp_rate_in.get()
                                           : new_samp_rate_in;
                    } else {
                        samp_rate_in = new_samp_rate_in;
                    }
                    // We now need to force the resolver for freq to run so it can
                    // update its phase increment
                    freq.force_dirty();
                }
            });
        // Resolver for the output rate: like the previous one, but flipped.
        add_property_resolver({scaling_out},
            {scaling_in},
            [this,
                chan,
                &interp        = *interp,
                &samp_rate_out = *samp_rate_out,
                &samp_rate_in  = *samp_rate_in,
                &scaling_in    = *scaling_in,
                &scaling_out   = *scaling_out]() {
                RFNOC_LOG_TRACE("Calling resolver for `scaling_out'@" << chan);
                // If any of these are dirty, the interp resolver will kick in
                // and calculate the scaling itself, so we don't do it here to
                // avoid conflict.
                if (!interp.is_dirty() && !samp_rate_in.is_dirty()
                    && !samp_rate_out.is_dirty() && scaling_out.is_valid()) {
                    scaling_in = scaling_out.get() * _residual_scaling.at(chan);
                }
            });
        // Resolvers for type: These are constants
        add_property_resolver(
            {type_in}, {type_in}, [&type_in = *type_in]() { type_in.set(IO_TYPE_SC16); });
        add_property_resolver({type_out}, {type_out}, [&type_out = *type_out]() {
            type_out.set(IO_TYPE_SC16);
        });
    }

    void register_issue_stream_cmd()
    {
        register_action_handler(ACTION_KEY_STREAM_CMD,
            [this](const res_source_info& src, action_info::sptr action) {
                stream_cmd_action_info::sptr stream_cmd_action =
                    std::dynamic_pointer_cast<stream_cmd_action_info>(action);
                if (!stream_cmd_action) {
                    throw uhd::runtime_error(
                        "Received stream_cmd of invalid action type!");
                }
                issue_stream_cmd_action_handler(src, stream_cmd_action);
            });
    }

    void issue_stream_cmd_action_handler(
        const res_source_info& src, stream_cmd_action_info::sptr stream_cmd_action)
    {
        res_source_info dst_edge{res_source_info::invert_edge(src.type), src.instance};
        const size_t chan = src.instance;
        uhd::stream_cmd_t::stream_mode_t stream_mode =
            stream_cmd_action->stream_cmd.stream_mode;
        RFNOC_LOG_TRACE("Received stream command: " << char(stream_mode) << " to "
                                                    << src.to_string()
                                                    << ", id==" << stream_cmd_action->id);
        auto new_action        = stream_cmd_action_info::make(stream_mode);
        new_action->stream_cmd = stream_cmd_action->stream_cmd;
        if (stream_mode == uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE
            || stream_mode == uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE) {
            if (src.type == res_source_info::INPUT_EDGE) {
                new_action->stream_cmd.num_samps *= _interp.at(chan).get();
            } else {
                new_action->stream_cmd.num_samps /= _interp.at(chan).get();
            }
            RFNOC_LOG_TRACE("Forwarding num_samps stream command, new value is "
                            << new_action->stream_cmd.num_samps);
        } else {
            RFNOC_LOG_TRACE("Forwarding continuous stream command...")
        }

        post_action(dst_edge, new_action);
    }

    void register_issue_tune_request()
    {
        RFNOC_LOG_TRACE("DUC register_isssue_tune_request");
        register_action_handler(ACTION_KEY_TUNE_REQUEST,
            [this](const res_source_info& src, action_info::sptr action) {
                tune_request_action_info::sptr tune_request_action =
                    std::dynamic_pointer_cast<tune_request_action_info>(action);
                if (!tune_request_action) {
                    throw uhd::runtime_error(
                        "Received tune_request of invalid action type!");
                }
                issue_tune_request_action_handler(src, tune_request_action);
            });
    }

    void issue_tune_request_action_handler(
        const res_source_info& src, tune_request_action_info::sptr tune_request_action)
    {
        RFNOC_LOG_TRACE("DUC isssue_tune_request_action_handler");
        res_source_info dst_edge{res_source_info::invert_edge(src.type), src.instance};
        const size_t chan                = src.instance;
        uhd::tune_request_t tune_request = tune_request_action->tune_request;

        RFNOC_LOG_TRACE("Received tune_request to "
                        << src.to_string() << ", id==" << tune_request_action->id);

        uhd::freq_range_t dsp_range    = get_frequency_range(chan);
        uhd::freq_range_t tune_range   = tune_request_action->overall_freq_range;
        tune_request_action->dsp_range = dsp_range;

        if (tune_range.empty()) {
            tune_range = dsp_range;
        }

        if (src.type == res_source_info::OUTPUT_EDGE) {
            auto set_dsp_freq = [this, chan](
                                    double freq) { set_freq(freq, chan, boost::none); };

            double clipped_requested_freq = tune_range.clip(tune_request.target_freq);
            tune_request_action->tune_result.target_dsp_freq = abs(
                tune_request_action->tune_result.actual_rf_freq - clipped_requested_freq);

            //------------------------------------------------------------------
            //-- Set the DSP frequency depending upon the DSP frequency policy.
            //------------------------------------------------------------------
            double target_dsp_freq = 0.0;
            switch (tune_request.dsp_freq_policy) {
                case uhd::tune_request_t::POLICY_AUTO:
                    // invert the sign on the dsp freq for transmit (spinning up vs down)
                    tune_request_action->tune_result.target_dsp_freq *= TX_SIGN;

                    /* If we are using the AUTO tuning policy, then we prevent the
                     * CORDIC from spinning us outside of the range of the baseband
                     * filter, regardless of what the user requested. This could happen
                     * if the user requested a center frequency so far outside of the
                     * tunable range of the FE that the CORDIC would spin outside the
                     * filtered baseband. */
                    target_dsp_freq = tune_request_action->tune_result.target_dsp_freq
                                      - tune_request_action->tune_result.actual_dsp_freq;
                    break;

                case uhd::tune_request_t::POLICY_MANUAL:
                    /* If the user has specified a manual tune policy, we will allow
                     * tuning outside of the baseband filter, but will still clip the
                     * target DSP frequency to within the bounds of the CORDIC to
                     * prevent undefined behavior (likely an overflow). */
                    target_dsp_freq = dsp_range.clip(tune_request.dsp_freq);
                    break;

                case uhd::tune_request_t::POLICY_NONE:
                    break; // does not set
            }
            RFNOC_LOG_TRACE(
                str(boost::format("Target DSP Freq: %.6fMHz") % (target_dsp_freq / 1e6)));

            //------------------------------------------------------------------
            //-- Tune the DSP
            //------------------------------------------------------------------
            if (tune_request.dsp_freq_policy != uhd::tune_request_t::POLICY_NONE) {
                set_dsp_freq(target_dsp_freq);
            }
            const double actual_dsp_freq = get_freq(chan);

            RFNOC_LOG_TRACE(
                str(boost::format("Actual DSP Freq: %.6fMHz") % (actual_dsp_freq / 1e6)));

            tune_request_action->tune_result.target_dsp_freq = target_dsp_freq;
            tune_request_action->tune_result.actual_dsp_freq += actual_dsp_freq;

            RFNOC_LOG_TRACE(
                "Tune_result details DUC : target_rf_frq = "
                << tune_request_action->tune_result.target_rf_freq
                << " target dsp freq = "
                << tune_request_action->tune_result.target_dsp_freq << " clipped rf_freq "
                << tune_request_action->tune_result.clipped_rf_freq
                << " actual_rf_freq = " << tune_request_action->tune_result.actual_rf_freq
                << " actual dsp_freq= "
                << tune_request_action->tune_result.actual_dsp_freq);

            auto new_action                = tune_request_action_info::make(tune_request);
            new_action->tune_request       = tune_request_action->tune_request;
            new_action->tune_result        = tune_request_action->tune_result;
            new_action->dsp_range          = tune_request_action->dsp_range;
            new_action->overall_freq_range = tune_request_action->overall_freq_range;
            post_action(dst_edge, new_action);
        } else {
            auto new_action                = tune_request_action_info::make(tune_request);
            new_action->tune_request       = tune_request_action->tune_request;
            new_action->tune_result        = tune_request_action->tune_result;
            new_action->dsp_range          = tune_request_action->dsp_range;
            new_action->overall_freq_range = tune_request_action->overall_freq_range;
            post_action(dst_edge, new_action);
        }
    }

    /**************************************************************************
     * FPGA communication (register IO)
     *************************************************************************/
    /*! Update the interpolation value
     *
     * \param interp The new interpolation value.
     * \throws uhd::assertion_error if interp is not valid.
     */
    void set_interp(int interp, const size_t chan)
    {
        RFNOC_LOG_TRACE("Set interp to " << interp);
        // Step 1: Calculate number of halfbands
        uint32_t hb_enable  = 0;
        uint32_t cic_interp = interp;
        while ((cic_interp % 2 == 0) and hb_enable < _num_halfbands) {
            hb_enable++;
            cic_interp /= 2;
        }
        // Step 2: Make sure we can handle the rest with the CIC
        UHD_ASSERT_THROW(hb_enable <= _num_halfbands);
        UHD_ASSERT_THROW(cic_interp > 0 and cic_interp <= _cic_max_interp);
        const uint32_t interp_word = (hb_enable << 8) | cic_interp;
        _duc_reg_iface.poke32(SR_INTERP_ADDR, interp_word, chan);

        // Rate change = M/N, where N = 1
        _duc_reg_iface.poke32(SR_M_ADDR, interp, chan);
        _duc_reg_iface.poke32(SR_N_ADDR, 1, chan);

        // Configure time increment in ticks per M output samples
        _duc_reg_iface.poke32(
            SR_TIME_INCR_ADDR, uint32_t(get_tick_rate() / get_output_rate(chan)), chan);

        if (cic_interp > 1 and hb_enable == 0) {
            RFNOC_LOG_WARNING(
                "The requested interpolation is odd; the user should expect passband "
                "CIC rolloff.\n"
                "Select an even interpolation to ensure that a halfband filter is "
                "enabled.\n");
        }

        // DDS gain:
        constexpr double DDS_GAIN = 1.0;
        // Calculate algorithmic gain of CIC for a given interpolation.
        // For Ettus CIC R=interp, M=1, N=4. Gain = (R * M) ^ (N - 1)
        const double cic_gain = std::pow(double(cic_interp * 1), /*N*/ 4 - 1);
        // The Ettus CIC also tries its best to compensate for the gain by
        // shifting the CIC output. This reduces the gain by a factor of
        // 2**ceil(log2(cic_gain))
        const double total_gain =
            DDS_GAIN * cic_gain / std::pow(2, uhd::math::ceil_log2(cic_gain));
        update_scaling(total_gain, chan);
    }

    //! Update scaling based on the current gain
    //
    // Calculates the closest fixpoint value that this block can correct for in
    // hardware (fixpoint). The residual gain is written to _residual_scaling.
    void update_scaling(const double dsp_gain, const size_t chan)
    {
        constexpr double FIXPOINT_SCALING = 1 << 15;
        const double compensation_factor  = 1. / dsp_gain;
        // Convert to fixpoint
        const double target_factor  = FIXPOINT_SCALING * compensation_factor;
        const int32_t actual_factor = static_cast<int32_t>(std::lround(target_factor));
        // Write DUC with scaling correction for CIC and DDS that maximizes
        // dynamic range
        _duc_reg_iface.poke32(SR_SCALE_IQ_ADDR, actual_factor, chan);

        // Calculate the error introduced by using fixedpoint representation for
        // the scaler, can be corrected in host later.
        _residual_scaling[chan] = dsp_gain * double(actual_factor) / FIXPOINT_SCALING;
    }

    /*! Return the closest possible interpolation value to the one requested
     */
    int coerce_interp(const double requested_interp) const
    {
        UHD_ASSERT_THROW(requested_interp >= 0);
        return static_cast<int>(_valid_interps.clip(requested_interp, true));
    }

    //! Set the DDS frequency shift the signal to \p requested_freq
    double _set_freq(
        const double requested_freq, const double dds_rate, const size_t chan)
    {
        double actual_freq;
        int32_t freq_word;
        std::tie(actual_freq, freq_word) =
            get_freq_and_freq_word(requested_freq, dds_rate);

        _duc_reg_iface.poke32(
            SR_FREQ_ADDR, uint32_t(freq_word), chan, get_command_time(chan));
        return actual_freq;
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Block compat number
    const uint32_t _fpga_compat;
    //! Number of halfbands
    const size_t _num_halfbands;
    //! Max CIC interpolation
    const size_t _cic_max_interp;

    //! List of valid interpolation values
    uhd::meta_range_t _valid_interps;

    //! Cache the current residual scaling
    std::vector<double> _residual_scaling;

    //! Properties for type_in (one per port)
    std::vector<property_t<std::string>> _type_in;
    //! Properties for type_out (one per port)
    std::vector<property_t<std::string>> _type_out;
    //! Properties for samp_rate_in (one per port)
    std::vector<property_t<double>> _samp_rate_in;
    //! Properties for samp_rate_out (one per port)
    std::vector<property_t<double>> _samp_rate_out;
    //! Properties for scaling_in (one per port)
    std::vector<property_t<double>> _scaling_in;
    //! Properties for scaling_out (one per port)
    std::vector<property_t<double>> _scaling_out;
    //! Properties for interp (one per port)
    std::vector<property_t<int>> _interp;
    //! Properties for freq (one per port)
    std::vector<property_t<double>> _freq;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    duc_block_control, 0xD0C00000, "DUC", CLOCK_KEY_GRAPH, "bus_clk")
