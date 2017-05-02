//
// Copyright 2017 Ettus Research (National Instruments)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "adi/common.h"

#include <mpm/ad937x/adi_ctrl.hpp>
#include <uhd/exception.hpp>

#include <iostream>
#include <chrono>
#include <thread>

ad9371_spiSettings_t::ad9371_spiSettings_t(
        mpm::types::regs_iface* spi_iface_
) :
    spi_iface(spi_iface_)
{
    spi_settings.chipSelectIndex = 0;       // set later
    spi_settings.writeBitPolarity = 1;      // unused
    spi_settings.longInstructionWord = 1;
    spi_settings.MSBFirst = 1;
    spi_settings.CPHA = 0;
    spi_settings.CPOL = 0;
    spi_settings.enSpiStreaming = 0;        // unused
    spi_settings.autoIncAddrUp = 0;         // unused
    spi_settings.fourWireMode = 1;          // unused
    spi_settings.spiClkFreq_Hz = 250000000; // currently unused
}

// TODO: change // not implemented to  meaningful errors

// close hardware pointers
commonErr_t CMB_closeHardware(void)
{
    // not implemented
    return COMMONERR_FAILED;
}

// GPIO function
commonErr_t CMB_setGPIO(uint32_t GPIO)
{
    // not implemented
    return COMMONERR_FAILED;
}

// hardware reset function
commonErr_t CMB_hardReset(uint8_t spiChipSelectIndex)
{
    // TODO: implement
    return COMMONERR_OK;
}

//
// SPI read/write functions
//

// allows the platform HAL to work with devices with various SPI settings
commonErr_t CMB_setSPIOptions(spiSettings_t *spiSettings)
{
    // not implemented
    return COMMONERR_OK;
}

// value of 0 deasserts all chip selects
commonErr_t CMB_setSPIChannel(uint16_t chipSelectIndex)
{
    // not implemented
    return COMMONERR_OK;
}

// single SPI byte write function
commonErr_t CMB_SPIWriteByte(spiSettings_t *spiSettings, uint16_t addr, uint8_t data)
{
    if (spiSettings == nullptr || spiSettings->MSBFirst == 0) {
        // TODO: crash and burn for these errors?
        return COMMONERR_FAILED;
    }

    ad9371_spiSettings_t *spi = ad9371_spiSettings_t::make(spiSettings);
    try {
        spi->spi_iface->poke8(addr, data);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        std::cout << "AAAAAAAAAAAAH" << std::endl;
    }
    return COMMONERR_FAILED;
}

commonErr_t CMB_SPIWriteBytes(spiSettings_t *spiSettings, uint16_t *addr, uint8_t *data, uint32_t count)
{
    if (spiSettings == nullptr ||
        addr == nullptr ||
        data == nullptr ||
        spiSettings->MSBFirst == 0)
    {
        // TODO: crash and burn for these errors?
        return COMMONERR_FAILED;
    }

    ad9371_spiSettings_t *spi = ad9371_spiSettings_t::make(spiSettings);
    try {
        for (size_t i = 0; i < count; ++i)
        {
            uint32_t data_word = (0) | (addr[i] << 8) | (data[i]);

            spi->spi_iface->poke8(addr[i], data[i]);
        }
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        std::cout << "AAAAAAAAAAAAH" << std::endl;
    }
    return COMMONERR_FAILED;
}

// single SPI byte read function
commonErr_t CMB_SPIReadByte (spiSettings_t *spiSettings, uint16_t addr, uint8_t *readdata)
{
    if (spiSettings == nullptr ||
        readdata == nullptr ||
        spiSettings->MSBFirst == 0)
    {
        return COMMONERR_FAILED;
    }

    ad9371_spiSettings_t *spi = ad9371_spiSettings_t::make(spiSettings);
    try {
        *readdata = spi->spi_iface->peek8(addr);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        std::cout << "AAAAAAAAAAAAH READ" << std::endl;
        // ... error handling ...
    }
    return COMMONERR_FAILED;
}

// write a field in a single register
commonErr_t CMB_SPIWriteField(
        spiSettings_t *spiSettings,
        uint16_t addr, uint8_t  field_val,
        uint8_t mask, uint8_t start_bit
) {
    ad9371_spiSettings_t *spi = ad9371_spiSettings_t::make(spiSettings);

    try {
        uint8_t current_value = spi->spi_iface->peek8(addr);
        uint8_t new_value = ((current_value & ~mask) | (field_val << start_bit));
        spi->spi_iface->poke8(addr, new_value);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        std::cout << "AAAAAAAAAAAAH WRITE FIELD" << std::endl;
        // ... error handling ...
    }
    return COMMONERR_FAILED;
}


// read a field in a single register
commonErr_t CMB_SPIReadField(
        spiSettings_t *spiSettings,
        uint16_t addr, uint8_t *field_val,
        uint8_t mask, uint8_t start_bit
) {
    ad9371_spiSettings_t *spi = ad9371_spiSettings_t::make(spiSettings);

    try {
        uint8_t value = spi->spi_iface->peek8(addr);
        *field_val = static_cast<uint8_t>((value & mask) >> start_bit);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        std::cout << "AAAAAAAAAAAAH READ FIELD" << std::endl;
        /* ... error handling ... */
    }
    return COMMONERR_FAILED;
}

// platform timer functions

commonErr_t CMB_wait_ms(uint32_t time_ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(time_ms));
    return COMMONERR_OK;
}
commonErr_t CMB_wait_us(uint32_t time_us)
{
    std::this_thread::sleep_for(std::chrono::microseconds(time_us));
    return COMMONERR_OK;
}

commonErr_t CMB_setTimeout_ms(spiSettings_t *spiSettings, uint32_t timeOut_ms)
{
    ad9371_spiSettings_t *mpm_spi = ad9371_spiSettings_t::make(spiSettings);
    mpm_spi->timeout_start = std::chrono::steady_clock::now();
    mpm_spi->timeout_duration = std::chrono::milliseconds(timeOut_ms);
    return COMMONERR_OK;
}
commonErr_t CMB_setTimeout_us(spiSettings_t *spiSettings, uint32_t timeOut_us)
{
    ad9371_spiSettings_t *mpm_spi = ad9371_spiSettings_t::make(spiSettings);
    mpm_spi->timeout_start = std::chrono::steady_clock::now();
    mpm_spi->timeout_duration = std::chrono::microseconds(timeOut_us);
    return COMMONERR_OK;
}
commonErr_t CMB_hasTimeoutExpired(spiSettings_t *spiSettings)
{
    ad9371_spiSettings_t *mpm_spi = ad9371_spiSettings_t::make(spiSettings);
    auto current_time = std::chrono::steady_clock::now();
    if ((std::chrono::steady_clock::now() - mpm_spi->timeout_start) > mpm_spi->timeout_duration)
    {
        return COMMONERR_FAILED;
    }
    else {
        return COMMONERR_OK;
    }
}

// platform logging functions
commonErr_t CMB_openLog(const char *filename)
{
    // not implemented
    return COMMONERR_FAILED;
}
commonErr_t CMB_closeLog(void)
{
    // not implemented
    return COMMONERR_FAILED;
}

commonErr_t CMB_writeToLog(ADI_LOGLEVEL level, uint8_t deviceIndex, uint32_t errorCode, const char *comment)
{
    std::cout << "[CMB_writeToLog] level==" << level << " errorCode==" << errorCode << " " << comment;
    return COMMONERR_OK;
}
commonErr_t CMB_flushLog(void)
{
    // not implemented
    return COMMONERR_FAILED;
}

/* platform FPGA AXI register read/write functions */
commonErr_t CMB_regRead(uint32_t offset, uint32_t *data)
{
    // not implemented
    return COMMONERR_FAILED;
}
commonErr_t CMB_regWrite(uint32_t offset, uint32_t data)
{
    // not implemented
    return COMMONERR_FAILED;
}

/* platform DDR3 memory read/write functions */
commonErr_t CMB_memRead(uint32_t offset, uint32_t *data, uint32_t len)
{
    // not implemented
    return COMMONERR_FAILED;
}
commonErr_t CMB_memWrite(uint32_t offset, uint32_t *data, uint32_t len)
{
    // not implemented
    return COMMONERR_FAILED;
}
