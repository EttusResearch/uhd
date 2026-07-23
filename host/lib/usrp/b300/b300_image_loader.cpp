//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_image_loader.hpp"
#include "b300_impl.hpp"
#include "b300_mb_eeprom.hpp"
#include "b300_pcie_mgr.hpp"
#include "b300_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/types/byte_vector.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/usrp/cores/i2c_core_100_wb32.hpp>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace uhd { namespace usrp { namespace b300 {

static const uint32_t READ_DATA_DATA_VALID = (1 << 31);
static const uint16_t READ_DATA_DATA_MASK  = 0xFFFF;

static const uint32_t STATUS_OUTPUT_BUSY = (1 << 2);
static const uint32_t STATUS_INITIALIZED = (1 << 0);

static const uint32_t TX_FIFO_SIZE = (0xFFFF << 16);
static const uint32_t RX_FIFO_SIZE = 0xFFFF;

static const uint32_t TX_FIFO_EMPTINESS   = (0xFFFF << 16);
static const uint32_t RX_FIFO_FULLNESS    = 0xFFFF;
static const uint32_t RX_FIFO_EMTPY_CHECK = 0x200000;

static const uint32_t FLASH_BUS_WIDTH         = 0x2;
static const uint32_t FLASH_PAGE_SIZE         = 0x100;
static const uint32_t FLASH_SECTOR_SIZE       = 0x40000;
static const uint32_t FLASH_SUBSECTOR_SIZE    = 0x2000;
static const uint32_t FLASH_MAX_IMAGE_SIZE    = 0xF2471C;
static const uint32_t FLASH_BOARD_INFO_ADDR   = 0x0;
static const uint32_t FLASH_USER_POINTER_ADDR = 0x2000;
static const uint32_t FLASH_USER_ADDR         = 0x2000000;
static const uint16_t FLASH_WRITE_IN_PROGRESS = (1 << 0);

static const uint8_t RESET                     = 0xFF;
static const uint8_t ALIGN                     = 0xAA;
static const uint8_t READ_CPLD_REV_YM          = 0x09;
static const uint8_t READ_CPLD_REV_DH          = 0x0A;
static const uint8_t READ_SIG_UPPER            = 0x07;
static const uint8_t READ_SIG_LOWER            = 0x08;
static const uint8_t SET_UPPER_FLASH_ADDR      = 0x13;
static const uint8_t SET_LOWER_FLASH_ADDR      = 0x14;
static const uint8_t INIT_FLASH_READ           = 0x15;
static const uint8_t READ_FLASH_AND_INCR_ADDR  = 0x16;
static const uint8_t INIT_FLASH_WRITE          = 0x17;
static const uint8_t WRITE_FLASH_AND_INCR_ADDR = 0x18;
static const uint8_t END_FLASH_TRANSFER        = 0x19;
static const uint8_t READ_FLASH_STATUS         = 0x1A;
static const uint8_t ERASE_FLASH_SECTOR        = 0x1B;
static const uint8_t ERASE_FLASH_SUBSECTOR     = 0x1C;
static const uint8_t READ_RST_CTRL             = 0x1D;
static const uint8_t WRITE_RST_CTRL            = 0x1E;

static const uint16_t RESET_DATA = 0xFFFF;
static const uint16_t ALIGN_DATA = 0xAAAA;
static const uint16_t DATA_MASK  = 0xFFFF;

b300_image_loader_helper::b300_image_loader_helper(
    std::function<uint32_t(uint32_t)>&& read,
    std::function<void(uint32_t, uint32_t)>&& write)
    : _read32(std::move(read)), _write32(std::move(write))
{
    // Test FPGA-CPLD interface scratch pad
    std::vector<uint32_t> test_values = {0, 0xAABBCCDD};
    for (uint32_t val : test_values) {
        _write32(FLASH_SCRATCH_OFFSET, val);
        uint32_t scratch = _read32(FLASH_SCRATCH_OFFSET);
        if (scratch != val) {
            throw std::runtime_error("FPGA-CPLD interface test failed!");
        }
    }

    // Reset and Align the CPLD
    _writeData(RESET, RESET_DATA);
    _writeData(ALIGN, ALIGN_DATA);

    // Wait for the CPLD to initialize
    bool cpld_initialized = false;
    auto start_time       = std::chrono::steady_clock::now();
    auto max_time         = std::chrono::seconds(1);

    do {
        // Wait polling interval
        std::this_thread::sleep_for(std::chrono::microseconds(25));
        // Read status register
        cpld_initialized = (_read32(FLASH_STATUS_OFFSET) & STATUS_INITIALIZED) != 0;
    } while (
        !cpld_initialized && (std::chrono::steady_clock::now() - start_time) < max_time);

    if (!cpld_initialized) {
        throw uhd::runtime_error("CPLD initialization failed!");
    }

    // Flush the read FIFO
    auto flush_start_time = std::chrono::steady_clock::now();
    auto flush_timeout    = std::chrono::seconds(10);
    while (_readData(true)
           && ((std::chrono::steady_clock::now() - flush_start_time) < flush_timeout)) {
        // Continue draining FIFO until empty or timeout
    }

    // Flush the Rx FIFO
    uint32_t fifo_status = _read32(FLASH_FIFO_STATUS_OFFSET);
    if (fifo_status != RX_FIFO_EMTPY_CHECK) {
        uint32_t fullness = (fifo_status & RX_FIFO_FULLNESS) / 2;
        for (uint32_t i = 0; i < fullness; ++i) {
            _read32(FLASH_FAST_READ_OFFSET);
        }
    }

    // Test CPLD Revision that it matches one of the valid CPLD revisions.
    const std::vector<uint32_t> allowed_cpld_revisions =
        std::vector<uint32_t>{0x26010611, 0x18043017};
    uint32_t read_cpld_revision =
        (static_cast<uint32_t>(_sendCommandGetData(READ_CPLD_REV_YM)) << 16)
        | _sendCommandGetData(READ_CPLD_REV_DH);
    if (std::find(allowed_cpld_revisions.begin(),
            allowed_cpld_revisions.end(),
            read_cpld_revision)
        == allowed_cpld_revisions.end()) {
        throw uhd::runtime_error(
            "Unexpected Revision for CPLD: " + std::to_string(read_cpld_revision));
    }

    // Test CPLD Signature
    if (((static_cast<uint32_t>(_sendCommandGetData(READ_SIG_UPPER)) << 16)
            | _sendCommandGetData(READ_SIG_LOWER))
        != 0x79fc1477) {
        throw uhd::runtime_error("Unexpected CPLD signature!");
    }
}

uint16_t b300_image_loader_helper::_sendCommandGetData(uint8_t cmd, uint16_t data)
{
    _writeData(cmd, data);
    return _readData();
}

uint16_t b300_image_loader_helper::_readData(bool ignore_timeout)
{
    try {
        auto start_time        = std::chrono::steady_clock::now();
        auto max_time          = std::chrono::seconds(1);
        bool data_valid        = false;
        uint32_t read_data_reg = 0;

        do {
            // Read the data from ReadData register
            read_data_reg = _read32(FLASH_READ_DATA_OFFSET);
            data_valid    = (read_data_reg & READ_DATA_DATA_VALID) != 0;
        } while (
            !data_valid && ((std::chrono::steady_clock::now() - start_time) < max_time));

        if (!data_valid) {
            if (!ignore_timeout) {
                UHD_LOG_ERROR("B300 IMAGE LOADER", "Timeout waiting for data valid bit!");
            }
            return 0;
        }

        return static_cast<uint16_t>(read_data_reg & READ_DATA_DATA_MASK);

    } catch (const std::exception& e) {
        UHD_LOG_ERROR("B300 IMAGE LOADER", "Error reading data: " << e.what());
        return 0;
    }
}

std::vector<uint16_t> b300_image_loader_helper::_readDataBlock(
    uint32_t address, uint32_t words_to_read)
{
    if (words_to_read == 0) {
        return {};
    }

    std::vector<uint16_t> data_array;

    // Configure the transfer
    if (address % FLASH_BUS_WIDTH != 0) {
        UHD_LOG_ERROR("B300 IMAGE LOADER",
            "Address 0x" << std::hex << address << " not aligned to Flash bus width "
                         << FLASH_BUS_WIDTH);
        return {};
    }

    // Set address and start read transfer
    _writeData(SET_UPPER_FLASH_ADDR, (address >> 16) & DATA_MASK);
    _writeData(SET_LOWER_FLASH_ADDR, address & DATA_MASK);
    _writeData(INIT_FLASH_READ, 0);

    // Query FIFO sizes
    uint32_t fifo_size_reg = _read32(FLASH_FIFO_SIZE_OFFSET);
    uint32_t tx_fifo_size  = (fifo_size_reg & TX_FIFO_SIZE) >> 16;
    uint32_t rx_fifo_size  = fifo_size_reg & RX_FIFO_SIZE;
    uint32_t fifo_size =
        std::min(tx_fifo_size, rx_fifo_size) / 2; // Fast access reads 2 words at a time

    // Set command for fast access
    _write32(FLASH_FAST_COMMAND_OFFSET, READ_FLASH_AND_INCR_ADDR);

    // Prime transmit FIFO
    uint32_t fifo_half_full_size = fifo_size / 2;
    uint32_t total_fast_reads    = words_to_read / 2;
    uint32_t write_batch_size    = std::min(fifo_half_full_size, total_fast_reads);
    for (uint32_t i = 0; i < write_batch_size; ++i) {
        _write32(FLASH_FAST_WRITE_OFFSET, 0); // Data doesn't matter for reads
    }
    uint32_t write_count = write_batch_size;

    // Continue issuing reads and collecting results
    while (write_count < total_fast_reads) {
        write_batch_size = std::min(fifo_half_full_size, total_fast_reads - write_count);

        // Enqueue writes
        for (uint32_t i = 0; i < write_batch_size; ++i) {
            _write32(FLASH_FAST_WRITE_OFFSET, 0);
        }
        write_count += write_batch_size;

        // Read results
        uint32_t read_batch_size             = std::min(fifo_half_full_size,
            total_fast_reads - static_cast<uint32_t>(data_array.size()) / 2);
        std::vector<uint32_t> data           = _fastReadData(read_batch_size);
        std::vector<uint16_t> converted_data = _u32ArrayToU16Array(data);
        data_array.insert(data_array.end(), converted_data.begin(), converted_data.end());
    }

    // Read remaining fast reads
    std::vector<uint32_t> data =
        _fastReadData(total_fast_reads - static_cast<uint32_t>(data_array.size()) / 2);
    std::vector<uint16_t> converted_data = _u32ArrayToU16Array(data);
    data_array.insert(data_array.end(), converted_data.begin(), converted_data.end());

    // Handle odd word count
    if (words_to_read % 2 != 0) {
        data_array.push_back(_sendCommandGetData(READ_FLASH_AND_INCR_ADDR));
    }

    _writeData(END_FLASH_TRANSFER, 0);

    data_array.resize(words_to_read); // Return only requested number of words
    return data_array;
}

std::vector<uint16_t> b300_image_loader_helper::_u32ArrayToU16Array(
    const std::vector<uint32_t>& data)
{
    std::vector<uint16_t> data_array;
    data_array.reserve(data.size() * 2);
    for (uint32_t d : data) {
        uint16_t word_low  = d & 0xFFFF;
        uint16_t word_high = (d >> 16) & 0xFFFF;
        data_array.push_back(word_low);
        data_array.push_back(word_high);
    }
    return data_array;
}

std::vector<uint16_t> b300_image_loader_helper::_u8ArrayToU16Array(
    const std::vector<uint8_t>& data)
{
    std::vector<uint16_t> data_array;
    data_array.reserve((data.size() + 1) / 2);
    for (size_t i = 0; i < data.size(); i += 2) {
        uint8_t word_low  = data[i];
        uint8_t word_high = (i + 1 < data.size()) ? data[i + 1] : 0;
        data_array.push_back(word_low | (static_cast<uint16_t>(word_high) << 8));
    }
    return data_array;
}

std::vector<uint8_t> b300_image_loader_helper::_u16ArrayToU8Array(
    const std::vector<uint16_t>& data)
{
    std::vector<uint8_t> data_array;
    data_array.reserve(data.size() * 2);
    for (uint16_t d : data) {
        uint8_t word_low  = d & 0xFF;
        uint8_t word_high = (d >> 8) & 0xFF;
        data_array.push_back(word_low);
        data_array.push_back(word_high);
    }
    return data_array;
}

std::vector<uint32_t> b300_image_loader_helper::_fastReadData(uint32_t num_double_words)
{
    std::vector<uint32_t> data;
    data.reserve(num_double_words);
    uint32_t fast_read_attempts = 0;

    while (data.size() < num_double_words) {
        uint32_t fifo_status  = _read32(FLASH_FIFO_STATUS_OFFSET);
        uint32_t burst_length = (fifo_status & RX_FIFO_FULLNESS) / 2;
        burst_length =
            std::min(num_double_words - static_cast<uint32_t>(data.size()), burst_length);

        for (uint32_t burst_count = 0; burst_count < burst_length; ++burst_count) {
            data.push_back(_read32(FLASH_FAST_READ_OFFSET));
        }

        fast_read_attempts++;
        if (fast_read_attempts == 10) {
            throw std::runtime_error("Fast read failed");
        }
    }

    return data;
}

uint8_t b300_image_loader_helper::_read8(uint32_t offset)
{
    uint32_t read_address      = offset - (offset % 2);
    std::vector<uint16_t> temp = _readDataBlock(read_address, 1);
    if (temp.empty())
        return 0;
    return static_cast<uint8_t>(temp[0] >> ((offset % 2) * 8));
}

std::vector<uint8_t> b300_image_loader_helper::_readBlock8Internal(
    uint32_t offset, uint32_t byte_count)
{
    std::vector<uint8_t> values;
    values.reserve(byte_count);
    uint32_t next_offset = offset;
    uint32_t bytes_left  = byte_count;

    if (next_offset % 2 != 0) {
        values.push_back(_read8(next_offset));
        next_offset++;
        bytes_left--;
    }

    uint32_t words_to_read = bytes_left / 2;
    if (words_to_read > 0) {
        std::vector<uint16_t> data          = _readDataBlock(next_offset, words_to_read);
        std::vector<uint8_t> converted_data = _u16ArrayToU8Array(data);
        values.insert(values.end(), converted_data.begin(), converted_data.end());

        bytes_left -= words_to_read * 2;
        next_offset += words_to_read * 2;
    }

    if (bytes_left != 0) {
        values.push_back(_read8(next_offset));
    }

    return values;
}

uint32_t b300_image_loader_helper::_writeUserBitstream(const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> header_data = _readBlock8Internal(FLASH_BOARD_INFO_ADDR, 0x10);

    if (header_data[8] != 1
        || (header_data[0xC] == 0xFF && header_data[0xD] == 0xFF
            && header_data[0xE] == 0xFF && header_data[0xF] == 0xFF)) {
        throw uhd::runtime_error("Flash Header is Not Programmed!");
    }

    std::vector<uint8_t> bitstream_pointer =
        _readBlock8Internal(FLASH_USER_POINTER_ADDR, 4);
    if (bitstream_pointer[0] == 0xFF && bitstream_pointer[1] == 0xFF
        && bitstream_pointer[2] == 0xFF && bitstream_pointer[3] == 0xFF) {
        throw uhd::runtime_error("Bitstream Pointer is Not Programmed!");
    }

    if (data.size() > FLASH_MAX_IMAGE_SIZE) {
        throw uhd::runtime_error("Image Size Exceeds Maximum Allowed Size!");
    }

    _writeToFlash(FLASH_USER_ADDR, data);
    return FLASH_USER_ADDR;
}

std::vector<uint8_t> b300_image_loader_helper::writeBinToFlash(
    const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw uhd::runtime_error("Error opening file: " + filename);
    }

    std::vector<uint8_t> write_bytes(
        (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    uint32_t flash_offset = _writeUserBitstream(write_bytes);
    std::vector<uint8_t> read_bytes =
        _readBlock8Internal(flash_offset, write_bytes.size());

    if (write_bytes == read_bytes) {
        _sendCommandGetData(READ_RST_CTRL);
        _writeData(WRITE_RST_CTRL, 0x2);
        if ((_sendCommandGetData(READ_RST_CTRL) & 0xF7) != 0x02) {
            throw uhd::runtime_error("Write Reset Control Failed");
        }
        _sendCommandGetData(READ_RST_CTRL);
        UHD_LOG_INFO(
            "B300 IMAGE LOADER", "Image written successfully. Reboot for new image.");
    } else {
        throw uhd::runtime_error("Verify Failed!");
    }

    return write_bytes;
}

bool b300_image_loader_helper::_writeToFlash(
    uint32_t offset, const std::vector<uint8_t>& u8_data)
{
    uint32_t byte_count = u8_data.size();
    if (byte_count == 0) {
        throw uhd::runtime_error("Attempted to write empty data to flash!");
    }

    uint32_t current_offset = offset;
    UHD_LOG_INFO("B300 IMAGE LOADER", "Writing image to flash...");

    // Write all of the data one sector or subsector at a time
    while (current_offset < (offset + byte_count)) {
        uint32_t bytes_remaining = byte_count + offset - current_offset;
        uint32_t offset_to_write = 0;
        std::vector<uint16_t> data_to_write;
        uint32_t words_to_write     = 0;
        uint32_t data_bytes_written = 0;

        // Read-modify-write logic for partial sectors/subsectors
        uint32_t offset_within_subsector = current_offset % FLASH_SUBSECTOR_SIZE;

        if (offset_within_subsector != 0 || bytes_remaining < FLASH_SUBSECTOR_SIZE) {
            // Allocate scratch buffer for subsector
            std::vector<uint8_t> sub_sector_data(FLASH_SUBSECTOR_SIZE);

            offset_to_write = current_offset - offset_within_subsector;
            words_to_write  = FLASH_SUBSECTOR_SIZE / 2;
            data_bytes_written =
                std::min(bytes_remaining, FLASH_SUBSECTOR_SIZE - offset_within_subsector);

            // Read existing subsector data
            std::vector<uint16_t> existing_words =
                _readDataBlock(offset_to_write, words_to_write);

            // Convert 16-bit words back to bytes (little endian)
            for (size_t i = 0; i < existing_words.size(); ++i) {
                sub_sector_data[i * 2]     = existing_words[i] & 0xFF;
                sub_sector_data[i * 2 + 1] = (existing_words[i] >> 8) & 0xFF;
            }

            // Copy new data into the appropriate location in the buffer
            uint32_t data_start_idx = current_offset - offset;
            for (uint32_t i = 0; i < data_bytes_written; ++i) {
                sub_sector_data[offset_within_subsector + i] =
                    u8_data[data_start_idx + i];
            }

            // Convert bytes back to 16-bit words for writing
            data_to_write = _u8ArrayToU16Array(sub_sector_data);
        } else {
            // Full sector/subsector write
            offset_to_write         = current_offset;
            uint32_t data_start_idx = current_offset - offset;

            if (current_offset % FLASH_SECTOR_SIZE == 0
                && bytes_remaining >= FLASH_SECTOR_SIZE) {
                words_to_write     = FLASH_SECTOR_SIZE / 2;
                data_bytes_written = FLASH_SECTOR_SIZE;
            } else {
                words_to_write     = FLASH_SUBSECTOR_SIZE / 2;
                data_bytes_written = FLASH_SUBSECTOR_SIZE;
            }

            // Convert bytes to 16-bit words (little endian)
            std::vector<uint8_t> data_slice(u8_data.begin() + data_start_idx,
                u8_data.begin() + data_start_idx + data_bytes_written);
            data_to_write = _u8ArrayToU16Array(data_slice);
        }

        // Erase and write the data block
        _eraseAndWriteDataBlock(offset_to_write, data_to_write, words_to_write);
        current_offset += data_bytes_written;
    }

    UHD_LOG_INFO("B300 IMAGE LOADER", "Verifying image...");
    std::vector<uint8_t> read_bytes = _readBlock8Internal(offset, byte_count);
    std::vector<uint8_t> write_slice(u8_data.begin(), u8_data.begin() + byte_count);
    if (read_bytes != write_slice) {
        UHD_LOG_ERROR("B300 IMAGE LOADER", "Verify Failed!");
    }

    return true;
}

void b300_image_loader_helper::_eraseAndWriteDataBlock(
    uint32_t offset, const std::vector<uint16_t>& u16_data, uint32_t word_count)
{
    uint32_t bytes_per_word = FLASH_BUS_WIDTH;
    uint32_t words_per_page = FLASH_PAGE_SIZE / bytes_per_word;
    uint32_t byte_count     = word_count * bytes_per_word;

    if ((offset % FLASH_SUBSECTOR_SIZE == 0 && byte_count == FLASH_SUBSECTOR_SIZE)) {
        _eraseBlock(offset, ERASE_FLASH_SUBSECTOR);
    } else if ((offset % FLASH_SUBSECTOR_SIZE == 0 && byte_count == FLASH_SECTOR_SIZE)) {
        _eraseBlock(offset, ERASE_FLASH_SECTOR);
    } else {
        throw std::runtime_error("Attempting to write invalid flash block size!");
    }

    // Write the u16_data one page at a time
    for (uint32_t words_written = 0; words_written < word_count;
         words_written += words_per_page) {
        uint32_t words_in_this_page =
            std::min(words_per_page, word_count - words_written);
        std::vector<uint16_t> page_data(u16_data.begin() + words_written,
            u16_data.begin() + words_written + words_in_this_page);
        _writeDataBlock(offset + (words_written * bytes_per_word), page_data);
    }
}

void b300_image_loader_helper::_eraseBlock(uint32_t offset, uint8_t command)
{
    _writeData(SET_UPPER_FLASH_ADDR, (offset >> 16) & DATA_MASK);
    _writeData(SET_LOWER_FLASH_ADDR, offset & DATA_MASK);

    // Erase the whole sector or subsector
    _writeData(command, 0);

    // Wait for the erase request to complete
    uint16_t flash_status  = 0;
    bool write_in_progress = true;
    auto start_time        = std::chrono::steady_clock::now();

    do {
        _writeData(READ_FLASH_STATUS, 0);
        flash_status      = _readData();
        write_in_progress = flash_status & FLASH_WRITE_IN_PROGRESS;
    } while (write_in_progress
             && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(10));

    if (write_in_progress) {
        UHD_LOG_DEBUG("B300 IMAGE LOADER", "erase_block Failed to complete");
        return;
    }
}

uint32_t b300_image_loader_helper::_writeDataBlock(
    uint32_t address, const std::vector<uint16_t>& u16_array)
{
    uint32_t words_to_write = u16_array.size();
    if (words_to_write == 0) {
        return 0;
    }

    try {
        uint32_t words_written = 0;

        // Configure the transfer
        if (address % FLASH_BUS_WIDTH != 0) {
            UHD_LOG_ERROR("B300 IMAGE LOADER",
                "Address 0x" << std::hex << address << " not aligned to Flash bus width "
                             << FLASH_BUS_WIDTH);
            return 0;
        }

        if (words_to_write > FLASH_PAGE_SIZE / 2) {
            UHD_LOG_ERROR("B300 IMAGE LOADER",
                "Can only write up to one page at a time. Requested: "
                    << words_to_write << ", Max: " << FLASH_PAGE_SIZE / 2);
            return 0;
        }

        // Set address and start write transfer
        _writeData(SET_UPPER_FLASH_ADDR, (address >> 16) & DATA_MASK);
        _writeData(SET_LOWER_FLASH_ADDR, address & DATA_MASK);
        _writeData(INIT_FLASH_WRITE, 0);

        // Use fast access interface (2 words at a time)
        uint32_t total_fast_writes = words_to_write / 2;

        // Set command for fast access
        _write32(FLASH_FAST_COMMAND_OFFSET, WRITE_FLASH_AND_INCR_ADDR);

        uint32_t write_count = 0;
        while (write_count < total_fast_writes) {
            // Check TX FIFO space
            uint32_t fifo_status  = _read32(FLASH_FIFO_STATUS_OFFSET);
            uint32_t burst_length = ((fifo_status & TX_FIFO_EMPTINESS) >> 16) / 2;
            burst_length = std::min(total_fast_writes - write_count, burst_length);

            // Write data in bursts
            for (uint32_t i = 0; i < burst_length; ++i) {
                // Combine two 16-bit words into one 32-bit word (little endian)
                uint32_t data_idx  = write_count * 2;
                uint16_t word_low  = u16_array[data_idx] & 0xFFFF;
                uint16_t word_high = (data_idx + 1 < u16_array.size())
                                         ? (u16_array[data_idx + 1] & 0xFFFF)
                                         : 0;
                uint32_t data32    = word_low | (static_cast<uint32_t>(word_high) << 16);

                _write32(FLASH_FAST_WRITE_OFFSET, data32);
                write_count++;
                words_written += 2;
            }
        }

        // Wait for write to complete
        _waitForWriteToComplete();

        // Handle odd word count
        if (words_to_write % 2 != 0) {
            _writeData(WRITE_FLASH_AND_INCR_ADDR, u16_array[words_to_write - 1]);
            words_written++;
        }

        // Cleanup
        _writeData(END_FLASH_TRANSFER, 0);

        // Wait for Write to complete
        auto start_time        = std::chrono::steady_clock::now();
        auto max_time          = std::chrono::seconds(2);
        bool write_in_progress = true;

        do {
            uint16_t flash_status = _sendCommandGetData(READ_FLASH_STATUS);
            write_in_progress     = flash_status & FLASH_WRITE_IN_PROGRESS;
        } while (write_in_progress
                 && std::chrono::steady_clock::now() - start_time < max_time);

        if (write_in_progress) {
            UHD_LOG_DEBUG("B300 IMAGE LOADER",
                "Error: Timeout waiting for Flash write to complete");
            return 0;
        }

        return words_written;

    } catch (const std::exception& e) {
        UHD_LOG_DEBUG("B300 IMAGE LOADER", "Error writing data block: " << e.what());
        try {
            _writeData(END_FLASH_TRANSFER, 0);
        } catch (...) {
            // Ignore cleanup errors
        }
        return 0;
    }
}

void b300_image_loader_helper::_writeData(uint8_t command, uint16_t data)
{
    uint32_t combined = (static_cast<uint32_t>(command & 0xFF) << 16) | (data & 0xFFFF);
    _write32(FLASH_OUTPUT_OFFSET, combined);
    _waitForWriteToComplete();
}

void b300_image_loader_helper::_waitForWriteToComplete()
{
    auto start_time  = std::chrono::steady_clock::now();
    bool output_busy = true;

    do {
        output_busy = (_read32(FLASH_STATUS_OFFSET) & STATUS_OUTPUT_BUSY) != 0;
    } while (output_busy
             && std::chrono::steady_clock::now() - start_time < std::chrono::seconds(2));
}

static bool b300_image_loader(const image_loader::image_loader_args_t& image_loader_args)
{
    // See if any b3xx with the given args is found
    device_addrs_t devs = b300_find(image_loader_args.args);

    if (devs.empty())
        return false;

    if (not std::filesystem::exists(image_loader_args.fpga_path)) {
        throw uhd::runtime_error(
            "Could not find image at path " + image_loader_args.fpga_path);
    }

    for (size_t i = 0; i < devs.size(); ++i) {
        uhd::property_tree::sptr dummy_tree;
        auto pcie_mgr =
            std::make_shared<b300_pcie_manager>(devs[i]["resource"], dummy_tree, "");

        b300_image_loader_helper image_loader_helper(
            [pcie_mgr](uint32_t addr) { return pcie_mgr->peek32(addr); },
            [pcie_mgr](uint32_t addr, uint32_t value) { pcie_mgr->poke32(addr, value); });
        image_loader_helper.writeBinToFlash(image_loader_args.fpga_path);
    }

    return true;
}

UHD_STATIC_BLOCK(register_b300_image_loader)
{
    std::string recovery_instructions =
        "This device is likely in an unusable state. Power-cycle the\n"
        "device, and the firmware/FPGA will be reloaded the next time\n"
        "UHD uses the device.";

    image_loader::register_image_loader("b3xx", b300_image_loader, recovery_instructions);
}

}}} // namespace uhd::usrp::b300
