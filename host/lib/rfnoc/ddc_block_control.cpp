//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/cores/dsp_core_utils.hpp>
#include <uhdlib/utils/compat_check.hpp>
#include <uhdlib/utils/math.hpp>
#include <cmath>
#include <set>
#include <string>

namespace {

constexpr double DEFAULT_SCALING         = 1.0;
constexpr int DEFAULT_DECIM              = 1;
constexpr double DEFAULT_FREQ            = 0.0;
const uhd::rfnoc::io_type_t DEFAULT_TYPE = uhd::rfnoc::IO_TYPE_SC16;

//! Space (in bytes) between register banks per channel
constexpr uint32_t REG_CHAN_OFFSET = 2048;

} // namespace

using namespace uhd::rfnoc;

const uint16_t ddc_block_control::MINOR_COMPAT = 1;
const uint16_t ddc_block_control::MAJOR_COMPAT = 0;

const uint32_t ddc_block_control::RB_COMPAT_NUM    = 0; // read this first
const uint32_t ddc_block_control::RB_NUM_HB        = 8;
const uint32_t ddc_block_control::RB_CIC_MAX_DECIM = 16;

const uint32_t ddc_block_control::SR_N_ADDR         = 128 * 8;
const uint32_t ddc_block_control::SR_M_ADDR         = 129 * 8;
const uint32_t ddc_block_control::SR_CONFIG_ADDR    = 130 * 8;
const uint32_t ddc_block_control::SR_FREQ_ADDR      = 132 * 8;
const uint32_t ddc_block_control::SR_SCALE_IQ_ADDR  = 133 * 8;
const uint32_t ddc_block_control::SR_DECIM_ADDR     = 134 * 8;
const uint32_t ddc_block_control::SR_MUX_ADDR       = 135 * 8;
const uint32_t ddc_block_control::SR_COEFFS_ADDR    = 136 * 8;
const uint32_t ddc_block_control::SR_TIME_INCR_ADDR = 137 * 8;

class ddc_block_control_impl : public ddc_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(ddc_block_control)
    , _ddc_reg_iface(*this, 0, REG_CHAN_OFFSET),
        _fpga_compat(regs().peek32(RB_COMPAT_NUM)),
        _num_halfbands(regs().peek32(RB_NUM_HB)),
        _cic_max_decim(regs().peek32(RB_CIC_MAX_DECIM)),
        _residual_scaling(get_num_input_ports(), DEFAULT_SCALING)
    {
        UHD_ASSERT_THROW(get_num_input_ports() == get_num_output_ports());
        UHD_ASSERT_THROW(_cic_max_decim > 0 && _cic_max_decim <= 0xFF);
        uhd::assert_fpga_compat(MAJOR_COMPAT,
            MINOR_COMPAT,
            _fpga_compat,
            get_unique_id(),
            get_unique_id(),
            false /* Let it slide if minors mismatch */
        );
        RFNOC_LOG_DEBUG("Loading DDC with " << _num_halfbands
                                            << " halfbands and "
                                               "max CIC decimation "
                                            << _cic_max_decim);
        // This line is not strictly necessary, as ONE_TO_ONE is the default.
        // We set it make it explicit how this block works.
        set_mtu_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        // Load list of valid decimation values
        std::set<size_t> decims{1}; // 1 is always a valid decimation
        for (size_t hb = 0; hb < _num_halfbands; hb++) {
            for (size_t cic_decim = 1; cic_decim <= _cic_max_decim; cic_decim++) {
                decims.insert((1 << hb) * cic_decim);
            }
        }
        for (size_t decim : decims) {
            _valid_decims.push_back(uhd::range_t(double(decim)));
        }

        // Initialize properties. It is very important to first reserve the
        // space, because we use push_back() further down, and properties must
        // not change their base address after registration and resolver
        // creation.
        _samp_rate_in.reserve(get_num_ports());
        _samp_rate_out.reserve(get_num_ports());
        _scaling_in.reserve(get_num_ports());
        _scaling_out.reserve(get_num_ports());
        _decim.reserve(get_num_ports());
        _freq.reserve(get_num_ports());
        _type_in.reserve(get_num_ports());
        _type_out.reserve(get_num_ports());
        for (size_t chan = 0; chan < get_num_ports(); chan++) {
            _register_props(chan);
        }
        register_issue_stream_cmd();
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
        const double input_rate = get_input_rate(chan);
        // TODO add steps
        return uhd::freq_range_t(-input_rate / 2, input_rate / 2);
    }

    double get_input_rate(const size_t chan) const override
    {
        return _samp_rate_in.at(chan).is_valid() ? _samp_rate_in.at(chan).get() : 1.0;
    }

    void set_input_rate(const double rate, const size_t chan) override
    {
        set_property<double>("samp_rate", rate, {res_source_info::INPUT_EDGE, chan});
    }

    double get_output_rate(const size_t chan) const override
    {
        return _samp_rate_out.at(chan).is_valid() ? _samp_rate_out.at(chan).get() : 1.0;
    }

    uhd::meta_range_t get_output_rates(const size_t chan) const override
    {
        uhd::meta_range_t result;
        if (!_samp_rate_in.at(chan).is_valid()) {
            result.push_back(uhd::range_t(1.0));
            return result;
        }
        const double input_rate = _samp_rate_in.at(chan).get();
        // The decimations are stored in order (from smallest to biggest), so
        // iterate in reverse order so we can add rates from smallest to biggest
        for (auto it = _valid_decims.rbegin(); it != _valid_decims.rend(); ++it) {
            result.push_back(uhd::range_t(input_rate / it->start()));
        }
        return result;
    }

    double set_output_rate(const double rate, const size_t chan) override
    {
        if (_samp_rate_in.at(chan).is_valid()) {
            const int coerced_decim = coerce_decim(get_input_rate(chan) / rate);
            set_property<int>("decim", coerced_decim, chan);
        } else {
            RFNOC_LOG_DEBUG("Property samp_rate@"
                            << chan << " is not valid, attempting to set output rate "
                            << (rate / 1e6) << " Msps via the edge property.");
            set_property<double>("samp_rate", rate, {res_source_info::OUTPUT_EDGE, chan});
        }
        return _samp_rate_out.at(chan).get();
    }

    // Somewhat counter-intuitively, we post a stream command as a message to
    // ourselves. That's because it's easier to re-use the message handler than
    // it is to reuse the issue_stream_cmd() API call, because this API call
    // will always be forwarded to the upstream block, whereas the message
    // handler goes both ways.
    // This way, calling issue_stream_cmd() is the same as posting a message to
    // our output port.
    void issue_stream_cmd(const uhd::stream_cmd_t& stream_cmd, const size_t port) override
    {
        RFNOC_LOG_TRACE("issue_stream_cmd(stream_mode=" << char(stream_cmd.stream_mode)
                                                        << ", port=" << port);
        res_source_info dst_edge{res_source_info::OUTPUT_EDGE, port};
        auto new_action        = stream_cmd_action_info::make(stream_cmd.stream_mode);
        new_action->stream_cmd = stream_cmd;
        issue_stream_cmd_action_handler(dst_edge, new_action);
    }

protected:
    //! Block-specific register interface
    multichan_register_iface _ddc_reg_iface;

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
        _decim.push_back(property_t<int>(
            PROP_KEY_DECIM, DEFAULT_DECIM, {res_source_info::USER, chan}));
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
        UHD_ASSERT_THROW(_decim.size() == chan + 1);
        UHD_ASSERT_THROW(_freq.size() == chan + 1);
        UHD_ASSERT_THROW(_type_in.size() == chan + 1);
        UHD_ASSERT_THROW(_type_out.size() == chan + 1);

        // give us some shorthands for the rest of this function
        property_t<double>* samp_rate_in  = &_samp_rate_in.back();
        property_t<double>* samp_rate_out = &_samp_rate_out.back();
        property_t<double>* scaling_in    = &_scaling_in.back();
        property_t<double>* scaling_out   = &_scaling_out.back();
        property_t<int>* decim            = &_decim.back();
        property_t<double>* freq          = &_freq.back();
        property_t<std::string>* type_in  = &_type_in.back();
        property_t<std::string>* type_out = &_type_out.back();

        // register them
        register_property(samp_rate_in);
        register_property(samp_rate_out);
        register_property(scaling_in);
        register_property(scaling_out);
        register_property(decim);
        register_property(freq);
        register_property(type_in);
        register_property(type_out);

        /**********************************************************************
         * Add resolvers
         *********************************************************************/
        // Resolver for _decim: this gets executed when the user directly
        // modifies _decim. the desired behaviour is to coerce it first, then
        // keep the input rate constant, and re-calculate the output rate.
        add_property_resolver({decim},
            {decim, samp_rate_out, samp_rate_in, scaling_in},
            [this,
                chan,
                &decim         = *decim,
                &samp_rate_out = *samp_rate_out,
                &samp_rate_in  = *samp_rate_in,
                &scaling_in    = *scaling_in]() {
                RFNOC_LOG_TRACE("Calling resolver for `decim'@" << chan);
                decim = coerce_decim(double(decim.get()));
                if (decim.is_dirty()) {
                    set_decim(decim.get(), chan);
                }
                if (samp_rate_in.is_valid()) {
                    samp_rate_out = samp_rate_in.get() / decim.get();
                } else if (samp_rate_out.is_valid()) {
                    samp_rate_in = samp_rate_out.get() * decim.get();
                }
                if (scaling_in.is_valid()) {
                    scaling_in.force_dirty();
                }
            });
        // Resolver for _freq: this gets executed when the user directly
        // modifies _freq.
        add_property_resolver(
            {freq}, {freq}, [this, chan, &samp_rate_in = *samp_rate_in, &freq = *freq]() {
                RFNOC_LOG_TRACE("Calling resolver for `freq'@" << chan);
                if (samp_rate_in.is_valid()) {
                    const double new_freq =
                        _set_freq(freq.get(), samp_rate_in.get(), chan);
                    // If the frequency we just set is sufficiently close to the old
                    // frequency, don't bother updating the property in software
                    if (!uhd::math::frequencies_are_equal(new_freq, freq.get())) {
                        freq = new_freq;
                    }
                } else {
                    RFNOC_LOG_DEBUG("Not setting frequency until sampling rate is set.");
                }
            });
        // Resolver for the input rate:
        // If this is called, then most likely, the input sampling rate was set.
        // In that case, we try and keep the output sampling rate as it was, and
        // modify decim to match the input/output ratio. If we can't exactly hit
        // the previous output rate, then we coerce the desired decim to a valid
        // decim value, and update the output rate.
        // Note: This means that if the user set decim explicitly, then this
        // resolver can undo the user's intentions. However, it is the option
        // that retains the consistency of the graph as much as possible, and
        // allows the user to call set_output_rate() on this block before the
        // graph was committed.
        //
        // The scaling is modified in the same resolver to avoid circular
        // dependencies. Note that changing the decimation will change the
        // scaling ratio (input to output scaling), so we need to write to the
        // decimation register in this resolver as well as the decim resolver
        // in order to make sure that _residual_scaling is correctly set.
        // Otherwise, the decim resolver and this resolver would conflict each
        // other when writing to scaling_out.
        //
        // This resolver may affect the frequency: If the input sampling rate
        // was changed, then the phase accumulator increment needs to be
        // recalculated in order to retain the current value of the frequency
        // offset, which is given in Hz (not in radians).
        add_property_resolver({samp_rate_in, scaling_in},
            {decim, samp_rate_out, freq, scaling_out},
            [this,
                chan,
                &decim         = *decim,
                &freq          = *freq,
                &samp_rate_out = *samp_rate_out,
                &samp_rate_in  = *samp_rate_in,
                &scaling_in    = *scaling_in,
                &scaling_out   = *scaling_out]() {
                RFNOC_LOG_TRACE(
                    "Calling resolver for `samp_rate_in/scaling_in'@" << chan);
                if (samp_rate_in.is_valid()) {
                    RFNOC_LOG_TRACE("New samp_rate_in is " << samp_rate_in.get());
                    if (samp_rate_out.is_valid()) {
                        decim = coerce_decim(samp_rate_in.get() / samp_rate_out.get());
                        set_decim(decim.get(), chan);
                        const double new_samp_rate_out = samp_rate_in.get() / decim.get();
                        // Only update the samp_rate_out if the new value is not the same
                        // frequency. However, we still want to call the operator= to make
                        // sure metadata gets handled
                        samp_rate_out = (uhd::math::frequencies_are_equal(
                                            samp_rate_out, new_samp_rate_out))
                                            ? samp_rate_out.get()
                                            : new_samp_rate_out;
                        RFNOC_LOG_TRACE("New samp_rate_out is " << samp_rate_out.get());
                    } else if (decim.is_valid()) {
                        samp_rate_out = samp_rate_in.get() / decim.get();
                    }
                    // If the input rate changes, we need to update the DDS, too,
                    // since it works on frequencies normalized by the input rate.
                    freq.force_dirty();
                }
                if (scaling_in.is_valid()) {
                    scaling_out = scaling_in.get() * _residual_scaling.at(chan);
                }
            });
        // Resolver for the output rate: like the previous one, but flipped.
        add_property_resolver({samp_rate_out, scaling_out},
            {decim, samp_rate_in, scaling_out},
            [this,
                chan,
                &decim         = *decim,
                &samp_rate_out = *samp_rate_out,
                &samp_rate_in  = *samp_rate_in,
                &scaling_in    = *scaling_in,
                &scaling_out   = *scaling_out]() {
                RFNOC_LOG_TRACE(
                    "Calling resolver for `samp_rate_out/scaling_out'@" << chan);
                if (samp_rate_out.is_valid()) {
                    if (samp_rate_in.is_valid()) {
                        decim = coerce_decim(samp_rate_in.get() / samp_rate_out.get());
                        set_decim(decim.get(), chan);
                    }
                    // If decim is dirty, it will trigger the decim resolver.
                    // However, the decim resolver will set the output rate based
                    // on the input rate, so we need to force the input rate first.
                    if (decim.is_dirty()) {
                        const double new_samp_rate_in = samp_rate_out.get() * decim.get();
                        // Only update the samp_rate_in if the new value is not the same
                        // frequency. However, we still want to call the operator= to make
                        // sure metadata gets handled
                        if (samp_rate_in.is_valid()) {
                            samp_rate_in = (uhd::math::frequencies_are_equal(
                                               samp_rate_in, new_samp_rate_in))
                                               ? samp_rate_in.get()
                                               : new_samp_rate_in;
                        } else {
                            samp_rate_in = new_samp_rate_in;
                        }
                        RFNOC_LOG_TRACE("New samp_rate_in is " << samp_rate_in.get());
                    }
                }
                if (scaling_in.is_valid()) {
                    scaling_out = scaling_in.get() * _residual_scaling.at(chan);
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
            if (src.type == res_source_info::OUTPUT_EDGE) {
                new_action->stream_cmd.num_samps *= _decim.at(chan).get();
            } else {
                new_action->stream_cmd.num_samps /= _decim.at(chan).get();
            }
            RFNOC_LOG_TRACE("Forwarding num_samps stream command, new value is "
                            << new_action->stream_cmd.num_samps);
        } else {
            RFNOC_LOG_TRACE("Forwarding continuous stream command...")
        }

        post_action(dst_edge, new_action);
    }

    /**************************************************************************
     * FPGA communication (register IO)
     *************************************************************************/
    /*! Update the decimation value
     *
     * \param decim The new decimation value. It must be valid decimation value.
     * \throws uhd::assertion_error if decim is not valid.
     */
    void set_decim(int decim, const size_t chan)
    {
        RFNOC_LOG_TRACE("Set decim to " << decim);
        // Step 1: Calculate number of halfbands
        uint32_t hb_enable = 0;
        uint32_t cic_decim = decim;
        while ((cic_decim % 2 == 0) and hb_enable < _num_halfbands) {
            hb_enable++;
            cic_decim /= 2;
        }
        // Step 2: Make sure we can handle the rest with the CIC
        UHD_ASSERT_THROW(hb_enable <= _num_halfbands);
        UHD_ASSERT_THROW(cic_decim > 0 and cic_decim <= _cic_max_decim);
        const uint32_t decim_word = (hb_enable << 8) | cic_decim;
        _ddc_reg_iface.poke32(SR_DECIM_ADDR, decim_word, chan);

        // Rate change = M/N
        _ddc_reg_iface.poke32(SR_N_ADDR, decim, chan);
        _ddc_reg_iface.poke32(SR_M_ADDR, 1, chan);

        // Configure time increment in ticks per M output samples
        _ddc_reg_iface.poke32(
            SR_TIME_INCR_ADDR, uint32_t(get_tick_rate() / get_output_rate(chan)), chan);

        if (cic_decim > 1 and hb_enable == 0) {
            RFNOC_LOG_WARNING(
                "The requested decimation is odd; the user should expect passband "
                "CIC rolloff.\n"
                "Select an even decimation to ensure that a halfband filter is "
                "enabled.\n"
                "Decimations factorable by 4 will enable 2 halfbands, those "
                "factorable by 8 will enable 3 halfbands.\n"
                "decimation = dsp_rate/samp_rate -> "
                << decim);
        }

        constexpr double DDS_GAIN = 2.0;
        // Calculate algorithmic gain of CIC for a given decimation.
        // For Ettus CIC R=decim, M=1, N=4. Gain = (R * M) ^ N
        // The Ettus CIC also tries its best to compensate for the gain by
        // shifting the CIC output. This reduces the gain by a factor of
        // 2**ceil(log2(cic_gain))
        const double cic_gain = std::pow(double(cic_decim * 1), 4);
        // DDS gain:
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
        const int32_t actual_factor = std::lround(target_factor);
        // Write DDC with scaling correction for CIC and DDS that maximizes
        // dynamic range
        _ddc_reg_iface.poke32(SR_SCALE_IQ_ADDR, actual_factor, chan);

        // Calculate the error introduced by using fixedpoint representation for
        // the scaler, can be corrected in host later.
        _residual_scaling[chan] = dsp_gain * double(actual_factor) / FIXPOINT_SCALING;
    }

    /*! Return the closest possible decimation value to the one requested
     */
    int coerce_decim(const double requested_decim) const
    {
        UHD_ASSERT_THROW(requested_decim >= 0);
        return static_cast<int>(_valid_decims.clip(requested_decim, true));
    }

    //! Set the DDS frequency shift the signal to \p requested_freq
    double _set_freq(
        const double requested_freq, const double dds_rate, const size_t chan)
    {
        static int freq_word_width = 24;
        double actual_freq;
        int32_t freq_word;
        std::tie(actual_freq, freq_word) =
            get_freq_and_freq_word(requested_freq, dds_rate, freq_word_width);

        // Only the upper 24 bits of the SR_FREQ_ADDR register are used, so shift the word
        freq_word <<= (32 - freq_word_width);

        _ddc_reg_iface.poke32(
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
    //! Max CIC decim
    const size_t _cic_max_decim;

    //! List of valid decimation values
    uhd::meta_range_t _valid_decims;

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
    //! Properties for decim (one per port)
    std::vector<property_t<int>> _decim;
    //! Properties for freq (one per port)
    std::vector<property_t<double>> _freq;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    ddc_block_control, 0xDDC00000, "DDC", CLOCK_KEY_GRAPH, "bus_clk")
