/**
* Copyright 2015 - 2025 Analog Devices Inc.
* SPDX-License-Identifier: Apache-2.0
*/

/**
*   \file   adi_adrv903x_ver_redirect.c
* 
*   \brief  Auto generated function pointer shim layer to ease linking to one of multiple 
            versions of the API at runtime
*
*   Generated from ADRV903X API Version: 2.12.1.4
*/

#include <stdio.h>
#include <dlfcn.h>

#include "adi_adrv903x_ver_redirect.h"

/* The redirect-table comprising a fn ptr for each PUBLIC API fn */
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcCfgSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_AgcCfg_t agcConfig[], const uint32_t numOfAgcCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcCfgGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_AgcCfg_t * const agcConfigReadBack) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcFreezeSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint32_t freezeEnable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcFreezeGet)(adi_adrv903x_Device_t* const device, uint32_t* const freezeEnable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcFreezeOnGpioSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannels, const adi_adrv903x_GpioPinSel_e gpioPin, const uint8_t enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcFreezeOnGpioGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_GpioPinSel_e* const gpioPin) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcGainIndexRangeSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_AgcGainRange_t agcGainRange[], const uint32_t numOfAgcRangeCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcGainIndexRangeGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_AgcGainRange_t * const agcGainConfigReadback) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcReset)(adi_adrv903x_Device_t * const device, const uint32_t rxChannelMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDualBandLnaGainTableWrite)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint8_t gainIndexOffset, const adi_adrv903x_RxDualBandLnaGainTableRow_t gainTableRow[], const uint32_t arraySize) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDualBandLnaGainTableRead)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint8_t gainIndexOffset, adi_adrv903x_RxDualBandLnaGainTableRow_t gainTableRow[], const uint32_t arraySize, uint8_t* const numGainIndicesRead) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandCfgSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_AgcDualBandCfg_t agcDualBandConfig[], const uint32_t numOfAgcDualBandCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandCfgGet)(adi_adrv903x_Device_t * const device, adi_adrv903x_AgcDualBandCfg_t * const agcDualBandConfigReadBack) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandGpioCfgSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_AgcDualBandGpioCfg_t * agcDualBandGpioConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandGpioCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_AgcDualBandGpioCfg_t* const agcDualBandGpioConfigReadBack) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcGpioReSyncSet)(adi_adrv903x_Device_t * const device, const uint32_t rxChannelMask, const adi_adrv903x_GpioPinSel_e gpioSelection, const uint8_t enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcGpioReSyncGet)(adi_adrv903x_Device_t * const device, const uint32_t rxChannelMask, adi_adrv903x_GpioPinSel_e * const gpioSelection, uint8_t * const enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcDualBandActiveExternalLnaGainWordGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, uint8_t * const bandAExternalLnaGainWord, uint8_t * const bandBExternalLnaGainWord) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcUpperLevelBlockerGet)(adi_adrv903x_Device_t * const device, uint8_t * const agcULBlockerBitMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcLowerLevelBlockerGet)(adi_adrv903x_Device_t * const device, uint8_t * const agcLLBlockerBitMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcHighThresholdPeakDetectorGet)(adi_adrv903x_Device_t * const device, uint8_t * const thresholdPeakDetectorBitMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AgcLowThresholdPeakDetectorGet)(adi_adrv903x_Device_t * const device, uint8_t * const thresholdPeakDetectorBitMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsRun)(adi_adrv903x_Device_t* const device, const adi_adrv903x_InitCals_t* const initCals) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsDetailedStatusGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_InitCalStatus_t* const initStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsDetailedStatusGet_v2)( adi_adrv903x_Device_t* const device, adi_adrv903x_InitCalErrData_t* const initCalErrData) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsWait)( adi_adrv903x_Device_t* const device, const uint32_t timeoutMs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsWait_v2)( adi_adrv903x_Device_t* const device, const uint32_t timeoutMs, adi_adrv903x_InitCalErrData_t* const initCalErrData) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsCheckCompleteGet)(adi_adrv903x_Device_t* const device, uint8_t* const areCalsRunning, const uint8_t calErrorCheck) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsCheckCompleteGet_v2)( adi_adrv903x_Device_t* const device, uint8_t* const areCalsRunning, adi_adrv903x_InitCalErrData_t* const initCalErrData) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalsAbort)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalsEnableSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TrackingCalibrationMask_t calMask, const uint32_t channelMask, const adi_adrv903x_TrackingCalEnableDisable_e enableDisableFlag) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalsEnableSet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TrackingCalibrationMask_e calMask, const adi_adrv903x_ChannelTrackingCals_t* const channelMask, const adi_adrv903x_TrackingCalEnableDisable_e enableDisableFlag) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalsEnableGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TrackingCalEnableMasks_t* const enableMasks) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalAllStateGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TrackingCalState_t* const calState) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TrackingCalStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TrackingCalibrationMask_e calId, const adi_adrv903x_Channels_e channel, adi_adrv903x_CalStatus_t* const calStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitCalStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_InitCalibrations_e calId, const adi_adrv903x_Channels_e channel, adi_adrv903x_CalStatus_t* const calStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxHrmDataGet)(adi_adrv903x_Device_t* const device, const uint8_t channelMask, adi_adrv903x_TxHrmData_t txHrmDataArray[], uint32_t arrayLength) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxHrmDataSet)(adi_adrv903x_Device_t* const device, const uint8_t channelMask, const adi_adrv903x_TxHrmData_t txHrmDataArray[], uint32_t arrayLength) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CalPvtStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Channels_e channel, const uint32_t objId, uint8_t calStatusGet[], uint32_t length) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CalSpecificStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Channels_e channel, const uint32_t objId, uint8_t calStatusGet[], uint32_t length) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DigDcOffsetEnableSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint32_t rxChannelEnableDisable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DigDcOffsetEnableGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, uint8_t* const isEnabled) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxLolReset)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxLolReset_t* const txLolReset) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxQecReset)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxQecReset_t* const txQecReset) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DigDcOffsetCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_SetDcOffset_t* const dcOffSetCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DigDcOffsetCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_CpuCmd_GetDcOffsetResp_t* const dcOffSetCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Lock)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Unlock)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_HwOpen)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SpiConfigSettings_t* const spiSettings) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_HwClose)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_HwReset)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Initialize)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Init_t* const init) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Shutdown)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_MultichipSyncSet)(adi_adrv903x_Device_t* const device, const uint8_t enableSync) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_MultichipSyncSet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_McsSyncMode_e mcsMode) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_MultichipSyncStatusGet)(adi_adrv903x_Device_t* const device, uint32_t* const mcsStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_ProfilesVerify)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Init_t* const init) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SpiConfigSettings_t* const spiCtrlSettings) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiConfigSettings_t* const spi) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AuxSpiCfgSet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_AuxSpiConfig_t* const config) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AuxSpiCfgGet)( adi_adrv903x_Device_t* const device, adi_adrv903x_AuxSpiConfig_t* const config) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiRuntimeOptionsSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SpiOptions_t* const spiOptions) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiRuntimeOptionsGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiOptions_t* const spiOptions) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiVerify)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_ApiVersionGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_Version_t* const apiVersion) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeviceRevGet)(adi_adrv903x_Device_t* const device, uint8_t* const siRevision) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_ProductIdGet)(adi_adrv903x_Device_t* const device, uint8_t* const productId) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeviceCapabilityGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_CapabilityModel_t* const devCapability) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiDoutPadDriveStrengthSet)(adi_adrv903x_Device_t* const device, const uint8_t driveStrength) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiHysteresisSet)( adi_adrv903x_Device_t* const device, const uint32_t enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DigitalHysteresisSet)( adi_adrv903x_Device_t* const device, const uint32_t enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuImageWrite)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuType_e cpuType, const uint32_t byteOffset, const uint8_t binary[], const uint32_t byteCount) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuProfileWrite)(adi_adrv903x_Device_t * const device, const uint32_t byteOffset, const uint8_t binary[], const uint32_t byteCount) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuStart)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuStartStatusCheck)(adi_adrv903x_Device_t* const device, const uint32_t timeout_us) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuConfigSet)( adi_adrv903x_Device_t* const device, const uint32_t objId, const uint16_t offset, const uint8_t configDataSet[], const uint32_t length) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuConfigGet)( adi_adrv903x_Device_t* const device, const uint32_t objId, const uint16_t offset, uint8_t configDataGet[], const uint32_t length) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuDebugModeEnable)(adi_adrv903x_Device_t* const device, const uint32_t enableKey) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuConfigUnlock)(adi_adrv903x_Device_t* const device, const uint32_t configKey) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuControlCmdExec)(adi_adrv903x_Device_t* const device, const uint32_t objId, const uint16_t cpuCmd, const adi_adrv903x_Channels_e channel, const uint8_t cpuCtrlData[], const uint32_t lengthSet, uint32_t* const lengthResp, uint8_t ctrlResp[], const uint32_t lengthGet) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuLogFilterSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuLogEvent_e eventFilter, const adi_adrv903x_CpuLogCpuId_e cpuIdFilter, const adi_adrv903x_CpuLogObjIdFilter_t* const objIdFilter) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuDebugCmdExec)( adi_adrv903x_Device_t* const device, const uint32_t objId, const uint16_t cpuCmd, const adi_adrv903x_Channels_e channel, const uint8_t cpuDebugData[], const uint32_t lengthSet, uint32_t* const lengthResp, uint8_t debugResp[], const uint32_t lengthGet) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuChannelMappingGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_CpuType_e cpuTypes[], const uint8_t numSerdesLanes) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuSysPvtStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Channels_e channel, const uint32_t objId, uint8_t cpuSysStatusGet[], const uint32_t length) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuSysStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Channels_e channel, const uint32_t objId, adi_adrv903x_CpuSysStatus_t* const cpuSysStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuFwVersionGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_CpuFwVersion_t* const cpuFwVersion) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_HealthMonitorCpuStatusGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_HealthMonitorCpuStatus_t* const healthMonitorStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuCheckException)(adi_adrv903x_Device_t* const device, uint32_t* const isException) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SwBreakPointEntry_t * breakPointCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_SwBreakPointEntry_t * const breakPointCfgRead) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointHitRead)(adi_adrv903x_Device_t* const device, adi_adrv903x_SwBreakPointEntry_t * const cpu0BreakpointHit, adi_adrv903x_SwBreakPointEntry_t * const cpu1BreakpointHit) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGpioCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpioOutputForBreakpointHit, const adi_adrv903x_GpioPinSel_e gpioInputToResumeSleepingTasks) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGpioCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_GpioPinSel_e * const gpioOutputForBreakpointHit, adi_adrv903x_GpioPinSel_e * const gpioInputToResumeSleepingTasks) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointResume)(adi_adrv903x_Device_t* const device, const adi_adrv903x_SwBreakPointEntry_t * breakpointToResume, uint8_t resumeAll) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointResumeFromHalt)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGlobalHaltMaskSet)(adi_adrv903x_Device_t* const device, const uint32_t globalHaltMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_BreakpointGlobalHaltMaskGet)(adi_adrv903x_Device_t* const device, uint32_t * const globalHaltMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxOrxDataCaptureStart)( adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e channelSelect, const adi_adrv903x_RxOrxDataCaptureLocation_e captureLocation, uint32_t captureData[], const uint32_t captureLength, const uint8_t trigger, const uint32_t timeout_us) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AdcSampleXbarSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, const adi_adrv903x_AdcSampleXbarCfg_t* const adcXbar) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_AdcSampleXbarGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, adi_adrv903x_AdcSampleXbarCfg_t* const adcXbar) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DacSampleXbarSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const adi_adrv903x_DacSampleXbarCfg_t* const dacXbar) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DacSampleXbarGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DacSampleXbarCfg_t* const dacXbar) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, adi_adrv903x_FramerCfg_t* const framerCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DeframerCfg_t* const deframerCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerCfgGetScaled)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const adi_adrv903x_TxChannels_e chanSel, adi_adrv903x_DeframerCfg_t* const deframerCfg, const uint8_t bypass) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLinkStateGet)(adi_adrv903x_Device_t* const device, uint8_t* const framerLinkState) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLinkStateSet)(adi_adrv903x_Device_t* device, const uint8_t framerSelMask, uint8_t const enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPrbsCountReset)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLinkStateGet)(adi_adrv903x_Device_t* device, uint8_t* const deframerLinkState) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLinkStateSet)(adi_adrv903x_Device_t* device, const uint8_t deframerSelMask, uint8_t const enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPrbsCheckerStateSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DfrmPrbsCfg_t * const dfrmPrbsCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPrbsCheckerStateGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_DfrmPrbsCfg_t * const dfrmPrbsCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSysrefCtrlSet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, uint8_t const enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSysrefCtrlGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, uint8_t * const enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerSysrefCtrlSet)(adi_adrv903x_Device_t* const device, const uint8_t deframerSelMask, uint8_t const enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerSysrefCtrlGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, uint8_t * const enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerTestDataSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_FrmTestDataCfg_t * const frmTestDataCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerTestDataGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, adi_adrv903x_FrmTestDataCfg_t * const frmTestDataCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPrbsErrCountGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_DfrmPrbsErrCounters_t * const counters) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SerializerReset)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SerializerReset_v2)(adi_adrv903x_Device_t* const device, adi_adrv903x_CpuCmd_SerReset_t* const pSerResetParms, adi_adrv903x_CpuCmd_SerResetResp_t* const pSerResetResp) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLmfcOffsetSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSelect, const uint16_t lmfcOffset) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLmfcOffsetGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSelect, uint16_t * const lmfcOffset) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmLmfcOffsetSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSelect, const uint16_t lmfcOffset) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmLmfcOffsetGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSelect, uint16_t * const lmfcOffset) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmPhaseDiffGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSelect, uint16_t * const phaseDiff) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_FramerSel_e framerSel, adi_adrv903x_FramerStatus_t * const framerStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DeframerStatus_t * const deframerStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerStatusGet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DeframerStatus_v2_t * const deframerStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCounterStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel, const uint8_t laneNumber, adi_adrv903x_DfrmErrCounterStatus_t * const errCounterStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCounterReset)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const uint8_t laneNumber, uint32_t const errCounterMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCounterThresholdSet)(adi_adrv903x_Device_t* const device, const uint8_t threshold) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Dfrm204cErrCounterStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel, const uint8_t laneNumber, adi_adrv903x_Dfrm204cErrCounterStatus_t * const errCounterStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Dfrm204cErrCounterReset)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmLinkConditionGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_DeframerSel_e deframerSel, uint8_t * const dfrmLinkCondition) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmFifoDepthGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const uint8_t laneNumber, uint8_t * const fifoDepth) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmCoreBufDepthGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, uint8_t * const coreBufDepth) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIlasMismatchGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DfrmCompareData_t* const dfrmData) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIlasMismatchGet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, adi_adrv903x_DfrmCompareData_v2_t* const dfrmData) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLoopbackSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerLoopbackDisable)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, const adi_adrv903x_AdcSampleXbarCfg_t* const adcXbar) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLoopbackSet)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLoopbackDisable)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLoopbackDisable_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, const adi_adrv903x_AdcSampleXbarCfg_t* const adcXbar) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLaneLoopbackSet)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerLaneLoopbackDisable)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbModeSet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, const uint8_t syncbMode) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbModeGet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, uint8_t* const syncbMode) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbStatusSet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, const uint8_t syncbStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbStatusGet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, uint8_t* const syncbStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbErrCntGet)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask, uint8_t* const syncbErrCnt) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerSyncbErrCntReset)(adi_adrv903x_Device_t* const device, const uint8_t framerSelMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerSyncbErrCntGet)(adi_adrv903x_Device_t* const device, const uint8_t deframerSelMask, uint8_t* const syncbErrCnt) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerSyncbErrCntReset)(adi_adrv903x_Device_t* const device, const uint8_t deframerSelMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerErrorCtrl)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSel, const adi_adrv903x_SerdesErrAction_e action) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeframerErrorCtrl)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSel, const adi_adrv903x_SerdesErrAction_e action) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIrqMaskGet)( adi_adrv903x_Device_t* const device, uint16_t* const irqMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIrqMaskSet)( adi_adrv903x_Device_t* const device, const uint16_t irqMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIrqSourceReset)( adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmIrqSourceGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_DeframerSel_e deframerSelect, adi_adrv903x_DeframerIrqVector_t* const irqSourceVector) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCntrCntrlSet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_DfrmErrCounterIrqSel_e interruptEnable, const uint8_t laneNumber, const uint8_t errCounterControl, const uint8_t errCounterHoldCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DfrmErrCntrCntrlGet)( adi_adrv903x_Device_t* const device, const uint8_t laneNumber, uint8_t* const errCounterControl, uint8_t* const errCounterHoldCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RunEyeSweep)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_RunEyeSweep_t* const runEyeSweep, adi_adrv903x_CpuCmd_RunEyeSweepResp_t* const runEyeSweepResp) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RunEyeSweep_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_RunEyeSweep_t* const runEyeSweep, adi_adrv903x_CpuCmd_RunEyeSweepResp_t* const runEyeSweepResp) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RunVerticalEyeSweep)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_RunVertEyeSweep_t* const runVerticalEyeSweep, adi_adrv903x_CpuCmd_RunVertEyeSweepResp_t* const runVerticalEyeSweepResp) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RunVerticalEyeSweep_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuCmd_RunVertEyeSweep_t* const runVerticalEyeSweep, adi_adrv903x_CpuCmd_RunVertEyeSweepResp_t* const runVerticalEyeSweepResp) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_FramerTestDataInjectError)(adi_adrv903x_Device_t* const device, const adi_adrv903x_FramerSel_e framerSelect, const uint8_t laneMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SerLaneCfgSet)(adi_adrv903x_Device_t* const device, const uint8_t laneNumber, const adi_adrv903x_SerLaneCfg_t* const serLaneCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SerLaneCfgGet)(adi_adrv903x_Device_t* const device, const uint8_t laneNumber, adi_adrv903x_SerLaneCfg_t* const serLaneCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SerdesInitCalStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GenericStrBuf_t* const filePath, const uint8_t laneNumber, const adi_adrv903x_GenericStrBuf_t* const msg, adi_adrv903x_SerdesInitCalStatus_t* const calStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SerdesTrackingCalStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GenericStrBuf_t* const filePath, const uint8_t laneNumber, const adi_adrv903x_GenericStrBuf_t* const msg, adi_adrv903x_SerdesTrackingCalStatus_t* calStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeserLanesVcmCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DeserLanesVcmCfg_t* const deserLanesVcmCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SerdesRxLaneSintCodesGet)(adi_adrv903x_Device_t* const device, const uint8_t laneNumber, adi_adrv903x_CpuCmd_GetRxLaneSintCodesResp_t* const serdesRxLaneSintCodes) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_ErrInfoGet)(const adi_adrv903x_ErrSource_e errSrc, const int64_t errCode, const char** const errMsgPtr, const char** const errCausePtr, adi_adrv903x_ErrAction_e* const actionCodePtr, const char** const actionMsgPtr) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_ErrDataGet)(const adi_adrv903x_Device_t* const device, const adi_common_ErrFrameId_e frameId, adi_adrv903x_ErrSource_e* const errSrcPtr, int64_t* const errCodePtr, const char** const errMsgPtr, const char** const errCausePtr, adi_adrv903x_ErrAction_e* const actionCodePtr, const char** const actionMsgPtr) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioForceHiZAllPins)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioForceHiZ)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpio, const uint8_t oRide) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioStatusRead)( adi_adrv903x_Device_t* const device, adi_adrv903x_GpioStatus_t* const status) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioConfigGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpio, adi_adrv903x_GpioSignal_e* const signal, uint32_t* const channelMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioConfigAllGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_GpioSignal_e signalArray[], uint32_t channelMaskArray[], const uint32_t arraySize) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogConfigGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioAnaPinSel_e gpio, adi_adrv903x_GpioSignal_e* const signal, uint32_t* const channelMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogConfigAllGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_GpioSignal_e signalArray[], uint32_t channelMaskArray[], const uint32_t arraySize) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioMonitorOutSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpio, const adi_adrv903x_GpioSignal_e signal, const uint8_t channel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioMonitorOutRelease)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpio) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualInputDirSet)( adi_adrv903x_Device_t* const device, const uint32_t gpioInputMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualOutputDirSet)( adi_adrv903x_Device_t* const device, const uint32_t gpioOutputMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualInputPinLevelGet)( adi_adrv903x_Device_t* const device, uint32_t * const gpioInPinLevel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualOutputPinLevelGet)( adi_adrv903x_Device_t* const device, uint32_t * const gpioOutPinLevel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioManualOutputPinLevelSet)( adi_adrv903x_Device_t* const device, const uint32_t gpioPinMask, const uint32_t gpioOutPinLevel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogForceHiZAllPins)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogForceHiZ)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioAnaPinSel_e gpio, const uint8_t oRide) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualInputDirSet)( adi_adrv903x_Device_t* const device, const uint16_t gpioAnalogInputMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualOutputDirSet)( adi_adrv903x_Device_t* const device, const uint16_t gpioAnalogOutputMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualInputPinLevelGet)( adi_adrv903x_Device_t* const device, uint16_t * const gpioAnalogInPinLevel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualOutputPinLevelGet)( adi_adrv903x_Device_t* const device, uint16_t * const gpioAnalogOutPinLevel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioAnalogManualOutputPinLevelSet)( adi_adrv903x_Device_t* const device, const uint16_t gpioAnalogPinMask, const uint16_t gpioAnalogOutPinLevel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntPinMaskCfgSet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpIntPinSelect_e pinSelect, const adi_adrv903x_GpIntPinMaskCfg_t* const pinMaskCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntPinMaskCfgGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpIntPinSelect_e pinSelect, adi_adrv903x_GpIntPinMaskCfg_t* const pinMaskCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntStatusGet)( adi_adrv903x_Device_t* const device, adi_adrv903x_GpIntMask_t* const gpIntStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntStatusClear)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpIntMask_t* const gpIntClear) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntStickyBitMaskSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpIntMask_t* const gpIntStickyBitMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpIntStickyBitMaskGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_GpIntMask_t* const gpIntStickyBitMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioHysteresisSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioDigitalPin_e pinName, const uint32_t enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioHysteresisGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioDigitalPin_e pinName, uint32_t* const enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioDriveStrengthSet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioDigitalPin_e pinName, const adi_adrv903x_CmosPadDrvStr_e driveStrength) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioDriveStrengthGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioDigitalPin_e pinName, adi_adrv903x_CmosPadDrvStr_e* const driveStrength) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_SpiFlush)(adi_adrv903x_Device_t* const device, const uint8_t data[], uint32_t* const count) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Registers32Write)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr[], const uint32_t writeData[], const uint32_t mask[], const uint32_t count) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Register32Write)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, uint32_t addr, const uint32_t writeData, const uint32_t mask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Register32Read)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, uint32_t addr, uint32_t* const readData, const uint32_t mask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Registers32Read)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, uint32_t readData[], uint32_t mask[], const uint32_t count) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RegistersByteWrite)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, const uint8_t writeData[], const uint32_t count) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RegistersByteRead)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, uint8_t readData[], const uint8_t mask[], const uint32_t count) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Registers32bOnlyRead)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, uint8_t readData[], const uint32_t count) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_Registers32bOnlyWrite)(adi_adrv903x_Device_t* const device, adi_adrv903x_SpiCache_t* const spiCache, const uint32_t addr, const uint8_t writeData[], const uint32_t count) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamImageWrite)(adi_adrv903x_Device_t* const device, uint32_t byteOffset, const uint8_t binary[], uint32_t byteCount) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTxEnableSet)(adi_adrv903x_Device_t* const device, const uint32_t orxChannelMask, const uint32_t orxChannelEnable, const uint32_t rxChannelMask, const uint32_t rxChannelEnable, const uint32_t txChannelMask, const uint32_t txChannelEnable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTxEnableGet)(adi_adrv903x_Device_t* const device, uint32_t* const orxChannelMask, uint32_t* const rxChannelMask, uint32_t* const txChannelMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_ChannelEnableGet)(adi_adrv903x_Device_t* const device, uint32_t* const orxChannelMask, uint32_t* const rxChannelMask, uint32_t* const txChannelMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlCfgSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RadioCtrlModeCfg_t* const radioCtrlCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlTxRxEnCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RadioCtrlTxRxEnCfg_t* const txRxEnCfg, uint8_t pinIndex, uint8_t configSel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlTxRxEnCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RadioCtrlTxRxEnCfg_t* const txRxEnCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_RadioCtrlModeCfg_t* const radioCtrlCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_LoFrequencySet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_LoConfig_t* const loConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_LoFrequencyGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_LoConfigReadback_t* const loConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CfgPllToChanCtrl)(adi_adrv903x_Device_t* const device, uint8_t rf0MuxTx0_3, uint8_t rf0MuxTx4_7, uint8_t rf0MuxRx0_3, uint8_t rf0MuxRx4_7) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_LoLoopFilterSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_LoName_e loName, const adi_adrv903x_LoLoopFilterCfg_t* const loLoopFilterConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_LoLoopFilterGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_LoName_e loName, adi_adrv903x_LoLoopFilterCfg_t* const loLoopFilterConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_PllStatusGet)(adi_adrv903x_Device_t* const device, uint32_t* const pllLockStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTxLoFreqGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxTxLoFreqReadback_t* const rxTxLoFreq) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TemperatureGet)(adi_adrv903x_Device_t* const device, const uint16_t avgMask, adi_adrv903x_DevTempData_t* const deviceTemperature) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TemperatureEnableGet)(adi_adrv903x_Device_t* const device, uint16_t* const tempEnData) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TemperatureEnableSet)(adi_adrv903x_Device_t* const device, uint16_t* const tempEnData) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamGpioConfigSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_StreamGpioPinCfg_t* const streamGpioPinCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamGpioConfigGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_StreamGpioPinCfg_t* const streamGpioPinCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxNcoFreqCalculate)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e orxChannel, const uint32_t txSynthesisBwLower_kHz, const uint32_t txSynthesisBwUpper_kHz, int32_t* const ncoShiftFreqAdc_kHz, int32_t* const ncoShiftFreqDatapath_kHz) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingInit)(adi_adrv903x_Device_t* const device) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingConfigGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TxToOrxMappingConfig_t * const mappingConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingSet)(adi_adrv903x_Device_t* const device, const uint8_t mapping) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e orxChannel, adi_adrv903x_TxChannels_e* const txChannel) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetAttenSet)( adi_adrv903x_Device_t* const device, const uint32_t mapping, const uint8_t presetAtten_dB, const uint8_t immediateUpdate) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetAttenGet_v2)( adi_adrv903x_Device_t* const device, const adi_adrv903x_TxToOrxMappingPinTable_e mapping, uint8_t* const presetAtten_dB) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetAttenGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint8_t* const presetAtten_dB) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetNcoSet)( adi_adrv903x_Device_t* const device, const uint32_t mapping, const adi_adrv903x_TxToOrxMappingPresetNco_t* const presetNco, const uint8_t immediateUpdate) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetNcoGet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxToOrxMappingPinTable_e mapping, adi_adrv903x_TxToOrxMappingPresetNco_t* const presetNco) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxToOrxMappingPresetNcoGet)( adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxToOrxMappingPresetNco_t* const presetNco) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamVersionGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_Version_t* const streamVersion) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalConfigSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxRadioCtrlAntennaCalConfig_t * const configRx, adi_adrv903x_TxRadioCtrlAntennaCalConfig_t * const configTx) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalConfigGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxRadioCtrlAntennaCalConfig_t * const configRx, adi_adrv903x_TxRadioCtrlAntennaCalConfig_t * const configTx) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalErrorGet)(adi_adrv903x_Device_t* const device, const uint32_t channelSel, uint8_t * const errStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalErrorClear)(adi_adrv903x_Device_t* const device, const uint32_t channelMask, const uint8_t errClearMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamProcErrorGet)( adi_adrv903x_Device_t* const device, adi_adrv903x_StreamErrArray_t* const streamErr) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalGpioChannelSet)(adi_adrv903x_Device_t* const device, const uint32_t txChannelMask, const uint32_t rxChannelMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalGpioChannelGet)(adi_adrv903x_Device_t* const device, uint32_t * const txChannelMask, uint32_t * const rxChannelMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RadioCtrlAntCalConfigSet_v2)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxRadioCtrlAntennaCalConfig_t * const configRx, adi_adrv903x_TxRadioCtrlAntennaCalConfig_t * const configTx, const uint8_t rxGain, const uint8_t enableFreeze) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioCtrlRxTxMapSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxRxCtrlGpioMap_t txRxCtrlGpioMap[], const uint32_t numGpios) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioCtrlRxTxMapGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_GpioPinSel_e gpioPin, const adi_adrv903x_TxRxCtrlGpioLogicalPin_e gpioLogicalPin, adi_adrv903x_TxRxCtrlGpioMap_t* const txRxCtrlGpio) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_GpioCtrlRxTxMapClear)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxRxCtrlGpioMap_t txRxCtrlGpioMap[], const uint32_t numGpios) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableWrite)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint8_t gainIndexOffset, const adi_adrv903x_RxGainTableRow_t gainTableRow[], const uint32_t arraySize ) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableRead)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, const uint8_t gainIndexOffset, adi_adrv903x_RxGainTableRow_t gainTableRow[], const uint32_t arraySize, uint16_t* const numGainIndicesRead) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxMinMaxGainIndexSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint8_t minGainIndex, const uint8_t maxGainIndex) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableExtCtrlPinsSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const uint32_t channelEnable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxGain_t rxGain[], const uint32_t arraySize) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxMgcGainGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxGain_t * const rxGain) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxGain_t * const rxGain) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDataFormatGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxDataFormatRt_t * const rxDataFormat) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxSlicerPositionGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, uint8_t * const slicerPosition) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxLoSourceGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_LoSel_e * const rxLoSource) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxNcoShifterSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxNcoConfig_t * const rxNcoConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxNcoShifterGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_RxNcoConfigReadbackResp_t* const rxRbConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxNcoSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_ORxNcoConfig_t * const orxNcoConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxNcoSet_v2)(adi_adrv903x_Device_t* const device, const adi_adrv903x_ORxNcoConfig_t * const orxNcoConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxNcoGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_ORxNcoConfigReadbackResp_t* const orxRbConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDecimatedPowerCfgSet)(adi_adrv903x_Device_t * const device, adi_adrv903x_RxDecimatedPowerCfg_t rxDecPowerCfg[], const uint32_t numOfDecPowerCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDecimatedPowerCfgGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, const adi_adrv903x_DecPowerMeasurementBlock_e decPowerBlockSelection, adi_adrv903x_RxDecimatedPowerCfg_t * const rxDecPowerCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_ORxDecimatedPowerCfgSet)(adi_adrv903x_Device_t * const device, adi_adrv903x_ORxDecimatedPowerCfg_t orxDecPowerCfg[], const uint32_t numOfDecPowerCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_ORxDecimatedPowerCfgGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e orxChannel, adi_adrv903x_ORxDecimatedPowerCfg_t * const orxDecPowerCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxDecimatedPowerGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_RxChannels_e rxChannel, const adi_adrv903x_DecPowerMeasurementBlock_e decPowerBlockSelection, uint8_t * const powerReadBack) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxAttenSet)(adi_adrv903x_Device_t* const device, const uint32_t channelMask, const uint8_t attenDb) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_OrxAttenGet)(adi_adrv903x_Device_t* const device, const uint8_t channelId, uint8_t* const attenDb) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainCtrlModeSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxGainCtrlModeCfg_t gainCtrlModeCfg[], const uint32_t arraySize) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainCtrlModeGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxGainCtrlMode_e* gainCtrlMode) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTempGainCompSet)(adi_adrv903x_Device_t* const device, const uint32_t rxChannelMask, const int8_t gainValue) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTempGainCompGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, int8_t* const gainValue) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTestDataSet)(adi_adrv903x_Device_t* device, const uint32_t rxChannelMask, const adi_adrv903x_RxTestDataCfg_t* const rxTestDataCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxTestDataGet)(adi_adrv903x_Device_t* device, const adi_adrv903x_RxChannels_e rxChannel, adi_adrv903x_RxTestDataCfg_t* const rxTestDataCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxLoPowerDownSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannelMask, const uint8_t enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxSpurBaseBandFreqSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannelMask, const int32_t bbFreqKhz, const uint8_t enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxSpurBaseBandFreqGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannelMask, adi_adrv903x_RxSpurFreqConfigResp_t* const rxSpurRbConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenTableRead)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, const uint32_t txAttenIndexOffset, adi_adrv903x_TxAttenTableRow_t txAttenTableRows[], const uint32_t numTxAttenEntries) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenSet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxAtten_t txAttenuation[], const uint32_t numTxAttenConfigs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxAtten_t * const txAttenuation) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenCfgSet)(adi_adrv903x_Device_t* const device, const uint32_t chanMask, adi_adrv903x_TxAttenCfg_t* const txAttenCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxAttenCfg_t* const txAttenCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenS0S1Set)(adi_adrv903x_Device_t* const device, const uint32_t chanMask, const uint32_t levelMilliDB, const uint8_t isS0) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenS0S1Get)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint32_t* const levelMilliDB, const uint8_t isS0) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenUpdateCfgSet)(adi_adrv903x_Device_t* const device, const uint32_t chanMask, const adi_adrv903x_TxAttenUpdateCfg_t* const cfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenUpdateCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxAttenUpdateCfg_t* const cfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenUpdate)(adi_adrv903x_Device_t *const device, const uint32_t chanMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxLoSourceGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_LoSel_e * const txLoSource) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPowerMonitorCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_PowerMonitorCfgRt_t txPowerMonitorCfg[], const uint32_t numPowerProtectCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPowerMonitorCfgGet)(adi_adrv903x_Device_t * const device, adi_adrv903x_PowerMonitorCfgRt_t* const txPowerMonitorCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionErrorGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint32_t* const eventBits) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionErrorClear)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, const uint32_t eventBits) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxSlewRateDetectorCfgSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_SlewRateDetectorCfgRt_t txSlewRateDetectorCfg[], const uint32_t numSlewRateDetCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxSlewRateDetectorCfgGet)(adi_adrv903x_Device_t * const device, adi_adrv903x_SlewRateDetectorCfgRt_t * const txSlewRateDetectorCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxSlewRateStatisticsRead)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, const uint8_t clearStats, uint16_t * const statisticsReadback) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionRampSampleHoldEnableSet)(adi_adrv903x_Device_t* const device, const uint32_t txChannelMask, const uint8_t enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionRampSampleHoldEnableGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint8_t* const enable) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionRampCfgSet)(adi_adrv903x_Device_t* const device, adi_adrv903x_ProtectionRampCfgRt_t txProtectionRampCfg[], const uint32_t numProtectionRampCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxProtectionRampCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_ProtectionRampCfgRt_t* const txProtectionRampCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPowerMonitorStatusGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxPowerMonitorStatus_t * const powerMonitorStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxNcoShifterSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxNcoMixConfig_t * const txNcoConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxNcoShifterGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TxNcoMixConfigReadbackResp_t* const txRbConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxTestToneSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxTestNcoConfig_t * const txNcoConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxTestToneGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TxTestNcoConfigReadbackResp_t* const txRbConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxDecimatedPowerCfgSet)(adi_adrv903x_Device_t * const device, adi_adrv903x_TxDecimatedPowerCfg_t txDecPowerCfg[], const uint32_t numOfDecPowerCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxDecimatedPowerCfgGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxDecimatedPowerCfg_t * const txDecPowerCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxDecimatedPowerGet)(adi_adrv903x_Device_t * const device, const adi_adrv903x_TxChannels_e txChannel, uint8_t * const powerReadBack, uint8_t * const powerPeakReadBack) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenPhaseSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxAttenPhaseCfg_t * const txAttenPhaseCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxAttenPhaseGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_TxAttenPhaseCfg_t* const txRbConfig) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DtxCfg_t dtxCfg[], const uint32_t numDtxCfgs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxCfgGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_DtxCfg_t* const dtxCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxGpioCfgSet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_DtxGpioCfg_t* dtxGpioCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxGpioCfgGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_DtxGpioCfg_t* const dtxGpioCfg) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxForceSet)(adi_adrv903x_Device_t* const device, const uint32_t txChannelMask, const uint8_t dtxForce) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DtxStatusGet)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, uint8_t * const dtxStatus) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPfirCoeffsWrite)(adi_adrv903x_Device_t* const device, const uint32_t txChannelMask, adi_adrv903x_TxPfirCoeff_t* const pfirCoeffs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_TxPfirCoeffsRead)(adi_adrv903x_Device_t* const device, const adi_adrv903x_TxChannels_e txChannel, adi_adrv903x_TxPfirCoeff_t* const pfirCoeffs) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuImageLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_cpuBinaryInfo_t* const cpuBinaryInfo) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StreamImageLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_streamBinaryInfo_t* const streamBinaryInfoPtr) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxGainTableInfo_t rxGainTableInfo[], const uint32_t rxGainTableArrSize) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeviceInfoExtract)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuProfileBinaryInfo_t* const cpuProfileBinaryInfoPtr) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuProfileImageLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuProfileBinaryInfo_t* const cpuProfileBinaryInfoPtr) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_DeviceCopy)( adi_adrv903x_Device_t* const deviceSrc, adi_adrv903x_Device_t* const deviceDest) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_PreMcsInit)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Init_t* const init, const adi_adrv903x_TrxFileInfo_t* const trxBinaryInfoPtr) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_PreMcsInit_NonBroadcast)(adi_adrv903x_Device_t* const device, const adi_adrv903x_Init_t* const init) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_PostMcsInit)(adi_adrv903x_Device_t* const device, const adi_adrv903x_PostMcsInit_t* const utilityInit) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StandbyEnter)(adi_adrv903x_Device_t* const device, adi_adrv903x_StandbyRecover_t* const standbyRecover) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StandbyRecover)(adi_adrv903x_Device_t* const device, adi_adrv903x_StandbyRecover_t* const standbyRecover) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StandbyExit)(adi_adrv903x_Device_t* const device, adi_adrv903x_StandbyRecover_t* const standbyRecover) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_StandbyExitStatusGet)(adi_adrv903x_Device_t* const device, adi_adrv903x_StandbyRecover_t* const standbyRecover, uint8_t * const done) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairFileLoad)(adi_adrv903x_Device_t* const device, const adi_adrv903x_JrxRepairBinaryInfo_t* const fileInfo, adi_adrv903x_JrxRepairHistory_t* const repairHistory) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairFileSave)(adi_adrv903x_Device_t* const device, const adi_adrv903x_JrxRepairBinaryInfo_t * const fileInfo, adi_adrv903x_JrxRepairHistory_t* const repairHistory) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairHistoryCheck)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepair_t* const jrxRepair, int16_t* const checkTemp) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairLaneAssess)(adi_adrv903x_Device_t* const device, uint8_t *const badLaneMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairEnter)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepair_t* const jrxRepair) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairBiasSurvey)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepairHistory_t* const repairHistory, adi_adrv903x_JrxRepairBiasSurvey_t* const biasSurvey) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairFastAttackRun)(adi_adrv903x_Device_t* const device, adi_adrv903x_InitCalErrData_t* const initCalErrData) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairTest)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepairTest_t* const testResult) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairApply)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepairHistory_t* const repairHistory) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairSwCEnableGet)(adi_adrv903x_Device_t* const device, uint8_t* const swcEnMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairSwCEnableSet)(adi_adrv903x_Device_t* const device, const uint8_t swcEnMask) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairExit)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepair_t* const jrxRepair) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairExecute)(adi_adrv903x_Device_t* const device, adi_adrv903x_JrxRepair_t* const jrxRepair) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairVcmLanesFix)(adi_adrv903x_Device_t* const device, uint8_t laneMask, uint8_t enableFix) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_JrxRepairInitialization)(adi_adrv903x_Device_t* const device, uint8_t enableRepair) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuMemDump)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuMemDumpBinaryInfo_t* const cpuMemDumpBinaryInfoPtr, const uint8_t forceException, uint32_t* const dumpSize) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_CpuMemDump_vRamOnly)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuMemDumpBinaryInfo_t* const cpuMemDumpBinaryInfoPtr, const uint8_t forceException, uint32_t* const dumpSize) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableChecksumRead)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxGainTableInfo_t* const rxGainTableInfoPtr, uint32_t* const rxGainTableChecksum) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_RxGainTableChecksumCalculate)(adi_adrv903x_Device_t* const device, const adi_adrv903x_RxChannels_e rxChannel, uint32_t* const rxGainTableChecksum) = NULL;
adi_adrv903x_ErrAction_e (*adi_adrv903x_InitDataExtract)(adi_adrv903x_Device_t* const device, const adi_adrv903x_CpuProfileBinaryInfo_t* const cpuBinaryInfo, adi_adrv903x_Version_t* const apiVer, adi_adrv903x_CpuFwVersion_t* const fwVer, adi_adrv903x_Version_t* const streamVer, adi_adrv903x_Init_t* const init, adi_adrv903x_PostMcsInit_t* const postMcsInit, adi_adrv903x_ExtractInitDataOutput_e* const checkOutput) = NULL;


/* There is a not-implmented fn per device type, not per API implementation/release or API extension. */
static adi_adrv903x_ErrAction_e adi_adrv903x_NotImplemented(void)
{
    return ADI_ADRV903X_ERR_ACT_CHECK_MULTIVERSIONING;
}

/* Maps from fnname to fnptr */
typedef struct fnNamesAndNullImpl {
    const char* fnName;
    void** fnPtrPtr;
} fnNamesAndNullImpl_t;

static fnNamesAndNullImpl_t fnNamesAndDefaults[] = {
/*  {"adi_adrv903x_FuncName", (void**) &adi_adrv903x_FuncName, (void*) adrv903x_not_implemented},*/
        {"adi_adrv903x_AgcCfgSet", (void**) &adi_adrv903x_AgcCfgSet},
    {"adi_adrv903x_AgcCfgGet", (void**) &adi_adrv903x_AgcCfgGet},
    {"adi_adrv903x_AgcFreezeSet", (void**) &adi_adrv903x_AgcFreezeSet},
    {"adi_adrv903x_AgcFreezeGet", (void**) &adi_adrv903x_AgcFreezeGet},
    {"adi_adrv903x_AgcFreezeOnGpioSet", (void**) &adi_adrv903x_AgcFreezeOnGpioSet},
    {"adi_adrv903x_AgcFreezeOnGpioGet", (void**) &adi_adrv903x_AgcFreezeOnGpioGet},
    {"adi_adrv903x_AgcGainIndexRangeSet", (void**) &adi_adrv903x_AgcGainIndexRangeSet},
    {"adi_adrv903x_AgcGainIndexRangeGet", (void**) &adi_adrv903x_AgcGainIndexRangeGet},
    {"adi_adrv903x_AgcReset", (void**) &adi_adrv903x_AgcReset},
    {"adi_adrv903x_RxDualBandLnaGainTableWrite", (void**) &adi_adrv903x_RxDualBandLnaGainTableWrite},
    {"adi_adrv903x_RxDualBandLnaGainTableRead", (void**) &adi_adrv903x_RxDualBandLnaGainTableRead},
    {"adi_adrv903x_AgcDualBandCfgSet", (void**) &adi_adrv903x_AgcDualBandCfgSet},
    {"adi_adrv903x_AgcDualBandCfgGet", (void**) &adi_adrv903x_AgcDualBandCfgGet},
    {"adi_adrv903x_AgcDualBandGpioCfgSet", (void**) &adi_adrv903x_AgcDualBandGpioCfgSet},
    {"adi_adrv903x_AgcDualBandGpioCfgGet", (void**) &adi_adrv903x_AgcDualBandGpioCfgGet},
    {"adi_adrv903x_AgcGpioReSyncSet", (void**) &adi_adrv903x_AgcGpioReSyncSet},
    {"adi_adrv903x_AgcGpioReSyncGet", (void**) &adi_adrv903x_AgcGpioReSyncGet},
    {"adi_adrv903x_AgcDualBandActiveExternalLnaGainWordGet", (void**) &adi_adrv903x_AgcDualBandActiveExternalLnaGainWordGet},
    {"adi_adrv903x_AgcUpperLevelBlockerGet", (void**) &adi_adrv903x_AgcUpperLevelBlockerGet},
    {"adi_adrv903x_AgcLowerLevelBlockerGet", (void**) &adi_adrv903x_AgcLowerLevelBlockerGet},
    {"adi_adrv903x_AgcHighThresholdPeakDetectorGet", (void**) &adi_adrv903x_AgcHighThresholdPeakDetectorGet},
    {"adi_adrv903x_AgcLowThresholdPeakDetectorGet", (void**) &adi_adrv903x_AgcLowThresholdPeakDetectorGet},
    {"adi_adrv903x_InitCalsRun", (void**) &adi_adrv903x_InitCalsRun},
    {"adi_adrv903x_InitCalsDetailedStatusGet", (void**) &adi_adrv903x_InitCalsDetailedStatusGet},
    {"adi_adrv903x_InitCalsDetailedStatusGet_v2", (void**) &adi_adrv903x_InitCalsDetailedStatusGet_v2},
    {"adi_adrv903x_InitCalsWait", (void**) &adi_adrv903x_InitCalsWait},
    {"adi_adrv903x_InitCalsWait_v2", (void**) &adi_adrv903x_InitCalsWait_v2},
    {"adi_adrv903x_InitCalsCheckCompleteGet", (void**) &adi_adrv903x_InitCalsCheckCompleteGet},
    {"adi_adrv903x_InitCalsCheckCompleteGet_v2", (void**) &adi_adrv903x_InitCalsCheckCompleteGet_v2},
    {"adi_adrv903x_InitCalsAbort", (void**) &adi_adrv903x_InitCalsAbort},
    {"adi_adrv903x_TrackingCalsEnableSet", (void**) &adi_adrv903x_TrackingCalsEnableSet},
    {"adi_adrv903x_TrackingCalsEnableSet_v2", (void**) &adi_adrv903x_TrackingCalsEnableSet_v2},
    {"adi_adrv903x_TrackingCalsEnableGet", (void**) &adi_adrv903x_TrackingCalsEnableGet},
    {"adi_adrv903x_TrackingCalAllStateGet", (void**) &adi_adrv903x_TrackingCalAllStateGet},
    {"adi_adrv903x_TrackingCalStatusGet", (void**) &adi_adrv903x_TrackingCalStatusGet},
    {"adi_adrv903x_InitCalStatusGet", (void**) &adi_adrv903x_InitCalStatusGet},
    {"adi_adrv903x_TxHrmDataGet", (void**) &adi_adrv903x_TxHrmDataGet},
    {"adi_adrv903x_TxHrmDataSet", (void**) &adi_adrv903x_TxHrmDataSet},
    {"adi_adrv903x_CalPvtStatusGet", (void**) &adi_adrv903x_CalPvtStatusGet},
    {"adi_adrv903x_CalSpecificStatusGet", (void**) &adi_adrv903x_CalSpecificStatusGet},
    {"adi_adrv903x_DigDcOffsetEnableSet", (void**) &adi_adrv903x_DigDcOffsetEnableSet},
    {"adi_adrv903x_DigDcOffsetEnableGet", (void**) &adi_adrv903x_DigDcOffsetEnableGet},
    {"adi_adrv903x_TxLolReset", (void**) &adi_adrv903x_TxLolReset},
    {"adi_adrv903x_TxQecReset", (void**) &adi_adrv903x_TxQecReset},
    {"adi_adrv903x_DigDcOffsetCfgSet", (void**) &adi_adrv903x_DigDcOffsetCfgSet},
    {"adi_adrv903x_DigDcOffsetCfgGet", (void**) &adi_adrv903x_DigDcOffsetCfgGet},
    {"adi_adrv903x_Lock", (void**) &adi_adrv903x_Lock},
    {"adi_adrv903x_Unlock", (void**) &adi_adrv903x_Unlock},
    {"adi_adrv903x_HwOpen", (void**) &adi_adrv903x_HwOpen},
    {"adi_adrv903x_HwClose", (void**) &adi_adrv903x_HwClose},
    {"adi_adrv903x_HwReset", (void**) &adi_adrv903x_HwReset},
    {"adi_adrv903x_Initialize", (void**) &adi_adrv903x_Initialize},
    {"adi_adrv903x_Shutdown", (void**) &adi_adrv903x_Shutdown},
    {"adi_adrv903x_MultichipSyncSet", (void**) &adi_adrv903x_MultichipSyncSet},
    {"adi_adrv903x_MultichipSyncSet_v2", (void**) &adi_adrv903x_MultichipSyncSet_v2},
    {"adi_adrv903x_MultichipSyncStatusGet", (void**) &adi_adrv903x_MultichipSyncStatusGet},
    {"adi_adrv903x_ProfilesVerify", (void**) &adi_adrv903x_ProfilesVerify},
    {"adi_adrv903x_SpiCfgSet", (void**) &adi_adrv903x_SpiCfgSet},
    {"adi_adrv903x_SpiCfgGet", (void**) &adi_adrv903x_SpiCfgGet},
    {"adi_adrv903x_AuxSpiCfgSet", (void**) &adi_adrv903x_AuxSpiCfgSet},
    {"adi_adrv903x_AuxSpiCfgGet", (void**) &adi_adrv903x_AuxSpiCfgGet},
    {"adi_adrv903x_SpiRuntimeOptionsSet", (void**) &adi_adrv903x_SpiRuntimeOptionsSet},
    {"adi_adrv903x_SpiRuntimeOptionsGet", (void**) &adi_adrv903x_SpiRuntimeOptionsGet},
    {"adi_adrv903x_SpiVerify", (void**) &adi_adrv903x_SpiVerify},
    {"adi_adrv903x_ApiVersionGet", (void**) &adi_adrv903x_ApiVersionGet},
    {"adi_adrv903x_DeviceRevGet", (void**) &adi_adrv903x_DeviceRevGet},
    {"adi_adrv903x_ProductIdGet", (void**) &adi_adrv903x_ProductIdGet},
    {"adi_adrv903x_DeviceCapabilityGet", (void**) &adi_adrv903x_DeviceCapabilityGet},
    {"adi_adrv903x_SpiDoutPadDriveStrengthSet", (void**) &adi_adrv903x_SpiDoutPadDriveStrengthSet},
    {"adi_adrv903x_SpiHysteresisSet", (void**) &adi_adrv903x_SpiHysteresisSet},
    {"adi_adrv903x_DigitalHysteresisSet", (void**) &adi_adrv903x_DigitalHysteresisSet},
    {"adi_adrv903x_CpuImageWrite", (void**) &adi_adrv903x_CpuImageWrite},
    {"adi_adrv903x_CpuProfileWrite", (void**) &adi_adrv903x_CpuProfileWrite},
    {"adi_adrv903x_CpuStart", (void**) &adi_adrv903x_CpuStart},
    {"adi_adrv903x_CpuStartStatusCheck", (void**) &adi_adrv903x_CpuStartStatusCheck},
    {"adi_adrv903x_CpuConfigSet", (void**) &adi_adrv903x_CpuConfigSet},
    {"adi_adrv903x_CpuConfigGet", (void**) &adi_adrv903x_CpuConfigGet},
    {"adi_adrv903x_CpuDebugModeEnable", (void**) &adi_adrv903x_CpuDebugModeEnable},
    {"adi_adrv903x_CpuConfigUnlock", (void**) &adi_adrv903x_CpuConfigUnlock},
    {"adi_adrv903x_CpuControlCmdExec", (void**) &adi_adrv903x_CpuControlCmdExec},
    {"adi_adrv903x_CpuLogFilterSet", (void**) &adi_adrv903x_CpuLogFilterSet},
    {"adi_adrv903x_CpuDebugCmdExec", (void**) &adi_adrv903x_CpuDebugCmdExec},
    {"adi_adrv903x_CpuChannelMappingGet", (void**) &adi_adrv903x_CpuChannelMappingGet},
    {"adi_adrv903x_CpuSysPvtStatusGet", (void**) &adi_adrv903x_CpuSysPvtStatusGet},
    {"adi_adrv903x_CpuSysStatusGet", (void**) &adi_adrv903x_CpuSysStatusGet},
    {"adi_adrv903x_CpuFwVersionGet", (void**) &adi_adrv903x_CpuFwVersionGet},
    {"adi_adrv903x_HealthMonitorCpuStatusGet", (void**) &adi_adrv903x_HealthMonitorCpuStatusGet},
    {"adi_adrv903x_CpuCheckException", (void**) &adi_adrv903x_CpuCheckException},
    {"adi_adrv903x_BreakpointSet", (void**) &adi_adrv903x_BreakpointSet},
    {"adi_adrv903x_BreakpointGet", (void**) &adi_adrv903x_BreakpointGet},
    {"adi_adrv903x_BreakpointHitRead", (void**) &adi_adrv903x_BreakpointHitRead},
    {"adi_adrv903x_BreakpointGpioCfgSet", (void**) &adi_adrv903x_BreakpointGpioCfgSet},
    {"adi_adrv903x_BreakpointGpioCfgGet", (void**) &adi_adrv903x_BreakpointGpioCfgGet},
    {"adi_adrv903x_BreakpointResume", (void**) &adi_adrv903x_BreakpointResume},
    {"adi_adrv903x_BreakpointResumeFromHalt", (void**) &adi_adrv903x_BreakpointResumeFromHalt},
    {"adi_adrv903x_BreakpointGlobalHaltMaskSet", (void**) &adi_adrv903x_BreakpointGlobalHaltMaskSet},
    {"adi_adrv903x_BreakpointGlobalHaltMaskGet", (void**) &adi_adrv903x_BreakpointGlobalHaltMaskGet},
    {"adi_adrv903x_RxOrxDataCaptureStart", (void**) &adi_adrv903x_RxOrxDataCaptureStart},
    {"adi_adrv903x_AdcSampleXbarSet", (void**) &adi_adrv903x_AdcSampleXbarSet},
    {"adi_adrv903x_AdcSampleXbarGet", (void**) &adi_adrv903x_AdcSampleXbarGet},
    {"adi_adrv903x_DacSampleXbarSet", (void**) &adi_adrv903x_DacSampleXbarSet},
    {"adi_adrv903x_DacSampleXbarGet", (void**) &adi_adrv903x_DacSampleXbarGet},
    {"adi_adrv903x_FramerCfgGet", (void**) &adi_adrv903x_FramerCfgGet},
    {"adi_adrv903x_DeframerCfgGet", (void**) &adi_adrv903x_DeframerCfgGet},
    {"adi_adrv903x_DeframerCfgGetScaled", (void**) &adi_adrv903x_DeframerCfgGetScaled},
    {"adi_adrv903x_FramerLinkStateGet", (void**) &adi_adrv903x_FramerLinkStateGet},
    {"adi_adrv903x_FramerLinkStateSet", (void**) &adi_adrv903x_FramerLinkStateSet},
    {"adi_adrv903x_DfrmPrbsCountReset", (void**) &adi_adrv903x_DfrmPrbsCountReset},
    {"adi_adrv903x_DeframerLinkStateGet", (void**) &adi_adrv903x_DeframerLinkStateGet},
    {"adi_adrv903x_DeframerLinkStateSet", (void**) &adi_adrv903x_DeframerLinkStateSet},
    {"adi_adrv903x_DfrmPrbsCheckerStateSet", (void**) &adi_adrv903x_DfrmPrbsCheckerStateSet},
    {"adi_adrv903x_DfrmPrbsCheckerStateGet", (void**) &adi_adrv903x_DfrmPrbsCheckerStateGet},
    {"adi_adrv903x_FramerSysrefCtrlSet", (void**) &adi_adrv903x_FramerSysrefCtrlSet},
    {"adi_adrv903x_FramerSysrefCtrlGet", (void**) &adi_adrv903x_FramerSysrefCtrlGet},
    {"adi_adrv903x_DeframerSysrefCtrlSet", (void**) &adi_adrv903x_DeframerSysrefCtrlSet},
    {"adi_adrv903x_DeframerSysrefCtrlGet", (void**) &adi_adrv903x_DeframerSysrefCtrlGet},
    {"adi_adrv903x_FramerTestDataSet", (void**) &adi_adrv903x_FramerTestDataSet},
    {"adi_adrv903x_FramerTestDataGet", (void**) &adi_adrv903x_FramerTestDataGet},
    {"adi_adrv903x_DfrmPrbsErrCountGet", (void**) &adi_adrv903x_DfrmPrbsErrCountGet},
    {"adi_adrv903x_SerializerReset", (void**) &adi_adrv903x_SerializerReset},
    {"adi_adrv903x_SerializerReset_v2", (void**) &adi_adrv903x_SerializerReset_v2},
    {"adi_adrv903x_FramerLmfcOffsetSet", (void**) &adi_adrv903x_FramerLmfcOffsetSet},
    {"adi_adrv903x_FramerLmfcOffsetGet", (void**) &adi_adrv903x_FramerLmfcOffsetGet},
    {"adi_adrv903x_DfrmLmfcOffsetSet", (void**) &adi_adrv903x_DfrmLmfcOffsetSet},
    {"adi_adrv903x_DfrmLmfcOffsetGet", (void**) &adi_adrv903x_DfrmLmfcOffsetGet},
    {"adi_adrv903x_DfrmPhaseDiffGet", (void**) &adi_adrv903x_DfrmPhaseDiffGet},
    {"adi_adrv903x_FramerStatusGet", (void**) &adi_adrv903x_FramerStatusGet},
    {"adi_adrv903x_DeframerStatusGet", (void**) &adi_adrv903x_DeframerStatusGet},
    {"adi_adrv903x_DeframerStatusGet_v2", (void**) &adi_adrv903x_DeframerStatusGet_v2},
    {"adi_adrv903x_DfrmErrCounterStatusGet", (void**) &adi_adrv903x_DfrmErrCounterStatusGet},
    {"adi_adrv903x_DfrmErrCounterReset", (void**) &adi_adrv903x_DfrmErrCounterReset},
    {"adi_adrv903x_DfrmErrCounterThresholdSet", (void**) &adi_adrv903x_DfrmErrCounterThresholdSet},
    {"adi_adrv903x_Dfrm204cErrCounterStatusGet", (void**) &adi_adrv903x_Dfrm204cErrCounterStatusGet},
    {"adi_adrv903x_Dfrm204cErrCounterReset", (void**) &adi_adrv903x_Dfrm204cErrCounterReset},
    {"adi_adrv903x_DfrmLinkConditionGet", (void**) &adi_adrv903x_DfrmLinkConditionGet},
    {"adi_adrv903x_DfrmFifoDepthGet", (void**) &adi_adrv903x_DfrmFifoDepthGet},
    {"adi_adrv903x_DfrmCoreBufDepthGet", (void**) &adi_adrv903x_DfrmCoreBufDepthGet},
    {"adi_adrv903x_DfrmIlasMismatchGet", (void**) &adi_adrv903x_DfrmIlasMismatchGet},
    {"adi_adrv903x_DfrmIlasMismatchGet_v2", (void**) &adi_adrv903x_DfrmIlasMismatchGet_v2},
    {"adi_adrv903x_FramerLoopbackSet", (void**) &adi_adrv903x_FramerLoopbackSet},
    {"adi_adrv903x_FramerLoopbackDisable", (void**) &adi_adrv903x_FramerLoopbackDisable},
    {"adi_adrv903x_DeframerLoopbackSet", (void**) &adi_adrv903x_DeframerLoopbackSet},
    {"adi_adrv903x_DeframerLoopbackDisable", (void**) &adi_adrv903x_DeframerLoopbackDisable},
    {"adi_adrv903x_DeframerLoopbackDisable_v2", (void**) &adi_adrv903x_DeframerLoopbackDisable_v2},
    {"adi_adrv903x_DeframerLaneLoopbackSet", (void**) &adi_adrv903x_DeframerLaneLoopbackSet},
    {"adi_adrv903x_DeframerLaneLoopbackDisable", (void**) &adi_adrv903x_DeframerLaneLoopbackDisable},
    {"adi_adrv903x_FramerSyncbModeSet", (void**) &adi_adrv903x_FramerSyncbModeSet},
    {"adi_adrv903x_FramerSyncbModeGet", (void**) &adi_adrv903x_FramerSyncbModeGet},
    {"adi_adrv903x_FramerSyncbStatusSet", (void**) &adi_adrv903x_FramerSyncbStatusSet},
    {"adi_adrv903x_FramerSyncbStatusGet", (void**) &adi_adrv903x_FramerSyncbStatusGet},
    {"adi_adrv903x_FramerSyncbErrCntGet", (void**) &adi_adrv903x_FramerSyncbErrCntGet},
    {"adi_adrv903x_FramerSyncbErrCntReset", (void**) &adi_adrv903x_FramerSyncbErrCntReset},
    {"adi_adrv903x_DeframerSyncbErrCntGet", (void**) &adi_adrv903x_DeframerSyncbErrCntGet},
    {"adi_adrv903x_DeframerSyncbErrCntReset", (void**) &adi_adrv903x_DeframerSyncbErrCntReset},
    {"adi_adrv903x_FramerErrorCtrl", (void**) &adi_adrv903x_FramerErrorCtrl},
    {"adi_adrv903x_DeframerErrorCtrl", (void**) &adi_adrv903x_DeframerErrorCtrl},
    {"adi_adrv903x_DfrmIrqMaskGet", (void**) &adi_adrv903x_DfrmIrqMaskGet},
    {"adi_adrv903x_DfrmIrqMaskSet", (void**) &adi_adrv903x_DfrmIrqMaskSet},
    {"adi_adrv903x_DfrmIrqSourceReset", (void**) &adi_adrv903x_DfrmIrqSourceReset},
    {"adi_adrv903x_DfrmIrqSourceGet", (void**) &adi_adrv903x_DfrmIrqSourceGet},
    {"adi_adrv903x_DfrmErrCntrCntrlSet", (void**) &adi_adrv903x_DfrmErrCntrCntrlSet},
    {"adi_adrv903x_DfrmErrCntrCntrlGet", (void**) &adi_adrv903x_DfrmErrCntrCntrlGet},
    {"adi_adrv903x_RunEyeSweep", (void**) &adi_adrv903x_RunEyeSweep},
    {"adi_adrv903x_RunEyeSweep_v2", (void**) &adi_adrv903x_RunEyeSweep_v2},
    {"adi_adrv903x_RunVerticalEyeSweep", (void**) &adi_adrv903x_RunVerticalEyeSweep},
    {"adi_adrv903x_RunVerticalEyeSweep_v2", (void**) &adi_adrv903x_RunVerticalEyeSweep_v2},
    {"adi_adrv903x_FramerTestDataInjectError", (void**) &adi_adrv903x_FramerTestDataInjectError},
    {"adi_adrv903x_SerLaneCfgSet", (void**) &adi_adrv903x_SerLaneCfgSet},
    {"adi_adrv903x_SerLaneCfgGet", (void**) &adi_adrv903x_SerLaneCfgGet},
    {"adi_adrv903x_SerdesInitCalStatusGet", (void**) &adi_adrv903x_SerdesInitCalStatusGet},
    {"adi_adrv903x_SerdesTrackingCalStatusGet", (void**) &adi_adrv903x_SerdesTrackingCalStatusGet},
    {"adi_adrv903x_DeserLanesVcmCfgSet", (void**) &adi_adrv903x_DeserLanesVcmCfgSet},
    {"adi_adrv903x_SerdesRxLaneSintCodesGet", (void**) &adi_adrv903x_SerdesRxLaneSintCodesGet},
    {"adi_adrv903x_ErrInfoGet", (void**) &adi_adrv903x_ErrInfoGet},
    {"adi_adrv903x_ErrDataGet", (void**) &adi_adrv903x_ErrDataGet},
    {"adi_adrv903x_GpioForceHiZAllPins", (void**) &adi_adrv903x_GpioForceHiZAllPins},
    {"adi_adrv903x_GpioForceHiZ", (void**) &adi_adrv903x_GpioForceHiZ},
    {"adi_adrv903x_GpioStatusRead", (void**) &adi_adrv903x_GpioStatusRead},
    {"adi_adrv903x_GpioConfigGet", (void**) &adi_adrv903x_GpioConfigGet},
    {"adi_adrv903x_GpioConfigAllGet", (void**) &adi_adrv903x_GpioConfigAllGet},
    {"adi_adrv903x_GpioAnalogConfigGet", (void**) &adi_adrv903x_GpioAnalogConfigGet},
    {"adi_adrv903x_GpioAnalogConfigAllGet", (void**) &adi_adrv903x_GpioAnalogConfigAllGet},
    {"adi_adrv903x_GpioMonitorOutSet", (void**) &adi_adrv903x_GpioMonitorOutSet},
    {"adi_adrv903x_GpioMonitorOutRelease", (void**) &adi_adrv903x_GpioMonitorOutRelease},
    {"adi_adrv903x_GpioManualInputDirSet", (void**) &adi_adrv903x_GpioManualInputDirSet},
    {"adi_adrv903x_GpioManualOutputDirSet", (void**) &adi_adrv903x_GpioManualOutputDirSet},
    {"adi_adrv903x_GpioManualInputPinLevelGet", (void**) &adi_adrv903x_GpioManualInputPinLevelGet},
    {"adi_adrv903x_GpioManualOutputPinLevelGet", (void**) &adi_adrv903x_GpioManualOutputPinLevelGet},
    {"adi_adrv903x_GpioManualOutputPinLevelSet", (void**) &adi_adrv903x_GpioManualOutputPinLevelSet},
    {"adi_adrv903x_GpioAnalogForceHiZAllPins", (void**) &adi_adrv903x_GpioAnalogForceHiZAllPins},
    {"adi_adrv903x_GpioAnalogForceHiZ", (void**) &adi_adrv903x_GpioAnalogForceHiZ},
    {"adi_adrv903x_GpioAnalogManualInputDirSet", (void**) &adi_adrv903x_GpioAnalogManualInputDirSet},
    {"adi_adrv903x_GpioAnalogManualOutputDirSet", (void**) &adi_adrv903x_GpioAnalogManualOutputDirSet},
    {"adi_adrv903x_GpioAnalogManualInputPinLevelGet", (void**) &adi_adrv903x_GpioAnalogManualInputPinLevelGet},
    {"adi_adrv903x_GpioAnalogManualOutputPinLevelGet", (void**) &adi_adrv903x_GpioAnalogManualOutputPinLevelGet},
    {"adi_adrv903x_GpioAnalogManualOutputPinLevelSet", (void**) &adi_adrv903x_GpioAnalogManualOutputPinLevelSet},
    {"adi_adrv903x_GpIntPinMaskCfgSet", (void**) &adi_adrv903x_GpIntPinMaskCfgSet},
    {"adi_adrv903x_GpIntPinMaskCfgGet", (void**) &adi_adrv903x_GpIntPinMaskCfgGet},
    {"adi_adrv903x_GpIntStatusGet", (void**) &adi_adrv903x_GpIntStatusGet},
    {"adi_adrv903x_GpIntStatusClear", (void**) &adi_adrv903x_GpIntStatusClear},
    {"adi_adrv903x_GpIntStickyBitMaskSet", (void**) &adi_adrv903x_GpIntStickyBitMaskSet},
    {"adi_adrv903x_GpIntStickyBitMaskGet", (void**) &adi_adrv903x_GpIntStickyBitMaskGet},
    {"adi_adrv903x_GpioHysteresisSet", (void**) &adi_adrv903x_GpioHysteresisSet},
    {"adi_adrv903x_GpioHysteresisGet", (void**) &adi_adrv903x_GpioHysteresisGet},
    {"adi_adrv903x_GpioDriveStrengthSet", (void**) &adi_adrv903x_GpioDriveStrengthSet},
    {"adi_adrv903x_GpioDriveStrengthGet", (void**) &adi_adrv903x_GpioDriveStrengthGet},
    {"adi_adrv903x_SpiFlush", (void**) &adi_adrv903x_SpiFlush},
    {"adi_adrv903x_Registers32Write", (void**) &adi_adrv903x_Registers32Write},
    {"adi_adrv903x_Register32Write", (void**) &adi_adrv903x_Register32Write},
    {"adi_adrv903x_Register32Read", (void**) &adi_adrv903x_Register32Read},
    {"adi_adrv903x_Registers32Read", (void**) &adi_adrv903x_Registers32Read},
    {"adi_adrv903x_RegistersByteWrite", (void**) &adi_adrv903x_RegistersByteWrite},
    {"adi_adrv903x_RegistersByteRead", (void**) &adi_adrv903x_RegistersByteRead},
    {"adi_adrv903x_Registers32bOnlyRead", (void**) &adi_adrv903x_Registers32bOnlyRead},
    {"adi_adrv903x_Registers32bOnlyWrite", (void**) &adi_adrv903x_Registers32bOnlyWrite},
    {"adi_adrv903x_StreamImageWrite", (void**) &adi_adrv903x_StreamImageWrite},
    {"adi_adrv903x_RxTxEnableSet", (void**) &adi_adrv903x_RxTxEnableSet},
    {"adi_adrv903x_RxTxEnableGet", (void**) &adi_adrv903x_RxTxEnableGet},
    {"adi_adrv903x_ChannelEnableGet", (void**) &adi_adrv903x_ChannelEnableGet},
    {"adi_adrv903x_RadioCtrlCfgSet", (void**) &adi_adrv903x_RadioCtrlCfgSet},
    {"adi_adrv903x_RadioCtrlTxRxEnCfgSet", (void**) &adi_adrv903x_RadioCtrlTxRxEnCfgSet},
    {"adi_adrv903x_RadioCtrlTxRxEnCfgGet", (void**) &adi_adrv903x_RadioCtrlTxRxEnCfgGet},
    {"adi_adrv903x_RadioCtrlCfgGet", (void**) &adi_adrv903x_RadioCtrlCfgGet},
    {"adi_adrv903x_LoFrequencySet", (void**) &adi_adrv903x_LoFrequencySet},
    {"adi_adrv903x_LoFrequencyGet", (void**) &adi_adrv903x_LoFrequencyGet},
    {"adi_adrv903x_CfgPllToChanCtrl", (void**) &adi_adrv903x_CfgPllToChanCtrl},
    {"adi_adrv903x_LoLoopFilterSet", (void**) &adi_adrv903x_LoLoopFilterSet},
    {"adi_adrv903x_LoLoopFilterGet", (void**) &adi_adrv903x_LoLoopFilterGet},
    {"adi_adrv903x_PllStatusGet", (void**) &adi_adrv903x_PllStatusGet},
    {"adi_adrv903x_RxTxLoFreqGet", (void**) &adi_adrv903x_RxTxLoFreqGet},
    {"adi_adrv903x_TemperatureGet", (void**) &adi_adrv903x_TemperatureGet},
    {"adi_adrv903x_TemperatureEnableGet", (void**) &adi_adrv903x_TemperatureEnableGet},
    {"adi_adrv903x_TemperatureEnableSet", (void**) &adi_adrv903x_TemperatureEnableSet},
    {"adi_adrv903x_StreamGpioConfigSet", (void**) &adi_adrv903x_StreamGpioConfigSet},
    {"adi_adrv903x_StreamGpioConfigGet", (void**) &adi_adrv903x_StreamGpioConfigGet},
    {"adi_adrv903x_OrxNcoFreqCalculate", (void**) &adi_adrv903x_OrxNcoFreqCalculate},
    {"adi_adrv903x_TxToOrxMappingInit", (void**) &adi_adrv903x_TxToOrxMappingInit},
    {"adi_adrv903x_TxToOrxMappingConfigGet", (void**) &adi_adrv903x_TxToOrxMappingConfigGet},
    {"adi_adrv903x_TxToOrxMappingSet", (void**) &adi_adrv903x_TxToOrxMappingSet},
    {"adi_adrv903x_TxToOrxMappingGet", (void**) &adi_adrv903x_TxToOrxMappingGet},
    {"adi_adrv903x_TxToOrxMappingPresetAttenSet", (void**) &adi_adrv903x_TxToOrxMappingPresetAttenSet},
    {"adi_adrv903x_TxToOrxMappingPresetAttenGet_v2", (void**) &adi_adrv903x_TxToOrxMappingPresetAttenGet_v2},
    {"adi_adrv903x_TxToOrxMappingPresetAttenGet", (void**) &adi_adrv903x_TxToOrxMappingPresetAttenGet},
    {"adi_adrv903x_TxToOrxMappingPresetNcoSet", (void**) &adi_adrv903x_TxToOrxMappingPresetNcoSet},
    {"adi_adrv903x_TxToOrxMappingPresetNcoGet_v2", (void**) &adi_adrv903x_TxToOrxMappingPresetNcoGet_v2},
    {"adi_adrv903x_TxToOrxMappingPresetNcoGet", (void**) &adi_adrv903x_TxToOrxMappingPresetNcoGet},
    {"adi_adrv903x_StreamVersionGet", (void**) &adi_adrv903x_StreamVersionGet},
    {"adi_adrv903x_RadioCtrlAntCalConfigSet", (void**) &adi_adrv903x_RadioCtrlAntCalConfigSet},
    {"adi_adrv903x_RadioCtrlAntCalConfigGet", (void**) &adi_adrv903x_RadioCtrlAntCalConfigGet},
    {"adi_adrv903x_RadioCtrlAntCalErrorGet", (void**) &adi_adrv903x_RadioCtrlAntCalErrorGet},
    {"adi_adrv903x_RadioCtrlAntCalErrorClear", (void**) &adi_adrv903x_RadioCtrlAntCalErrorClear},
    {"adi_adrv903x_StreamProcErrorGet", (void**) &adi_adrv903x_StreamProcErrorGet},
    {"adi_adrv903x_RadioCtrlAntCalGpioChannelSet", (void**) &adi_adrv903x_RadioCtrlAntCalGpioChannelSet},
    {"adi_adrv903x_RadioCtrlAntCalGpioChannelGet", (void**) &adi_adrv903x_RadioCtrlAntCalGpioChannelGet},
    {"adi_adrv903x_RadioCtrlAntCalConfigSet_v2", (void**) &adi_adrv903x_RadioCtrlAntCalConfigSet_v2},
    {"adi_adrv903x_GpioCtrlRxTxMapSet", (void**) &adi_adrv903x_GpioCtrlRxTxMapSet},
    {"adi_adrv903x_GpioCtrlRxTxMapGet", (void**) &adi_adrv903x_GpioCtrlRxTxMapGet},
    {"adi_adrv903x_GpioCtrlRxTxMapClear", (void**) &adi_adrv903x_GpioCtrlRxTxMapClear},
    {"adi_adrv903x_RxGainTableWrite", (void**) &adi_adrv903x_RxGainTableWrite},
    {"adi_adrv903x_RxGainTableRead", (void**) &adi_adrv903x_RxGainTableRead},
    {"adi_adrv903x_RxMinMaxGainIndexSet", (void**) &adi_adrv903x_RxMinMaxGainIndexSet},
    {"adi_adrv903x_RxGainTableExtCtrlPinsSet", (void**) &adi_adrv903x_RxGainTableExtCtrlPinsSet},
    {"adi_adrv903x_RxGainSet", (void**) &adi_adrv903x_RxGainSet},
    {"adi_adrv903x_RxMgcGainGet", (void**) &adi_adrv903x_RxMgcGainGet},
    {"adi_adrv903x_RxGainGet", (void**) &adi_adrv903x_RxGainGet},
    {"adi_adrv903x_RxDataFormatGet", (void**) &adi_adrv903x_RxDataFormatGet},
    {"adi_adrv903x_RxSlicerPositionGet", (void**) &adi_adrv903x_RxSlicerPositionGet},
    {"adi_adrv903x_RxLoSourceGet", (void**) &adi_adrv903x_RxLoSourceGet},
    {"adi_adrv903x_RxNcoShifterSet", (void**) &adi_adrv903x_RxNcoShifterSet},
    {"adi_adrv903x_RxNcoShifterGet", (void**) &adi_adrv903x_RxNcoShifterGet},
    {"adi_adrv903x_OrxNcoSet", (void**) &adi_adrv903x_OrxNcoSet},
    {"adi_adrv903x_OrxNcoSet_v2", (void**) &adi_adrv903x_OrxNcoSet_v2},
    {"adi_adrv903x_OrxNcoGet", (void**) &adi_adrv903x_OrxNcoGet},
    {"adi_adrv903x_RxDecimatedPowerCfgSet", (void**) &adi_adrv903x_RxDecimatedPowerCfgSet},
    {"adi_adrv903x_RxDecimatedPowerCfgGet", (void**) &adi_adrv903x_RxDecimatedPowerCfgGet},
    {"adi_adrv903x_ORxDecimatedPowerCfgSet", (void**) &adi_adrv903x_ORxDecimatedPowerCfgSet},
    {"adi_adrv903x_ORxDecimatedPowerCfgGet", (void**) &adi_adrv903x_ORxDecimatedPowerCfgGet},
    {"adi_adrv903x_RxDecimatedPowerGet", (void**) &adi_adrv903x_RxDecimatedPowerGet},
    {"adi_adrv903x_OrxAttenSet", (void**) &adi_adrv903x_OrxAttenSet},
    {"adi_adrv903x_OrxAttenGet", (void**) &adi_adrv903x_OrxAttenGet},
    {"adi_adrv903x_RxGainCtrlModeSet", (void**) &adi_adrv903x_RxGainCtrlModeSet},
    {"adi_adrv903x_RxGainCtrlModeGet", (void**) &adi_adrv903x_RxGainCtrlModeGet},
    {"adi_adrv903x_RxTempGainCompSet", (void**) &adi_adrv903x_RxTempGainCompSet},
    {"adi_adrv903x_RxTempGainCompGet", (void**) &adi_adrv903x_RxTempGainCompGet},
    {"adi_adrv903x_RxTestDataSet", (void**) &adi_adrv903x_RxTestDataSet},
    {"adi_adrv903x_RxTestDataGet", (void**) &adi_adrv903x_RxTestDataGet},
    {"adi_adrv903x_RxLoPowerDownSet", (void**) &adi_adrv903x_RxLoPowerDownSet},
    {"adi_adrv903x_RxSpurBaseBandFreqSet", (void**) &adi_adrv903x_RxSpurBaseBandFreqSet},
    {"adi_adrv903x_RxSpurBaseBandFreqGet", (void**) &adi_adrv903x_RxSpurBaseBandFreqGet},
    {"adi_adrv903x_TxAttenTableRead", (void**) &adi_adrv903x_TxAttenTableRead},
    {"adi_adrv903x_TxAttenSet", (void**) &adi_adrv903x_TxAttenSet},
    {"adi_adrv903x_TxAttenGet", (void**) &adi_adrv903x_TxAttenGet},
    {"adi_adrv903x_TxAttenCfgSet", (void**) &adi_adrv903x_TxAttenCfgSet},
    {"adi_adrv903x_TxAttenCfgGet", (void**) &adi_adrv903x_TxAttenCfgGet},
    {"adi_adrv903x_TxAttenS0S1Set", (void**) &adi_adrv903x_TxAttenS0S1Set},
    {"adi_adrv903x_TxAttenS0S1Get", (void**) &adi_adrv903x_TxAttenS0S1Get},
    {"adi_adrv903x_TxAttenUpdateCfgSet", (void**) &adi_adrv903x_TxAttenUpdateCfgSet},
    {"adi_adrv903x_TxAttenUpdateCfgGet", (void**) &adi_adrv903x_TxAttenUpdateCfgGet},
    {"adi_adrv903x_TxAttenUpdate", (void**) &adi_adrv903x_TxAttenUpdate},
    {"adi_adrv903x_TxLoSourceGet", (void**) &adi_adrv903x_TxLoSourceGet},
    {"adi_adrv903x_TxPowerMonitorCfgSet", (void**) &adi_adrv903x_TxPowerMonitorCfgSet},
    {"adi_adrv903x_TxPowerMonitorCfgGet", (void**) &adi_adrv903x_TxPowerMonitorCfgGet},
    {"adi_adrv903x_TxProtectionErrorGet", (void**) &adi_adrv903x_TxProtectionErrorGet},
    {"adi_adrv903x_TxProtectionErrorClear", (void**) &adi_adrv903x_TxProtectionErrorClear},
    {"adi_adrv903x_TxSlewRateDetectorCfgSet", (void**) &adi_adrv903x_TxSlewRateDetectorCfgSet},
    {"adi_adrv903x_TxSlewRateDetectorCfgGet", (void**) &adi_adrv903x_TxSlewRateDetectorCfgGet},
    {"adi_adrv903x_TxSlewRateStatisticsRead", (void**) &adi_adrv903x_TxSlewRateStatisticsRead},
    {"adi_adrv903x_TxProtectionRampSampleHoldEnableSet", (void**) &adi_adrv903x_TxProtectionRampSampleHoldEnableSet},
    {"adi_adrv903x_TxProtectionRampSampleHoldEnableGet", (void**) &adi_adrv903x_TxProtectionRampSampleHoldEnableGet},
    {"adi_adrv903x_TxProtectionRampCfgSet", (void**) &adi_adrv903x_TxProtectionRampCfgSet},
    {"adi_adrv903x_TxProtectionRampCfgGet", (void**) &adi_adrv903x_TxProtectionRampCfgGet},
    {"adi_adrv903x_TxPowerMonitorStatusGet", (void**) &adi_adrv903x_TxPowerMonitorStatusGet},
    {"adi_adrv903x_TxNcoShifterSet", (void**) &adi_adrv903x_TxNcoShifterSet},
    {"adi_adrv903x_TxNcoShifterGet", (void**) &adi_adrv903x_TxNcoShifterGet},
    {"adi_adrv903x_TxTestToneSet", (void**) &adi_adrv903x_TxTestToneSet},
    {"adi_adrv903x_TxTestToneGet", (void**) &adi_adrv903x_TxTestToneGet},
    {"adi_adrv903x_TxDecimatedPowerCfgSet", (void**) &adi_adrv903x_TxDecimatedPowerCfgSet},
    {"adi_adrv903x_TxDecimatedPowerCfgGet", (void**) &adi_adrv903x_TxDecimatedPowerCfgGet},
    {"adi_adrv903x_TxDecimatedPowerGet", (void**) &adi_adrv903x_TxDecimatedPowerGet},
    {"adi_adrv903x_TxAttenPhaseSet", (void**) &adi_adrv903x_TxAttenPhaseSet},
    {"adi_adrv903x_TxAttenPhaseGet", (void**) &adi_adrv903x_TxAttenPhaseGet},
    {"adi_adrv903x_DtxCfgSet", (void**) &adi_adrv903x_DtxCfgSet},
    {"adi_adrv903x_DtxCfgGet", (void**) &adi_adrv903x_DtxCfgGet},
    {"adi_adrv903x_DtxGpioCfgSet", (void**) &adi_adrv903x_DtxGpioCfgSet},
    {"adi_adrv903x_DtxGpioCfgGet", (void**) &adi_adrv903x_DtxGpioCfgGet},
    {"adi_adrv903x_DtxForceSet", (void**) &adi_adrv903x_DtxForceSet},
    {"adi_adrv903x_DtxStatusGet", (void**) &adi_adrv903x_DtxStatusGet},
    {"adi_adrv903x_TxPfirCoeffsWrite", (void**) &adi_adrv903x_TxPfirCoeffsWrite},
    {"adi_adrv903x_TxPfirCoeffsRead", (void**) &adi_adrv903x_TxPfirCoeffsRead},
    {"adi_adrv903x_CpuImageLoad", (void**) &adi_adrv903x_CpuImageLoad},
    {"adi_adrv903x_StreamImageLoad", (void**) &adi_adrv903x_StreamImageLoad},
    {"adi_adrv903x_RxGainTableLoad", (void**) &adi_adrv903x_RxGainTableLoad},
    {"adi_adrv903x_DeviceInfoExtract", (void**) &adi_adrv903x_DeviceInfoExtract},
    {"adi_adrv903x_CpuProfileImageLoad", (void**) &adi_adrv903x_CpuProfileImageLoad},
    {"adi_adrv903x_DeviceCopy", (void**) &adi_adrv903x_DeviceCopy},
    {"adi_adrv903x_PreMcsInit", (void**) &adi_adrv903x_PreMcsInit},
    {"adi_adrv903x_PreMcsInit_NonBroadcast", (void**) &adi_adrv903x_PreMcsInit_NonBroadcast},
    {"adi_adrv903x_PostMcsInit", (void**) &adi_adrv903x_PostMcsInit},
    {"adi_adrv903x_StandbyEnter", (void**) &adi_adrv903x_StandbyEnter},
    {"adi_adrv903x_StandbyRecover", (void**) &adi_adrv903x_StandbyRecover},
    {"adi_adrv903x_StandbyExit", (void**) &adi_adrv903x_StandbyExit},
    {"adi_adrv903x_StandbyExitStatusGet", (void**) &adi_adrv903x_StandbyExitStatusGet},
    {"adi_adrv903x_JrxRepairFileLoad", (void**) &adi_adrv903x_JrxRepairFileLoad},
    {"adi_adrv903x_JrxRepairFileSave", (void**) &adi_adrv903x_JrxRepairFileSave},
    {"adi_adrv903x_JrxRepairHistoryCheck", (void**) &adi_adrv903x_JrxRepairHistoryCheck},
    {"adi_adrv903x_JrxRepairLaneAssess", (void**) &adi_adrv903x_JrxRepairLaneAssess},
    {"adi_adrv903x_JrxRepairEnter", (void**) &adi_adrv903x_JrxRepairEnter},
    {"adi_adrv903x_JrxRepairBiasSurvey", (void**) &adi_adrv903x_JrxRepairBiasSurvey},
    {"adi_adrv903x_JrxRepairFastAttackRun", (void**) &adi_adrv903x_JrxRepairFastAttackRun},
    {"adi_adrv903x_JrxRepairTest", (void**) &adi_adrv903x_JrxRepairTest},
    {"adi_adrv903x_JrxRepairApply", (void**) &adi_adrv903x_JrxRepairApply},
    {"adi_adrv903x_JrxRepairSwCEnableGet", (void**) &adi_adrv903x_JrxRepairSwCEnableGet},
    {"adi_adrv903x_JrxRepairSwCEnableSet", (void**) &adi_adrv903x_JrxRepairSwCEnableSet},
    {"adi_adrv903x_JrxRepairExit", (void**) &adi_adrv903x_JrxRepairExit},
    {"adi_adrv903x_JrxRepairExecute", (void**) &adi_adrv903x_JrxRepairExecute},
    {"adi_adrv903x_JrxRepairVcmLanesFix", (void**) &adi_adrv903x_JrxRepairVcmLanesFix},
    {"adi_adrv903x_JrxRepairInitialization", (void**) &adi_adrv903x_JrxRepairInitialization},
    {"adi_adrv903x_CpuMemDump", (void**) &adi_adrv903x_CpuMemDump},
    {"adi_adrv903x_CpuMemDump_vRamOnly", (void**) &adi_adrv903x_CpuMemDump_vRamOnly},
    {"adi_adrv903x_RxGainTableChecksumRead", (void**) &adi_adrv903x_RxGainTableChecksumRead},
    {"adi_adrv903x_RxGainTableChecksumCalculate", (void**) &adi_adrv903x_RxGainTableChecksumCalculate},
    {"adi_adrv903x_InitDataExtract", (void**) &adi_adrv903x_InitDataExtract},

};

ADI_API int adi_adrv903x_VersionLoad(const char* const soPath)
{
    return adi_adrv903x_VersionLoad_vLogCtl(soPath, stderr);
}

ADI_API int adi_adrv903x_VersionLoad_vLogCtl(const char* const soPath, FILE* const logStream)
{
    void *soHandle = NULL;
    char* errStr = NULL;
    unsigned idx = 0;

    /* Open the .so. Use DEEPBIND to prevent calls to API 'private' fns being fixed
     * up to the incorrect library when loading multiple API libs. */
    soHandle = dlopen(soPath, RTLD_NOW | RTLD_DEEPBIND );
    if (!soHandle)
    {
        fprintf (stderr, "Failed to open '%s' (%s)\n", soPath, dlerror());
        return -1;
    }

    /* Exract and set the fn ptrs */
    for (idx = 0; idx < (sizeof(fnNamesAndDefaults) / sizeof(fnNamesAndDefaults[0])); idx++)
    {
        *fnNamesAndDefaults[idx].fnPtrPtr = dlsym(soHandle, fnNamesAndDefaults[idx].fnName);
        errStr = dlerror();
        if (errStr)
        {
            /* Fn does not exist in so - use null Impl */
            fprintf (logStream, "Shared lib does not implement '%s'\n", fnNamesAndDefaults[idx].fnName);
            
            /* Disable compiler message "ISO C forbids conversion of function pointer to object pointer type [-Werror=pedantic]" */
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wpedantic"
            *fnNamesAndDefaults[idx].fnPtrPtr = adi_adrv903x_NotImplemented;
            #pragma GCC diagnostic pop
        }
    }

    return 0;
}
