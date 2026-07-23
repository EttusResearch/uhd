//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhdlib/usrp/cores/spi_core_adrv.hpp>
#include <adi_adrv903x_types.h>
#include <algorithm>
#include <utility>
#include <vector>

namespace {
constexpr uint32_t SINGLE_INSTRUCTION_OFFSET = 0x04;
constexpr uint32_t CONTROL_OFFSET            = 0x3C;
constexpr uint32_t DATA_OFFSET               = 0x40;

// REG_CONTROL field positions
constexpr uint32_t CTRL_DIR_BIT_SHIFT   = 31; // 0=write, 1=read
constexpr uint32_t CTRL_MODE_BITS_SHIFT = 28;
constexpr uint32_t CTRL_NUM_BITS_SHIFT  = 16;
constexpr uint32_t CTRL_NUM_BYTES_MASK  = 0xFF;
constexpr uint32_t CTRL_ADDR_MASK       = 0xFFFF;

// Modes for SPI communication
constexpr uint32_t RAW                           = 0;
constexpr uint32_t SINGLE_INSTRUCTION_SEQUENTIAL = 1;
constexpr uint32_t SINGLE_INSTRUCTION_REPEATED   = 2;
constexpr uint32_t STREAMING                     = 3;
constexpr uint32_t SINGLE_INSTRUCTION            = 4;

// Maximum entries in the FPGA RX FIFO (= 2^(NUM_BYTES_W-2), one word per 4 bytes).
// _single_instruction_read must not queue more than this many triplets before
// draining.
constexpr size_t RX_FIFO_SIZE = (CTRL_NUM_BYTES_MASK + 1) / 4;
// Maximum words per block write/read (AXIS-Ctrl limit is 15 words per packet)
constexpr size_t MAX_BLOCK_WORDS = 15;

// Return the 16-bit SPI address embedded in addr-data triplet idx of a packed Single
// Instruction stream.
inline uint16_t get_addr_from_triplet(const uint8_t* data, size_t idx)
{
    return static_cast<uint16_t>((data[3 * idx] << 8) | data[3 * idx + 1]);
}
} // namespace

class spi_core_adrv_impl : public spi_core_adrv
{
public:
    spi_core_adrv_impl(block_poke32_fn_t block_poke32_fn,
        block_peek32_fn_t block_peek32_fn,
        burst_poke32_fn_t burst_poke32_fn,
        burst_peek32_fn_t burst_peek32_fn,
        uint32_t base)
        : _block_poke32(std::move(block_poke32_fn))
        , _block_peek32(std::move(block_peek32_fn))
        , _burst_poke32(std::move(burst_poke32_fn))
        , _burst_peek32(std::move(burst_peek32_fn))
        , _single_instruction_addr(base + SINGLE_INSTRUCTION_OFFSET)
        , _control_addr(base + CONTROL_OFFSET)
        , _data_addr(base + DATA_OFFSET)
    {
    }

    void adrv_spi_write(const uint8_t data[], const uint32_t num_bytes) override
    {
        // Note: For mode detection, we only check the first write in the
        // transaction for mode register writes. This assumes that mode changes
        // are not buried in the middle of the request.
        if (_streaming_mode) {
            // Streaming / FIFO mode: SPI wire format is [addr_hi, addr_lo, d0,
            // d1, d2, ...].
            UHD_ASSERT_THROW(num_bytes >= 3);
            UHD_ASSERT_THROW((num_bytes - 2) <= CTRL_NUM_BYTES_MASK);
            const uint16_t spi_addr = static_cast<uint16_t>((data[0] << 8) | data[1]);
            const size_t num_data   = num_bytes - 2;
            // Check for STREAMING -> SI transition (CONFIG_B bit 7 set)
            if (spi_addr == ADRV903X_ADDR_SPI_INTERFACE_CONFIG_B
                && (data[2] & ADRV903X_CONFIG_B_SINGLE_INSTRUCTION)) {
                _streaming_mode = false;
            }
            _write(STREAMING, spi_addr, data + 2, num_data);
        } else {
            // Single Instruction mode: SPI wire format is [addr_hi0,
            // addr_lo0, d0, addr_hi1, addr_lo1, d1, ... ].
            UHD_ASSERT_THROW(num_bytes >= 3 && num_bytes % 3 == 0);
            const size_t num_triplets = num_bytes / 3;
            // Check for SINGLE INSTRUCTION -> STREAMING transition.
            const uint16_t base_addr = get_addr_from_triplet(data, 0);
            if (base_addr == ADRV903X_ADDR_SPI_INTERFACE_CONFIG_B
                && !(data[2] & ADRV903X_CONFIG_B_SINGLE_INSTRUCTION)) {
                _streaming_mode = true;
            }
            // Classify the address pattern and dispatch to the most efficient
            // transaction type.
            const uint32_t mode = _determine_transaction_mode(data, num_triplets);
            if (mode == SINGLE_INSTRUCTION) {
                // Mixed addresses with <= 3 triplets. Use SINGLE_INSTRUCTION register.
                _single_instruction_write(data, num_triplets);
            } else if (mode == RAW) {
                // Mixed addresses with > 3 triplets. Use RAW mode.
                UHD_ASSERT_THROW(num_bytes <= CTRL_NUM_BYTES_MASK);
                _write(RAW, 0, data, num_bytes);
            } else if (mode == SINGLE_INSTRUCTION_SEQUENTIAL
                       || mode == SINGLE_INSTRUCTION_REPEATED) {
                // Address is constant or incrementing.
                std::vector<uint8_t> packed(num_triplets);
                for (size_t i = 0; i < num_triplets; ++i)
                    packed[i] = data[3 * i + 2];
                _write(mode, base_addr, packed.data(), num_triplets);
            } else {
                throw uhd::runtime_error(
                    "ADRV SPI Core was unable to determine SPI Mode.");
            }
        }
    }

    void adrv_spi_read(
        const uint8_t tx_data[], uint8_t rx_data[], const uint32_t num_bytes) override
    {
        if (_streaming_mode) {
            // Streaming / FIFO mode: SPI wire format is [addr_hi, addr_lo, d0,
            // d1, d2, ...].
            UHD_ASSERT_THROW(num_bytes >= 3);
            UHD_ASSERT_THROW((num_bytes - 2) <= CTRL_NUM_BYTES_MASK);
            const uint16_t spi_addr =
                static_cast<uint16_t>((tx_data[0] << 8) | tx_data[1]);
            const size_t num_data = num_bytes - 2;
            rx_data[0]            = tx_data[0];
            rx_data[1]            = tx_data[1];
            _read(STREAMING, spi_addr, rx_data + 2, num_data);
        } else {
            // Single Instruction mode: SPI wire format is [addr_hi0,
            // addr_lo0, d0, addr_hi1, addr_lo1, d1, ... ].
            UHD_ASSERT_THROW(num_bytes >= 3 && num_bytes % 3 == 0);
            const size_t num_triplets = num_bytes / 3;
            UHD_ASSERT_THROW(num_triplets <= CTRL_NUM_BYTES_MASK);
            const uint16_t base_addr = get_addr_from_triplet(tx_data, 0);
            // Classify the address pattern and dispatch to the most efficient
            // transaction type.
            const uint32_t mode = _determine_transaction_mode(tx_data, num_triplets);
            if (mode == SINGLE_INSTRUCTION) {
                // Mixed addresses with <= 3 triplets. Use SINGLE_INSTRUCTION register.
                _single_instruction_read(tx_data, rx_data, num_triplets);
            } else if (mode == RAW) {
                // Mixed addresses with > 3 triplets. Use RAW mode.
                UHD_ASSERT_THROW(num_bytes <= CTRL_NUM_BYTES_MASK);
                _raw_read(tx_data, rx_data, num_bytes);
            } else if (mode == SINGLE_INSTRUCTION_SEQUENTIAL
                       || mode == SINGLE_INSTRUCTION_REPEATED) {
                // Address is constant or incrementing.
                std::vector<uint8_t> rx(num_triplets);
                _read(mode, base_addr, rx.data(), num_triplets);
                for (size_t i = 0; i < num_triplets; ++i) {
                    rx_data[3 * i]     = tx_data[3 * i];
                    rx_data[3 * i + 1] = tx_data[3 * i + 1];
                    rx_data[3 * i + 2] = rx[i];
                }
            } else {
                throw uhd::runtime_error(
                    "ADRV SPI Core was unable to determine SPI Mode.");
            }
        }
    }

private:
    // Issue num_triplets writes to REG_SI from a packed SI triplet stream.
    // Used for SINGLE_INSTRUCTION. data = [addr_hi0, addr_lo0, data0, addr_hi1,
    // addr_lo1, data1, ...].
    void _single_instruction_write(const uint8_t* data, size_t num_triplets)
    {
        std::vector<uint32_t> si_vals;
        si_vals.reserve(num_triplets);
        for (size_t i = 0; i < num_triplets; ++i) {
            const uint8_t* t = data + 3 * i;
            si_vals.push_back(static_cast<uint32_t>(t[0])
                              | (static_cast<uint32_t>(t[1]) << 8)
                              | (static_cast<uint32_t>(t[2]) << 16));
        }
        _burst_poke32(_single_instruction_addr, si_vals);
    }

    // Issue num_triplets reads using SINGLE_INSTRUCTION. tx_data is a packed SI
    // triplet stream. Each rx word holds 3 MISO bytes little-endian and is
    // unpacked back into rx_data. Writes are interleaved with reads in chunks
    // of RX_FIFO_SIZE to avoid overflowing the FPGA RX FIFO. tx_data/rx_data =
    // [addr_hi0, addr_lo0, data0, addr_hi1, addr_lo1, data1, ...].
    void _single_instruction_read(
        const uint8_t* tx_data, uint8_t* rx_data, size_t num_triplets)
    {
        for (size_t offset = 0; offset < num_triplets; offset += RX_FIFO_SIZE) {
            const size_t chunk = std::min(num_triplets - offset, RX_FIFO_SIZE);
            std::vector<uint32_t> si_vals;
            si_vals.reserve(chunk);
            for (size_t i = 0; i < chunk; ++i) {
                const uint8_t* t = tx_data + 3 * (offset + i);
                si_vals.push_back(
                    static_cast<uint32_t>(t[0]) | (static_cast<uint32_t>(t[1]) << 8));
            }
            _burst_poke32(_single_instruction_addr, si_vals);
            const auto rx_words = _burst_peek32(_data_addr, chunk);
            for (size_t i = 0; i < chunk; ++i) {
                rx_data[3 * (offset + i)] = static_cast<uint8_t>(rx_words[i] & 0xFF);
                rx_data[3 * (offset + i) + 1] =
                    static_cast<uint8_t>((rx_words[i] >> 8) & 0xFF);
                rx_data[3 * (offset + i) + 2] =
                    static_cast<uint8_t>((rx_words[i] >> 16) & 0xFF);
            }
        }
    }

    // Build and send a RAW read transaction: ctrl word + TX byte stream,
    // then drain the RX byte stream.
    void _raw_read(const uint8_t* tx_data, uint8_t* rx_data, size_t num_bytes)
    {
        UHD_ASSERT_THROW(num_bytes <= CTRL_NUM_BYTES_MASK);
        const uint32_t ctrl = (1u << CTRL_DIR_BIT_SHIFT) | (RAW << CTRL_MODE_BITS_SHIFT)
                              | ((static_cast<uint32_t>(num_bytes) & CTRL_NUM_BYTES_MASK)
                                  << CTRL_NUM_BITS_SHIFT);
        _send(ctrl, tx_data, num_bytes);
        _recv(rx_data, num_bytes);
    }

    // Build the ctrl word and send a write transaction to the FPGA. The ctrl word is
    // followed by data bytes packed as little-endian 32-bit words.
    void _write(uint32_t mode, uint16_t spi_addr, const uint8_t* data, size_t num_bytes)
    {
        UHD_ASSERT_THROW(num_bytes <= CTRL_NUM_BYTES_MASK);
        const uint32_t ctrl = (mode << CTRL_MODE_BITS_SHIFT)
                              | ((static_cast<uint32_t>(num_bytes) & CTRL_NUM_BYTES_MASK)
                                  << CTRL_NUM_BITS_SHIFT)
                              | static_cast<uint32_t>(spi_addr & CTRL_ADDR_MASK);
        _send(ctrl, data, num_bytes);
    }

    // Build and write the ctrl word then read the resulting data from FPGA.
    void _read(uint32_t mode, uint16_t spi_addr, uint8_t* data, size_t num_bytes)
    {
        UHD_ASSERT_THROW(num_bytes <= CTRL_NUM_BYTES_MASK);
        const uint32_t ctrl = (1u << CTRL_DIR_BIT_SHIFT) | (mode << CTRL_MODE_BITS_SHIFT)
                              | ((static_cast<uint32_t>(num_bytes) & CTRL_NUM_BYTES_MASK)
                                  << CTRL_NUM_BITS_SHIFT)
                              | static_cast<uint32_t>(spi_addr & CTRL_ADDR_MASK);
        _block_poke32(_control_addr, {ctrl});
        _recv(data, num_bytes);
    }

    // Writes a pre-built ctrl word to REG_CONTROL followed by
    // data bytes (packed little-endian) to REG_DATA, chunked to
    // MAX_BLOCK_WORDS per ctrlport packet.
    void _send(uint32_t ctrl, const uint8_t* data, size_t num_bytes)
    {
        size_t word_offset     = 1;
        const size_t num_words = (num_bytes + 3) / 4;
        std::vector<uint32_t> tx_words(word_offset + num_words, 0);
        for (size_t i = 0; i < num_bytes; ++i) {
            tx_words[word_offset + i / 4] |= static_cast<uint32_t>(data[i])
                                             << ((i % 4) * 8);
        }
        tx_words[0]              = ctrl;
        const size_t first_chunk = std::min(tx_words.size(), MAX_BLOCK_WORDS);
        _block_poke32(_control_addr,
            std::vector<uint32_t>(tx_words.begin(), tx_words.begin() + first_chunk));
        for (size_t offset = first_chunk; offset < tx_words.size();
             offset += MAX_BLOCK_WORDS) {
            const size_t chunk = std::min(tx_words.size() - offset, MAX_BLOCK_WORDS);
            _block_poke32(_data_addr,
                std::vector<uint32_t>(
                    tx_words.begin() + offset, tx_words.begin() + offset + chunk));
        }
    }

    // Reads num_bytes from the FPGA's RX FIFO via data register, chunked to
    // MAX_BLOCK_WORDS per ctrlport packet.
    void _recv(uint8_t* data, size_t num_bytes)
    {
        const size_t num_words = (num_bytes + 3) / 4;
        std::vector<uint32_t> rx_words;
        rx_words.reserve(num_words);
        for (size_t offset = 0; offset < num_words; offset += MAX_BLOCK_WORDS) {
            const size_t chunk     = std::min(num_words - offset, MAX_BLOCK_WORDS);
            const auto chunk_words = _block_peek32(_data_addr, chunk);
            rx_words.insert(rx_words.end(), chunk_words.begin(), chunk_words.end());
        }
        for (size_t i = 0; i < num_bytes; ++i) {
            data[i] = static_cast<uint8_t>((rx_words[i / 4] >> ((i % 4) * 8)) & 0xFF);
        }
    }

    // Determine the best transaction mode for a given SPI transaction. Triplet refers to
    // a series of 2 address bytes + 1 data byte.
    //
    //   SINGLE_INSTRUCTION_SEQUENTIAL : each new triplet has an incremented address
    //   SINGLE_INSTRUCTION_REPEATED : every triplet shares the same address
    //   RAW    : each triplet has new addresses that aren't just increments
    //   SINGLE_INSTRUCTION     : Same as RAW, but 3 or fewer triplets, so they can fit
    //   in the SINGLE_INSTRUCTION register.
    uint32_t _determine_transaction_mode(const uint8_t* data, size_t num_triplets)
    {
        const uint16_t base = get_addr_from_triplet(data, 0);
        size_t i            = 1;
        for (; i < num_triplets; ++i) {
            if (get_addr_from_triplet(data, i) != static_cast<uint16_t>(base + i))
                break;
        }
        if (i == num_triplets)
            return SINGLE_INSTRUCTION_SEQUENTIAL;
        for (i = 1; i < num_triplets; ++i) {
            if (get_addr_from_triplet(data, i) != base)
                return (num_triplets <= 3) ? SINGLE_INSTRUCTION : RAW;
        }
        return SINGLE_INSTRUCTION_REPEATED;
    }

    block_poke32_fn_t _block_poke32;
    block_peek32_fn_t _block_peek32;
    burst_poke32_fn_t _burst_poke32;
    burst_peek32_fn_t _burst_peek32;
    const uint32_t _single_instruction_addr;
    const uint32_t _control_addr;
    const uint32_t _data_addr;
    // ADI chip starts in Single Instruction mode. Transitions are detected
    // by checking for writes to CONFIG_B (addr 0x0001); bit 7 is the SI-mode enable:
    //   SINGLE INSTRUCTION -> STREAMING : bit 7 == 0
    //   STREAMING -> SINGLE INSTRUCTION : bit 7 == 1
    // FIFO mode is always entered from STREAMING and exits back to STREAMING,
    // with an identical wire format, so no separate tracking is needed.
    bool _streaming_mode = false;
};

spi_core_adrv::sptr spi_core_adrv::make(block_poke32_fn_t block_poke32_fn,
    block_peek32_fn_t block_peek32_fn,
    burst_poke32_fn_t burst_poke32_fn,
    burst_peek32_fn_t burst_peek32_fn,
    uint32_t base)
{
    return std::make_shared<spi_core_adrv_impl>(std::move(block_poke32_fn),
        std::move(block_peek32_fn),
        std::move(burst_poke32_fn),
        std::move(burst_peek32_fn),
        base);
}
