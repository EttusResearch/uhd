//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/fft_block_control.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhdlib/rfnoc/prop_accessor.hpp>
#include <uhdlib/utils/compat_check.hpp>
#include <boost/format.hpp>

#define VERSION_DEPENDENT_ADDR(addr) ((_fpga_compat.get_major() == 1) ? addr##_V1 : addr)

using namespace uhd::rfnoc;

namespace {

constexpr uint16_t MAJOR_COMPAT = 3;
constexpr uint16_t MINOR_COMPAT = 0;

constexpr int DEFAULT_LENGTH               = 256;
constexpr int DEFAULT_FFT_SCALING          = 0b10101010; // _get_default_scaling(256)
constexpr float DEFAULT_FFT_SCALING_FACTOR = 1.0 / DEFAULT_LENGTH;
constexpr fft_shift DEFAULT_SHIFT          = fft_shift::NORMAL;
constexpr fft_direction DEFAULT_DIRECTION  = fft_direction::FORWARD;
constexpr fft_magnitude DEFAULT_MAGNITUDE  = fft_magnitude::COMPLEX;
constexpr bool DEFAULT_BYPASS_MODE         = false;

// FFT IP constraints
constexpr uint32_t MIN_FFT_SIZE = 8;

const uhd::rfnoc::io_type_t DEFAULT_TYPE = uhd::rfnoc::IO_TYPE_SC16;
// As long as we can only do sc16 in this block, we skip deriving the
// byte-per-sample value from elsewhere
constexpr size_t BYTES_PER_SAMPLE = 4;

} // namespace

// FFT Block v2, with CP insertion and removal feature
// See fft_core_regs_pkg.sv for register offsets and descriptions
const uint32_t fft_block_control::REG_COMPAT_ADDR           = 0x00;
const uint32_t fft_block_control::REG_PORT_CONFIG_ADDR      = 0x04;
const uint32_t fft_block_control::REG_CAPABILITIES_ADDR     = 0x08;
const uint32_t fft_block_control::REG_CAPABILITIES2_ADDR    = 0x0C;
const uint32_t fft_block_control::REG_RESET_ADDR            = 0x10;
const uint32_t fft_block_control::REG_LENGTH_LOG2_ADDR      = 0x14;
const uint32_t fft_block_control::REG_SCALING_ADDR          = 0x18;
const uint32_t fft_block_control::REG_DIRECTION_ADDR        = 0x1C;
const uint32_t fft_block_control::REG_CP_INS_LEN_ADDR       = 0x20;
const uint32_t fft_block_control::REG_CP_INS_LIST_LOAD_ADDR = 0x24;
const uint32_t fft_block_control::REG_CP_INS_LIST_CLR_ADDR  = 0x28;
const uint32_t fft_block_control::REG_CP_INS_LIST_OCC_ADDR  = 0x2C;
const uint32_t fft_block_control::REG_CP_REM_LEN_ADDR       = 0x30;
const uint32_t fft_block_control::REG_CP_REM_LIST_LOAD_ADDR = 0x34;
const uint32_t fft_block_control::REG_CP_REM_LIST_CLR_ADDR  = 0x38;
const uint32_t fft_block_control::REG_CP_REM_LIST_OCC_ADDR  = 0x3C;
const uint32_t fft_block_control::REG_OVERFLOW_ADDR         = 0x40;
const uint32_t fft_block_control::REG_BYPASS_ADDR           = 0x44;
const uint32_t fft_block_control::REG_ORDER_ADDR            = 0x48;
const uint32_t fft_block_control::REG_MAGNITUDE_ADDR        = 0x4C;

// FFT Block v1
const uint32_t fft_block_control::REG_RESET_ADDR_V1       = 131 * 8; // 0x418
const uint32_t fft_block_control::REG_LENGTH_LOG2_ADDR_V1 = 132 * 8; // 0x420
const uint32_t fft_block_control::REG_MAGNITUDE_ADDR_V1   = 133 * 8; // 0x428
const uint32_t fft_block_control::REG_DIRECTION_ADDR_V1   = 134 * 8; // 0x430
const uint32_t fft_block_control::REG_SCALING_ADDR_V1     = 135 * 8; // 0x438
const uint32_t fft_block_control::REG_ORDER_ADDR_V1       = 136 * 8; // 0x440

class fft_block_control_impl : public fft_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(fft_block_control)
    ,
        _fpga_compat(get_noc_id() == FFT_BLOCK_V1
                         ? uhd::compat_num32{1, 0}
                         : uhd::compat_num32{regs().peek32(REG_COMPAT_ADDR)}),
        _capabilities((_fpga_compat.get_major() == 1)
                          ? 0x0000000c // max fft size = 4096
                                       // no CP insertion or CP removal capabilities
                          : regs().peek32(REG_CAPABILITIES_ADDR)),
        _capabilities2((_fpga_compat.get_major() == 1)
                           ? 0x0000000e // magnitude squared, magnitude and FFT shift is
                                        // supported
                           : regs().peek32(REG_CAPABILITIES2_ADDR)),
        _max_length(1 << ((_capabilities >> 0) & 0xFF)),
        _max_cp_length((1 << ((_capabilities >> 8) & 0xFF)) - 1),
        _max_cp_removal_list_length((1 << ((_capabilities >> 16) & 0xFF)) - 1),
        _max_cp_insertion_list_length((1 << ((_capabilities >> 24) & 0xFF)) - 1),
        _magnitude_squared_supported((_capabilities2 >> 3) & 0x1),
        _magnitude_supported((_capabilities2 >> 2) & 0x1),
        _fft_shift_supported((_capabilities2 >> 1) & 0x1),
        _bypass_mode_supported((_capabilities2 >> 0) & 0x1)
    {
        if (_fpga_compat.get_major() >= 2) {
            uhd::assert_fpga_compat(MAJOR_COMPAT,
                MINOR_COMPAT,
                _fpga_compat.get(),
                get_unique_id(),
                get_unique_id(),
                false /* Let it slide if minors mismatch */
            );
        }
        RFNOC_LOG_DEBUG("Compat number: " << _fpga_compat.to_string());
        RFNOC_LOG_DEBUG("Max. FFT size: " << _max_length);
        if (_max_cp_length == 0) {
            RFNOC_LOG_DEBUG("CP insertion and CP removal is not supported");
        } else {
            RFNOC_LOG_DEBUG("Max. CP length: " << _max_cp_length);
            RFNOC_LOG_DEBUG(
                "Max. CP removal list length: " << _max_cp_removal_list_length);
            RFNOC_LOG_DEBUG(
                "Max. CP insertion list length: " << _max_cp_insertion_list_length);
        }
        RFNOC_LOG_DEBUG("Magnitude squared supported: " << _magnitude_squared_supported);
        RFNOC_LOG_DEBUG("Magnitude supported: " << _magnitude_supported);
        RFNOC_LOG_DEBUG("FFT shift supported: " << _fft_shift_supported);
        RFNOC_LOG_DEBUG("Bypass mode supported: " << _bypass_mode_supported);
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

    void set_scaling_factor(const double factor) override
    {
        set_property<double>(PROP_KEY_FFT_SCALING_FACTOR, static_cast<double>(factor));
    }

    void set_scaling(const uint32_t scaling) override
    {
        set_property<int>(PROP_KEY_FFT_SCALING, static_cast<int>(scaling));
    }

    uint32_t get_scaling() const override
    {
        return static_cast<uint16_t>(_scaling.get());
    }

    void set_length(const uint32_t length) override
    {
        set_property<int>(PROP_KEY_LENGTH, static_cast<int>(length));
    }

    uint32_t get_length() const override
    {
        return static_cast<size_t>(_length.get());
    }

    void set_bypass_mode(const bool bypass) override
    {
        set_property<bool>(PROP_KEY_BYPASS_MODE, static_cast<bool>(bypass));
    }

    bool get_bypass_mode() const override
    {
        return static_cast<bool>(_bypass_mode_property.get());
    }

    void set_cp_insertion_list(const std::vector<uint32_t> cp_lengths) override
    {
        set_property<std::vector<uint32_t>>(PROP_KEY_CP_INSERTION_LIST, cp_lengths);
    }

    std::vector<uint32_t> get_cp_insertion_list(void) const override
    {
        return static_cast<std::vector<uint32_t>>(_cp_insertion_list.get());
    }

    void set_cp_removal_list(const std::vector<uint32_t> cp_lengths) override
    {
        set_property<std::vector<uint32_t>>(PROP_KEY_CP_REMOVAL_LIST, cp_lengths);
    }

    std::vector<uint32_t> get_cp_removal_list(void) const override
    {
        return static_cast<std::vector<uint32_t>>(_cp_removal_list.get());
    }

    uint32_t get_max_length() const override
    {
        return _max_length;
    }

    uint32_t get_max_cp_length() const override
    {
        return _max_cp_length;
    }

    uint32_t get_max_cp_removal_list_length() const override
    {
        return _max_cp_removal_list_length;
    }

    uint32_t get_max_cp_insertion_list_length() const override
    {
        return _max_cp_insertion_list_length;
    }

private:
    void _reset()
    {
        regs().poke32(VERSION_DEPENDENT_ADDR(REG_RESET_ADDR), uint32_t(1));
    }

    /*
     * Get default scaling mask for a given denominator (scaling factor is
     * 1/denominator). The function distributes the scaling factor evenly across
     * the FFT stages.
     *
     * Examples:
     * - denominator = 1 -> no scaling
     * - denominator = 2 -> the scaling will be set to 0b000000000001
     *   -> scale by 2 in the first FFT stage
     * - denominator = 4 -> the scaling will be set to 0b000000001010
     *   -> scale by 4 in both the first and the second FFT stage
     * - denominator = 32 -> the scaling will be set to 0b000000011010
     *   -> scale by 4 in both the first and the second FFT stage
     *   and by 2 in the third FFT stage
     */
    uint32_t _get_default_scaling(const uint32_t denominator)
    {
        uint32_t denom_log2 = std::ceil(std::log2(denominator));
        uint32_t stage;
        uint32_t fft_scale = 0;
        for (int32_t i = ((denom_log2 + 1) / 2) - 1; i >= 0; i--) {
            if (i == int32_t(denom_log2 / 2)) {
                stage = 0b01;
            } else {
                stage = 0b10;
            }
            fft_scale = (fft_scale << 2) | stage;
        }
        return fft_scale;
    }

    void _set_length(uint32_t length)
    {
        // Check that the FFT length is within bounds and is a power of 2
        uint32_t max_length = get_max_length();
        if (length < MIN_FFT_SIZE || length > max_length) {
            throw uhd::value_error("FFT length must be a power of two and within ["
                                   + std::to_string(MIN_FFT_SIZE) + ", "
                                   + std::to_string(max_length) + "]");
        }
        // Find the log2(length) via highest bit set
        uint32_t length_log2      = 0;
        uint32_t requested_length = length;
        while ((length >>= 1) != 0) {
            length_log2++;
        }
        uint32_t coerced_length = (1 << length_log2);
        if (requested_length != coerced_length) {
            RFNOC_LOG_WARNING("FFT length "
                              << requested_length
                              << " not an integral power of two; coercing to "
                              << coerced_length);
            this->_length.set(coerced_length);
        }
        RFNOC_LOG_DEBUG("Setting FFT length to " << coerced_length);
        regs().poke32(
            VERSION_DEPENDENT_ADDR(REG_LENGTH_LOG2_ADDR), uint32_t(length_log2));

        // Check to make sure register was updated correctly
        length_log2 = regs().peek32(VERSION_DEPENDENT_ADDR(REG_LENGTH_LOG2_ADDR));
        uint32_t readback_length = (1 << length_log2);
        if (readback_length != coerced_length) {
            throw uhd::value_error("FFT Size readback failed! "
                                   "Expected: "
                                   + std::to_string(coerced_length)
                                   + ", "
                                     "Received: "
                                   + std::to_string(readback_length));
        }
    }

    void _set_magnitude(int mag)
    {
        if (mag < static_cast<int>(fft_magnitude::COMPLEX)
            || mag > static_cast<int>(fft_magnitude::MAGNITUDE_SQUARED)) {
            throw uhd::value_error("Magnitude value must be [0, 2]");
        } else if ((static_cast<fft_magnitude>(mag) == fft_magnitude::MAGNITUDE)
                   and not _magnitude_supported) {
            throw uhd::value_error("Magnitude value MAGNITUDE(1) is not supported");
        } else if ((static_cast<fft_magnitude>(mag) == fft_magnitude::MAGNITUDE_SQUARED)
                   and not _magnitude_squared_supported) {
            throw uhd::value_error(
                "Magnitude value MAGNITUDE_SQUARED(2) is not supported");
        }
        regs().poke32(VERSION_DEPENDENT_ADDR(REG_MAGNITUDE_ADDR), uint32_t(mag));
        int readback_mag = int(regs().peek32(VERSION_DEPENDENT_ADDR(REG_MAGNITUDE_ADDR)));
        if (readback_mag != mag) {
            throw uhd::value_error("Magnitude readback failed! "
                                   "Expected: "
                                   + std::to_string(mag)
                                   + ", "
                                     "Received: "
                                   + std::to_string(readback_mag));
        }
    }

    void _set_direction(fft_direction direction)
    {
        if (direction != fft_direction::REVERSE && direction != fft_direction::FORWARD) {
            throw uhd::value_error(
                "FFT Direction value " + std::to_string(static_cast<uint32_t>(direction))
                + " is invalid, must be equal to either "
                + std::to_string(static_cast<uint32_t>(fft_direction::FORWARD))
                + " (Forward aka FFT) or "
                + std::to_string(static_cast<uint32_t>(fft_direction::REVERSE))
                + " (Reverse aka IFFT)");
        }
        RFNOC_LOG_DEBUG("Setting FFT Direction to " << static_cast<uint32_t>(direction));
        regs().poke32(
            VERSION_DEPENDENT_ADDR(REG_DIRECTION_ADDR), static_cast<uint32_t>(direction));

        fft_direction readback_direction = get_direction();
        if (readback_direction != direction) {
            throw uhd::value_error(
                "FFT Direction readback failed! "
                "Expected: "
                + std::to_string(static_cast<uint32_t>(direction))
                + ", "
                  "Received: "
                + std::to_string(static_cast<uint32_t>(readback_direction)));
        }
    }

    void _set_scaling(uint32_t scaling)
    {
        // Check that FFT scale is within bounds
        const uint32_t scale_width =
            static_cast<uint32_t>(2 * std::ceil(log2(double(_length)) / 2));
        const uint32_t max_scale = (1 << scale_width) - 1;
        if (scaling > max_scale) {
            throw uhd::value_error(str(
                boost::format(
                    "FFT scaling mask 0x%08x is greater then max. supported mask 0x%08x")
                % scaling % max_scale));
        }
        RFNOC_LOG_DEBUG("Setting FFT Scaling to " << scaling);
        regs().poke32(VERSION_DEPENDENT_ADDR(REG_SCALING_ADDR), scaling);

        uint32_t readback_scaling =
            regs().peek32(VERSION_DEPENDENT_ADDR(REG_SCALING_ADDR));
        if (readback_scaling != scaling) {
            throw uhd::value_error("FFT Scaling readback failed! "
                                   "Expected: "
                                   + std::to_string(scaling)
                                   + ", "
                                     "Received: "
                                   + std::to_string(readback_scaling));
        }
    }

    void _set_shift_config(int shift)
    {
        if (shift < static_cast<int>(fft_shift::NORMAL)
            || shift > static_cast<int>(fft_shift::NATURAL)) {
            throw uhd::value_error("Shift value must be [0, 2]");
        } else if ((static_cast<fft_shift>(shift) != fft_shift::NORMAL)
                   and not _fft_shift_supported) {
            throw uhd::value_error("Shift capability is not present in the FFT block. "
                                   "Only value NORMAL(0) is supported");
        }
        this->regs().poke32(VERSION_DEPENDENT_ADDR(REG_ORDER_ADDR), uint32_t(shift));
    }

    void _set_bypass_mode(bool bypass)
    {
        if (bypass and not _bypass_mode_supported) {
            throw uhd::value_error("Bypass mode is not supported in the FFT block. Only "
                                   "value False is allowed");
        }
        if (_fpga_compat.get_major() == 1) {
            return;
        }
        regs().poke32(REG_BYPASS_ADDR, uint32_t(bypass));
    }

    void _clear_cp_insertion_fifo()
    {
        regs().poke32(REG_CP_INS_LIST_CLR_ADDR, 1);
        uint32_t readback_fifo_occupancy = regs().peek32(REG_CP_INS_LIST_OCC_ADDR);
        if (readback_fifo_occupancy != 0) {
            throw uhd::value_error("Clearing Cyclic Prefix Insertion FIFO failed!");
        }
    }

    void _clear_cp_removal_fifo()
    {
        regs().poke32(REG_CP_REM_LIST_CLR_ADDR, 1);
        uint32_t readback_fifo_occupancy = regs().peek32(REG_CP_REM_LIST_OCC_ADDR);
        if (readback_fifo_occupancy != 0) {
            throw uhd::value_error("Clearing Cyclic Prefix Removal FIFO failed!");
        }
    }

    void _set_cp_insertion_list(const std::vector<uint32_t> cp_lengths)
    {
        if (_fpga_compat.get_major() == 1) {
            if (cp_lengths.size() > 0) {
                throw uhd::value_error("Block does not support CP insertion");
            }
            return;
        }
        if (cp_lengths.size() > _max_cp_insertion_list_length) {
            throw uhd::value_error(
                "Vector size of insertion Cyclic Prefix Lengths is too large. "
                "Must be between 1 and "
                + std::to_string(_max_cp_insertion_list_length) + ".");
        }
        for (auto& cp_length : cp_lengths) {
            if (cp_length > get_max_cp_length()) {
                throw uhd::value_error("CP insertion length " + std::to_string(cp_length)
                                       + " is too "
                                         "large. Must be between 1 and "
                                       + std::to_string(get_max_cp_length()) + ".");
            }
        }

        _clear_cp_insertion_fifo();

        // Write cyclic prefix lengths into config FIFO
        uint32_t readback_cp_length;
        for (auto& cp_length : cp_lengths) {
            regs().poke32(REG_CP_INS_LEN_ADDR, cp_length);
            regs().poke32(REG_CP_INS_LIST_LOAD_ADDR, 1);
            RFNOC_LOG_DEBUG("Setting insertion Cyclic Prefix Length " << cp_length);
            readback_cp_length = regs().peek32(REG_CP_INS_LEN_ADDR);
            if (readback_cp_length != cp_length) {
                throw uhd::value_error("Insertion Cyclic Prefix length readback failed! "
                                       "Expected: "
                                       + std::to_string(cp_length) + ", " + "Received: "
                                       + std::to_string(readback_cp_length));
            }
        }

        uint32_t cp_insertion_fifo_fifo_occupancy =
            regs().peek32(REG_CP_INS_LIST_OCC_ADDR);
        if (cp_insertion_fifo_fifo_occupancy != cp_lengths.size()) {
            throw uhd::value_error(
                "Checking Cyclic Prefix Insertion FIFO after writing failed. "
                "Expected FIFO occupancy: "
                + std::to_string(cp_lengths.size()) + ", Actual FIFO occupancy: "
                + std::to_string(cp_insertion_fifo_fifo_occupancy));
        }
    }

    void _set_cp_removal_list(const std::vector<uint32_t> cp_lengths)
    {
        if (_fpga_compat.get_major() == 1) {
            if (cp_lengths.size() > 0) {
                throw uhd::value_error("Block does not support CP insertion");
            }
            return;
        }
        if (cp_lengths.size() > _max_cp_removal_list_length) {
            throw uhd::value_error(
                "Vector size of removal Cyclic Prefix Lengths is too large. "
                "Must be between 1 and "
                + std::to_string(_max_cp_removal_list_length) + ".");
        }
        for (auto& cp_length : cp_lengths) {
            if (cp_length > get_max_cp_length()) {
                throw uhd::value_error("CP removal length " + std::to_string(cp_length)
                                       + " is too "
                                         "large. Must be between 1 and "
                                       + std::to_string(get_max_cp_length()) + ".");
            }
        }

        _clear_cp_removal_fifo();

        // Write cyclic prefix lengths into config FIFO
        uint32_t readback_cp_length;
        for (auto& cp_length : cp_lengths) {
            regs().poke32(REG_CP_REM_LEN_ADDR, cp_length);
            regs().poke32(REG_CP_REM_LIST_LOAD_ADDR, 1);
            RFNOC_LOG_DEBUG("Setting Cyclic Prefix Length " << cp_length);
            readback_cp_length = regs().peek32(REG_CP_REM_LEN_ADDR);
            if (readback_cp_length != cp_length) {
                throw uhd::value_error("Removal Cyclic Prefix readback failed! "
                                       "Expected: "
                                       + std::to_string(cp_length)
                                       + ", "
                                         "Received: "
                                       + std::to_string(readback_cp_length));
            }
        }

        uint32_t cp_removal_fifo_fifo_occupancy = regs().peek32(REG_CP_REM_LIST_OCC_ADDR);
        if (cp_removal_fifo_fifo_occupancy != cp_lengths.size()) {
            throw uhd::value_error(
                "Checking Cyclic Prefix Removal FIFO after writing failed. "
                "Expected FIFO occupancy: "
                + std::to_string(cp_lengths.size()) + ", Actual FIFO occupancy: "
                + std::to_string(cp_removal_fifo_fifo_occupancy));
        }
    }

    void _atomic_item_size_check()
    {
        if (_fpga_compat.get_major() == 1) {
            // FFT v1 requires atomic item size to be equal to the
            // FFT length (in bytes)
            size_t ais = _length.get() * BYTES_PER_SAMPLE;
            if (_atomic_item_size_in != ais || _atomic_item_size_out != ais) {
                RFNOC_LOG_WARNING("Atomic item size for FFT v1 needs to match FFT length "
                                  "(in bytes), setting atomic item size to: "
                                  << ais);
                _atomic_item_size_in  = ais;
                _atomic_item_size_out = ais;
            }
        } else if ((_cp_insertion_list.get().size() > 0)
                   || (_cp_removal_list.get().size() > 0)) {
            // If the cyclic prefix insertion list is not empty,
            // packages which are generated by the FFT block are of
            // different size (FFT length; CP insertion length)
            // Hence, the atomic item size needs to be set to the
            // greatest common divisor of the possible packet sizes.
            // For simplicity, set it to 1 * bytes per sample
            size_t ais = 1 * BYTES_PER_SAMPLE;
            if (_atomic_item_size_in != ais || _atomic_item_size_out != ais) {
                RFNOC_LOG_WARNING(
                    "Atomic item size cannot be modified because FFT block is configured "
                    "with CP insertion or CP removal, setting atomic item size to: "
                    << ais);
                _atomic_item_size_in  = ais;
                _atomic_item_size_out = ais;
            }
            _atomic_item_size_in  = 1 * BYTES_PER_SAMPLE;
            _atomic_item_size_out = 1 * BYTES_PER_SAMPLE;
        } else if ((_atomic_item_size_in > (_length.get() * BYTES_PER_SAMPLE))
                   || (_atomic_item_size_out > (_length.get() * BYTES_PER_SAMPLE))) {
            // if the atomic item size is greater than the FFT length
            // (in bytes), limit it by the FFT length (in bytes)
            size_t ais = _length.get() * BYTES_PER_SAMPLE;
            if (_atomic_item_size_in != ais || _atomic_item_size_out != ais) {
                RFNOC_LOG_WARNING("Requested atomic item size exceeded FFT length (in "
                                  "bytes), setting atomic item size to: "
                                  << ais);
                _atomic_item_size_in  = ais;
                _atomic_item_size_out = ais;
            }
        }
    }

    /**************************************************************************
     * Initialization
     *************************************************************************/
    void _register_props()
    {
        register_property(&_length);
        register_property(&_atomic_item_size_in);
        register_property(&_atomic_item_size_out);
        // use add_property_resolver to allow setting coerced length
        add_property_resolver({&_length},
            {&_length, &_atomic_item_size_in, &_atomic_item_size_out},
            [this]() {
                uint32_t length = static_cast<uint32_t>(this->_length.get());
                RFNOC_LOG_TRACE("Calling resolver for `length'");
                _set_length(length);
                _atomic_item_size_check();
            });

        register_property(&_magnitude, [this]() {
            int mag = this->_magnitude.get();
            RFNOC_LOG_TRACE("Calling resolver for `magnitude'");
            _set_magnitude(mag);
        });

        register_property(&_direction, [this]() {
            fft_direction direction = static_cast<fft_direction>(_direction.get());
            RFNOC_LOG_TRACE("Calling resolver for `direction'");
            _set_direction(direction);
        });

        register_property(&_scaling, [this]() {
            uint32_t scaling = static_cast<uint32_t>(_scaling.get());
            RFNOC_LOG_TRACE("Calling resolver for `fft_scaling'");
            _set_scaling(scaling);
        });

        register_property(&_scaling_factor);
        add_property_resolver(
            {&_scaling_factor}, {&_scaling, &_scaling_factor}, [this]() {
                double factor           = static_cast<double>(_scaling_factor.get());
                uint32_t scaling_length = uint32_t(1.0 / factor);
                uint32_t scaling        = _get_default_scaling(scaling_length);
                RFNOC_LOG_TRACE("Calling resolver for `fft_scaling_factor'");
                _set_scaling(scaling);
                this->_scaling.set(scaling);
            });

        register_property(&_shift, [this]() {
            int shift = this->_shift.get();
            RFNOC_LOG_TRACE("Calling resolver for `shift_config'");
            _set_shift_config(shift);
        });

        register_property(&_bypass_mode_property, [this]() {
            bool bypass = this->_bypass_mode_property.get();
            RFNOC_LOG_TRACE("Calling resolver for `bypass_mode'");
            _set_bypass_mode(bypass);
        });

        register_property(&_cp_insertion_list);
        add_property_resolver({&_cp_insertion_list},
            {&_cp_insertion_list, &_atomic_item_size_in, &_atomic_item_size_out},
            [this]() {
                std::vector<uint32_t> cp_insertion_list = this->_cp_insertion_list.get();
                RFNOC_LOG_TRACE("Calling resolver for `cp_insertion_list'");
                _set_cp_insertion_list(cp_insertion_list);
                _atomic_item_size_check();
            });

        register_property(&_cp_removal_list);
        add_property_resolver({&_cp_removal_list},
            {&_cp_removal_list, &_atomic_item_size_in, &_atomic_item_size_out},
            [this]() {
                std::vector<uint32_t> cp_removal_list = this->_cp_removal_list.get();
                RFNOC_LOG_TRACE("Calling resolver for `cp_removal_list'");
                _set_cp_removal_list(cp_removal_list);
                _atomic_item_size_check();
            });

        register_property(&_max_length_property);
        add_property_resolver({&_max_length_property}, {&_max_length_property}, [this]() {
            RFNOC_LOG_TRACE("Calling resolver for `max_length'");
            this->_max_length_property.set(this->_max_length);
        });
        register_property(&_max_cp_length_property);
        add_property_resolver(
            {&_max_cp_length_property}, {&_max_cp_length_property}, [this]() {
                RFNOC_LOG_TRACE("Calling resolver for `max_cp_length'");
                this->_max_cp_length_property.set(this->_max_cp_length);
            });
        register_property(&_max_cp_insertion_list_length_property);
        add_property_resolver({&_max_cp_insertion_list_length_property},
            {&_max_cp_insertion_list_length_property},
            [this]() {
                RFNOC_LOG_TRACE("Calling resolver for `max_cp_insertion_list_length'");
                this->_max_cp_insertion_list_length_property.set(
                    this->_max_cp_insertion_list_length);
            });
        register_property(&_max_cp_removal_list_length_property);
        add_property_resolver({&_max_cp_removal_list_length_property},
            {&_max_cp_removal_list_length_property},
            [this]() {
                RFNOC_LOG_TRACE("Calling resolver for `max_cp_removal_list_length'");
                this->_max_cp_removal_list_length_property.set(
                    this->_max_cp_removal_list_length);
            });

        // register edge properties
        register_property(&_type_in);
        register_property(&_type_out);
        // add resolvers for type (keeps it constant)
        add_property_resolver({&_type_in}, {&_type_in}, [&type_in = _type_in]() {
            type_in.set(IO_TYPE_SC16);
        });
        add_property_resolver({&_type_out}, {&_type_out}, [&type_out = _type_out]() {
            type_out.set(IO_TYPE_SC16);
        });

        // Add resolvers for atomic item size
        add_property_resolver({&_atomic_item_size_in},
            {&_atomic_item_size_in, &_atomic_item_size_out},
            [this]() {
                RFNOC_LOG_TRACE("Calling resolver for `atomic_item_size' (triggered from "
                                "input edge)");
                _atomic_item_size_out = _atomic_item_size_in.get();
                _atomic_item_size_check();
            });
        add_property_resolver({&_atomic_item_size_out},
            {&_atomic_item_size_in, &_atomic_item_size_out},
            [this]() {
                RFNOC_LOG_TRACE("Calling resolver for `atomic_item_size' (triggered from "
                                "output edge)");
                _atomic_item_size_in = _atomic_item_size_out.get();
                _atomic_item_size_check();
            });
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Block compat number
    const uhd::compat_num32 _fpga_compat;
    //! Block capabilities
    const uint32_t _capabilities;
    const uint32_t _capabilities2;
    const uint32_t _max_length;
    const uint32_t _max_cp_length;
    const uint32_t _max_cp_removal_list_length;
    const uint32_t _max_cp_insertion_list_length;
    const bool _magnitude_squared_supported;
    const bool _magnitude_supported;
    const bool _fft_shift_supported;
    const bool _bypass_mode_supported;

    property_t<int> _length{PROP_KEY_LENGTH, DEFAULT_LENGTH, {res_source_info::USER}};
    property_t<int> _magnitude = property_t<int>{
        PROP_KEY_MAGNITUDE, static_cast<int>(DEFAULT_MAGNITUDE), {res_source_info::USER}};
    property_t<int> _direction = property_t<int>{
        PROP_KEY_DIRECTION, static_cast<int>(DEFAULT_DIRECTION), {res_source_info::USER}};
    property_t<int> _scaling = property_t<int>{
        PROP_KEY_FFT_SCALING, DEFAULT_FFT_SCALING, {res_source_info::USER}};
    property_t<double> _scaling_factor = property_t<double>{
        PROP_KEY_FFT_SCALING_FACTOR, DEFAULT_FFT_SCALING_FACTOR, {res_source_info::USER}};
    property_t<int> _shift = property_t<int>{
        PROP_KEY_SHIFT_CONFIG, static_cast<int>(DEFAULT_SHIFT), {res_source_info::USER}};
    property_t<bool> _bypass_mode_property = property_t<bool>{PROP_KEY_BYPASS_MODE,
        static_cast<bool>(DEFAULT_BYPASS_MODE),
        {res_source_info::USER}};
    property_t<std::vector<uint32_t>> _cp_insertion_list =
        property_t<std::vector<uint32_t>>{
            PROP_KEY_CP_INSERTION_LIST, std::vector<uint32_t>(), {res_source_info::USER}};
    property_t<std::vector<uint32_t>> _cp_removal_list =
        property_t<std::vector<uint32_t>>{
            PROP_KEY_CP_REMOVAL_LIST, std::vector<uint32_t>(), {res_source_info::USER}};
    property_t<uint32_t> _max_length_property =
        property_t<uint32_t>{PROP_KEY_MAX_LENGTH, _max_length, {res_source_info::USER}};
    property_t<uint32_t> _max_cp_length_property = property_t<uint32_t>{
        PROP_KEY_MAX_CP_LENGTH, _max_cp_length, {res_source_info::USER}};
    property_t<uint32_t> _max_cp_insertion_list_length_property =
        property_t<uint32_t>{PROP_KEY_MAX_CP_INSERTION_LIST_LENGTH,
            _max_cp_insertion_list_length,
            {res_source_info::USER}};
    property_t<uint32_t> _max_cp_removal_list_length_property =
        property_t<uint32_t>{PROP_KEY_MAX_CP_REMOVAL_LIST_LENGTH,
            _max_cp_removal_list_length,
            {res_source_info::USER}};

    // clang-format off
    // (clang-format messes up the asterisks)
    property_t<size_t> _atomic_item_size_in{PROP_KEY_ATOMIC_ITEM_SIZE,
        (_fpga_compat.get_major() == 1) ? (DEFAULT_LENGTH * BYTES_PER_SAMPLE) : BYTES_PER_SAMPLE,
        {res_source_info::INPUT_EDGE}};
    property_t<size_t> _atomic_item_size_out{PROP_KEY_ATOMIC_ITEM_SIZE,
        (_fpga_compat.get_major() == 1) ? (DEFAULT_LENGTH * BYTES_PER_SAMPLE) : BYTES_PER_SAMPLE,
        {res_source_info::OUTPUT_EDGE}};
    // clang-format on
    property_t<std::string> _type_in = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE}};
    property_t<std::string> _type_out = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE}};
};


static std::vector<noc_id_t> FFT_NOC_IDS{FFT_BLOCK_V1, FFT_BLOCK_V2};
UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    fft_block_control, FFT_NOC_IDS, "FFT", CLOCK_KEY_GRAPH, "bus_clk")
