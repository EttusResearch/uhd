//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/adrv9032_ctrl.hpp>
#include <uhdlib/usrp/common/adrv9032_device.hpp>
#include <adi_common_error_types.h>

#include <uhdlib/utils/log.hpp>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#    include <windows.h>
#else
#    include <time.h>
#    include <unistd.h>
#endif

adi_hal_Err_e uhd_HwOpen(void* const /*devHalCfg*/)
{
    // We don't need to do anything else to open up communication with the device on top
    // of what has already been done in UHD.
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_HwClose(void* const /*devHalCfg*/)
{
    // We don't need to do anything else to close communication with the device on top
    // of what has already been done in UHD.
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_HwReset(void* const devHalCfg, const uint8_t pinLevel)
{
    if (NULL != devHalCfg) {
        uhd_adrv9032_hal_cfg_t* halCfg = (uhd_adrv9032_hal_cfg_t*)devHalCfg;
        if (halCfg->reset_poke_fn) {
            // FPGA Inverts register value
            halCfg->reset_poke_fn(pinLevel ? 0x0 : 0x1);
        } else {
            UHD_LOG_ERROR(
                "ADRV9032", "Reset poke function is not set, cannot toggle reset line.");
            return ADI_HAL_ERR_NULL_PTR;
        }
    } else {
        UHD_LOG_ERROR("ADRV9032", "Device HAL configuration is NULL.");
        return ADI_HAL_ERR_NULL_PTR;
    }
    return ADI_HAL_ERR_OK;
}

void* uhd_hal_DevHalCfgCreate(const uint32_t /*interfaceMask*/,
    const uint8_t /*spiChipSelect*/,
    const char* const /*logFilename*/)
{
    // Unused, devHalCfg is handled in adrv9032_ctrl instead.
    return nullptr;
}

adi_hal_Err_e uhd_DevHalCfgFree(void* const /*devHalCfg*/)
{
    // Unused, devHalCfg is handled in adrv9032_ctrl instead.
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_SpiWrite(
    void* const devHalCfg, const uint8_t txData[], const uint32_t numTxBytes)
{
    if (NULL != devHalCfg) {
        uhd_adrv9032_hal_cfg_t* halCfg = (uhd_adrv9032_hal_cfg_t*)devHalCfg;
        // Per the requirement in adi_platform.h, if numTxBytes is 0, no transaction
        // should take place and the function should return OK.
        if (numTxBytes == 0) {
            return ADI_HAL_ERR_OK;
        }
        halCfg->spi_iface->adrv_spi_write(txData, numTxBytes);
        return ADI_HAL_ERR_OK;
    }
    return ADI_HAL_ERR_NULL_PTR;
}

adi_hal_Err_e uhd_hal_SpiRead(void* const devHalCfg,
    const uint8_t txData[],
    uint8_t rxData[],
    const uint32_t numRxBytes)
{
    if (NULL != devHalCfg) {
        uhd_adrv9032_hal_cfg_t* halCfg = (uhd_adrv9032_hal_cfg_t*)devHalCfg;
        // Per the requirement in adi_platform.h, if numRxBytes is 0, no transaction
        // should take place and the function should return OK.
        if (numRxBytes == 0) {
            return ADI_HAL_ERR_OK;
        }
        halCfg->spi_iface->adrv_spi_read(txData, rxData, numRxBytes);
        return ADI_HAL_ERR_OK;
    }
    return ADI_HAL_ERR_NULL_PTR;
}

adi_hal_Err_e uhd_hal_I2cWrite(
    void* const /*devHalCfg*/, const uint8_t /*txData*/[], const uint32_t /*numTxBytes*/)
{
    // Implement if we need any I2C communication with the FPGA
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_I2cRead(void* const /*devHalCfg*/,
    const uint8_t /*txData*/[],
    const uint32_t /*numTxBytes*/,
    uint8_t /*rxData*/[],
    const uint32_t /*numRxBytes*/)
{
    // Implement if we need any I2C communication with the FPGA
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_LogFileOpen(
    void* const /*devHalCfg*/, const char* const /*filename*/)
{
    // Currently handling all logging in LogWrite
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_LogLevelSet(
    void* const /*devHalCfg*/, const uint32_t /*logLevelMask*/)
{
    // Currently handling all logging in LogWrite
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_LogLevelGet(
    void* const /*devHalCfg*/, uint32_t* const /*logLevelMask*/)
{
    // Currently handling all logging in LogWrite
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_LogStatusGet(
    void* const /*devHalCfg*/, adi_hal_LogStatusGet_t* const /*logStatus*/)
{
    // Currently handling all logging in LogWrite
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_LogConsoleSet(
    void* const /*devHalCfg*/, const adi_hal_LogConsole_e /*logConsoleFlag*/)
{
    // Currently handling all logging in LogWrite
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_LogWrite(void* const /*devHalCfg*/,
    const adi_hal_LogLevel_e logLevel,
    const uint8_t /*indent*/,
    const char* const comment,
    va_list argp)
{
    char buffer[1000];
    vsnprintf(buffer, sizeof(buffer), comment, argp);
    switch (logLevel) {
        case ADI_HAL_LOG_ERR:
            UHD_LOG_ERROR("ADRV9032", buffer);
            break;
        case ADI_HAL_LOG_WARN:
            UHD_LOG_WARNING("ADRV9032", buffer);
            break;
        case ADI_HAL_LOG_MSG:
            UHD_LOG_INFO("ADRV9032", buffer);
            break;
        case ADI_HAL_LOG_API:
        case ADI_HAL_LOG_API_PRIV:
        case ADI_HAL_LOG_SPI:
        case ADI_HAL_LOG_BF:
            UHD_LOG_TRACE("ADRV9032", buffer);
            break;
        default:
            UHD_LOG_TRACE("ADRV9032", buffer);
            break;
    }
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_LogFileClose(void* const /*devHalCfg*/)
{
    // Currently handling all logging in LogWrite
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_Wait_ms(void* const /*devHalCfg*/, const uint32_t time_ms)
{
#if defined(_WIN32) || defined(_WIN64)
    Sleep(time_ms);
#else
    usleep(time_ms * 1000);
#endif
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_Wait_us(void* const /*devHalCfg*/, const uint32_t time_us)
{
#if defined(_WIN32) || defined(_WIN64)
    Sleep(time_us / 1000);
#else
    usleep(time_us);
#endif
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_BbicRegisterRead(
    void* const /*devHalCfg*/, const uint32_t /*regAddr*/, uint32_t* const /*regData*/)
{
    // Implement if the ADI API needs to read any FPGA registers
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_BbicRegisterWrite(
    void* const /*devHalCfg*/, const uint32_t /*regAddr*/, const uint32_t /*regData*/)
{
    // Implement if the ADI API needs to write any FPGA registers
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_BbicRegistersRead(void* const /*devHalCfg*/,
    const uint32_t /*regAddr*/,
    uint32_t* const /*regData*/,
    const uint32_t /*numDataWords*/)
{
    // Implement if the ADI API needs to read any FPGA registers
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_BbicRegistersWrite(void* const /*devHalCfg*/,
    const uint32_t /*regAddr*/,
    const uint32_t* const /*regData*/,
    const uint32_t /*numDataWords*/)
{
    // Implement if the ADI API needs to write any FPGA registers
    return ADI_HAL_ERR_OK;
}

adi_hal_thread_t uhd_hal_ThreadSelf(void)
{
    // Unused
    return 0;
}

adi_hal_Err_e uhd_hal_MutexInit(adi_hal_mutex_t* const /*mutex*/)
{
    // Not implementing for UHD, locking handled in adrv9032_manager.cpp
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_MutexLock(adi_hal_mutex_t* const /*mutex*/)
{
    // Not implementing for UHD, locking handled in adrv9032_manager.cpp
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_MutexUnlock(adi_hal_mutex_t* const /*mutex*/)
{
    // Not implementing for UHD, locking handled in adrv9032_manager.cpp
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_MutexDestroy(adi_hal_mutex_t* const /*mutex*/)
{
    // Not implementing for UHD, locking handled in adrv9032_manager.cpp
    return ADI_HAL_ERR_OK;
}

adi_hal_Err_e uhd_hal_BoardIdentify(char** /*boardNames*/, int32_t* /*numBoards*/)
{
    return ADI_HAL_ERR_OK;
}
