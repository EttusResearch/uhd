/**
* Copyright 2015 - 2025 Analog Devices Inc.
* SPDX-License-Identifier: Apache-2.0
*/

/**
* \file adi_platform.c
*
* \brief Definitions for ADI Specific Platforms
*
* ADRV903X API Version: 2.12.1.4
*/

#include "adi_platform.h"

#include "../platforms/common/adi_logging.h"

#include "../platforms/common/tls.h"
#include <uhdlib/usrp/common/adrv9032_device.hpp>

/*
 * Function pointer assignment for default configuration
 */

/* Initialization interface to open, init, close drivers and pointers to resources */
adi_hal_Err_e (*adi_hal_HwOpen)(void* const devHalCfg)                          = NULL;

adi_hal_Err_e (*adi_hal_HwClose)(void* const devHalCfg)                         = NULL;

adi_hal_Err_e (*adi_hal_HwReset)(void* const devHalCfg, const uint8_t pinLevel) = NULL;

void*         (*adi_hal_DevHalCfgCreate)(   const uint32_t      interfaceMask,
                                            const uint8_t       spiChipSelect,
                                            const char* const   logFilename)    = NULL;

adi_hal_Err_e (*adi_hal_DevHalCfgFree)(void* devHalCfg)                         = NULL;

/* SPI Interface */
adi_hal_Err_e (*adi_hal_SpiWrite)(  void* const     devHalCfg,
                                    const uint8_t   txData[],
                                    const uint32_t  numTxBytes) = NULL;

adi_hal_Err_e (*adi_hal_SpiRead)(   void* const     devHalCfg,
                                    const uint8_t   txData[],
                                    uint8_t         rxData[],
                                    const uint32_t  numRxBytes) = NULL;

/* I2C Interface */
adi_hal_Err_e(*adi_hal_I2cWrite)(   void* const     devHalCfg,
                                    const uint8_t   txData[],
                                    const uint32_t  numTxBytes) = NULL;

adi_hal_Err_e(*adi_hal_I2cRead)(    void* const     devHalCfg,
                                    const uint8_t   txData[],
                                    const uint32_t  numTxBytes,
                                    uint8_t         rxData[],
                                    const uint32_t  numRxBytes) = NULL;

/* Logging interface */
adi_hal_Err_e (*adi_hal_LogFileOpen)(   void* const         devHalCfg,
                                        const char* const   filename)   = NULL;

adi_hal_Err_e (*adi_hal_LogLevelSet)(   void* const     devHalCfg,
                                        const uint32_t  logLevelMask)   = NULL;

adi_hal_Err_e (*adi_hal_LogLevelGet)(   void* const     devHalCfg,
                                        uint32_t* const logLevelMask)   = NULL;

adi_hal_Err_e (*adi_hal_LogStatusGet)(  void* const                     devHalCfg,
                                        adi_hal_LogStatusGet_t* const  logStatus)   = NULL;

adi_hal_Err_e (*adi_hal_LogConsoleSet)( void* const devHalCfg,
                                        const adi_hal_LogConsole_e  logConsoleFlag) = NULL;

adi_hal_Err_e (*adi_hal_LogWrite)(  void* const                 devHalCfg,
                                    const adi_hal_LogLevel_e    logLevel,
                                    const uint8_t               indent,
                                    const char* const           comment,
                                    va_list                     argp)   = NULL;

adi_hal_Err_e (*adi_hal_LogFileClose)(void* const devHalCfg) = NULL;

/* Timer interface */
adi_hal_Err_e (*adi_hal_Wait_ms)(void* const devHalCfg, const uint32_t time_ms) = NULL;
adi_hal_Err_e (*adi_hal_Wait_us)(void* const devHalCfg, const uint32_t time_us) = NULL;

/* BBIC control interface */
adi_hal_Err_e (*adi_hal_BbicRegisterRead)(  void* const     devHalCfg,
                                            const uint32_t  addr,
                                            uint32_t* const data)           = NULL;

adi_hal_Err_e (*adi_hal_BbicRegisterWrite)( void* const     devHalCfg,
                                            const uint32_t  addr,
                                            const uint32_t  data)           = NULL;

adi_hal_Err_e (*adi_hal_BbicRegistersRead)( void* const     devHalCfg,
                                            const uint32_t  addr,
                                            uint32_t        data[],
                                            const uint32_t  numDataWords)   = NULL;

adi_hal_Err_e (*adi_hal_BbicRegistersWrite)(void* const     devHalCfg,
                                            const uint32_t  addr,
                                            const uint32_t  data[],
                                            const uint32_t  numDataWords)   = NULL;

/* Thread Interface */
adi_hal_thread_t (*adi_hal_ThreadSelf)(void) = NULL;

adi_hal_Err_e (*adi_hal_TlsSet)(const adi_hal_TlsType_e tlsType, void* const value) = NULL;

void* (*adi_hal_TlsGet)(const adi_hal_TlsType_e tlsType) = NULL;

/* Mutex Interface */
adi_hal_Err_e(*adi_hal_MutexInit)(adi_hal_mutex_t* const mutex) = NULL;
adi_hal_Err_e(*adi_hal_MutexLock)(adi_hal_mutex_t* const mutex) = NULL;
adi_hal_Err_e(*adi_hal_MutexUnlock)(adi_hal_mutex_t* const mutex) = NULL;
adi_hal_Err_e(*adi_hal_MutexDestroy)(adi_hal_mutex_t* const mutex) = NULL;

adi_hal_Err_e(*adi_hal_BoardIdentify)(char** boardNames, int32_t* numBoards) = NULL;

ADI_API adi_hal_Err_e adi_hal_PlatformSetup(const adi_hal_Platforms_e platform)
{
    adi_hal_Err_e error = ADI_HAL_ERR_PARAM;

    switch (platform) {
        case USRP_B310:
            adi_hal_HwOpen             = uhd_HwOpen;
            adi_hal_HwClose            = uhd_HwClose;
            adi_hal_HwReset            = uhd_HwReset;
            adi_hal_DevHalCfgCreate    = uhd_hal_DevHalCfgCreate;
            adi_hal_DevHalCfgFree      = uhd_DevHalCfgFree;
            adi_hal_SpiWrite           = uhd_hal_SpiWrite;
            adi_hal_SpiRead            = uhd_hal_SpiRead;
            adi_hal_I2cWrite           = uhd_hal_I2cWrite;
            adi_hal_I2cRead            = uhd_hal_I2cRead;
            adi_hal_LogFileOpen        = uhd_hal_LogFileOpen;
            adi_hal_LogLevelSet        = uhd_hal_LogLevelSet;
            adi_hal_LogLevelGet        = uhd_hal_LogLevelGet;
            adi_hal_LogStatusGet       = uhd_hal_LogStatusGet;
            adi_hal_LogConsoleSet      = uhd_hal_LogConsoleSet;
            adi_hal_LogWrite           = uhd_hal_LogWrite;
            adi_hal_LogFileClose       = uhd_hal_LogFileClose;
            adi_hal_Wait_ms            = uhd_hal_Wait_ms;
            adi_hal_Wait_us            = uhd_hal_Wait_us;
            adi_hal_BbicRegisterRead   = uhd_hal_BbicRegisterRead;
            adi_hal_BbicRegisterWrite  = uhd_hal_BbicRegisterWrite;
            adi_hal_BbicRegistersRead  = uhd_hal_BbicRegistersRead;
            adi_hal_BbicRegistersWrite = uhd_hal_BbicRegistersWrite;
            adi_hal_ThreadSelf         = uhd_hal_ThreadSelf;
            adi_hal_TlsSet             = common_TlsSet;
            adi_hal_TlsGet             = common_TlsGet;
            adi_hal_MutexInit          = uhd_hal_MutexInit;
            adi_hal_MutexLock          = uhd_hal_MutexLock;
            adi_hal_MutexUnlock        = uhd_hal_MutexUnlock;
            adi_hal_MutexDestroy       = uhd_hal_MutexDestroy;
            adi_hal_BoardIdentify      = uhd_hal_BoardIdentify;
            error                      = ADI_HAL_ERR_OK;
            break;
        default:
            error = ADI_HAL_ERR_PARAM;
            break;
    }

    return error;
}
