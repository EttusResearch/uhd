//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/ofdm_block_control.hpp>

using namespace uhd::rfnoc;

constexpr int            ofdm_block_control::DEFAULT_FFT_SIZE      = 4096;
constexpr int            ofdm_block_control::DEFAULT_FFT_SCALING   = 1706; // Conservative 1/N scaling
constexpr ofdm_direction ofdm_block_control::DEFAULT_FFT_DIRECTION = ofdm_direction::REVERSE;
constexpr uint32_t       ofdm_block_control::DEFAULT_CP_LEN        = 0;

const std::string ofdm_block_control::PROP_KEY_FFT_SIZE      = "fft_size";
const std::string ofdm_block_control::PROP_KEY_FFT_SCALING   = "fft_scaling";
const std::string ofdm_block_control::PROP_KEY_FFT_DIRECTION = "ofdm_direction";

const uint32_t ofdm_block_control::REG_USER_RESET_ADDR                    =  0; // Any write to this reg forces a reset for 16 clock cycles.
const uint32_t ofdm_block_control::REG_FFT_SIZE_LOG2_ADDR                 =  8; // Log2 of FFT size
const uint32_t ofdm_block_control::REG_FFT_SCALING_ADDR                   =  9; // FFT scaling word, see Xilinx documentation
const uint32_t ofdm_block_control::REG_FFT_DIRECTION_ADDR                 = 10; // FFT or IFFT
const uint32_t ofdm_block_control::REG_CP_INSERTION_LEN_ADDR              = 16; // Cyclic Prefix (CP) length (insertion)
const uint32_t ofdm_block_control::REG_CP_INSERTION_CP_LEN_FIFO_LOAD_ADDR = 17; // Any write to this reg will load a length into the cyclic prefix insertion FIFO
const uint32_t ofdm_block_control::REG_CP_INSERTION_CP_LEN_FIFO_CLR_ADDR  = 18; // Any write to this reg will clear the cyclic prefix insertion FIFO
const uint32_t ofdm_block_control::REG_CP_INSERTION_CP_LEN_FIFO_OCC_ADDR  = 19; // Read only, fullness of the cyclic prefix insertion FIFO
const uint32_t ofdm_block_control::REG_CP_REMOVAL_LEN_ADDR                = 32; // Cyclic Prefix (CP) length (removal)
const uint32_t ofdm_block_control::REG_CP_REMOVAL_CP_LEN_FIFO_LOAD_ADDR   = 33; // Any write to this reg will load a length into the cyclic prefix removal FIFO
const uint32_t ofdm_block_control::REG_CP_REMOVAL_CP_LEN_FIFO_CLR_ADDR    = 34; // Any write to this reg will clear the cyclic prefix removal FIFO
const uint32_t ofdm_block_control::REG_CP_REMOVAL_CP_LEN_FIFO_OCC_ADDR    = 35; // Read only, fullness of the cyclic prefix removal FIFO

// FFT IP constraints
constexpr uint32_t MIN_FFT_SIZE = 8;
constexpr uint32_t MAX_FFT_SIZE = 4096;

constexpr uint32_t CP_INSERTION_FIFO_DEPTH = 32;
constexpr uint32_t CP_REMOVAL_FIFO_DEPTH   = 32;

class ofdm_block_control_impl : public ofdm_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(ofdm_block_control)
    {
        set_prop_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        set_action_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
        reset();
        _register_props();
    }

    void reset()
    {
        regs().poke32(REG_USER_RESET_ADDR, 1);
        regs().peek32(REG_USER_RESET_ADDR);
    }

    void write_reg(const uint32_t addr, const uint32_t wr_val)
    {
        RFNOC_LOG_DEBUG(boost::format("write_reg(): Addr: 0x%08x, Val: 0x%08x") % addr % wr_val);
        regs().poke32(4*addr, wr_val);
    }

    uint32_t read_reg(const uint32_t addr)
    {
        uint32_t rd_val;
        rd_val = regs().peek32(4*addr);
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

    void set_cp_insertion_length(const uint32_t cp_length)
    {
        clear_cp_insertion_fifo();
        write_reg(REG_CP_INSERTION_LEN_ADDR, cp_length);
        write_reg(REG_CP_INSERTION_CP_LEN_FIFO_LOAD_ADDR, 1);

        uint32_t readback_cp_length = get_cp_insertion_length();
        if (readback_cp_length != cp_length) {
            throw uhd::value_error(
                "Insertion Cyclic Prefix length readback failed! "
                "Expected: " + std::to_string(cp_length) + ", "
                "Received: " + std::to_string(readback_cp_length));
        }
    }

    void set_cp_removal_length(const uint32_t cp_length)
    {
        clear_cp_removal_fifo();
        write_reg(REG_CP_REMOVAL_LEN_ADDR, cp_length);
        write_reg(REG_CP_REMOVAL_CP_LEN_FIFO_LOAD_ADDR, 1);

        uint32_t readback_cp_length = get_cp_removal_length();
        if (readback_cp_length != cp_length) {
            throw uhd::value_error(
                "Removal Cyclic Prefix length readback failed! "
                "Expected: " + std::to_string(cp_length) + ", "
                "Received: " + std::to_string(readback_cp_length));
        }
    }

    uint32_t get_cp_insertion_length()
    {
        return (read_reg(REG_CP_INSERTION_LEN_ADDR));
    }

    uint32_t get_cp_removal_length()
    {
        return (read_reg(REG_CP_REMOVAL_LEN_ADDR));
    }

    void clear_cp_insertion_fifo()
    {
        write_reg(REG_CP_INSERTION_CP_LEN_FIFO_CLR_ADDR, 1);

        uint32_t readback_fifo_occupancy = get_cp_insertion_fifo_occupancy();
        if (readback_fifo_occupancy != 0) {
            throw uhd::value_error(
                "Clearing Cyclic Prefix Insertion FIFO failed!");
        }
    }

    void clear_cp_removal_fifo()
    {
        write_reg(REG_CP_REMOVAL_CP_LEN_FIFO_CLR_ADDR, 1);

        uint32_t readback_fifo_occupancy = get_cp_removal_fifo_occupancy();
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
        if (cp_lengths.size() > CP_INSERTION_FIFO_DEPTH) {
            throw uhd::value_error(
                "Vector size of insertion Cyclic Prefix Lengths is too large. "
                "Must be between 1 and " +
                std::to_string(CP_INSERTION_FIFO_DEPTH) + ".");
        }

        clear_cp_insertion_fifo();

        // Write cyclic prefix lengths into config FIFO
        uint32_t readback_cp_length;
        for (auto &cp_length : cp_lengths) {
            write_reg(REG_CP_INSERTION_LEN_ADDR, cp_length);
            write_reg(REG_CP_INSERTION_CP_LEN_FIFO_LOAD_ADDR, 1);
            RFNOC_LOG_DEBUG("Setting insertion Cyclic Prefix Length " << cp_length);
            readback_cp_length = get_cp_insertion_length();
            if (readback_cp_length != cp_length) {
                throw uhd::value_error(
                    "Insertion Cyclic Prefix length readback failed! "
                    "Expected: " + std::to_string(cp_length) + ", " +
                    "Received: " + std::to_string(readback_cp_length));
            }
        }
    }

    void load_cp_removal_fifo(const std::vector<uint32_t> cp_lengths)
    {
        // Check Cyclic Prefix length is size within bounds
        if (cp_lengths.size() == 0) {
            throw uhd::value_error("Vector of removal Cyclic Prefix Lengths cannot be empty");
        }
        if (cp_lengths.size() > CP_REMOVAL_FIFO_DEPTH) {
            throw uhd::value_error(
                "Vector size of removal Cyclic Prefix Lengths is too large. "
                "Must be between 1 and " +
                std::to_string(CP_REMOVAL_FIFO_DEPTH) + ".");
        }

        clear_cp_removal_fifo();

        // Write cyclic prefix lengths into config FIFO
        uint32_t readback_cp_length;
        for (auto &cp_length : cp_lengths) {
            write_reg(REG_CP_REMOVAL_LEN_ADDR, cp_length);
            write_reg(REG_CP_REMOVAL_CP_LEN_FIFO_LOAD_ADDR, 1);
            RFNOC_LOG_DEBUG("Setting Cyclic Prefix Length " << cp_length);
            readback_cp_length = get_cp_removal_length();
            if (readback_cp_length != cp_length) {
                throw uhd::value_error(
                    "Removal Cyclic Prefix readback failed! "
                    "Expected: " + std::to_string(cp_length) + ", "
                    "Received: " + std::to_string(readback_cp_length));
            }
        }
    }

    uint32_t get_cp_insertion_fifo_occupancy()
    {
        return (read_reg(REG_CP_INSERTION_CP_LEN_FIFO_OCC_ADDR));
    }

    uint32_t get_cp_removal_fifo_occupancy()
    {
        return (read_reg(REG_CP_REMOVAL_CP_LEN_FIFO_OCC_ADDR));
    }

private:
    void _register_props()
    {
        register_property(&_fft_size, [this]() {
            // Check FFT size within bounds and is a power of 2
            uint32_t fft_size = static_cast<uint32_t>(this->_fft_size.get());
            if (fft_size < MIN_FFT_SIZE || fft_size > MAX_FFT_SIZE) {
                throw uhd::value_error(
                    "FFT size must be a power of two and within [" +
                    std::to_string(MIN_FFT_SIZE) + ", " +
                    std::to_string(MAX_FFT_SIZE) + "]");
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
                this->_fft_size.set(coerced_fft_size);
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
            // Check FFT size within bounds and is a power of 2
            uint32_t fft_scaling = static_cast<uint32_t>(_fft_scaling.get());
            if (fft_scaling < 0 || fft_scaling > (1 << 12) - 1) {
                throw uhd::value_error("FFT Scaling must be in [0, 4095]");
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
