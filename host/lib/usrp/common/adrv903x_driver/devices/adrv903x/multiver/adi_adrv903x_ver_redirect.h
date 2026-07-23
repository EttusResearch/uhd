/**
* Copyright 2015 - 2025 Analog Devices Inc.
* SPDX-License-Identifier: Apache-2.0
*/

/**
*   \file   adi_adrv903x_ver_redirect.h
* 
*   \brief  Auto generated function pointer shim layer to ease linking to one of multiple 
            versions of the API at runtime
*
*   Generated from ADRV903X API Version: 2.12.1.4
*/

#ifndef API_VER_REDIRECT_H
#define API_VER_REDIRECT_H

#include <stdio.h>

/* Used to mark extern functions correctly for C vs C++ */
#ifndef ADI_API
  #ifdef __cplusplus
    #define ADI_API extern "C"
  #else
    #define ADI_API
  #endif
#endif

/* Used to mark extern functions correctly for C vs C++ */
#ifndef ADI_API_EX
  #ifdef __cplusplus
    #define ADI_API_EX ADI_API
  #else
    #define ADI_API_EX ADI_API extern
  #endif
#endif

#include "adi_adrv903x_all_types.h"

/** Single set of fn ptrs as seen by Application - compatible with the normal
 * API (i.e. no change in app required to use versioned/non-versioned libs)
 *
 * Application linking directly to specific API version pulls in fn decls
 * resolved at link time.
 *
 * Application linking to API-versioning shim pull in a bunch of function
 * pointers which are populated at run-time by calling
 * api_adrv903x_VersionLoad().
 */

ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcCfgSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_AgcCfg_t agcConfig[], const uint32_t numOfAgcCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcCfgGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_AgcCfg_t * const agcConfigReadBack);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcFreezeSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint32_t freezeEnable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcFreezeGet)(adi_adrv903x_Device_t* const device, uint32_t* const freezeEnable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcFreezeOnGpioSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannels, const adi_adrv903x_GpioPinSel_e gpioPin, const uint8_t enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcFreezeOnGpioGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_GpioPinSel_e* const gpioPin);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcGainIndexRangeSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_AgcGainRange_t agcGainRange[], const uint32_t numOfAgcRangeCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcGainIndexRangeGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_AgcGainRange_t * const agcGainConfigReadback);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcReset)(adi_adrv903x_Device_t * const device, const uint32_t rxChannelMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDualBandLnaGainTableWrite)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint8_t gainIndexOffset, const adi_adrv903x_RxDualBandLnaGainTableRow_t gainTableRow[], const uint32_t arraySize);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDualBandLnaGainTableRead)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint8_t gainIndexOffset, adi_adrv903x_RxDualBandLnaGainTableRow_t gainTableRow[], const uint32_t arraySize, uint8_t* const numGainIndicesRead);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandCfgSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_AgcDualBandCfg_t agcDualBandConfig[], const uint32_t numOfAgcDualBandCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandCfgGet)(adi_adrv903x_Device_t * const device, adi_adrv903x_AgcDualBandCfg_t * const agcDualBandConfigReadBack);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandGpioCfgSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_AgcDualBandGpioCfg_t * agcDualBandGpioConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandGpioCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_AgcDualBandGpioCfg_t* const agcDualBandGpioConfigReadBack);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcGpioReSyncSet)(adi_adrv903x_Device_t * const device, const uint32_t rxChannelMask, const adi_adrv903x_GpioPinSel_e gpioSelection, const uint8_t enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcGpioReSyncGet)(adi_adrv903x_Device_t * const device, const uint32_t rxChannelMask, adi_adrv903x_GpioPinSel_e * const gpioSelection, uint8_t * const enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandActiveExternalLnaGainWordGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, uint8_t * const bandAExternalLnaGainWord, uint8_t * const bandBExternalLnaGainWord);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcUpperLevelBlockerGet)(adi_adrv903x_Device_t * const device, uint8_t * const agcULBlockerBitMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcLowerLevelBlockerGet)(adi_adrv903x_Device_t * const device, uint8_t * const agcLLBlockerBitMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcHighThresholdPeakDetectorGet)(adi_adrv903x_Device_t * const device, uint8_t * const thresholdPeakDetectorBitMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcLowThresholdPeakDetectorGet)(adi_adrv903x_Device_t * const device, uint8_t * const thresholdPeakDetectorBitMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsRun)(adi_adrv903x_Device_t* const device, const adi_adrv903x_InitCals_t* const initCals);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsDetailedStatusGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_InitCalStatus_t* const initStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsDetailedStatusGet_v2)( adi_adrv903x_Device_t* const device, adi_adrv903x_InitCalErrData_t* const initCalErrData);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsWait)( adi_adrv903x_Device_t* const device, const uint32_t timeoutMs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsWait_v2)( adi_adrv903x_Device_t* const device, const uint32_t timeoutMs, adi_adrv903x_InitCalErrData_t* const initCalErrData);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsCheckCompleteGet)(adi_adrv903x_Device_t* const device, uint8_t* const areCalsRunning, const uint8_t calErrorCheck);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsCheckCompleteGet_v2)( adi_adrv903x_Device_t* const device, uint8_t* const areCalsRunning, adi_adrv903x_InitCalErrData_t* const initCalErrData);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsAbort)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalsEnableSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TrackingCalibrationMask_t calMask, const uint32_t channelMask, const adi_adrv903x_TrackingCalEnableDisable_e enableDisableFlag);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalsEnableSet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TrackingCalibrationMask_e calMask, const adi_adrv903x_ChannelTrackingCals_t* const channelMask, const adi_adrv903x_TrackingCalEnableDisable_e enableDisableFlag);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalsEnableGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TrackingCalEnableMasks_t* const enableMasks);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalAllStateGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TrackingCalState_t* const calState);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TrackingCalibrationMask_e calId, const adi_adrv903x_Channels_e channel, adi_adrv903x_CalStatus_t* const calStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_InitCalibrations_e calId, const adi_adrv903x_Channels_e channel, adi_adrv903x_CalStatus_t* const calStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxHrmDataGet)(adi_adrv903x_Device_t* const device, const uint8_t channelMask, adi_adrv903x_TxHrmData_t txHrmDataArray[], uint32_t arrayLength);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxHrmDataSet)(adi_adrv903x_Device_t* const device, const uint8_t channelMask, const adi_adrv903x_TxHrmData_t txHrmDataArray[], uint32_t arrayLength);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CalPvtStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Channels_e channel, const uint32_t objId, uint8_t calStatusGet[], uint32_t length);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CalSpecificStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Channels_e channel, const uint32_t objId, uint8_t calStatusGet[], uint32_t length);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DigDcOffsetEnableSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint32_t rxChannelEnableDisable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DigDcOffsetEnableGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, uint8_t* const isEnabled);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxLolReset)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxLolReset_t* const txLolReset);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxQecReset)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxQecReset_t* const txQecReset);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DigDcOffsetCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_SetDcOffset_t* const dcOffSetCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DigDcOffsetCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_CpuCmd_GetDcOffsetResp_t* const dcOffSetCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Lock)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Unlock)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_HwOpen)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SpiConfigSettings_t* const spiSettings);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_HwClose)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_HwReset)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Initialize)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Init_t* const init);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Shutdown)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_MultichipSyncSet)(adi_adrv903x_Device_t* const device, const uint8_t enableSync);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_MultichipSyncSet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_McsSyncMode_e mcsMode);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_MultichipSyncStatusGet)(adi_adrv903x_Device_t* const device, uint32_t* const mcsStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_ProfilesVerify)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Init_t* const init);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SpiConfigSettings_t* const spiCtrlSettings);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiConfigSettings_t* const spi);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AuxSpiCfgSet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_AuxSpiConfig_t* const config);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AuxSpiCfgGet)( adi_adrv903x_Device_t* const device, adi_adrv903x_AuxSpiConfig_t* const config);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiRuntimeOptionsSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SpiOptions_t* const spiOptions);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiRuntimeOptionsGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiOptions_t* const spiOptions);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiVerify)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_ApiVersionGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_Version_t* const apiVersion);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeviceRevGet)(adi_adrv903x_Device_t* const device, uint8_t* const siRevision);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_ProductIdGet)(adi_adrv903x_Device_t* const device, uint8_t* const productId);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeviceCapabilityGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_CapabilityModel_t* const devCapability);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiDoutPadDriveStrengthSet)(adi_adrv903x_Device_t* const device, const uint8_t driveStrength);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiHysteresisSet)( adi_adrv903x_Device_t* const device, const uint32_t enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DigitalHysteresisSet)( adi_adrv903x_Device_t* const device, const uint32_t enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuImageWrite)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuType_e cpuType, const uint32_t byteOffset, const uint8_t binary[], const uint32_t byteCount);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuProfileWrite)(adi_adrv903x_Device_t * const device, const uint32_t byteOffset, const uint8_t binary[], const uint32_t byteCount);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuStart)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuStartStatusCheck)(adi_adrv903x_Device_t* const device, const uint32_t timeout_us);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuConfigSet)( adi_adrv903x_Device_t* const device, const uint32_t objId, const uint16_t offset, const uint8_t configDataSet[], const uint32_t length);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuConfigGet)( adi_adrv903x_Device_t* const device, const uint32_t objId, const uint16_t offset, uint8_t configDataGet[], const uint32_t length);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuDebugModeEnable)(adi_adrv903x_Device_t* const device, const uint32_t enableKey);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuConfigUnlock)(adi_adrv903x_Device_t* const device, const uint32_t configKey);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuControlCmdExec)(adi_adrv903x_Device_t* const device, const uint32_t objId, const uint16_t cpuCmd, const adi_adrv903x_Channels_e channel, const uint8_t cpuCtrlData[], const uint32_t lengthSet, uint32_t* const lengthResp, uint8_t ctrlResp[], const uint32_t lengthGet);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuLogFilterSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuLogEvent_e eventFilter, const adi_adrv903x_CpuLogCpuId_e cpuIdFilter, const adi_adrv903x_CpuLogObjIdFilter_t* const objIdFilter);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuDebugCmdExec)( adi_adrv903x_Device_t* const device, const uint32_t objId, const uint16_t cpuCmd, const adi_adrv903x_Channels_e channel, const uint8_t cpuDebugData[], const uint32_t lengthSet, uint32_t* const lengthResp, uint8_t debugResp[], const uint32_t lengthGet);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuChannelMappingGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_CpuType_e cpuTypes[], const uint8_t numSerdesLanes);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuSysPvtStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Channels_e channel, const uint32_t objId, uint8_t cpuSysStatusGet[], const uint32_t length);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuSysStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Channels_e channel, const uint32_t objId, adi_adrv903x_CpuSysStatus_t* const cpuSysStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuFwVersionGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_CpuFwVersion_t* const cpuFwVersion);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_HealthMonitorCpuStatusGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_HealthMonitorCpuStatus_t* const healthMonitorStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuCheckException)(adi_adrv903x_Device_t* const device, uint32_t* const isException);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SwBreakPointEntry_t * breakPointCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_SwBreakPointEntry_t * const breakPointCfgRead);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointHitRead)(adi_adrv903x_Device_t* const device, adi_adrv903x_SwBreakPointEntry_t * const cpu0BreakpointHit, adi_adrv903x_SwBreakPointEntry_t * const cpu1BreakpointHit);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGpioCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpioOutputForBreakpointHit, const adi_adrv903x_GpioPinSel_e gpioInputToResumeSleepingTasks);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGpioCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_GpioPinSel_e * const gpioOutputForBreakpointHit, adi_adrv903x_GpioPinSel_e * const gpioInputToResumeSleepingTasks);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointResume)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SwBreakPointEntry_t * breakpointToResume, uint8_t resumeAll);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointResumeFromHalt)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGlobalHaltMaskSet)(adi_adrv903x_Device_t* const device, const uint32_t globalHaltMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGlobalHaltMaskGet)(adi_adrv903x_Device_t* const device, uint32_t * const globalHaltMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxOrxDataCaptureStart)( adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e channelSelect, const adi_adrv903x_RxOrxDataCaptureLocation_e captureLocation, uint32_t captureData[], const uint32_t captureLength, const uint8_t trigger, const uint32_t timeout_us);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AdcSampleXbarSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, const adi_adrv903x_AdcSampleXbarCfg_t* const adcXbar);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_AdcSampleXbarGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, adi_adrv903x_AdcSampleXbarCfg_t* const adcXbar);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DacSampleXbarSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const adi_adrv903x_DacSampleXbarCfg_t* const dacXbar);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DacSampleXbarGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DacSampleXbarCfg_t* const dacXbar);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, adi_adrv903x_FramerCfg_t* const framerCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DeframerCfg_t* const deframerCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerCfgGetScaled)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const adi_adrv903x_TxChannels_e chanSel, adi_adrv903x_DeframerCfg_t* const deframerCfg, const uint8_t bypass);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLinkStateGet)(adi_adrv903x_Device_t* const device, uint8_t* const framerLinkState);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLinkStateSet)(adi_adrv903x_Device_t* device, const uint8_t framerSelMask, uint8_t const enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPrbsCountReset)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLinkStateGet)(adi_adrv903x_Device_t* device, uint8_t* const deframerLinkState);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLinkStateSet)(adi_adrv903x_Device_t* device, const uint8_t deframerSelMask, uint8_t const enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPrbsCheckerStateSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DfrmPrbsCfg_t * const dfrmPrbsCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPrbsCheckerStateGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_DfrmPrbsCfg_t * const dfrmPrbsCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSysrefCtrlSet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, uint8_t const enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSysrefCtrlGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, uint8_t * const enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerSysrefCtrlSet)(adi_adrv903x_Device_t* const device, const uint8_t deframerSelMask, uint8_t const enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerSysrefCtrlGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, uint8_t * const enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerTestDataSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_FrmTestDataCfg_t * const frmTestDataCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerTestDataGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, adi_adrv903x_FrmTestDataCfg_t * const frmTestDataCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPrbsErrCountGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_DfrmPrbsErrCounters_t * const counters);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SerializerReset)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SerializerReset_v2)(adi_adrv903x_Device_t* const device, adi_adrv903x_CpuCmd_SerReset_t* const pSerResetParms, adi_adrv903x_CpuCmd_SerResetResp_t* const pSerResetResp);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLmfcOffsetSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSelect, const uint16_t lmfcOffset);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLmfcOffsetGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSelect, uint16_t * const lmfcOffset);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmLmfcOffsetSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSelect, const uint16_t lmfcOffset);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmLmfcOffsetGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSelect, uint16_t * const lmfcOffset);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPhaseDiffGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSelect, uint16_t * const phaseDiff);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_FramerSel_e framerSel, adi_adrv903x_FramerStatus_t * const framerStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DeframerStatus_t * const deframerStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerStatusGet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DeframerStatus_v2_t * const deframerStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCounterStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel, const uint8_t laneNumber, adi_adrv903x_DfrmErrCounterStatus_t * const errCounterStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCounterReset)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const uint8_t laneNumber, uint32_t const errCounterMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCounterThresholdSet)(adi_adrv903x_Device_t* const device, const uint8_t threshold);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Dfrm204cErrCounterStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel, const uint8_t laneNumber, adi_adrv903x_Dfrm204cErrCounterStatus_t * const errCounterStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Dfrm204cErrCounterReset)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmLinkConditionGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel, uint8_t * const dfrmLinkCondition);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmFifoDepthGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const uint8_t laneNumber, uint8_t * const fifoDepth);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmCoreBufDepthGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, uint8_t * const coreBufDepth);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIlasMismatchGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DfrmCompareData_t* const dfrmData);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIlasMismatchGet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DfrmCompareData_v2_t* const dfrmData);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLoopbackSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLoopbackDisable)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, const adi_adrv903x_AdcSampleXbarCfg_t* const adcXbar);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLoopbackSet)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLoopbackDisable)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLoopbackDisable_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, const adi_adrv903x_AdcSampleXbarCfg_t* const adcXbar);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLaneLoopbackSet)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLaneLoopbackDisable)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbModeSet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, const uint8_t syncbMode);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbModeGet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, uint8_t* const syncbMode);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbStatusSet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, const uint8_t syncbStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbStatusGet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, uint8_t* const syncbStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbErrCntGet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, uint8_t* const syncbErrCnt);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbErrCntReset)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerSyncbErrCntGet)(adi_adrv903x_Device_t* const device, const uint8_t deframerSelMask, uint8_t* const syncbErrCnt);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerSyncbErrCntReset)(adi_adrv903x_Device_t* const device, const uint8_t deframerSelMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerErrorCtrl)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, const adi_adrv903x_SerdesErrAction_e action);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerErrorCtrl)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const adi_adrv903x_SerdesErrAction_e action);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIrqMaskGet)( adi_adrv903x_Device_t* const device, uint16_t* const irqMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIrqMaskSet)( adi_adrv903x_Device_t* const device, const uint16_t irqMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIrqSourceReset)( adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIrqSourceGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSelect, adi_adrv903x_DeframerIrqVector_t* const irqSourceVector);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCntrCntrlSet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_DfrmErrCounterIrqSel_e interruptEnable, const uint8_t laneNumber, const uint8_t errCounterControl, const uint8_t errCounterHoldCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCntrCntrlGet)( adi_adrv903x_Device_t* const device, const uint8_t laneNumber, uint8_t* const errCounterControl, uint8_t* const errCounterHoldCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RunEyeSweep)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_RunEyeSweep_t* const runEyeSweep, adi_adrv903x_CpuCmd_RunEyeSweepResp_t* const runEyeSweepResp);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RunEyeSweep_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_RunEyeSweep_t* const runEyeSweep, adi_adrv903x_CpuCmd_RunEyeSweepResp_t* const runEyeSweepResp);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RunVerticalEyeSweep)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_RunVertEyeSweep_t* const runVerticalEyeSweep, adi_adrv903x_CpuCmd_RunVertEyeSweepResp_t* const runVerticalEyeSweepResp);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RunVerticalEyeSweep_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_RunVertEyeSweep_t* const runVerticalEyeSweep, adi_adrv903x_CpuCmd_RunVertEyeSweepResp_t* const runVerticalEyeSweepResp);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerTestDataInjectError)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSelect, const uint8_t laneMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SerLaneCfgSet)(adi_adrv903x_Device_t* const device, const uint8_t laneNumber, const adi_adrv903x_SerLaneCfg_t* const serLaneCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SerLaneCfgGet)(adi_adrv903x_Device_t* const device, const uint8_t laneNumber, adi_adrv903x_SerLaneCfg_t* const serLaneCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SerdesInitCalStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GenericStrBuf_t* const filePath, const uint8_t laneNumber, const adi_adrv903x_GenericStrBuf_t* const msg, adi_adrv903x_SerdesInitCalStatus_t* const calStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SerdesTrackingCalStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GenericStrBuf_t* const filePath, const uint8_t laneNumber, const adi_adrv903x_GenericStrBuf_t* const msg, adi_adrv903x_SerdesTrackingCalStatus_t* calStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeserLanesVcmCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeserLanesVcmCfg_t* const deserLanesVcmCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SerdesRxLaneSintCodesGet)(adi_adrv903x_Device_t* const device, const uint8_t laneNumber, adi_adrv903x_CpuCmd_GetRxLaneSintCodesResp_t* const serdesRxLaneSintCodes);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_ErrInfoGet)(const adi_adrv903x_ErrSource_e errSrc, const int64_t errCode, const char** const errMsgPtr, const char** const errCausePtr, adi_adrv903x_ErrAction_e* const actionCodePtr, const char** const actionMsgPtr);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_ErrDataGet)(const adi_adrv903x_Device_t* const device, const adi_common_ErrFrameId_e frameId, adi_adrv903x_ErrSource_e* const errSrcPtr, int64_t* const errCodePtr, const char** const errMsgPtr, const char** const errCausePtr, adi_adrv903x_ErrAction_e* const actionCodePtr, const char** const actionMsgPtr);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioForceHiZAllPins)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioForceHiZ)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpio, const uint8_t oRide);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioStatusRead)( adi_adrv903x_Device_t* const device, adi_adrv903x_GpioStatus_t* const status);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioConfigGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpio, adi_adrv903x_GpioSignal_e* const signal, uint32_t* const channelMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioConfigAllGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_GpioSignal_e signalArray[], uint32_t channelMaskArray[], const uint32_t arraySize);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogConfigGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioAnaPinSel_e gpio, adi_adrv903x_GpioSignal_e* const signal, uint32_t* const channelMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogConfigAllGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_GpioSignal_e signalArray[], uint32_t channelMaskArray[], const uint32_t arraySize);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioMonitorOutSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpio, const adi_adrv903x_GpioSignal_e signal, const uint8_t channel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioMonitorOutRelease)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpio);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualInputDirSet)( adi_adrv903x_Device_t* const device, const uint32_t gpioInputMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualOutputDirSet)( adi_adrv903x_Device_t* const device, const uint32_t gpioOutputMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualInputPinLevelGet)( adi_adrv903x_Device_t* const device, uint32_t * const gpioInPinLevel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualOutputPinLevelGet)( adi_adrv903x_Device_t* const device, uint32_t * const gpioOutPinLevel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualOutputPinLevelSet)( adi_adrv903x_Device_t* const device, const uint32_t gpioPinMask, const uint32_t gpioOutPinLevel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogForceHiZAllPins)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogForceHiZ)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioAnaPinSel_e gpio, const uint8_t oRide);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualInputDirSet)( adi_adrv903x_Device_t* const device, const uint16_t gpioAnalogInputMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualOutputDirSet)( adi_adrv903x_Device_t* const device, const uint16_t gpioAnalogOutputMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualInputPinLevelGet)( adi_adrv903x_Device_t* const device, uint16_t * const gpioAnalogInPinLevel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualOutputPinLevelGet)( adi_adrv903x_Device_t* const device, uint16_t * const gpioAnalogOutPinLevel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualOutputPinLevelSet)( adi_adrv903x_Device_t* const device, const uint16_t gpioAnalogPinMask, const uint16_t gpioAnalogOutPinLevel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntPinMaskCfgSet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpIntPinSelect_e pinSelect, const adi_adrv903x_GpIntPinMaskCfg_t* const pinMaskCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntPinMaskCfgGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpIntPinSelect_e pinSelect, adi_adrv903x_GpIntPinMaskCfg_t* const pinMaskCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntStatusGet)( adi_adrv903x_Device_t* const device, adi_adrv903x_GpIntMask_t* const gpIntStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntStatusClear)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpIntMask_t* const gpIntClear);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntStickyBitMaskSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpIntMask_t* const gpIntStickyBitMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntStickyBitMaskGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_GpIntMask_t* const gpIntStickyBitMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioHysteresisSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioDigitalPin_e pinName, const uint32_t enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioHysteresisGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioDigitalPin_e pinName, uint32_t* const enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioDriveStrengthSet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioDigitalPin_e pinName, const adi_adrv903x_CmosPadDrvStr_e driveStrength);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioDriveStrengthGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioDigitalPin_e pinName, adi_adrv903x_CmosPadDrvStr_e* const driveStrength);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiFlush)(adi_adrv903x_Device_t* const device, const uint8_t data[], uint32_t* const count);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Registers32Write)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr[], const uint32_t writeData[], const uint32_t mask[], const uint32_t count);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Register32Write)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, uint32_t addr, const uint32_t writeData, const uint32_t mask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Register32Read)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, uint32_t addr, uint32_t* const readData, const uint32_t mask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Registers32Read)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, uint32_t readData[], uint32_t mask[], const uint32_t count);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RegistersByteWrite)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, const uint8_t writeData[], const uint32_t count);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RegistersByteRead)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, uint8_t readData[], const uint8_t mask[], const uint32_t count);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Registers32bOnlyRead)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, uint8_t readData[], const uint32_t count);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_Registers32bOnlyWrite)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, const uint8_t writeData[], const uint32_t count);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamImageWrite)(adi_adrv903x_Device_t* const device, uint32_t byteOffset, const uint8_t binary[], uint32_t byteCount);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTxEnableSet)(adi_adrv903x_Device_t* const device, const uint32_t orxChannelMask, const uint32_t orxChannelEnable, const uint32_t rxChannelMask, const uint32_t rxChannelEnable, const uint32_t txChannelMask, const uint32_t txChannelEnable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTxEnableGet)(adi_adrv903x_Device_t* const device, uint32_t* const orxChannelMask, uint32_t* const rxChannelMask, uint32_t* const txChannelMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_ChannelEnableGet)(adi_adrv903x_Device_t* const device, uint32_t* const orxChannelMask, uint32_t* const rxChannelMask, uint32_t* const txChannelMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlCfgSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RadioCtrlModeCfg_t* const radioCtrlCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlTxRxEnCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RadioCtrlTxRxEnCfg_t* const txRxEnCfg, uint8_t pinIndex, uint8_t configSel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlTxRxEnCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RadioCtrlTxRxEnCfg_t* const txRxEnCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_RadioCtrlModeCfg_t* const radioCtrlCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_LoFrequencySet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_LoConfig_t* const loConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_LoFrequencyGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_LoConfigReadback_t* const loConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CfgPllToChanCtrl)(adi_adrv903x_Device_t* const device, uint8_t rf0MuxTx0_3, uint8_t rf0MuxTx4_7, uint8_t rf0MuxRx0_3, uint8_t rf0MuxRx4_7);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_LoLoopFilterSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_LoName_e loName, const adi_adrv903x_LoLoopFilterCfg_t* const loLoopFilterConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_LoLoopFilterGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_LoName_e loName, adi_adrv903x_LoLoopFilterCfg_t* const loLoopFilterConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_PllStatusGet)(adi_adrv903x_Device_t* const device, uint32_t* const pllLockStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTxLoFreqGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxTxLoFreqReadback_t* const rxTxLoFreq);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TemperatureGet)(adi_adrv903x_Device_t* const device, const uint16_t avgMask, adi_adrv903x_DevTempData_t* const deviceTemperature);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TemperatureEnableGet)(adi_adrv903x_Device_t* const device, uint16_t* const tempEnData);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TemperatureEnableSet)(adi_adrv903x_Device_t* const device, uint16_t* const tempEnData);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamGpioConfigSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_StreamGpioPinCfg_t* const streamGpioPinCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamGpioConfigGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_StreamGpioPinCfg_t* const streamGpioPinCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxNcoFreqCalculate)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e orxChannel, const uint32_t txSynthesisBwLower_kHz, const uint32_t txSynthesisBwUpper_kHz, int32_t* const ncoShiftFreqAdc_kHz, int32_t* const ncoShiftFreqDatapath_kHz);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingInit)(adi_adrv903x_Device_t* const device);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingConfigGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TxToOrxMappingConfig_t * const mappingConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingSet)(adi_adrv903x_Device_t* const device, const uint8_t mapping);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e orxChannel, adi_adrv903x_TxChannels_e* const txChannel);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetAttenSet)( adi_adrv903x_Device_t* const device, const uint32_t mapping, const uint8_t presetAtten_dB, const uint8_t immediateUpdate);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetAttenGet_v2)( adi_adrv903x_Device_t* const device, const adi_adrv903x_TxToOrxMappingPinTable_e mapping, uint8_t* const presetAtten_dB);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetAttenGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint8_t* const presetAtten_dB);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetNcoSet)( adi_adrv903x_Device_t* const device, const uint32_t mapping, const adi_adrv903x_TxToOrxMappingPresetNco_t* const presetNco, const uint8_t immediateUpdate);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetNcoGet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxToOrxMappingPinTable_e mapping, adi_adrv903x_TxToOrxMappingPresetNco_t* const presetNco);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetNcoGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxToOrxMappingPresetNco_t* const presetNco);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamVersionGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_Version_t* const streamVersion);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalConfigSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxRadioCtrlAntennaCalConfig_t * const configRx, adi_adrv903x_TxRadioCtrlAntennaCalConfig_t * const configTx);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalConfigGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxRadioCtrlAntennaCalConfig_t * const configRx, adi_adrv903x_TxRadioCtrlAntennaCalConfig_t * const configTx);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalErrorGet)(adi_adrv903x_Device_t* const device, const uint32_t channelSel, uint8_t * const errStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalErrorClear)(adi_adrv903x_Device_t* const device, const uint32_t channelMask, const uint8_t errClearMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamProcErrorGet)( adi_adrv903x_Device_t* const device, adi_adrv903x_StreamErrArray_t* const streamErr);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalGpioChannelSet)(adi_adrv903x_Device_t* const device, const uint32_t txChannelMask, const uint32_t rxChannelMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalGpioChannelGet)(adi_adrv903x_Device_t* const device, uint32_t * const txChannelMask, uint32_t * const rxChannelMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalConfigSet_v2)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxRadioCtrlAntennaCalConfig_t * const configRx, adi_adrv903x_TxRadioCtrlAntennaCalConfig_t * const configTx, const uint8_t rxGain, const uint8_t enableFreeze);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioCtrlRxTxMapSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxRxCtrlGpioMap_t txRxCtrlGpioMap[], const uint32_t numGpios);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioCtrlRxTxMapGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpioPin, const adi_adrv903x_TxRxCtrlGpioLogicalPin_e gpioLogicalPin, adi_adrv903x_TxRxCtrlGpioMap_t* const txRxCtrlGpio);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioCtrlRxTxMapClear)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxRxCtrlGpioMap_t txRxCtrlGpioMap[], const uint32_t numGpios);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableWrite)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint8_t gainIndexOffset, const adi_adrv903x_RxGainTableRow_t gainTableRow[], const uint32_t arraySize );
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableRead)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, const uint8_t gainIndexOffset, adi_adrv903x_RxGainTableRow_t gainTableRow[], const uint32_t arraySize, uint16_t* const numGainIndicesRead);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxMinMaxGainIndexSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint8_t minGainIndex, const uint8_t maxGainIndex);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableExtCtrlPinsSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint32_t channelEnable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxGain_t rxGain[], const uint32_t arraySize);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxMgcGainGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxGain_t * const rxGain);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxGain_t * const rxGain);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDataFormatGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxDataFormatRt_t * const rxDataFormat);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxSlicerPositionGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, uint8_t * const slicerPosition);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxLoSourceGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_LoSel_e * const rxLoSource);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxNcoShifterSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxNcoConfig_t * const rxNcoConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxNcoShifterGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxNcoConfigReadbackResp_t* const rxRbConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxNcoSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_ORxNcoConfig_t * const orxNcoConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxNcoSet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_ORxNcoConfig_t * const orxNcoConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxNcoGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_ORxNcoConfigReadbackResp_t* const orxRbConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDecimatedPowerCfgSet)(adi_adrv903x_Device_t * const device, adi_adrv903x_RxDecimatedPowerCfg_t rxDecPowerCfg[], const uint32_t numOfDecPowerCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDecimatedPowerCfgGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, const adi_adrv903x_DecPowerMeasurementBlock_e decPowerBlockSelection, adi_adrv903x_RxDecimatedPowerCfg_t * const rxDecPowerCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_ORxDecimatedPowerCfgSet)(adi_adrv903x_Device_t * const device, adi_adrv903x_ORxDecimatedPowerCfg_t orxDecPowerCfg[], const uint32_t numOfDecPowerCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_ORxDecimatedPowerCfgGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e orxChannel, adi_adrv903x_ORxDecimatedPowerCfg_t * const orxDecPowerCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDecimatedPowerGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, const adi_adrv903x_DecPowerMeasurementBlock_e decPowerBlockSelection, uint8_t * const powerReadBack);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxAttenSet)(adi_adrv903x_Device_t* const device, const uint32_t channelMask, const uint8_t attenDb);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxAttenGet)(adi_adrv903x_Device_t* const device, const uint8_t channelId, uint8_t* const attenDb);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainCtrlModeSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxGainCtrlModeCfg_t gainCtrlModeCfg[], const uint32_t arraySize);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainCtrlModeGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxGainCtrlMode_e* gainCtrlMode);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTempGainCompSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const int8_t gainValue);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTempGainCompGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, int8_t* const gainValue);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTestDataSet)(adi_adrv903x_Device_t* device, const uint32_t rxChannelMask, const adi_adrv903x_RxTestDataCfg_t* const rxTestDataCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTestDataGet)(adi_adrv903x_Device_t* device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxTestDataCfg_t* const rxTestDataCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxLoPowerDownSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannelMask, const uint8_t enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxSpurBaseBandFreqSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannelMask, const int32_t bbFreqKhz, const uint8_t enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxSpurBaseBandFreqGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannelMask, adi_adrv903x_RxSpurFreqConfigResp_t* const rxSpurRbConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenTableRead)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, const uint32_t txAttenIndexOffset, adi_adrv903x_TxAttenTableRow_t txAttenTableRows[], const uint32_t numTxAttenEntries);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxAtten_t txAttenuation[], const uint32_t numTxAttenConfigs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxAtten_t * const txAttenuation);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenCfgSet)(adi_adrv903x_Device_t* const device, const uint32_t chanMask, adi_adrv903x_TxAttenCfg_t* const txAttenCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxAttenCfg_t* const txAttenCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenS0S1Set)(adi_adrv903x_Device_t* const device, const uint32_t chanMask, const uint32_t levelMilliDB, const uint8_t isS0);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenS0S1Get)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint32_t* const levelMilliDB, const uint8_t isS0);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenUpdateCfgSet)(adi_adrv903x_Device_t* const device, const uint32_t chanMask, const adi_adrv903x_TxAttenUpdateCfg_t* const cfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenUpdateCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxAttenUpdateCfg_t* const cfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenUpdate)(adi_adrv903x_Device_t *const device, const uint32_t chanMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxLoSourceGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_LoSel_e * const txLoSource);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPowerMonitorCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_PowerMonitorCfgRt_t txPowerMonitorCfg[], const uint32_t numPowerProtectCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPowerMonitorCfgGet)(adi_adrv903x_Device_t * const device, adi_adrv903x_PowerMonitorCfgRt_t* const txPowerMonitorCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionErrorGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint32_t* const eventBits);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionErrorClear)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, const uint32_t eventBits);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxSlewRateDetectorCfgSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_SlewRateDetectorCfgRt_t txSlewRateDetectorCfg[], const uint32_t numSlewRateDetCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxSlewRateDetectorCfgGet)(adi_adrv903x_Device_t * const device, adi_adrv903x_SlewRateDetectorCfgRt_t * const txSlewRateDetectorCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxSlewRateStatisticsRead)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, const uint8_t clearStats, uint16_t * const statisticsReadback);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionRampSampleHoldEnableSet)(adi_adrv903x_Device_t* const device, const uint32_t txChannelMask, const uint8_t enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionRampSampleHoldEnableGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint8_t* const enable);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionRampCfgSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_ProtectionRampCfgRt_t txProtectionRampCfg[], const uint32_t numProtectionRampCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionRampCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_ProtectionRampCfgRt_t* const txProtectionRampCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPowerMonitorStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxPowerMonitorStatus_t * const powerMonitorStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxNcoShifterSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxNcoMixConfig_t * const txNcoConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxNcoShifterGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TxNcoMixConfigReadbackResp_t* const txRbConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxTestToneSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxTestNcoConfig_t * const txNcoConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxTestToneGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TxTestNcoConfigReadbackResp_t* const txRbConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxDecimatedPowerCfgSet)(adi_adrv903x_Device_t * const device, adi_adrv903x_TxDecimatedPowerCfg_t txDecPowerCfg[], const uint32_t numOfDecPowerCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxDecimatedPowerCfgGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxDecimatedPowerCfg_t * const txDecPowerCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxDecimatedPowerGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, uint8_t * const powerReadBack, uint8_t * const powerPeakReadBack);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenPhaseSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxAttenPhaseCfg_t * const txAttenPhaseCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenPhaseGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TxAttenPhaseCfg_t* const txRbConfig);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DtxCfg_t dtxCfg[], const uint32_t numDtxCfgs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_DtxCfg_t* const dtxCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxGpioCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DtxGpioCfg_t* dtxGpioCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxGpioCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_DtxGpioCfg_t* const dtxGpioCfg);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxForceSet)(adi_adrv903x_Device_t* const device, const uint32_t txChannelMask, const uint8_t dtxForce);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint8_t * const dtxStatus);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPfirCoeffsWrite)(adi_adrv903x_Device_t* const device, const uint32_t txChannelMask, adi_adrv903x_TxPfirCoeff_t* const pfirCoeffs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPfirCoeffsRead)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxPfirCoeff_t* const pfirCoeffs);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuImageLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_cpuBinaryInfo_t* const cpuBinaryInfo);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamImageLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_streamBinaryInfo_t* const streamBinaryInfoPtr);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxGainTableInfo_t rxGainTableInfo[], const uint32_t rxGainTableArrSize);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeviceInfoExtract)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuProfileBinaryInfo_t* const cpuProfileBinaryInfoPtr);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuProfileImageLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuProfileBinaryInfo_t* const cpuProfileBinaryInfoPtr);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_DeviceCopy)( adi_adrv903x_Device_t* const deviceSrc, adi_adrv903x_Device_t* const deviceDest);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_PreMcsInit)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Init_t* const init, const adi_adrv903x_TrxFileInfo_t* const trxBinaryInfoPtr);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_PreMcsInit_NonBroadcast)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Init_t* const init);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_PostMcsInit)(adi_adrv903x_Device_t* const device, const adi_adrv903x_PostMcsInit_t* const utilityInit);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StandbyEnter)(adi_adrv903x_Device_t* const device, adi_adrv903x_StandbyRecover_t* const standbyRecover);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StandbyRecover)(adi_adrv903x_Device_t* const device, adi_adrv903x_StandbyRecover_t* const standbyRecover);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StandbyExit)(adi_adrv903x_Device_t* const device, adi_adrv903x_StandbyRecover_t* const standbyRecover);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_StandbyExitStatusGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_StandbyRecover_t* const standbyRecover, uint8_t * const done);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairFileLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_JrxRepairBinaryInfo_t* const fileInfo, adi_adrv903x_JrxRepairHistory_t* const repairHistory);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairFileSave)(adi_adrv903x_Device_t* const device, const adi_adrv903x_JrxRepairBinaryInfo_t * const fileInfo, adi_adrv903x_JrxRepairHistory_t* const repairHistory);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairHistoryCheck)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepair_t* const jrxRepair, int16_t* const checkTemp);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairLaneAssess)(adi_adrv903x_Device_t* const device, uint8_t *const badLaneMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairEnter)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepair_t* const jrxRepair);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairBiasSurvey)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepairHistory_t* const repairHistory, adi_adrv903x_JrxRepairBiasSurvey_t* const biasSurvey);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairFastAttackRun)(adi_adrv903x_Device_t* const device, adi_adrv903x_InitCalErrData_t* const initCalErrData);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairTest)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepairTest_t* const testResult);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairApply)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepairHistory_t* const repairHistory);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairSwCEnableGet)(adi_adrv903x_Device_t* const device, uint8_t* const swcEnMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairSwCEnableSet)(adi_adrv903x_Device_t* const device, const uint8_t swcEnMask);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairExit)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepair_t* const jrxRepair);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairExecute)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepair_t* const jrxRepair);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairVcmLanesFix)(adi_adrv903x_Device_t* const device, uint8_t laneMask, uint8_t enableFix);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairInitialization)(adi_adrv903x_Device_t* const device, uint8_t enableRepair);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuMemDump)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuMemDumpBinaryInfo_t* const cpuMemDumpBinaryInfoPtr, const uint8_t forceException, uint32_t* const dumpSize);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuMemDump_vRamOnly)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuMemDumpBinaryInfo_t* const cpuMemDumpBinaryInfoPtr, const uint8_t forceException, uint32_t* const dumpSize);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableChecksumRead)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxGainTableInfo_t* const rxGainTableInfoPtr, uint32_t* const rxGainTableChecksum);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableChecksumCalculate)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, uint32_t* const rxGainTableChecksum);
ADI_API_EX adi_adrv903x_ErrAction_e (*adi_adrv903x_InitDataExtract)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuProfileBinaryInfo_t* const cpuBinaryInfo, adi_adrv903x_Version_t* const apiVer, adi_adrv903x_CpuFwVersion_t* const fwVer, adi_adrv903x_Version_t* const streamVer, adi_adrv903x_Init_t* const init, adi_adrv903x_PostMcsInit_t* const postMcsInit, adi_adrv903x_ExtractInitDataOutput_e* const checkOutput);


/**
 * \brief Loads the version of the ADRV903X device contained in the shared
 * library supplied for use by the application.
 *
 * Opens the specified library and searches it for implementations of all the
 * functions in the ADRV903X API. Then assigns the global function pointer
 * corresponding to each function to the implementation in the specified shared
 * library.
 *
 * \param[in] soPath The shared library to open. Unless this is a relative or fully
 * qualified path LD_LIBRARY_PATH env var or other shared object location
 * mechanism must be correctly set to find the shared lib.
 *
 * Note: The default is that applications statically link to the device
 * library. To  use this function the application must be compiled as a
 * multiversioning capable application. See the ADRV903X Software User Guide
 * for details.
 *
 * \returns Returns 0 on success.
 */
ADI_API int adi_adrv903x_VersionLoad(const char* const soPath);

/**
 * \brief Same as adi_adrv903x_VersionLoad but with control of log messages.
 *
 * \param[in] soPath Same as adi_adrv903x_VersionLoad.
 * \param[in] logStream A valid FILE* stream where error messages will be sent. If NULL
 *     error are not logged.
 *
 * \returns Same as adi_adrv903x_VersionLoad.
 */
ADI_API int adi_adrv903x_VersionLoad_vLogCtl(const char* const soPath, FILE* const logStream);

#endif
