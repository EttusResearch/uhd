//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/ofdm_block_control.hpp>
#include <uhdlib/utils/compat_check.hpp>

using namespace uhd::rfnoc;

const uint16_t           ofdm_block_control::MAJOR_COMPAT = 1;
const uint16_t           ofdm_block_control::MINOR_COMPAT = 0;

constexpr int            ofdm_block_control::DEFAULT_FFT_SIZE      = 4096;
constexpr int            ofdm_block_control::DEFAULT_FFT_SCALING   = 0b101010101010;
constexpr ofdm_direction ofdm_block_control::DEFAULT_FFT_DIRECTION = ofdm_direction::REVERSE;
constexpr uint32_t       ofdm_block_control::DEFAULT_CP_LEN        = 0;

const std::string ofdm_block_control::PROP_KEY_FFT_SIZE      = "fft_size";
const std::string ofdm_block_control::PROP_KEY_FFT_SCALING   = "fft_scaling";
const std::string ofdm_block_control::PROP_KEY_FFT_DIRECTION = "ofdm_direction";

// See fft_core_regs_pkg.sv for register offsets and descriptions
const uint32_t ofdm_block_control::REG_COMPAT_ADDR                  = 0x0;
const uint32_t ofdm_block_control::REG_CAPABILITIES_ADDR            = 0x1;
const uint32_t ofdm_block_control::REG_USER_RESET_ADDR              = 0x2;
const uint32_t ofdm_block_control::REG_FFT_SIZE_LOG2_ADDR           = 0x3;
const uint32_t ofdm_block_control::REG_FFT_SCALING_ADDR             = 0x4;
const uint32_t ofdm_block_control::REG_FFT_DIRECTION_ADDR           = 0x5;
const uint32_t ofdm_block_control::REG_CP_INS_LEN_ADDR              = 0x6;
const uint32_t ofdm_block_control::REG_CP_INS_CP_LEN_FIFO_LOAD_ADDR = 0x7;
const uint32_t ofdm_block_control::REG_CP_INS_CP_LEN_FIFO_CLR_ADDR  = 0x8;
const uint32_t ofdm_block_control::REG_CP_INS_CP_LEN_FIFO_OCC_ADDR  = 0x9;
const uint32_t ofdm_block_control::REG_CP_REM_LEN_ADDR              = 0xA;
const uint32_t ofdm_block_control::REG_CP_REM_CP_LEN_FIFO_LOAD_ADDR = 0xB;
const uint32_t ofdm_block_control::REG_CP_REM_CP_LEN_FIFO_CLR_ADDR  = 0xC;
const uint32_t ofdm_block_control::REG_CP_REM_CP_LEN_FIFO_OCC_ADDR  = 0xD;

// FFT IP constraints
constexpr uint32_t MIN_FFT_SIZE = 8;

constexpr uint32_t CP_INSERTION_FIFO_DEPTH = 32;
constexpr uint32_t CP_REMOVAL_FIFO_DEPTH   = 32;

class ofdm_block_control_impl : public ofdm_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(ofdm_block_control)
    , _fpga_compat(regs().peek32(REG_COMPAT_ADDR))
    , _capabilities(regs().peek32(REG_CAPABILITIES_ADDR))
    , _max_fft_size(1 << ((_capabilities >> 0) & 0xFF))
    , _max_cp_length((1 << ((_capabilities >> 8) & 0xFF)) - 1)
    , _max_cp_rem_list_length((1 << ((_capabilities >> 16) & 0xFF)) - 1)
    , _max_cp_ins_list_length((1 << ((_capabilities >> 24) & 0xFF)) - 1)
    {
        uhd::assert_fpga_compat(MAJOR_COMPAT,
            MINOR_COMPAT,
            _fpga_compat,
            get_unique_id(),
            get_unique_id(),
            false /* Let it slide if minors mismatch */
        );
        RFNOC_LOG_DEBUG("Max. FFT size: " << _max_fft_size);
        RFNOC_LOG_DEBUG("Max. CP length: " << _max_cp_length);
        RFNOC_LOG_DEBUG("Max. CP removal list length: " << _max_cp_rem_list_length);
        RFNOC_LOG_DEBUG("Max. CP insertion list length: " << _max_cp_ins_list_length);
        set_prop_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        set_action_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        _reset();
        _register_props();
    }

    void write_reg(const uint32_t addr, const uint32_t wr_val)
    {
        RFNOC_LOG_DEBUG(boost::format("write_reg(): Addr: 0x%08x, Val: 0x%08x") % addr % wr_val);
        regs().poke32(addr, wr_val);
    }

    uint32_t read_reg(const uint32_t addr)
    {
        uint32_t rd_val;
        rd_val = regs().peek32(addr);
        RFNOC_LOG_DEBUG(boost::format("read_reg(): Addr: 0x%08x, Val: 0x%08x") % addr % rd_val);
        return(rd_val);
    }

    void write_and_check_reg(const uint32_t addr, const uint32_t wr_val)
    {
        uint32_t rd_val;
        std::string s;

        write_reg(addr, wr_val);
        rd_val = read_reg(addr);
        if (rd_val != wr_val) {
            throw uhd::runtime_error((boost::format(
                "Readback value incorrect! Addr: 0x%08x, Expected: 0x%08x, Actual: 0x%08x")
                % addr % wr_val % rd_val).str());
        }
    }

    void set_fft_size(const uint32_t fft_size)
    {
        set_property<int>(PROP_KEY_FFT_SIZE, static_cast<int>(fft_size));
    }

    uint32_t get_fft_size()
    {
        uint32_t fft_size_log2 = read_reg(REG_FFT_SIZE_LOG2_ADDR);
        return (1 << fft_size_log2);
    }

    void set_fft_scaling(const uint32_t fft_scaling)
    {
        set_property<int>(PROP_KEY_FFT_SCALING, static_cast<int>(fft_scaling));
    }

    uint32_t get_fft_scaling()
    {
        return (read_reg(REG_FFT_SCALING_ADDR));
    }

    void set_fft_direction(const ofdm_direction direction)
    {
        set_property<int>(PROP_KEY_FFT_DIRECTION, static_cast<int>(direction));
    }

    ofdm_direction get_fft_direction()
    {
        return (static_cast<ofdm_direction>(read_reg(REG_FFT_DIRECTION_ADDR)));
    }

    void clear_cp_insertion_fifo()
    {
        write_reg(REG_CP_INS_CP_LEN_FIFO_CLR_ADDR, 1);

        uint32_t readback_fifo_occupancy = _get_cp_insertion_fifo_occupancy();
        if (readback_fifo_occupancy != 0) {
            throw uhd::value_error(
                "Clearing Cyclic Prefix Insertion FIFO failed!");
        }
    }

    void clear_cp_removal_fifo()
    {
        write_reg(REG_CP_REM_CP_LEN_FIFO_CLR_ADDR, 1);

        uint32_t readback_fifo_occupancy = _get_cp_removal_fifo_occupancy();
        if (readback_fifo_occupancy != 0) {
            throw uhd::value_error(
                "Clearing Cyclic Prefix Removal FIFO failed!");
        }
    }

    void load_cp_insertion_fifo(const std::vector<uint32_t> cp_lengths)
    {
        // Check Cyclic Prefix length is size within bounds
        if (cp_lengths.size() == 0) {
            throw uhd::value_error("Vector of Cyclic Prefix Lengths cannot be empty");
        }
        if (cp_lengths.size() > get_max_cp_ins_list_length()) {
            throw uhd::value_error(
                "Vector size of insertion Cyclic Prefix Lengths is too large. "
                "Must be between 1 and " +
                std::to_string(get_max_cp_ins_list_length()) + ".");
        }

        clear_cp_insertion_fifo();

        // Write cyclic prefix lengths into config FIFO
        uint32_t readback_cp_length;
        for (auto &cp_length : cp_lengths) {
            if (cp_length > get_max_cp_length()) {
                throw uhd::value_error(
                "CP insertion length " + std::to_string(cp_length) + " is too "
                "large. Must be between 1 and " +
                std::to_string(get_max_cp_length()) + ".");
            }
            write_reg(REG_CP_INS_LEN_ADDR, cp_length);
            write_reg(REG_CP_INS_CP_LEN_FIFO_LOAD_ADDR, 1);
            RFNOC_LOG_DEBUG("Setting insertion Cyclic Prefix Length " << cp_length);
            readback_cp_length = read_reg(REG_CP_INS_LEN_ADDR);
            if (readback_cp_length != cp_length) {
                throw uhd::value_error(
                    "Insertion Cyclic Prefix length readback failed! "
                    "Expected: " + std::to_string(cp_length) + ", " +
                    "Received: " + std::to_string(readback_cp_length));
            }
        }

        uint32_t cp_insertion_fifo_fifo_occupancy = _get_cp_insertion_fifo_occupancy();
        if (cp_insertion_fifo_fifo_occupancy != cp_lengths.size()) {
            throw uhd::value_error(
                    "Checking Cyclic Prefix Insertion FIFO after writing failed. "
                    "Expected FIFO occupancy: " + std::to_string(cp_lengths.size()) +
                    ", Actual FIFO occupancy: " +
                    std::to_string(cp_insertion_fifo_fifo_occupancy));
        }
    }

    void load_cp_removal_fifo(const std::vector<uint32_t> cp_lengths)
    {
        // Check Cyclic Prefix length is size within bounds
        if (cp_lengths.size() == 0) {
            throw uhd::value_error("Vector of removal Cyclic Prefix Lengths cannot be empty");
        }
        if (cp_lengths.size() > get_max_cp_rem_list_length()) {
            throw uhd::value_error(
                "Vector size of removal Cyclic Prefix Lengths is too large. "
                "Must be between 1 and " +
                std::to_string(get_max_cp_rem_list_length()) + ".");
        }

        clear_cp_removal_fifo();

        // Write cyclic prefix lengths into config FIFO
        uint32_t readback_cp_length;
        for (auto &cp_length : cp_lengths) {
            if (cp_length > get_max_cp_length()) {
                throw uhd::value_error(
                "CP removal length " + std::to_string(cp_length) + " is too "
                "large. Must be between 1 and " +
                std::to_string(get_max_cp_length()) + ".");
            }
            write_reg(REG_CP_REM_LEN_ADDR, cp_length);
            write_reg(REG_CP_REM_CP_LEN_FIFO_LOAD_ADDR, 1);
            RFNOC_LOG_DEBUG("Setting Cyclic Prefix Length " << cp_length);
            readback_cp_length = read_reg(REG_CP_REM_LEN_ADDR);
            if (readback_cp_length != cp_length) {
                throw uhd::value_error(
                    "Removal Cyclic Prefix readback failed! "
                    "Expected: " + std::to_string(cp_length) + ", "
                    "Received: " + std::to_string(readback_cp_length));
            }
        }

        uint32_t cp_removal_fifo_fifo_occupancy = _get_cp_removal_fifo_occupancy();
        if (cp_removal_fifo_fifo_occupancy != cp_lengths.size()) {
            throw uhd::value_error(
                    "Checking Cyclic Prefix Removal FIFO after writing failed. "
                    "Expected FIFO occupancy: " + std::to_string(cp_lengths.size()) +
                    ", Actual FIFO occupancy: " +
                    std::to_string(cp_removal_fifo_fifo_occupancy));
        }
    }

    uint32_t get_max_fft_size()
    {
        return _max_fft_size;
    }

    uint32_t get_max_cp_length()
    {
        return _max_cp_length;
    }

    uint32_t get_max_cp_rem_list_length()
    {
        return _max_cp_rem_list_length;
    }

    uint32_t get_max_cp_ins_list_length()
    {
        return _max_cp_ins_list_length;
    }

private:
    void _reset()
    {
        regs().poke32(REG_USER_RESET_ADDR, 1);
        regs().peek32(REG_USER_RESET_ADDR);
    }

    void _register_props()
    {
        register_property(&_fft_size, [this]() {
            // Check FFT size within bounds and is a power of 2
            uint32_t fft_size = static_cast<uint32_t>(this->_fft_size.get());
            uint32_t max_fft_size = get_max_fft_size();
            if (fft_size < MIN_FFT_SIZE || fft_size > max_fft_size) {
                throw uhd::value_error(
                    "FFT size must be a power of two and within [" +
                    std::to_string(MIN_FFT_SIZE) + ", " +
                    std::to_string(max_fft_size) + "]");
            }
            // Find the log2(fft_size) via highest bit set
            uint32_t fft_size_log2 = 0;
            uint32_t old_fft_size  = fft_size;
            while ((fft_size >>= 1) != 0) {
                fft_size_log2++;
            }
            uint32_t coerced_fft_size = (1 << fft_size_log2);
            if (old_fft_size != coerced_fft_size) {
                RFNOC_LOG_WARNING("FFT Size "
                                  << old_fft_size
                                  << " not an integral power of two; coercing to "
                                  << coerced_fft_size);
            }
            RFNOC_LOG_DEBUG("Setting FFT Size to " << coerced_fft_size);
            this->write_reg(REG_FFT_SIZE_LOG2_ADDR, uint32_t(fft_size_log2));

            // Check to make sure register was updated correctly
            uint32_t readback_fft_size = get_fft_size();
            if (readback_fft_size != coerced_fft_size) {
                throw uhd::value_error(
                    "FFT Size readback failed! "
                    "Expected: " + std::to_string(coerced_fft_size) + ", "
                    "Received: " + std::to_string(readback_fft_size));
            }
        });

        register_property(&_fft_scaling, [this]() {
            // Check that FFT scale is within bounds
            const uint32_t scale_width = static_cast<uint32_t>(
                2*std::ceil(std::log2(_max_fft_size)/2));
            const uint32_t max_scale = (1 << scale_width) - 1;
            uint32_t fft_scaling = static_cast<uint32_t>(_fft_scaling.get());
            if (fft_scaling > max_scale) {
                throw uhd::value_error(
                    "FFT Scaling must be in [0, " +
                    std::to_string(max_scale) + "]");
            }
            RFNOC_LOG_DEBUG("Setting FFT Scaling to " << fft_scaling);
            this->write_reg(REG_FFT_SCALING_ADDR, fft_scaling);

            uint32_t readback_fft_scaling = get_fft_scaling();
            if (readback_fft_scaling != fft_scaling) {
                throw uhd::value_error(
                    "FFT Scaling readback failed! "
                    "Expected: " + std::to_string(fft_scaling) + ", "
                    "Received: " + std::to_string(readback_fft_scaling));
            }
        });

        register_property(&_fft_direction, [this]() {
            ofdm_direction direction = static_cast<ofdm_direction>(_fft_direction.get());
            if (direction != ofdm_direction::REVERSE &&
                direction != ofdm_direction::FORWARD) {
                throw uhd::value_error(
                    "FFT Direction value " +
                    std::to_string(static_cast<uint32_t>(direction)) +
                    " is invalid, must be equal to either " +
                    std::to_string(static_cast<uint32_t>(ofdm_direction::FORWARD)) +
                    " (Forward aka FFT) or " +
                    std::to_string(static_cast<uint32_t>(ofdm_direction::REVERSE)) +
                    " (Reverse aka IFFT)");
            }
            RFNOC_LOG_DEBUG("Setting FFT Direction to " << static_cast<uint32_t>(direction));
            this->write_reg(REG_FFT_DIRECTION_ADDR, static_cast<uint32_t>(direction));

            ofdm_direction readback_direction = get_fft_direction();
            if (readback_direction != direction) {
                throw uhd::value_error(
                    "FFT Direction readback failed! "
                    "Expected: " +
                    std::to_string(static_cast<uint32_t>(readback_direction)) + ", "
                    "Received: " +
                    std::to_string(static_cast<uint32_t>(direction)));
            }
        });
    }

    uint32_t _get_cp_insertion_fifo_occupancy()
    {
        return (read_reg(REG_CP_INS_CP_LEN_FIFO_OCC_ADDR));
    }

    uint32_t _get_cp_removal_fifo_occupancy()
    {
        return (read_reg(REG_CP_REM_CP_LEN_FIFO_OCC_ADDR));
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Block compat number
    const uint32_t _fpga_compat;
    //! Block capabilities
    const uint32_t _capabilities;
    const uint32_t _max_fft_size;
    const uint32_t _max_cp_length;
    const uint32_t _max_cp_rem_list_length;
    const uint32_t _max_cp_ins_list_length;

    property_t<int> _fft_size{PROP_KEY_FFT_SIZE, DEFAULT_FFT_SIZE, {res_source_info::USER}};
    property_t<int> _fft_scaling{PROP_KEY_FFT_SCALING, DEFAULT_FFT_SCALING, {res_source_info::USER}};
    property_t<int> _fft_direction{PROP_KEY_FFT_DIRECTION, static_cast<int>(DEFAULT_FFT_DIRECTION), {res_source_info::USER}};

    property_t<std::string> _type_in = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE}};
    property_t<std::string> _type_out = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE}};
};


// FIXME: Change block ID to be different from NI ORU OFDM block
UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    ofdm_block_control, 0x0FD30000, "OFDM", CLOCK_KEY_GRAPH, "bus_clk")
