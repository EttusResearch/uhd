//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <adi_platform.h>

adi_hal_Err_e uhd_HwOpen(void* const devHalCfg);
adi_hal_Err_e uhd_HwClose(void* const devHalCfg);
adi_hal_Err_e uhd_HwReset(void* const devHalCfg, const uint8_t pinLevel);
void* uhd_hal_DevHalCfgCreate(const uint32_t interfaceMask,
    const uint8_t spiChipSelect,
    const char* const logFilename);
adi_hal_Err_e uhd_DevHalCfgFree(void* const devHalCfg);
adi_hal_Err_e uhd_hal_SpiWrite(
    void* const devHalCfg, const uint8_t txData[], const uint32_t numTxBytes);
adi_hal_Err_e uhd_hal_SpiRead(void* const devHalCfg,
    const uint8_t txData[],
    uint8_t rxData[],
    const uint32_t numRxBytes);
adi_hal_Err_e uhd_hal_I2cWrite(
    void* const devHalCfg, const uint8_t txData[], const uint32_t numTxBytes);
adi_hal_Err_e uhd_hal_I2cRead(void* const devHalCfg,
    const uint8_t txData[],
    const uint32_t numTxBytes,
    uint8_t rxData[],
    const uint32_t numRxBytes);
adi_hal_Err_e uhd_hal_LogFileOpen(void* const devHalCfg, const char* const filename);
adi_hal_Err_e uhd_hal_LogLevelSet(void* const devHalCfg, const uint32_t logLevelMask);
adi_hal_Err_e uhd_hal_LogLevelGet(void* const devHalCfg, uint32_t* const logLevelMask);
adi_hal_Err_e uhd_hal_LogStatusGet(
    void* const devHalCfg, adi_hal_LogStatusGet_t* const logStatus);
adi_hal_Err_e uhd_hal_LogConsoleSet(
    void* const devHalCfg, const adi_hal_LogConsole_e logConsoleFlag);
adi_hal_Err_e uhd_hal_LogWrite(void* const devHalCfg,
    const adi_hal_LogLevel_e logLevel,
    const uint8_t indent,
    const char* const comment,
    va_list argp);
adi_hal_Err_e uhd_hal_LogFileClose(void* const devHalCfg);
adi_hal_Err_e uhd_hal_Wait_ms(void* const devHalCfg, const uint32_t time_ms);
adi_hal_Err_e uhd_hal_Wait_us(void* const devHalCfg, const uint32_t time_us);
adi_hal_Err_e uhd_hal_BbicRegisterRead(
    void* const devHalCfg, const uint32_t regAddr, uint32_t* const regData);
adi_hal_Err_e uhd_hal_BbicRegisterWrite(
    void* const devHalCfg, const uint32_t regAddr, const uint32_t regData);
adi_hal_Err_e uhd_hal_BbicRegistersRead(void* const devHalCfg,
    const uint32_t regAddr,
    uint32_t* const regData,
    const uint32_t numDataWords);
adi_hal_Err_e uhd_hal_BbicRegistersWrite(void* const devHalCfg,
    const uint32_t regAddr,
    const uint32_t* const regData,
    const uint32_t numDataWords);
adi_hal_thread_t uhd_hal_ThreadSelf(void);
adi_hal_Err_e uhd_hal_MutexInit(adi_hal_mutex_t* const mutex);
adi_hal_Err_e uhd_hal_MutexLock(adi_hal_mutex_t* const mutex);
adi_hal_Err_e uhd_hal_MutexUnlock(adi_hal_mutex_t* const mutex);
adi_hal_Err_e uhd_hal_MutexDestroy(adi_hal_mutex_t* const mutex);
adi_hal_Err_e uhd_hal_BoardIdentify(char** boardNames, int32_t* numBoards);

#ifdef __cplusplus
}
#endif
