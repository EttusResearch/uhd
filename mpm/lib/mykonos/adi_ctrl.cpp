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
#include <mpm/spi/adi_ctrl.hpp>
#include <mpm/spi_iface.hpp>
#include <iostream>
//#include <boost/format.hpp>

#define CMB_TRACE_FUNCTIONS 0

#if CMB_TRACE_FUNCTIONS == 1
#define CMB_TRACE_FUNCTION std::cout << __FUNCTION__ << std::endl;
#else
#define CMB_TRACE_FUNCTION
#endif


/* close hardware pointers */
commonErr_t CMB_closeHardware(void)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}

/* GPIO function */
commonErr_t CMB_setGPIO(uint32_t GPIO)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}

/* hardware reset function */
commonErr_t CMB_hardReset(uint8_t spiChipSelectIndex)
{
    CMB_TRACE_FUNCTION;
    //std::cout << "Hard reset chip select " << spiChipSelectIndex << std::endl;
    return COMMONERR_OK;
}

/* SPI read/write functions */

/* allows the platform HAL to work with devices with various SPI settings */
commonErr_t CMB_setSPIOptions(spiSettings_t *spiSettings)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}

/* value of 0 deasserts all chip selects */
commonErr_t CMB_setSPIChannel(uint16_t chipSelectIndex)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}

/* single SPI byte write function */
commonErr_t CMB_SPIWriteByte(spiSettings_t *spiSettings, uint16_t addr, uint8_t data)
{
    mpm_spiSettings_t *mpm_spi = mpm_spiSettings_t::make(spiSettings);
    try {
        mpm_spi->spi_iface->write_byte(addr, data);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        /* ... error handling ... */
    }
    return COMMONERR_FAILED;
}

commonErr_t CMB_SPIWriteBytes(spiSettings_t *spiSettings, uint16_t *addr, uint8_t *data, uint32_t count)
{
    mpm_spiSettings_t *mpm_spi = mpm_spiSettings_t::make(spiSettings);
    try {
        mpm_spi->spi_iface->write_bytes(addr, data, count);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        /* ... error handling ... */
    }
    return COMMONERR_FAILED;
}

/* single SPI byte read function */
commonErr_t CMB_SPIReadByte (spiSettings_t *spiSettings, uint16_t addr, uint8_t *readdata)
{
    mpm_spiSettings_t *mpm_spi = mpm_spiSettings_t::make(spiSettings);
    try {
        *readdata = mpm_spi->spi_iface->read_byte(addr);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        /* ... error handling ... */
    }
    return COMMONERR_FAILED;
}

/* write a field in a single register */
commonErr_t CMB_SPIWriteField(
        spiSettings_t *spiSettings,
        uint16_t addr, uint8_t  field_val,
        uint8_t mask, uint8_t start_bit
) {
    mpm_spiSettings_t *mpm_spi = mpm_spiSettings_t::make(spiSettings);
    try {
        mpm_spi->spi_iface->write_field(addr, field_val, mask, start_bit);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        /* ... error handling ... */
    }
    return COMMONERR_FAILED;
}


/* read a field in a single register */
commonErr_t CMB_SPIReadField(
        spiSettings_t *spiSettings,
        uint16_t addr, uint8_t *field_val,
        uint8_t mask, uint8_t start_bit
) {
    mpm_spiSettings_t *mpm_spi = mpm_spiSettings_t::make(spiSettings);
    try {
        *field_val = mpm_spi->spi_iface->read_field(addr, mask, start_bit);
        return COMMONERR_OK;
    } catch (const std::exception &e) {
        /* ... error handling ... */
    }
    return COMMONERR_FAILED;
}

/* platform timer functions */
commonErr_t CMB_wait_ms(uint32_t time_ms)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}
commonErr_t CMB_wait_us(uint32_t time_us)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}
commonErr_t CMB_setTimeout_ms(uint32_t timeOut_ms)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}
commonErr_t CMB_setTimeout_us(uint32_t timeOut_us)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}
commonErr_t CMB_hasTimeoutExpired()
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_FAILED;
}

/* platform logging functions */
commonErr_t CMB_openLog(const char *filename)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}
commonErr_t CMB_closeLog(void)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}

commonErr_t CMB_writeToLog(ADI_LOGLEVEL level, uint8_t deviceIndex, uint32_t errorCode, const char *comment)
{
    CMB_TRACE_FUNCTION;
    std::cout << level << " " << errorCode << " " << comment << std::endl;
    return COMMONERR_OK;
}
commonErr_t CMB_flushLog(void)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}

/* platform FPGA AXI register read/write functions */
commonErr_t CMB_regRead(uint32_t offset, uint32_t *data)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}
commonErr_t CMB_regWrite(uint32_t offset, uint32_t data)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}

/* platform DDR3 memory read/write functions */
commonErr_t CMB_memRead(uint32_t offset, uint32_t *data, uint32_t len)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}
commonErr_t CMB_memWrite(uint32_t offset, uint32_t *data, uint32_t len)
{
    CMB_TRACE_FUNCTION;
    return COMMONERR_OK;
}
