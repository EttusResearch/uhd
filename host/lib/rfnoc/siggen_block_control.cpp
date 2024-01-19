//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/siggen_block_control.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <limits>
#include <string>

using namespace uhd::rfnoc;


// Register offsets
const uint32_t siggen_block_control::REG_BLOCK_SIZE       = 1 << 5;
const uint32_t siggen_block_control::REG_ENABLE_OFFSET    = 0x00;
const uint32_t siggen_block_control::REG_SPP_OFFSET       = 0x04;
const uint32_t siggen_block_control::REG_WAVEFORM_OFFSET  = 0x08;
const uint32_t siggen_block_control::REG_GAIN_OFFSET      = 0x0C;
const uint32_t siggen_block_control::REG_CONSTANT_OFFSET  = 0x10;
const uint32_t siggen_block_control::REG_PHASE_INC_OFFSET = 0x14;
const uint32_t siggen_block_control::REG_CARTESIAN_OFFSET = 0x18;

// User property names
const char* const PROP_KEY_ENABLE         = "enable";
const char* const PROP_KEY_WAVEFORM       = "waveform";
const char* const PROP_KEY_AMPLITUDE      = "amplitude";
const char* const PROP_KEY_CONSTANT_I     = "constant_i";
const char* const PROP_KEY_CONSTANT_Q     = "constant_q";
const char* const PROP_KEY_SINE_PHASE_INC = "sine_phase_increment";

namespace {
template <class T>
const T clamp(const double v)
{
    constexpr T min_t = std::numeric_limits<T>::min();
    constexpr T max_t = std::numeric_limits<T>::max();
    return (v < min_t) ? min_t : (v > max_t) ? max_t : T(v);
}
} // namespace

class siggen_block_control_impl : public siggen_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(siggen_block_control),
        _siggen_reg_iface(*this, 0, REG_BLOCK_SIZE)
    {
        _register_props();
    }

    void set_enable(const bool enable, const size_t port) override
    {
        set_property<bool>(PROP_KEY_ENABLE, enable, port);
    }

    bool get_enable(const size_t port) const override
    {
        return _prop_enable.at(port).get();
    }

    void set_waveform(const siggen_waveform waveform, const size_t port) override
    {
        set_property<int>(PROP_KEY_WAVEFORM, static_cast<int>(waveform), port);
    }

    siggen_waveform get_waveform(const size_t port) const override
    {
        return static_cast<siggen_waveform>(_prop_waveform.at(port).get());
    }

    void set_amplitude(const double amplitude, const size_t port) override
    {
        set_property<double>(PROP_KEY_AMPLITUDE, amplitude, port);
    }

    double get_amplitude(const size_t port) const override
    {
        return _prop_amplitude.at(port).get();
    }

    void set_constant(const std::complex<double> constant, const size_t port) override
    {
        set_property<double>(PROP_KEY_CONSTANT_I, constant.real(), port);
        set_property<double>(PROP_KEY_CONSTANT_Q, constant.imag(), port);
    }

    std::complex<double> get_constant(const size_t port) const override
    {
        return std::complex<double>(
            _prop_constant_i.at(port).get(), _prop_constant_q.at(port).get());
    }

    void set_sine_phase_increment(const double phase_inc, const size_t port) override
    {
        set_property<double>(PROP_KEY_SINE_PHASE_INC, phase_inc, port);
    }

    double get_sine_phase_increment(const size_t port) const override
    {
        return _prop_phase_inc.at(port).get();
    }

    void set_samples_per_packet(const size_t spp, const size_t port) override
    {
        set_property<int>(PROP_KEY_SPP, uhd::narrow_cast<int>(spp), port);
    }

    size_t get_samples_per_packet(const size_t port) const override
    {
        return _prop_spp.at(port).get();
    }

    /**************************************************************************
     * Initialization
     *************************************************************************/
private:
    void _register_props()
    {
        const size_t num_outputs = get_num_output_ports();
        _prop_enable.reserve(num_outputs);
        _prop_waveform.reserve(num_outputs);
        _prop_amplitude.reserve(num_outputs);
        _prop_constant_i.reserve(num_outputs);
        _prop_constant_q.reserve(num_outputs);
        _prop_phase_inc.reserve(num_outputs);
        _prop_spp.reserve(num_outputs);
        _prop_type_out.reserve(num_outputs);

        for (size_t port = 0; port < num_outputs; port++) {
            // register edge properties
            _prop_type_out.emplace_back(property_t<std::string>{
                PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE, port}});
            register_property(&_prop_type_out.back());

            // register user properties
            _prop_enable.emplace_back(
                property_t<bool>{PROP_KEY_ENABLE, false, {res_source_info::USER, port}});
            _prop_waveform.emplace_back(property_t<int>{PROP_KEY_WAVEFORM,
                static_cast<int>(siggen_waveform::CONSTANT),
                {res_source_info::USER, port}});
            _prop_amplitude.emplace_back(property_t<double>{
                PROP_KEY_AMPLITUDE, 1.0, {res_source_info::USER, port}});
            _prop_constant_i.emplace_back(property_t<double>{
                PROP_KEY_CONSTANT_I, 1.0, {res_source_info::USER, port}});
            _prop_constant_q.emplace_back(property_t<double>{
                PROP_KEY_CONSTANT_Q, 1.0, {res_source_info::USER, port}});
            _prop_phase_inc.emplace_back(property_t<double>{
                PROP_KEY_SINE_PHASE_INC, 1.0, {res_source_info::USER, port}});
            const int default_spp =
                static_cast<int>(
                    get_max_payload_size({res_source_info::OUTPUT_EDGE, port}))
                / uhd::convert::get_bytes_per_item(_prop_type_out.at(port).get());
            _prop_spp.emplace_back(property_t<int>{
                PROP_KEY_SPP, default_spp, {res_source_info::USER, port}});
            register_property(&_prop_enable.back(), [this, port]() {
                _siggen_reg_iface.poke32(REG_ENABLE_OFFSET,
                    uint32_t(_prop_enable.at(port).get() ? 1 : 0),
                    port);
            });
            register_property(&_prop_waveform.back());
            register_property(&_prop_amplitude.back());
            register_property(&_prop_constant_i.back(), [this, port]() {
                const double constant_i = _prop_constant_i.at(port).get();
                if (constant_i < -1.0 || constant_i > 1.0) {
                    throw uhd::value_error("Constant real value must be in [-1.0, 1.0]");
                }
                _set_constant_register(port);
            });
            register_property(&_prop_constant_q.back(), [this, port]() {
                const double constant_q = _prop_constant_q.at(port).get();
                if (constant_q < -1.0 || constant_q > 1.0) {
                    throw uhd::value_error(
                        "Constant imaginary value must be in [-1.0, 1.0]");
                }
                _set_constant_register(port);
            });
            register_property(&_prop_phase_inc.back(), [this, port]() {
                const double phase_inc = _prop_phase_inc.at(port).get();
                if (phase_inc < (-uhd::math::PI) || phase_inc > (uhd::math::PI)) {
                    throw uhd::value_error("Phase increment value must be in [-pi, pi]");
                }
                const int16_t phase_inc_scaled_rads_fp =
                    clamp<int16_t>((phase_inc / uhd::math::PI) * 8192.0);
                _siggen_reg_iface.poke32(
                    REG_PHASE_INC_OFFSET, phase_inc_scaled_rads_fp & 0xffff, port);
            });
            register_property(&_prop_spp.back(), [this, port]() {
                const uint32_t spp = _prop_spp.at(port).get();
                RFNOC_LOG_TRACE(
                    "Setting samples per packet to " << spp << " on port " << port);
                _siggen_reg_iface.poke32(REG_SPP_OFFSET, spp, port);
            });

            add_property_resolver({&_prop_waveform.back(), &_prop_amplitude.back()},
                {&_prop_amplitude.back()},
                [this, port]() {
                    // Range check the waveform and amplitude properties.
                    // If either are out of range, throw an exception and
                    // do not set any registers.
                    const int waveform_val = _prop_waveform.at(port).get();
                    const int low_limit    = static_cast<int>(siggen_waveform::CONSTANT);
                    const int high_limit   = static_cast<int>(siggen_waveform::NOISE);
                    if (waveform_val < low_limit || waveform_val > high_limit) {
                        throw uhd::value_error("Waveform value must be in ["
                                               + std::to_string(low_limit) + ", "
                                               + std::to_string(high_limit) + "]");
                    }
                    const double amplitude = _prop_amplitude.at(port).get();
                    if (amplitude < 0.0 || amplitude > 1.0) {
                        throw uhd::value_error("Amplitude value must be in [0.0, 1.0]");
                    }

                    // Set the waveform register appropriately.
                    _siggen_reg_iface.poke32(REG_WAVEFORM_OFFSET, waveform_val, port);

                    // Now set the other registers based on the waveform and
                    // the desired amplitude.
                    siggen_waveform waveform = static_cast<siggen_waveform>(waveform_val);
                    switch (waveform) {
                        case siggen_waveform::CONSTANT:
                            // The amplitude is fixed at 1 in constant mode.
                            _prop_amplitude.at(port).set(1.0);
                            _set_gain_register(1.0, port);
                            break;
                        case siggen_waveform::SINE_WAVE: {
                            // Set the phasor to the appropriate amplitude value and
                            // fix the gain to 1.

                            // The CORDIC IP scales the value written to the Cartesian
                            // coordinate register (i.e., the phasor that is rotated to
                            // generate the sinusoid) by this value, so we pre-scale the
                            // input value before writing. See the comment in the
                            // rfnoc_block_siggen_regs.vh header file for the derivation
                            // of this value.
                            constexpr double cordic_scale_value = 1.164435344782938;
                            _set_cartesian_register(amplitude / cordic_scale_value, port);
                            _set_gain_register(1.0, port);
                            break;
                        }
                        case siggen_waveform::NOISE:
                            // Use the gain register to set the gain of the random noise
                            // signal.
                            _set_gain_register(amplitude, port);
                            break;
                    }
                });

            add_property_resolver(
                {&_prop_spp.back(),
                    get_mtu_prop_ref({res_source_info::OUTPUT_EDGE, port})},
                {&_prop_spp.back()},
                [this, port]() {
                    int spp               = _prop_spp.at(port).get();
                    const int max_payload = static_cast<int>(
                        get_max_payload_size({res_source_info::OUTPUT_EDGE, port}));
                    const int max_samps =
                        max_payload
                        / uhd::convert::get_bytes_per_item(_prop_type_out.at(port).get());
                    if (spp > max_samps) {
                        RFNOC_LOG_WARNING("spp value " << spp << " exceeds MTU of "
                                                       << max_payload << "! Coercing to "
                                                       << max_samps);
                        spp = max_samps;
                    }
                    if (spp <= 0) {
                        spp = max_samps;
                        RFNOC_LOG_WARNING(
                            "spp must be greater than zero! Coercing to " << spp);
                    }
                    _prop_spp.at(port).set(spp);
                });

            // add resolver for type
            add_property_resolver({&_prop_type_out.back()},
                {&_prop_type_out.back()},
                [this, port]() { _prop_type_out.at(port).set(IO_TYPE_SC16); });
        }
    }

    void _set_constant_register(const size_t port)
    {
        const int16_t constant_i_fp =
            clamp<int16_t>(_prop_constant_i.at(port).get() * 32768.0);
        const int16_t constant_q_fp =
            clamp<int16_t>(_prop_constant_q.at(port).get() * 32768.0);
        const uint32_t constant_reg_value = (uint32_t(constant_i_fp) << 16)
                                            | (uint32_t(constant_q_fp) & 0xffff);

        _siggen_reg_iface.poke32(REG_CONSTANT_OFFSET, constant_reg_value, port);
    }

    void _set_gain_register(const double gain, const size_t port)
    {
        const int16_t gain_fp = clamp<int16_t>(gain * 32768.0);
        _siggen_reg_iface.poke32(REG_GAIN_OFFSET, gain_fp, port);
    }

    void _set_cartesian_register(const double amplitude, const size_t port)
    {
        // The rotator that rotates the phasor to generate the sinusoidal
        // data has an initial phase offset which is impossible to predict.
        // Thus, the Cartesian parameter is largely immaterial, as long as
        // the phasor's amplitude matches what the client has specified.
        // For simplicity, the Cartesian parameter is chosen to have a real
        // (X) component of the desired amplitude and an imaginary (Y)
        // component of 0.0.
        const int16_t cartesian_x_fp = clamp<int16_t>(amplitude * 32767.0);

        // Bits 31:16 represent the real component (the pre-scaled fixed-point
        // amplitude), while bits 15:0 represent the imaginary component (which
        // is zeroed).
        const uint32_t cartesian_reg_value = (uint32_t(cartesian_x_fp) << 16);
        _siggen_reg_iface.poke32(REG_CARTESIAN_OFFSET, cartesian_reg_value, port);
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    std::vector<property_t<bool>> _prop_enable;
    std::vector<property_t<int>> _prop_waveform;
    std::vector<property_t<double>> _prop_amplitude;
    std::vector<property_t<double>> _prop_constant_i;
    std::vector<property_t<double>> _prop_constant_q;
    std::vector<property_t<double>> _prop_phase_inc;
    std::vector<property_t<int>> _prop_spp;
    std::vector<property_t<std::string>> _prop_type_out;

    /**************************************************************************
     * Register interface
     *************************************************************************/
    multichan_register_iface _siggen_reg_iface;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    siggen_block_control, SIGGEN_BLOCK, "SigGen", CLOCK_KEY_GRAPH, "bus_clk")
