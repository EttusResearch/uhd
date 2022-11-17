//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/fosphor_block_control.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <string>

using namespace uhd::rfnoc;


// Register offsets
const uint32_t fosphor_block_control::REG_ENABLE_ADDR   = 0x00;
const uint32_t fosphor_block_control::REG_CLEAR_ADDR    = 0x04;
const uint32_t fosphor_block_control::REG_RANDOM_ADDR   = 0x08;
const uint32_t fosphor_block_control::REG_DECIM_ADDR    = 0x0c;
const uint32_t fosphor_block_control::REG_OFFSET_ADDR   = 0x10;
const uint32_t fosphor_block_control::REG_SCALE_ADDR    = 0x14;
const uint32_t fosphor_block_control::REG_TRISE_ADDR    = 0x18;
const uint32_t fosphor_block_control::REG_TDECAY_ADDR   = 0x1c;
const uint32_t fosphor_block_control::REG_ALPHA_ADDR    = 0x20;
const uint32_t fosphor_block_control::REG_EPSILON_ADDR  = 0x24;
const uint32_t fosphor_block_control::REG_WF_CTRL_ADDR  = 0x28;
const uint32_t fosphor_block_control::REG_WF_DECIM_ADDR = 0x2c;

// Mask bits
constexpr uint32_t RESET_HISTORY_BIT  = 0;
constexpr uint32_t RESET_HISTORY_MASK = (1 << RESET_HISTORY_BIT);
constexpr uint32_t RESET_CORE_BIT     = 1;
constexpr uint32_t RESET_CORE_MASK    = (1 << RESET_CORE_BIT);

constexpr uint32_t HISTOGRAM_ENABLE_BIT  = 0;
constexpr uint32_t HISTOGRAM_ENABLE_MASK = (1 << HISTOGRAM_ENABLE_BIT);
constexpr uint32_t WATERFALL_ENABLE_BIT  = 1;
constexpr uint32_t WATERFALL_ENABLE_MASK = (1 << WATERFALL_ENABLE_BIT);

constexpr uint32_t DITHER_ENABLE_BIT  = 0;
constexpr uint32_t DITHER_ENABLE_MASK = (1 << DITHER_ENABLE_BIT);
constexpr uint32_t NOISE_ENABLE_BIT   = 1;
constexpr uint32_t NOISE_ENABLE_MASK  = (1 << NOISE_ENABLE_BIT);

constexpr uint32_t WATERFALL_MODE_BIT  = 7;
constexpr uint32_t WATERFALL_MODE_MASK = (1 << WATERFALL_MODE_BIT);
constexpr uint32_t PREDIV_RATIO_MASK   = (1 << 0) | (1 << 1);

// User property names
const char* const PROP_KEY_ENABLE_HISTOGRAM     = "enable_histogram";
const char* const PROP_KEY_ENABLE_WATERFALL     = "enable_waterfall";
const char* const PROP_KEY_CLEAR_HISTORY        = "clear_history";
const char* const PROP_KEY_ENABLE_DITHER        = "enable_dither";
const char* const PROP_KEY_ENABLE_NOISE         = "enable_noise";
const char* const PROP_KEY_HIST_DECIMATION      = "hist_decimation";
const char* const PROP_KEY_OFFSET               = "offset";
const char* const PROP_KEY_SCALE                = "scale";
const char* const PROP_KEY_RISE_TIME            = "trise";
const char* const PROP_KEY_DECAY_TIME           = "tdecay";
const char* const PROP_KEY_ALPHA                = "alpha";
const char* const PROP_KEY_EPSILON              = "epsilon";
const char* const PROP_KEY_WF_PREDIVISION_RATIO = "wf_predivision_ratio";
const char* const PROP_KEY_WF_MODE              = "wf_mode";
const char* const PROP_KEY_WF_DECIMATION        = "wf_decimation";

// Edge property details
constexpr uint32_t HISTOGRAM_PORT = 0;
constexpr uint32_t WATERFALL_PORT = 1;


class fosphor_block_control_impl : public fosphor_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(fosphor_block_control)
    {
        // reset the core upon block construction
        this->regs().poke32(REG_CLEAR_ADDR, RESET_CORE_MASK);
        _register_props();
    }

    void set_enable_histogram(const bool enable_histogram) override
    {
        set_property<bool>(PROP_KEY_ENABLE_HISTOGRAM, enable_histogram);
    }

    bool get_enable_histogram() const override
    {
        return _prop_enable_histogram.get();
    }

    void set_enable_waterfall(const bool enable_waterfall) override
    {
        set_property<bool>(PROP_KEY_ENABLE_WATERFALL, enable_waterfall);
    }

    bool get_enable_waterfall() const override
    {
        return _prop_enable_waterfall.get();
    }

    void clear_history() override
    {
        set_property<bool>(PROP_KEY_CLEAR_HISTORY, true);
    }

    void set_enable_dither(const bool enable_dither) override
    {
        set_property<bool>(PROP_KEY_ENABLE_DITHER, enable_dither);
    }

    bool get_enable_dither() const override
    {
        return _prop_enable_dither.get();
    }

    void set_enable_noise(const bool enable_noise) override
    {
        set_property<bool>(PROP_KEY_ENABLE_NOISE, enable_noise);
    }

    bool get_enable_noise() const override
    {
        return _prop_enable_noise.get();
    }

    void set_histogram_decimation(const uint16_t decimation) override
    {
        set_property<int>(PROP_KEY_HIST_DECIMATION, decimation);
    }

    uint16_t get_histogram_decimation() const override
    {
        return _prop_hist_decimation.get();
    }

    void set_histogram_offset(const uint16_t offset) override
    {
        set_property<int>(PROP_KEY_OFFSET, offset);
    }

    uint16_t get_histogram_offset() const override
    {
        return _prop_offset.get();
    }

    void set_histogram_scale(const uint16_t scale) override
    {
        set_property<int>(PROP_KEY_SCALE, scale);
    }

    uint16_t get_histogram_scale() const override
    {
        return _prop_scale.get();
    }

    void set_histogram_rise_rate(const uint16_t rise_rate) override
    {
        set_property<int>(PROP_KEY_RISE_TIME, rise_rate);
    }

    uint16_t get_histogram_rise_rate() const override
    {
        return _prop_trise.get();
    }

    void set_histogram_decay_rate(const uint16_t decay_rate) override
    {
        set_property<int>(PROP_KEY_DECAY_TIME, decay_rate);
    }

    uint16_t get_histogram_decay_rate() const override
    {
        return _prop_tdecay.get();
    }

    void set_spectrum_alpha(const uint16_t alpha) override
    {
        set_property<int>(PROP_KEY_ALPHA, alpha);
    }

    uint16_t get_spectrum_alpha() const override
    {
        return _prop_alpha.get();
    }

    void set_spectrum_max_hold_decay(const uint16_t epsilon) override
    {
        set_property<int>(PROP_KEY_EPSILON, epsilon);
    }

    uint16_t get_spectrum_max_hold_decay() const override
    {
        return _prop_epsilon.get();
    }

    void set_waterfall_predivision(
        const fosphor_waterfall_predivision_ratio waterfall_predivision) override
    {
        set_property<int>(
            PROP_KEY_WF_PREDIVISION_RATIO, static_cast<int>(waterfall_predivision));
    }

    fosphor_waterfall_predivision_ratio get_waterfall_predivision() const override
    {
        return static_cast<fosphor_waterfall_predivision_ratio>(
            _prop_wf_prediv_ratio.get());
    }

    void set_waterfall_mode(const fosphor_waterfall_mode waterfall_mode) override
    {
        set_property<int>(PROP_KEY_WF_MODE, static_cast<int>(waterfall_mode));
    }

    fosphor_waterfall_mode get_waterfall_mode() const override
    {
        return static_cast<fosphor_waterfall_mode>(_prop_wf_mode.get());
    }

    void set_waterfall_decimation(const uint16_t waterfall_decimation) override
    {
        set_property<int>(PROP_KEY_WF_DECIMATION, waterfall_decimation);
    }

    uint16_t get_waterfall_decimation() const override
    {
        return _prop_wf_decim.get();
    }

    /**************************************************************************
     * Initialization
     *************************************************************************/
private:
    void _register_props()
    {
        // register user properties
        register_property(&_prop_enable_histogram, [this]() { _program_enables(); });
        register_property(&_prop_enable_waterfall, [this]() { _program_enables(); });
        register_property(&_prop_clear_history,
            [this]() { this->regs().poke32(REG_CLEAR_ADDR, RESET_HISTORY_MASK); });
        register_property(
            &_prop_enable_dither, [this]() { _program_randomness_enables(); });
        register_property(
            &_prop_enable_noise, [this]() { _program_randomness_enables(); });
        register_property(&_prop_hist_decimation, [this]() {
            int decim = _prop_hist_decimation.get();
            if (decim < 2 || decim > 1024) {
                throw uhd::value_error("Histogram decimation value must be in [2, 1024]");
            }
            this->regs().poke32(REG_DECIM_ADDR, uint32_t(decim - 2));
        });
        register_property(&_prop_offset, [this]() {
            int offset = _prop_offset.get();
            if (offset < 0 || offset > 65535) {
                throw uhd::value_error("Offset value must be in [0, 65535]");
            }
            this->regs().poke32(REG_OFFSET_ADDR, uint32_t(offset));
        });
        register_property(&_prop_scale, [this]() {
            int scale = _prop_scale.get();
            if (scale < 0 || scale > 65535) {
                throw uhd::value_error("Scale value must be in [0, 65535]");
            }
            this->regs().poke32(REG_SCALE_ADDR, uint32_t(scale));
        });
        register_property(&_prop_trise, [this]() {
            int trise = _prop_trise.get();
            if (trise < 0 || trise > 65535) {
                throw uhd::value_error("Rise rate value must be in [0, 65535]");
            }
            this->regs().poke32(REG_TRISE_ADDR, uint32_t(trise));
        });
        register_property(&_prop_tdecay, [this]() {
            int tdecay = _prop_tdecay.get();
            if (tdecay < 0 || tdecay > 65535) {
                throw uhd::value_error("Decay rate value must be in [0, 65535]");
            }
            this->regs().poke32(REG_TDECAY_ADDR, uint32_t(tdecay));
        });
        register_property(&_prop_alpha, [this]() {
            int alpha = _prop_alpha.get();
            if (alpha < 0 || alpha > 65535) {
                throw uhd::value_error("Alpha value must be in [0, 65535]");
            }
            this->regs().poke32(REG_ALPHA_ADDR, uint32_t(alpha));
        });
        register_property(&_prop_epsilon, [this]() {
            int epsilon = _prop_epsilon.get();
            if (epsilon < 0 || epsilon > 65535) {
                throw uhd::value_error("Max hold decay rate must be in [0, 65535]");
            }
            this->regs().poke32(REG_EPSILON_ADDR, uint32_t(epsilon));
        });
        register_property(&_prop_wf_prediv_ratio, [this]() {
            int prediv_ratio = _prop_wf_prediv_ratio.get();
            if (prediv_ratio
                    < static_cast<int>(fosphor_waterfall_predivision_ratio::RATIO_1_1)
                || prediv_ratio > static_cast<int>(
                       fosphor_waterfall_predivision_ratio::RATIO_1_256)) {
                throw uhd::value_error(
                    "Waterfall predivision ratio value must be in [0, 3]");
            }
            _program_waterfall_mode();
        });
        register_property(&_prop_wf_mode, [this]() {
            int wf_mode = _prop_wf_mode.get();
            if (wf_mode < static_cast<int>(fosphor_waterfall_mode::MAX_HOLD)
                || wf_mode > static_cast<int>(fosphor_waterfall_mode::AVERAGE)) {
                throw uhd::value_error("Waterfall mode value must be 0 or 1");
            }
            _program_waterfall_mode();
        });
        register_property(&_prop_wf_decim, [this]() {
            int wf_decim = _prop_wf_decim.get();
            if (wf_decim < 2 || wf_decim > 257) {
                throw uhd::value_error("Waterfall decimation value must be in [2, 257]");
            }
            this->regs().poke32(REG_WF_DECIM_ADDR, uint32_t(wf_decim - 2));
        });

        // register edge properties
        register_property(&_prop_type_in);
        register_property(&_prop_type_out_histogram);
        register_property(&_prop_type_out_wf);

        // add resolvers for type
        add_property_resolver({&_prop_type_in}, {&_prop_type_in}, [this]() {
            _prop_type_in.set(IO_TYPE_SC16);
        });
        add_property_resolver({&_prop_type_out_histogram},
            {&_prop_type_out_histogram},
            [this]() { _prop_type_out_histogram.set(IO_TYPE_U8); });
        add_property_resolver({&_prop_type_out_wf}, {&_prop_type_out_wf}, [this]() {
            _prop_type_out_wf.set(IO_TYPE_U8);
        });
    }

    void _program_enables()
    {
        uint32_t reg_value = this->regs().peek32(REG_ENABLE_ADDR)
                             & ~(HISTOGRAM_ENABLE_MASK | WATERFALL_ENABLE_MASK);
        uint32_t histogram_enable_bit =
            (_prop_enable_histogram.get()) ? HISTOGRAM_ENABLE_MASK : 0;
        uint32_t waterfall_enable_bit =
            (_prop_enable_waterfall.get()) ? WATERFALL_ENABLE_MASK : 0;
        this->regs().poke32(
            REG_ENABLE_ADDR, reg_value | histogram_enable_bit | waterfall_enable_bit);
    }

    void _program_randomness_enables()
    {
        uint32_t reg_value = this->regs().peek32(REG_RANDOM_ADDR)
                             & ~(DITHER_ENABLE_MASK | NOISE_ENABLE_MASK);
        uint32_t dither_enable_bit = (_prop_enable_dither.get()) ? DITHER_ENABLE_MASK : 0;
        uint32_t noise_enable_bit  = (_prop_enable_noise.get()) ? NOISE_ENABLE_MASK : 0;
        this->regs().poke32(
            REG_RANDOM_ADDR, reg_value | dither_enable_bit | noise_enable_bit);
    }

    void _program_waterfall_mode()
    {
        uint32_t reg_value = this->regs().peek32(REG_WF_CTRL_ADDR)
                             & ~(WATERFALL_MODE_MASK | PREDIV_RATIO_MASK);
        int prediv_ratio      = _prop_wf_prediv_ratio.get();
        int wf_mode           = _prop_wf_mode.get();
        uint32_t wf_mode_bits = (wf_mode << WATERFALL_MODE_BIT) | prediv_ratio;
        this->regs().poke32(REG_WF_CTRL_ADDR, reg_value | wf_mode_bits);
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    property_t<std::string> _prop_type_in = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE}};
    property_t<std::string> _prop_type_out_histogram = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_U8, {res_source_info::OUTPUT_EDGE, HISTOGRAM_PORT}};
    property_t<std::string> _prop_type_out_wf = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_U8, {res_source_info::OUTPUT_EDGE, WATERFALL_PORT}};

    property_t<bool> _prop_enable_histogram =
        property_t<bool>{PROP_KEY_ENABLE_HISTOGRAM, true, {res_source_info::USER}};
    property_t<bool> _prop_enable_waterfall =
        property_t<bool>{PROP_KEY_ENABLE_WATERFALL, true, {res_source_info::USER}};
    property_t<bool> _prop_clear_history =
        property_t<bool>{PROP_KEY_CLEAR_HISTORY, false, {res_source_info::USER}};
    property_t<bool> _prop_enable_dither =
        property_t<bool>{PROP_KEY_ENABLE_DITHER, true, {res_source_info::USER}};
    property_t<bool> _prop_enable_noise =
        property_t<bool>{PROP_KEY_ENABLE_NOISE, true, {res_source_info::USER}};
    property_t<int> _prop_hist_decimation =
        property_t<int>{PROP_KEY_HIST_DECIMATION, 2, {res_source_info::USER}};
    property_t<int> _prop_offset =
        property_t<int>{PROP_KEY_OFFSET, 0, {res_source_info::USER}};
    property_t<int> _prop_scale =
        property_t<int>{PROP_KEY_SCALE, 256, {res_source_info::USER}};
    property_t<int> _prop_trise =
        property_t<int>{PROP_KEY_RISE_TIME, 4096, {res_source_info::USER}};
    property_t<int> _prop_tdecay =
        property_t<int>{PROP_KEY_DECAY_TIME, 16384, {res_source_info::USER}};
    property_t<int> _prop_alpha =
        property_t<int>{PROP_KEY_ALPHA, 65280, {res_source_info::USER}};
    property_t<int> _prop_epsilon =
        property_t<int>{PROP_KEY_EPSILON, 1, {res_source_info::USER}};
    property_t<int> _prop_wf_prediv_ratio = property_t<int>{PROP_KEY_WF_PREDIVISION_RATIO,
        static_cast<int>(fosphor_waterfall_predivision_ratio::RATIO_1_1),
        {res_source_info::USER}};
    property_t<int> _prop_wf_mode         = property_t<int>{PROP_KEY_WF_MODE,
                static_cast<int>(fosphor_waterfall_mode::MAX_HOLD),
                {res_source_info::USER}};
    property_t<int> _prop_wf_decim =
        property_t<int>{PROP_KEY_WF_DECIMATION, 8, {res_source_info::USER}};
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    fosphor_block_control, FOSPHOR_BLOCK, "Fosphor", CLOCK_KEY_GRAPH, "bus_clk")
