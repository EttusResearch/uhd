//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "b300_regs.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace b300 {

class b300_image_loader_helper
{
public:
    b300_image_loader_helper(std::function<uint32_t(uint32_t)>&& readRegFunc,
        std::function<void(uint32_t, uint32_t)>&& writeRegFunc);

    std::vector<uint8_t> writeBinToFlash(const std::string& filename);

private:
    std::function<uint32_t(uint32_t)> _read32;
    std::function<void(uint32_t, uint32_t)> _write32;

    // Internal helper method
    uint16_t _sendCommandGetData(uint8_t cmd, uint16_t data = 0);

    // Data conversion utilities
    std::vector<uint16_t> _u32ArrayToU16Array(const std::vector<uint32_t>& data);
    std::vector<uint16_t> _u8ArrayToU16Array(const std::vector<uint8_t>& data);
    std::vector<uint8_t> _u16ArrayToU8Array(const std::vector<uint16_t>& data);

    // Flash operations
    std::vector<uint16_t> _readDataBlock(uint32_t address, uint32_t wordsToRead);
    uint32_t _writeDataBlock(uint32_t address, const std::vector<uint16_t>& u16Array);
    bool _writeToFlash(uint32_t offset, const std::vector<uint8_t>& u8Data);
    uint32_t _writeUserBitstream(const std::vector<uint8_t>& data);
    std::vector<uint32_t> _fastReadData(uint32_t numDoubleWords);
    std::vector<uint8_t> _readBlock8Internal(uint32_t offset, uint32_t byteCount);
    void _eraseAndWriteDataBlock(
        uint32_t offset, const std::vector<uint16_t>& u16Data, uint32_t wordCount);
    void _eraseBlock(uint32_t offset, uint8_t command);

    // Low-level register access
    void _writeData(uint8_t command, uint16_t data);
    void _waitForWriteToComplete();
    uint16_t _readData(bool ignore_timeout = false);
    uint8_t _read8(uint32_t offset);
};

}}} // namespace uhd::usrp::b300
