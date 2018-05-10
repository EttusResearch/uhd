//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "adi/common.h"

#include <mpm/ad937x/adi_ctrl.hpp>
#include <mpm/types/log_buf.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>

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
        return COMMONERR_FAILED;
    }

    ad9371_spiSettings_t *spi = ad9371_spiSettings_t::make(spiSettings);
    try {
        spi->spi_iface->poke8(addr, data);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        // TODO: spit out a reasonable error here (that will survive the C API transition)
        std::stringstream ss;
        ss << "Error in CMB_SPIWriteByte: " << e.what();
        CMB_writeToLog(
            ADIHAL_LOG_ERROR,
            spiSettings->chipSelectIndex,
            ad9371_spi_errors_t::SPI_WRITE_ERROR,
            ss.str().c_str());
    }
    return COMMONERR_FAILED;
}

// multi SPI byte write function (address, data pairs)
commonErr_t CMB_SPIWriteBytes(spiSettings_t *spiSettings, uint16_t *addr, uint8_t *data, uint32_t count)
{
    if (spiSettings == nullptr ||
        addr == nullptr ||
        data == nullptr ||
        spiSettings->MSBFirst == 0)
    {
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
        // TODO: spit out a reasonable error here (that will survive the C API transition)
        std::stringstream ss;
        ss << "Error in CMB_SPIWriteBytes: " << e.what();
        CMB_writeToLog(
            ADIHAL_LOG_ERROR,
            spiSettings->chipSelectIndex,
            ad9371_spi_errors_t::SPI_WRITE_ERROR,
            ss.str().c_str());
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
        // TODO: spit out a reasonable error here (that will survive the C API transition)
        std::stringstream ss;
        ss << "Error in CMB_SPIReadByte: " << e.what();
        CMB_writeToLog(
            ADIHAL_LOG_ERROR,
            spiSettings->chipSelectIndex,
            ad9371_spi_errors_t::SPI_READ_ERROR,
            ss.str().c_str());
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
        // TODO: spit out a reasonable error here (that will survive the C API transition)
        std::stringstream ss;
        ss << "Error in CMB_SPIWriteField: " << e.what();
        CMB_writeToLog(
            ADIHAL_LOG_ERROR,
            spiSettings->chipSelectIndex,
            ad9371_spi_errors_t::SPI_WRITE_ERROR,
            ss.str().c_str());
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
        // TODO: spit out a reasonable error here (that will survive the C API transition)
        std::stringstream ss;
        ss << "Error in CMB_SPIReadField: " << e.what();
        CMB_writeToLog(
            ADIHAL_LOG_ERROR,
            spiSettings->chipSelectIndex,
            ad9371_spi_errors_t::SPI_READ_ERROR,
            ss.str().c_str());
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

commonErr_t CMB_writeToLog(
        ADI_LOGLEVEL level,
        uint8_t deviceIndex,
        uint32_t errorCode,
        const char *comment
) {
    mpm::types::log_level_t mpm_log_level;
    if (level & ADIHAL_LOG_ERROR) {
        mpm_log_level = mpm::types::log_level_t::ERROR;
    }
    else if (level & ADIHAL_LOG_WARNING) {
        mpm_log_level = mpm::types::log_level_t::WARNING;
    }
    else {
        mpm_log_level = mpm::types::log_level_t::TRACE;
    }
    //FIXME: This caused segfault with the async pattern call to c++ from boost python
    // mpm::types::log_buf::make_singleton()->post(
    //     mpm_log_level,
    //     "AD937X",
    //     str(boost::format("[Device ID %d] [Error code: %d] %s")
    //         % int(deviceIndex) % errorCode % comment)
    // );

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
