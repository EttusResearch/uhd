//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/utils/noncopyable.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

/*!
 * SPI core driver for the ctrlport_spi_adrv FPGA core.
 *
 * This SPI core uses block_poke32/block_peek32 and burst_poke32/burst_peek32 to transfer
 * all data words in a single RFNoC CtrlPort packet. This is critical for large transfers
 * where per-word packet overhead would be prohibitive.
 *
 * The core supports five modes for SPI communication:
 *   SINGLE_INSTRUCTION_SEQUENTIAL : each new triplet has an incremented address
 *   SINGLE_INSTRUCTION_REPEATED : every triplet shares the same address
 *   RAW    : each triplet has new addresses that aren't just increments
 *   SINGLE_INSTRUCTION     : Same as RAW, but less than 3 triplets, so they can fit
 *   in the SINGLE_INSTRUCTION register.
 *   STREAMING : two address bytes followed by a stream of data bytes.
 *
 * Register layout (offsets from base address, from ctrlport_spi_adrv_pkg.sv):
 *   HALF_PER_OFFSET           : base + 0x00
 *   SINGLE_INSTRUCTION_OFFSET : base + 0x04  (single-instruction write/read)
 *   CONTROL_OFFSET            : base + 0x3C  (write to trigger a transaction)
 *   DATA_OFFSET               : base + 0x40  (16-word / 64-byte window)
 *
 * REG_CONTROL bit fields (RAW mode):
 *   [31]     dir       : 0 = write, 1 = read
 *   [29:28]  mode      : 0b00 = RAW
 *   [23:16]  num_bytes : total number of data bytes in this transaction
 *   [15:0]   spi_addr  : unused in RAW mode
 *
 * Byte ordering in REG_DATA words (little-endian):
 *   byte 0 -> word[0][7:0], byte 1 -> word[0][15:8], ...
 *   Each byte is serialized MSB-first on the wire.
 */
class spi_core_adrv : uhd::noncopyable
{
public:
    using sptr              = std::shared_ptr<spi_core_adrv>;
    using block_poke32_fn_t = std::function<void(uint32_t, const std::vector<uint32_t>&)>;
    using block_peek32_fn_t = std::function<std::vector<uint32_t>(uint32_t, size_t)>;
    using burst_poke32_fn_t = std::function<void(uint32_t, const std::vector<uint32_t>&)>;
    using burst_peek32_fn_t = std::function<std::vector<uint32_t>(uint32_t, size_t)>;

    virtual ~spi_core_adrv(void) = default;

    /*!
     * Write bytes to the ADRV SPI core.
     *
     * Detects mode for transaction and then writes data to the device in the most
     * efficient way.
     *
     * \param data      pointer to the byte array to transmit
     * \param num_bytes number of bytes to transmit
     */
    virtual void adrv_spi_write(const uint8_t data[], const uint32_t num_bytes) = 0;

    /*!
     * Read bytes from the ADRV SPI core.
     *
     * Detects mode for transaction and then reads data from the device in the most
     * efficient way.
     *
     * \param tx_data   pointer to the byte array to transmit
     * \param rx_data   pointer to the byte array to receive into
     * \param num_bytes number of bytes to transfer in each direction
     */
    virtual void adrv_spi_read(
        const uint8_t tx_data[], uint8_t rx_data[], const uint32_t num_bytes) = 0;

    /*!
     * Factory function.
     *
     * \param block_poke32_fn block write — must call regs().block_poke32()
     * \param block_peek32_fn block read  — must call regs().block_peek32()
     * \param burst_poke32_fn burst write — must call regs().burst_poke32()
     * \param burst_peek32_fn burst read  — must call regs().burst_peek32()
     * \param base            byte address of the ctrlport_spi_adrv window
     *                        (e.g. b300_regs::SR_ADRV_SPI)
     */
    static sptr make(block_poke32_fn_t block_poke32_fn,
        block_peek32_fn_t block_peek32_fn,
        burst_poke32_fn_t burst_poke32_fn,
        burst_peek32_fn_t burst_peek32_fn,
        uint32_t base);
};
