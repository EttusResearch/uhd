//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/fft_block_control.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <string>

using namespace uhd::rfnoc;

namespace {

constexpr int DEFAULT_LENGTH              = 256;
constexpr fft_shift DEFAULT_SHIFT         = fft_shift::NORMAL;
constexpr fft_direction DEFAULT_DIRECTION = fft_direction::FORWARD;
constexpr fft_magnitude DEFAULT_MAGNITUDE = fft_magnitude::COMPLEX;
constexpr int DEFAULT_FFT_SCALING         = 1706; // Conservative 1/N scaling

// FFT IP constraints
constexpr int MIN_FFT_LENGTH = 8;
constexpr int MAX_FFT_LENGTH = 1024;


const uhd::rfnoc::io_type_t DEFAULT_TYPE = uhd::rfnoc::IO_TYPE_SC16;
// As long as we can only do sc16 in this block, we skip deriving the
// byte-per-sample value from elsewhere
constexpr size_t BYTES_PER_SAMPLE = 4;

} // namespace


const uint32_t fft_block_control::REG_RESET_ADDR         = 131 * 8;
const uint32_t fft_block_control::REG_LENGTH_LOG2_ADDR   = 132 * 8;
const uint32_t fft_block_control::REG_MAGNITUDE_OUT_ADDR = 133 * 8;
const uint32_t fft_block_control::REG_DIRECTION_ADDR     = 134 * 8;
const uint32_t fft_block_control::REG_SCALING_ADDR       = 135 * 8;
const uint32_t fft_block_control::REG_SHIFT_CONFIG_ADDR  = 136 * 8;

class fft_block_control_impl : public fft_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(fft_block_control)
    {
        set_prop_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        set_action_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        _reset();
        _register_props();
    }

    void set_direction(const fft_direction direction) override
    {
        set_property<int>(PROP_KEY_DIRECTION, static_cast<int>(direction));
    }

    fft_direction get_direction() const override
    {
        return static_cast<fft_direction>(_direction.get());
    }

    void set_magnitude(const fft_magnitude magnitude) override
    {
        set_property<int>(PROP_KEY_MAGNITUDE, static_cast<int>(magnitude));
    }

    fft_magnitude get_magnitude() const override
    {
        return static_cast<fft_magnitude>(_magnitude.get());
    }

    void set_shift_config(const fft_shift shift) override
    {
        set_property<int>(PROP_KEY_SHIFT_CONFIG, static_cast<int>(shift));
    }

    fft_shift get_shift_config() const override
    {
        return static_cast<fft_shift>(_shift.get());
    }

    void set_scaling(const uint16_t scaling) override
    {
        set_property<int>(PROP_KEY_FFT_SCALING, scaling);
    }

    uint16_t get_scaling() const override
    {
        return static_cast<uint16_t>(_scaling.get());
    }

    void set_length(const size_t size) override
    {
        set_property<int>(PROP_KEY_LENGTH, size);
    }

    size_t get_length() const override
    {
        return static_cast<size_t>(_length.get());
    }


private:
    void _reset()
    {
        regs().poke32(REG_RESET_ADDR, uint32_t(1));
        regs().poke32(REG_RESET_ADDR, uint32_t(0));
    }

    /**************************************************************************
     * Initialization
     *************************************************************************/
    void _register_props()
    {
        // register block specific properties
        register_property(&_length);
        add_property_resolver({&_length}, {&_length}, [this]() {
            size_t length = this->_length.get();
            if (length < MIN_FFT_LENGTH || length > MAX_FFT_LENGTH) {
                throw uhd::value_error("Size value must be in ["
                                       + std::to_string(MIN_FFT_LENGTH) + ", "
                                       + std::to_string(MAX_FFT_LENGTH) + "]");
            }
            // Find the log2(length) via highest bit set
            size_t length_log2 = 0;
            size_t old_length  = length;
            while ((length >>= 1) != 0) {
                length_log2++;
            }
            size_t coerced_length = (1 << length_log2);
            if (old_length != coerced_length) {
                RFNOC_LOG_WARNING("Length "
                                  << old_length
                                  << " not an integral power of two; coercing to "
                                  << coerced_length);
                this->_length.set(coerced_length);
            }
            RFNOC_LOG_TRACE("Setting FFT length to " << coerced_length);
            this->regs().poke32(REG_LENGTH_LOG2_ADDR, uint32_t(length_log2));
        });

        register_property(&_magnitude, [this]() {
            int mag = this->_magnitude.get();
            if (mag < static_cast<int>(fft_magnitude::COMPLEX)
                || mag > static_cast<int>(fft_magnitude::MAGNITUDE_SQUARED)) {
                throw uhd::value_error("Magnitude value must be [0, 2]");
            }
            this->regs().poke32(REG_MAGNITUDE_OUT_ADDR, uint32_t(mag));
        });
        register_property(&_direction, [this]() {
            int dir = _direction.get();
            if (dir < static_cast<int>(fft_direction::REVERSE)
                || dir > static_cast<int>(fft_direction::FORWARD)) {
                throw uhd::value_error("Direction value must be in [0, 1]");
            }
            this->regs().poke32(REG_DIRECTION_ADDR, uint32_t(dir));
        });
        register_property(&_scaling, [this]() {
            int scale = _scaling.get();
            if (scale < 0 || scale > (1 << 12) - 1) {
                throw uhd::value_error("Scale value must be in [0, 4095]");
            }
            this->regs().poke32(REG_SCALING_ADDR, uint32_t(scale));
        });
        register_property(&_shift, [this]() {
            int shift = this->_shift.get();
            if (shift < static_cast<int>(fft_shift::NORMAL)
                || shift > static_cast<int>(fft_shift::NATURAL)) {
                throw uhd::value_error("Shift value must be [0, 2]");
            }
            this->regs().poke32(REG_SHIFT_CONFIG_ADDR, uint32_t(shift));
        });

        // Add resolvers & properties for atomic item size
        register_property(&_atomic_item_size_in);
        register_property(&_atomic_item_size_out);
        add_property_resolver(
            {&_length, &_atomic_item_size_in}, {&_atomic_item_size_in}, [this]() {
                _atomic_item_size_in = _length.get() * BYTES_PER_SAMPLE;
                RFNOC_LOG_TRACE(
                    "Setting atomic item size in to " << _atomic_item_size_in.get());
            });
        add_property_resolver(
            {&_length, &_atomic_item_size_out}, {&_atomic_item_size_out}, [this]() {
                _atomic_item_size_out = _length.get() * BYTES_PER_SAMPLE;
                RFNOC_LOG_TRACE(
                    "Setting atomic item size out to " << _atomic_item_size_out.get());
            });

        // register edge properties
        register_property(&_type_in);
        register_property(&_type_out);
        // add resolvers for type (keeps it constant)
        add_property_resolver({&_type_in}, {&_type_in}, [& type_in = _type_in]() {
            type_in.set(IO_TYPE_SC16);
        });
        add_property_resolver({&_type_out}, {&_type_out}, [& type_out = _type_out]() {
            type_out.set(IO_TYPE_SC16);
        });
    }

    property_t<int> _length{PROP_KEY_LENGTH, DEFAULT_LENGTH, {res_source_info::USER}};
    property_t<int> _magnitude = property_t<int>{
        PROP_KEY_MAGNITUDE, static_cast<int>(DEFAULT_MAGNITUDE), {res_source_info::USER}};
    property_t<int> _direction = property_t<int>{
        PROP_KEY_DIRECTION, static_cast<int>(DEFAULT_DIRECTION), {res_source_info::USER}};
    property_t<int> _scaling = property_t<int>{
        PROP_KEY_FFT_SCALING, DEFAULT_FFT_SCALING, {res_source_info::USER}};
    property_t<int> _shift = property_t<int>{
        PROP_KEY_SHIFT_CONFIG, static_cast<int>(DEFAULT_SHIFT), {res_source_info::USER}};

    // clang-format off
    // (clang-format messes up the asterisks)
    property_t<size_t> _atomic_item_size_in{PROP_KEY_ATOMIC_ITEM_SIZE,
        DEFAULT_LENGTH * BYTES_PER_SAMPLE,
        {res_source_info::INPUT_EDGE}};
    property_t<size_t> _atomic_item_size_out{PROP_KEY_ATOMIC_ITEM_SIZE,
        DEFAULT_LENGTH * BYTES_PER_SAMPLE,
        {res_source_info::OUTPUT_EDGE}};
    // clang-format on
    property_t<std::string> _type_in = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE}};
    property_t<std::string> _type_out = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE}};
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    fft_block_control, FFT_BLOCK, "FFT", CLOCK_KEY_GRAPH, "bus_clk")
