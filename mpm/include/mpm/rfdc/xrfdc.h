/******************************************************************************
*
* Copyright (C) 2017-2019 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xrfdc.h
* @addtogroup rfdc_v6_0
* @{
* @details
*
* The Xilinx� LogiCORE IP Zynq UltraScale+ RFSoC RF Data Converter IP core
* provides a configurable wrapper to allow the RF DAC and RF ADC blocks to be
* used in IP Integrator designs. Multiple tiles are available on each RFSoC
* and each tile can have a number of data converters (analog-to-digital (ADC)
* and digital-to-analog (DAC)). The RF ADCs can sample input frequencies up
* to 4 GHz at 4 GSPS with excellent noise spectral density. The RF DACs
* generate output carrier frequencies up to 4 GHz using the 2nd Nyquist zone
* with excellent noise spectral density at an update rate of 6.4 GSPS.
* The RF data converters also include power efficient digital down-converters
* (DDCs) and digital up-converters (DUCs) that include programmable interpolation
* and decimation, NCO and complex mixer. The DDCs and DUCs can also support
* dual-band operation.
* A maximum of 4 tiles are available on for DAC and ADC operations each. Each
* tile can have a maximum of 4 blocks/slices.
* This driver provides APIs to configure various functionalities. Similarly
* the driver provides APIs to read back configurations.
* Some of the features that the driver supports are:
* 1) Setting up and reading back fine mixer settings
* 2) Setting up and reading back coarse mixer settings
* 3) Reading back interpolation or decimation factors
* 4) Setting up and reading back QMC settings which include gain, phase etc
* 5) Setting up and reading back decoder mode settings
* 6) Setting up and reading back coarse delay settings
* All the APIs implemented in the driver provide appropriate range checks.
* An API has been provided for debug purpose which will dump all registers
* for a requested tile.
* Inline functions have also been provided to read back the parameters
* initially configured through the GUI.
*
* There are plans to add more features, e.g. Support for multi band, PLL
* configurations etc.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---    -------- -----------------------------------------------
* 1.0   sk     05/16/17 Initial release
* 2.0   sk     08/09/17 Fixed coarse Mixer configuration settings
*                       CR# 977266, 977872.
*                       Return error for Slice Event on 4G ADC Block.
*                       Corrected Interrupt Macro names and values.
*              08/16/17 Add support for SYSREF and PL event sources.
*              08/18/17 Add API to enable and disable FIFO.
*              08/23/17 Add API to configure Nyquist zone.
*              08/30/17 Add additional info to BlockStatus.
*              08/30/17 Add support for Coarse Mixer BYPASS mode.
*              08/31/17 Removed Tile Reset Assert and Deassert.
*              09/07/17 Add support for negative NCO freq.
*              09/15/17 Fixed NCO freq precision issue.
*              09/15/17 Fixed Immediate Event source issue and also
*                       updated the Immediate Macro value to 0.
* 2.1   sk     09/15/17 Remove Libmetal library dependency for MB.
*              09/18/17 Add API to clear the interrupts.
*       sk     09/21/17 Add __BAREMETAL__ compiler flag option
*                       for Baremetal.
*       sk     09/21/17 Add support for Over voltage and Over
*                       Range interrupts.
*       sk     09/22/17 Add s64 typedef for Linux.
*       sk     09/24/17 Fixed Get_Tile/BlockBaseAddr always
*                       giving ADC related address.
*       sk     09/25/17 Modified XRFdc_GetBlockStatus API to give
*                       correct information and also updates the
*                       description for Vector Param in intr handler
*                       Add API to get Output current and removed
*                       GetTermVoltage and GetOutputCurr inline functions.
* 2.2   sk     10/05/17 Fixed XRFdc_GetNoOfADCBlocks API for 4GSPS.
*                       Enable the decoder clock based on decoder mode.
*                       Add API to get the current FIFO status.
*                       Updated XRFdc_DumpRegs API for better readability
*                       of output register dump.
*                       Add support for 4GSPS CoarseMixer frequency.
*              10/11/17 Modify float types to double to increase precision.
*              10/12/17 Update BlockStatus API to give current status.
*                       In BYPASS mode, input datatype can be Real or IQ
*                       hence checked both while reading the mixer mode.
*              10/17/17 Fixed Set Threshold API Issue.
* 2.2   sk     10/18/17 Add support for FIFO and DATA overflow interrupt
* 2.3   sk     11/06/17 Fixed PhaseOffset truncation issue.
*                       Provide user configurability for FineMixerScale.
*              11/08/17 Return error for DAC R2C mode and ADC C2R mode.
*              11/10/17 Corrected FIFO and DATA Interrupt masks.
*              11/20/17 Fixed StartUp, Shutdown and Reset API for Tile_Id -1.
*              11/20/17 Remove unwanted ADC block checks in 4GSPS mode.
* 3.0   sk     12/11/17 Added DDC and DUC support.
*              12/13/17 Add CoarseMixMode field in Mixer_Settings structure.
*              12/15/17 Add support to switch calibration modes.
*              12/15/17 Add support for mixer frequencies > Fs/2 and < -Fs/2.
*       sg     13/01/18 Added PLL and external clock switch support
*                       Added API to get PLL lock status.
*                       Added API to get clock source.
*       sk     01/18/18 Add API to get driver version.
* 3.1   jm     01/24/18 Add Multi-tile sync support.
*       sk     01/25/18 Updated Set and Get Interpolation/Decimation factor
*                       API's to consider the actual factor value.
* 3.2   sk     02/02/18 Add API's to configure inverse-sinc.
*       sk     02/27/18 Add API's to configure Multiband.
*       sk     03/09/18 Update PLL structure in XRFdc_DynamicPLLConfig API.
*       sk     03/09/18 Update ADC and DAC datatypes in Mixer API and use
*                       input datatype for ADC in threshold and QMC APIs.
*       sk     03/09/18 Removed FIFO disable check in DDC and DUC APIs.
*       sk     03/09/18 Add support for Marker event source for DAC block.
*       jm     03/12/18 Fixed DAC latency calculation in MTS.
*       jm     03/12/18 Added support for reloading DTC scans.
*       jm     03/12/18 Add option to configure sysref capture after MTS.
*       sk     03/22/18 Updated PLL settings based on latest IP values.
* 4.0   sk     04/09/18 Added API to enable/disable the sysref.
*       sk     04/09/18 Updated max VCO to 13108MHz to support max DAC
*                       sample rate of 6.554MHz.
*       rk     04/17/18 Adjust calculated latency by sysref period, where doing
*                       so results in closer alignment to the target latency.
*       sk     04/17/18 Corrected Set/Get MixerSettings API description for
*                       FineMixerScale parameter.
*       sk     04/19/18 Enable VCO Auto selection while configuring the clock.
*       sk     04/24/18 Add API to get PLL Configurations.
*       sk     04/24/18 Add API to get the Link Coupling mode.
*       sk     04/28/18 Implement timeouts for PLL Lock, Startup and shutdown.
*       sk     05/30/18 Removed CalibrationMode check for DAC.
*       sk     06/05/18 Updated minimum Ref clock value to 102.40625MHz.
* 5.0   sk     06/25/18 Update DAC min sampling rate to 500MHz and also update
*                       VCO Range, PLL_DIVIDER and PLL_FPDIV ranges.
*                       Update PLL structure with calculated sampling rate.
*       sk     06/25/18 Add XRFdc_GetFabClkOutDiv() API to read fabric clk div.
*                       Add Inline APIs XRFdc_CheckBlockEnabled(),
*                       XRFdc_CheckTileEnabled().
*       sk     07/06/18 Add support to dump HSCOM regs in XRFdc_DumpRegs() API
*       sk     07/12/18 Fixed Multiband crossbar settings in C2C mode.
*       sk     07/19/18 Add MixerType member to MixerSettings structure and 
*                       Update Mixer Settings APIs to consider the MixerType
*                       variable.
*       sk     07/19/18 Add XRFdc_GetMultibandConfig() API to read Multiband
*                       configuration.
*       sk     07/20/18 Update the APIs to check the corresponding section
*                       (Digital/Analog)enable/disable.
*       sk     07/26/18 Fixed Doxygen, coverity warnings.
*       sk     08/03/18 Fixed MISRAC warnings.
*       sk     08/24/18 Move mixer related APIs to xrfdc_mixer.c file.
*                       Define asserts for Linux, Re-arranged XRFdc_RestartIPSM,
*                       XRFdc_CfgInitialize() and XRFdc_MultiBand()  APIs.
*                       Reorganize the code to improve readability and
*                       optimization.
*       mus    08/17/18 Removed structure paddings from XRFdc_Config structure.
*                       It has been done to have 1:1 mapping between
*                       XRFdc_Config structure and device tree property
*                       "param-list", over linux platform.
*       sk     09/24/18 Update powerup-state value based on PLL mode in
*                       XRFdc_DynamicPLLConfig() API.
*       sk     10/10/18 Check for DigitalPath enable in XRFdc_GetNyquistZone()
*                       and XRFdc_GetCalibrationMode() APIs for Multiband.
*       sk     10/13/18 Add support to read the REFCLKDIV param from design.
*                       Update XRFdc_SetPLLConfig() API to support range of
*                       REF_CLK_DIV values(1 to 4).
*                       Add XRFDC_MIXER_MODE_R2R option to support BYPASS mode
*                       for Real input.
* 5.1   cog    01/29/19 Replace structure reference ADC checks with
*                       function.
*       cog    01/29/19 Added XRFdc_SetDither() and XRFdc_GetDither() APIs.
*       cog    01/29/19 Rename DataType for mixer input to MixerInputDataType
*                       for readability.
*       cog    01/29/19 Refactoring of interpolation and decimation APIs and
*                       changed fabric rate for decimation X8 for non-high speed ADCs.
*       cog    01/29/19 New inline functions to determine max & min sampling rates.
* 6.0   cog    02/17/19 Added Inverse-Sinc Second Nyquist Zone Support
*       cog    02/17/19 Added new clock Distribution functionality.
*       cog    02/17/19 Refactored to improve delay balancing in clock
*                       distribution.
*       cog    02/17/19 Added delay calculation & metal log messages.
*       cog    02/17/19 Added Intratile clock settings.
*       cog    02/17/19 XRFdc_GetPLLConfig() now uses register values to get the
*                       PLL configuration for new IPs and is no longer static.
*       cog    02/17/19 Refactoring of interpolation and decimation APIs and
*                       changed fabric rate for decimation X8 for non-high speed ADCs.
*       cog    02/17/19 Added XRFdc_SetIMRPassMode() and XRFdc_SetIMRPassMode() APIs
*       cog    02/17/19 Added XRFdc_SetDACMode() and XRFdc_GetDACMode() APIs
*       cog    02/17/19	Added XRFdc_SetSignalDetector() and XRFdc_GetSignalDetector() APIs
*       cog    02/17/19 Added XRFdc_DisableCoefficientsOverride(), XRFdc_SetCalCoefficients
*                       and XRFdc_GetCalCoefficients APIs.
*       cog    02/19/19 New definitions for clock detection.
* 6.0   cog    02/20/19	Added handling for new ADC common mode over/under
*                       voltage interrupts.
*       cog    02/20/19	XRFdc_GetIntrStatus now populates a pointer with the
*                       status and returns an error code.
*       cog	   02/20/19	XRFdc_IntrClr, XRFdc_IntrDisable and XRFdc_IntrEnable
*                       now return error codes.
*       cog    02/21/19 Added XRFdc_SetCalFreeze() and XRFdc_GetCalFreeze() APIs
*       cog    04/15/19 Rename XRFdc_SetDACMode() and XRFdc_GetDACMode() APIs to
*                       XRFdc_SetDataPathMode() and XRFdc_GetDataPathMode() respectively.
*
* </pre>
*
******************************************************************************/


#ifndef RFDC_H_
#define RFDC_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/

#include "rfdc_throw.h"
#include <stdlib.h>
#include <stdint.h>

#ifdef __BAREMETAL__
#include "xil_assert.h"
#include "xdebug.h"
#include "sleep.h"
#endif
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/atomic.h>
#include <metal/io.h>
#include <metal/sleep.h>
#include "metal/alloc.h"
#include "xrfdc_hw.h"

/**************************** Type Definitions *******************************/
#define XRFdc_IsADC4GSPS(InstPtr) XRFdc_IsHighSpeedADC(InstPtr, 0)

#ifndef __BAREMETAL__
typedef __u32 u32;
typedef __u16 u16;
typedef __u8 u8;
typedef __s32 s32;
typedef __u64 u64;
typedef __s64 s64;
typedef __s8 s8;
#endif

/**
* The handler data type allows the user to define a callback function to
* respond to interrupt events in the system. This function is executed
* in interrupt context, so amount of processing should be minimized.
*
* @param	CallBackRef is the callback reference passed in by the upper
*		layer when setting the callback functions, and passed back to
*		the upper layer when the callback is invoked. Its type is
*		not important to the driver, so it is a void pointer.
* @param	Type indicates ADC/DAC.
* @param	Tile_Id indicates Tile number (0-3).
* @param	Block_Id indicates Block number (0-3).
* @param	StatusEvent indicates one or more interrupt occurred.
*/
typedef void (*XRFdc_StatusHandler) (void *CallBackRef, u32 Type, u32 Tile_Id,
				u32 Block_Id, u32 StatusEvent);
#ifndef __BAREMETAL__
#pragma pack(1)
#endif
/**
 * PLL settings.
 */
typedef struct {
	u32 Enabled;	/* PLL Enables status (not a setter) */
	double RefClkFreq;
	double SampleRate;
	u32 RefClkDivider;
	u32 FeedbackDivider;
	u32 OutputDivider;
	u32 FractionalMode;	/* Fractional mode is currently not supported */
	u64 FractionalData;	/* Fractional data is currently not supported */
	u32 FractWidth;	/* Fractional width is currently not supported */
} XRFdc_PLL_Settings;
/**
* ClkIntraTile Settings.
*/
typedef struct {
	u8 SourceTile;
	u8 PLLEnable;
	XRFdc_PLL_Settings PLLSettings;
	u8 DivisionFactor;
	u8 Delay;
	u8 DistributedClock;
} XRFdc_Tile_Clock_Settings;
/**
* Clk Distribution.
*/
typedef struct {
	u8 Enabled;
	u8 DistributionSource;
	u8 UpperBound;
	u8 LowerBound;
	u8 MaxDelay;
	u8 MinDelay;
	u8 IsDelayBalanced;
} XRFdc_Distribution;
/**
* Clk Distribution Settings.
*/
typedef struct {
	XRFdc_Tile_Clock_Settings DAC[4];
	XRFdc_Tile_Clock_Settings ADC[4];
	XRFdc_Distribution DistributionStatus[8];
} XRFdc_Distribution_Settings;
#ifndef __BAREMETAL__
#pragma pack()
#endif

/**
 * ADC Signal Detect Settings.
 */
typedef struct {
	u8 Mode;
	u8 TimeConstant;
	u8 Flush;
	u8 EnableIntegrator;
	u16 HighThreshold;
	u16 LowThreshold;
	u8  HysteresisEnable;
}XRFdc_Signal_Detector_Settings;
/**
 * QMC settings.
 */
typedef struct {
	u32 EnablePhase;
	u32 EnableGain;
	double GainCorrectionFactor;
	double PhaseCorrectionFactor;
	s32 OffsetCorrectionFactor;
	u32 EventSource;
} XRFdc_QMC_Settings;

/**
 * Coarse delay settings.
 */
typedef struct {
	u32 CoarseDelay;
	u32 EventSource;
} XRFdc_CoarseDelay_Settings;

/**
 * Mixer settings.
 */
typedef struct {
	double Freq;
	double PhaseOffset;
	u32 EventSource;
	u32 CoarseMixFreq;
	u32 MixerMode;
	u8 FineMixerScale;	/* NCO output scale, valid values 0,1 and 2 */
	u8 MixerType;
} XRFdc_Mixer_Settings;

/**
 * ADC block Threshold settings.
 */
typedef struct {
	u32 UpdateThreshold; /* Selects which threshold to update */
	u32 ThresholdMode[2]; /* Entry 0 for Threshold0 and 1 for Threshold1 */
	u32 ThresholdAvgVal[2]; /* Entry 0 for Threshold0 and 1 for Threshold1 */
	u32 ThresholdUnderVal[2]; /* Entry 0 for Threshold0 and 1 for Threshold1 */
	u32 ThresholdOverVal[2]; /* Entry 0 is for Threshold0 and 1 for Threshold1 */
} XRFdc_Threshold_Settings;

/**
 * RFSoC Calibration coefficients generic struct
 */
typedef struct {
	u32 Coeff0;
	u32 Coeff1;
	u32 Coeff2;
	u32 Coeff3;
	u32 Coeff4;
	u32 Coeff5;
	u32 Coeff6;
	u32 Coeff7;
} XRFdc_Calibration_Coefficients;

/**
 * RFSoC Calibration freeze settings struct
 */
typedef struct {
	u32 CalFrozen; /*Status indicates calibration freeze state*/
	u32 DisableFreezePin; /*Disable the calibration freeze pin*/
	u32 FreezeCalibration; /*Setter for freezing*/
} XRFdc_Cal_Freeze_Settings;

/**
 * RFSoC Tile status.
 */
typedef struct {
	u32 IsEnabled;	/* 1, if tile is enabled, 0 otherwise */
	u32 TileState;	/* Indicates Tile Current State */
	u8 BlockStatusMask; /* Bit mask for block status, 1 indicates block enable */
	u32 PowerUpState;
	u32 PLLState;
} XRFdc_TileStatus;

/**
 * RFSoC Data converter IP status.
 */
typedef struct {
	XRFdc_TileStatus DACTileStatus[4];
	XRFdc_TileStatus ADCTileStatus[4];
	u32 State;
} XRFdc_IPStatus;

/**
 * status of DAC or ADC blocks in the RFSoC Data converter.
 */
typedef struct {
	double SamplingFreq;
	u32 AnalogDataPathStatus;
	u32 DigitalDataPathStatus;
	u8 DataPathClocksStatus;	/* Indicates all required datapath
				clocks are enabled or not, 1 if all clocks enabled, 0 otherwise */
	u8 IsFIFOFlagsEnabled;	/* Indicates FIFO flags enabled or not,
				 1 if all flags enabled, 0 otherwise */
	u8 IsFIFOFlagsAsserted;	/* Indicates FIFO flags asserted or not,
				 1 if all flags asserted, 0 otherwise */
} XRFdc_BlockStatus;

#ifndef __BAREMETAL__
#pragma pack(1)
#endif
/**
 * DAC block Analog DataPath Config settings.
 */
typedef struct {
	u32 BlockAvailable;
	u32 InvSyncEnable;
	u32 MixMode;
	u32 DecoderMode;
} XRFdc_DACBlock_AnalogDataPath_Config;

/**
 * DAC block Digital DataPath Config settings.
 */
typedef struct {
	u32 MixerInputDataType;
	u32 DataWidth;
	u32 InterpolationMode;
	u32 FifoEnable;
	u32 AdderEnable;
	u32 MixerType;
} XRFdc_DACBlock_DigitalDataPath_Config;

/**
 * ADC block Analog DataPath Config settings.
 */
typedef struct {
	u32 BlockAvailable;
	u32 MixMode;
} XRFdc_ADCBlock_AnalogDataPath_Config;

/**
 * DAC block Digital DataPath Config settings.
 */
typedef struct {
	u32 MixerInputDataType;
	u32 DataWidth;
	u32 DecimationMode;
	u32 FifoEnable;
	u32 MixerType;
} XRFdc_ADCBlock_DigitalDataPath_Config;

/**
 * DAC Tile Config structure.
 */
typedef struct {
	u32 Enable;
	u32 PLLEnable;
	double SamplingRate;
	double RefClkFreq;
	double FabClkFreq;
	u32 FeedbackDiv;
	u32 OutputDiv;
	u32 RefClkDiv;
	u32 MultibandConfig;
	double MaxSampleRate;
	u32 NumSlices;
	XRFdc_DACBlock_AnalogDataPath_Config DACBlock_Analog_Config[4];
	XRFdc_DACBlock_DigitalDataPath_Config DACBlock_Digital_Config[4];
} XRFdc_DACTile_Config;

/**
 * ADC Tile Config Structure.
 */
typedef struct {
	u32 Enable;	/* Tile Enable status */
	u32 PLLEnable;	/* PLL enable Status */
	double SamplingRate;
	double RefClkFreq;
	double FabClkFreq;
	u32 FeedbackDiv;
	u32 OutputDiv;
	u32 RefClkDiv;
	u32 MultibandConfig;
	double MaxSampleRate;
	u32 NumSlices;
	XRFdc_ADCBlock_AnalogDataPath_Config ADCBlock_Analog_Config[4];
	XRFdc_ADCBlock_DigitalDataPath_Config ADCBlock_Digital_Config[4];
} XRFdc_ADCTile_Config;

/**
 * RFdc Config Structure.
 */
typedef struct {
	u32 DeviceId;
	metal_phys_addr_t BaseAddr;
	u32 ADCType;	/* ADC Type 4GSPS or 2GSPS*/
	u32 MasterADCTile;	/* ADC master Tile */
	u32 MasterDACTile;	/* DAC Master Tile */
	u32 ADCSysRefSource;
	u32 DACSysRefSource;
	u32 IPType;
	XRFdc_DACTile_Config DACTile_Config[4];
	XRFdc_ADCTile_Config ADCTile_Config[4];
} XRFdc_Config;
#ifndef __BAREMETAL__
#pragma pack()
#endif
/**
 * DAC Block Analog DataPath Structure.
 */
typedef struct {
	u32 Enabled;	/* DAC Analog Data Path Enable */
	u32 MixedMode;
	double TerminationVoltage;
	double OutputCurrent;
	u32 InverseSincFilterEnable;
	u32 DecoderMode;
	void *FuncHandler;
	u32 NyquistZone;
	u8 AnalogPathEnabled;
	u8 AnalogPathAvailable;
	XRFdc_QMC_Settings QMC_Settings;
	XRFdc_CoarseDelay_Settings CoarseDelay_Settings;
} XRFdc_DACBlock_AnalogDataPath;

/**
 * DAC Block Digital DataPath Structure.
 */
typedef struct {
	u32 MixerInputDataType;
	u32 DataWidth;
	int ConnectedIData;
	int ConnectedQData;
	u32 InterpolationFactor;
	u8 DigitalPathEnabled;
	u8 DigitalPathAvailable;
	XRFdc_Mixer_Settings Mixer_Settings;
} XRFdc_DACBlock_DigitalDataPath;

/**
 * ADC Block Analog DataPath Structure.
 */
typedef struct {
	u32 Enabled;	/* ADC Analog Data Path Enable */
	XRFdc_QMC_Settings QMC_Settings;
	XRFdc_CoarseDelay_Settings CoarseDelay_Settings;
	XRFdc_Threshold_Settings Threshold_Settings;
	u32 NyquistZone;
	u8 CalibrationMode;
	u8 AnalogPathEnabled;
	u8 AnalogPathAvailable;
} XRFdc_ADCBlock_AnalogDataPath;

/**
 * ADC Block Digital DataPath Structure.
 */
typedef struct {
	u32 MixerInputDataType;
	u32 DataWidth;
	u32 DecimationFactor;
	int ConnectedIData;
	int ConnectedQData;
	u8 DigitalPathEnabled;
	u8 DigitalPathAvailable;
	XRFdc_Mixer_Settings Mixer_Settings;
} XRFdc_ADCBlock_DigitalDataPath;

/**
 * DAC Tile Structure.
 */
typedef struct {
	u32 TileBaseAddr;	/* Tile  BaseAddress*/
	u32 NumOfDACBlocks;	/* Number of DAC block enabled */
	XRFdc_PLL_Settings PLL_Settings;
	u8 MultibandConfig;
	XRFdc_DACBlock_AnalogDataPath DACBlock_Analog_Datapath[4];
	XRFdc_DACBlock_DigitalDataPath DACBlock_Digital_Datapath[4];
} XRFdc_DAC_Tile;

/**
 * ADC Tile Structure.
 */
typedef struct {
	u32 TileBaseAddr;
	u32 NumOfADCBlocks;	/* Number of ADC block enabled */
	XRFdc_PLL_Settings PLL_Settings;
	u8 MultibandConfig;
	XRFdc_ADCBlock_AnalogDataPath ADCBlock_Analog_Datapath[4];
	XRFdc_ADCBlock_DigitalDataPath ADCBlock_Digital_Datapath[4];
} XRFdc_ADC_Tile;

/**
 * RFdc Structure.
 */
typedef struct {
	XRFdc_Config RFdc_Config;	/* Config Structure */
	u32 IsReady;
	u32 ADC4GSPS;
	metal_phys_addr_t BaseAddr;	/* BaseAddress */
	struct metal_io_region *io;	/* Libmetal IO structure */
	struct metal_device *device;	/* Libmetal device structure */
	XRFdc_DAC_Tile DAC_Tile[4];
	XRFdc_ADC_Tile ADC_Tile[4];
	XRFdc_StatusHandler StatusHandler;	/* Event handler function */
	void *CallBackRef;			/* Callback reference for event handler */
	u8 UpdateMixerScale;	/* Set to 1, if user overwrite mixer scale */
} XRFdc;

/***************** Macros (Inline Functions) Definitions *********************/

#    ifndef __BAREMETAL__
#        define Xil_AssertNonvoid(Expression) \
            {                                 \
                if (!(Expression)) {          \
                    rfdc_throw(#Expression);  \
                }                             \
            }
#        define Xil_AssertVoid(Expression)   \
            {                                \
                if (!(Expression)) {         \
                    rfdc_throw(#Expression); \
                }                            \
            }
#        define Xil_AssertVoidAlways()       \
            {                                \
                rfdc_throw("Assert false"); \
            }
#    endif

#define MAX(x,y)						(x>y)?x:y
#define MIN(x,y)						(x<y)?x:y
#define XRFDC_SUCCESS                     0U
#define XRFDC_FAILURE                     1U
#define XRFDC_COMPONENT_IS_READY     	0x11111111U
#define XRFDC_NUM_SLICES_HSADC			2
#define XRFDC_NUM_SLICES_LSADC			4
#ifndef __BAREMETAL__
#define XRFDC_PLATFORM_DEVICE_DIR		"/sys/bus/platform/devices/"
#define XRFDC_BUS_NAME					"platform"
#define XRFDC_SIGNATURE	"usp_rf_data_converter" /* String in RFDC node name */
#define XRFDC_CONFIG_DATA_PROPERTY		"param-list" /* device tree property */
#define XRFDC_COMPATIBLE_PROPERTY		"compatible" /* device tree property */
#define XRFDC_NUM_INSTANCES_PROPERTY	"num-insts" /* device tree property */
#define XRFDC_COMPATIBLE_STRING			"xlnx,usp-rf-data-converter-"
#define XRFDC_DEVICE_ID_SIZE			4U
#define XRFDC_NUM_INST_SIZE				4U
#define XRFDC_CONFIG_DATA_SIZE			sizeof(XRFdc_Config)
#endif
#define XRFDC_REGION_SIZE	0x40000U
#define XRFDC_DRP_BASE(type, tile) ((type) == XRFDC_ADC_TILE ?		\
			XRFDC_ADC_TILE_DRP_ADDR(tile) : XRFDC_DAC_TILE_DRP_ADDR(tile))

#define XRFDC_CTRL_STS_BASE(Type, Tile) ((Type) == XRFDC_ADC_TILE ?	\
	XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile) :		\
	XRFDC_DAC_TILE_CTRL_STATS_ADDR(Tile))

#define XRFDC_BLOCK_BASE(Type, Tile, Block) ((Type) == XRFDC_ADC_TILE ?	\
	(XRFDC_ADC_TILE_DRP_ADDR(Tile) + XRFDC_BLOCK_ADDR_OFFSET(Block)) : \
	(XRFDC_DAC_TILE_DRP_ADDR(Tile) + XRFDC_BLOCK_ADDR_OFFSET(Block)))

#define XRFDC_ADC_TILE				0U
#define XRFDC_DAC_TILE				1U
#define XRFDC_TILE_ID_MAX			0x3U
#define XRFDC_BLOCK_ID_MAX			0x3U
#define XRFDC_EVNT_SRC_IMMEDIATE	0x00000000U
#define XRFDC_EVNT_SRC_SLICE		0x00000001U
#define XRFDC_EVNT_SRC_TILE			0x00000002U
#define XRFDC_EVNT_SRC_SYSREF		0x00000003U
#define XRFDC_EVNT_SRC_MARKER		0x00000004U
#define XRFDC_EVNT_SRC_PL			0x00000005U
#define XRFDC_EVENT_MIXER			0x00000001U
#define XRFDC_EVENT_CRSE_DLY		0x00000002U
#define XRFDC_EVENT_QMC				0x00000004U
#define XRFDC_SELECT_ALL_TILES		-1
#define XRFDC_ADC_4GSPS				1U

#define XRFDC_CRSE_DLY_MAX		0x7U
#define XRFDC_CRSE_DLY_MAX_EXT	0x28U
#define XRFDC_NCO_FREQ_MULTIPLIER		((0x1LLU << 48U) - 2) /* 2^48 -2 */
#define XRFDC_NCO_FREQ_MIN_MULTIPLIER	(0x1LLU << 48U) /* 2^48 */
#define XRFDC_NCO_PHASE_MULTIPLIER		(1U << 17U) /* 2^17 */
#define XRFDC_QMC_PHASE_MULT			(1U << 11U) /* 2^11 */
#define XRFDC_QMC_GAIN_MULT				(1U << 14U) /* 2^14 */

#define XRFDC_DATA_TYPE_IQ			0x00000001U
#define XRFDC_DATA_TYPE_REAL		0x00000000U

#define XRFDC_TRSHD_OFF				0x0U
#define XRFDC_TRSHD_STICKY_OVER		0x00000001U
#define XRFDC_TRSHD_STICKY_UNDER	0x00000002U
#define XRFDC_TRSHD_HYSTERISIS		0x00000003U

/* Mixer modes */
#define XRFDC_MIXER_MODE_OFF				0x0U
#define XRFDC_MIXER_MODE_C2C				0x1U
#define XRFDC_MIXER_MODE_C2R				0x2U
#define XRFDC_MIXER_MODE_R2C				0x3U
#define XRFDC_MIXER_MODE_R2R				0x4U

#define XRFDC_I_IQ_COS_MINSIN	0x00000C00U
#define XRFDC_Q_IQ_SIN_COS		0x00001000U
#define XRFDC_EN_I_IQ			0x00000001U
#define XRFDC_EN_Q_IQ			0x00000004U

#define XRFDC_MIXER_TYPE_COARSE		0x1U
#define XRFDC_MIXER_TYPE_FINE		0x2U

#define XRFDC_MIXER_TYPE_OFF	0x3U

#define XRFDC_COARSE_MIX_OFF						0x0U
#define XRFDC_COARSE_MIX_SAMPLE_FREQ_BY_TWO			0x2U
#define XRFDC_COARSE_MIX_SAMPLE_FREQ_BY_FOUR		0x4U
#define XRFDC_COARSE_MIX_MIN_SAMPLE_FREQ_BY_FOUR	0x8U
#define XRFDC_COARSE_MIX_BYPASS							0x10U

#define XRFDC_COARSE_MIX_MODE_C2C_C2R	0x1U
#define XRFDC_COARSE_MIX_MODE_R2C	0x2U

#define XRFDC_CRSE_MIX_OFF					0x924U
#define XRFDC_CRSE_MIX_BYPASS				0x0U
#define XRFDC_CRSE_4GSPS_ODD_FSBYTWO		0x492U
#define XRFDC_CRSE_MIX_I_ODD_FSBYFOUR		0x2CBU
#define XRFDC_CRSE_MIX_Q_ODD_FSBYFOUR		0x659U
#define XRFDC_CRSE_MIX_I_Q_FSBYTWO			0x410U
#define XRFDC_CRSE_MIX_I_FSBYFOUR			0x298U
#define XRFDC_CRSE_MIX_Q_FSBYFOUR			0x688U
#define XRFDC_CRSE_MIX_I_MINFSBYFOUR		0x688U
#define XRFDC_CRSE_MIX_Q_MINFSBYFOUR		0x298U
#define XRFDC_CRSE_MIX_R_I_FSBYFOUR			0x8A0U
#define XRFDC_CRSE_MIX_R_Q_FSBYFOUR			0x70CU
#define XRFDC_CRSE_MIX_R_I_MINFSBYFOUR		0x8A0U
#define XRFDC_CRSE_MIX_R_Q_MINFSBYFOUR		0x31CU

#define XRFDC_MIXER_SCALE_AUTO				0x0U
#define XRFDC_MIXER_SCALE_1P0				0x1U
#define XRFDC_MIXER_SCALE_0P7				0x2U

#define XRFDC_MIXER_PHASE_OFFSET_UP_LIMIT	180
#define XRFDC_MIXER_PHASE_OFFSET_LOW_LIMIT	(-180)
#define XRFDC_UPDATE_THRESHOLD_0			0x1U
#define XRFDC_UPDATE_THRESHOLD_1			0x2U
#define XRFDC_UPDATE_THRESHOLD_BOTH			0x4U
#define XRFDC_THRESHOLD_CLRMD_MANUAL_CLR	0x1U
#define XRFDC_THRESHOLD_CLRMD_AUTO_CLR		0x2U
#define XRFDC_DECODER_MAX_SNR_MODE			0x1U
#define XRFDC_DECODER_MAX_LINEARITY_MODE	0x2U
#define XRFDC_OUTPUT_CURRENT_32MA			32U
#define XRFDC_OUTPUT_CURRENT_20MA			20U

#define XRFDC_ADC_MIXER_MODE_IQ		0x1U
#define XRFDC_DAC_MIXER_MODE_REAL	0x2U

#define XRFDC_ODD_NYQUIST_ZONE		0x1U
#define XRFDC_EVEN_NYQUIST_ZONE		0x2U

#define XRFDC_INTERP_DECIM_OFF		0x0U
#define XRFDC_INTERP_DECIM_1X		0x1U
#define XRFDC_INTERP_DECIM_2X		0x2U
#define XRFDC_INTERP_DECIM_3X		0x3U
#define XRFDC_INTERP_DECIM_4X		0x4U
#define XRFDC_INTERP_DECIM_5X		0x5U
#define XRFDC_INTERP_DECIM_6X		0x6U
#define XRFDC_INTERP_DECIM_8X		0x8U
#define XRFDC_INTERP_DECIM_10X		0xAU
#define XRFDC_INTERP_DECIM_12X		0xCU
#define XRFDC_INTERP_DECIM_16X		0x10U
#define XRFDC_INTERP_DECIM_20X		0x14U
#define XRFDC_INTERP_DECIM_24X		0x18U
#define XRFDC_INTERP_DECIM_40X		0x28U

#define XRFDC_FAB_CLK_DIV1		0x1U
#define XRFDC_FAB_CLK_DIV2		0x2U
#define XRFDC_FAB_CLK_DIV4		0x3U
#define XRFDC_FAB_CLK_DIV8		0x4U
#define XRFDC_FAB_CLK_DIV16		0x5U

#define XRFDC_CALIB_MODE1		0x1U
#define XRFDC_CALIB_MODE2		0x2U
#define XRFDC_TI_DCB_MODE1_4GSPS		0x00007800U
#define XRFDC_TI_DCB_MODE1_2GSPS		0x00005000U

/* PLL Configuration */
#define XRFDC_PLL_UNLOCKED		0x1U
#define XRFDC_PLL_LOCKED		0x2U

#define XRFDC_EXTERNAL_CLK		0x0U
#define XRFDC_INTERNAL_PLL_CLK		0x1U

#define PLL_FPDIV_MIN			13U
#define PLL_FPDIV_MAX			128U
#define PLL_DIVIDER_MIN			2U
#define PLL_DIVIDER_MAX			28U
#define VCO_RANGE_MIN			8500U
#define VCO_RANGE_MAX			13200U
#define XRFDC_PLL_LPF1_VAL		0x6U
#define XRFDC_PLL_CRS2_VAL		0x7008U
#define XRFDC_VCO_UPPER_BAND	0x0U
#define XRFDC_VCO_LOWER_BAND	0x1U
#define XRFDC_REF_CLK_DIV_1		0x1U
#define XRFDC_REF_CLK_DIV_2		0x2U
#define XRFDC_REF_CLK_DIV_3		0x3U
#define XRFDC_REF_CLK_DIV_4		0x4U

#define XRFDC_SINGLEBAND_MODE		0x1U
#define XRFDC_MULTIBAND_MODE_2X		0x2U
#define XRFDC_MULTIBAND_MODE_4X		0x4U

#define XRFDC_MB_DATATYPE_C2C		0x1U
#define XRFDC_MB_DATATYPE_R2C		0x2U
#define XRFDC_MB_DATATYPE_C2R		0x4U

#define XRFDC_SB_C2C_BLK0	0x82U
#define XRFDC_SB_C2C_BLK1	0x64U
#define XRFDC_SB_C2R		0x40U
#define XRFDC_MB_C2C_BLK0	0x5EU
#define XRFDC_MB_C2C_BLK1	0x5DU
#define XRFDC_MB_C2R_BLK0	0x5CU
#define XRFDC_MB_C2R_BLK1	0x0U

#define XRFDC_MIXER_MODE_BYPASS		0x2U

#define XRFDC_LINK_COUPLING_DC	0x0U
#define XRFDC_LINK_COUPLING_AC	0x1U

#define XRFDC_MB_MODE_SB				0x0U
#define XRFDC_MB_MODE_2X_BLK01			0x1U
#define XRFDC_MB_MODE_2X_BLK23			0x2U
#define XRFDC_MB_MODE_2X_BLK01_BLK23	0x3U
#define XRFDC_MB_MODE_4X				0x4U

#define XRFDC_MILLI		1000U
#define XRFDC_DAC_SAMPLING_MIN	500
#define XRFDC_DAC_SAMPLING_MAX	6554
#define XRFDC_ADC_4G_SAMPLING_MIN	1000
#define XRFDC_ADC_4G_SAMPLING_MAX	4116
#define XRFDC_ADC_2G_SAMPLING_MIN	500
#define XRFDC_ADC_2G_SAMPLING_MAX	2058
#define XRFDC_REFFREQ_MIN	102.40625
#define XRFDC_REFFREQ_MAX	614

#define XRFDC_DIGITALPATH_ENABLE	0x1U
#define XRFDC_ANALOGPATH_ENABLE		0x1U

#define XRFDC_BLK_ID0	0x0U
#define XRFDC_BLK_ID1	0x1U
#define XRFDC_BLK_ID2	0x2U
#define XRFDC_BLK_ID3	0x3U
#define XRFDC_BLK_ID4	0x4U

#define XRFDC_TILE_ID0	0x0U
#define XRFDC_TILE_ID1	0x1U
#define XRFDC_TILE_ID2	0x2U
#define XRFDC_TILE_ID3	0x3U
#define XRFDC_TILE_ID4	0x4U

#define XRFDC_NUM_OF_BLKS1		0x1U
#define XRFDC_NUM_OF_BLKS2		0x2U
#define XRFDC_NUM_OF_BLKS3		0x3U
#define XRFDC_NUM_OF_BLKS4		0x4U

#define XRFDC_NUM_OF_TILES1		0x1U
#define XRFDC_NUM_OF_TILES2		0x2U
#define XRFDC_NUM_OF_TILES3		0x3U
#define XRFDC_NUM_OF_TILES4		0x4U

#define XRFDC_SM_STATE0		0x0U
#define XRFDC_SM_STATE1		0x1U
#define XRFDC_SM_STATE15	0xFU

#define XRFDC_DECIM_4G_DATA_TYPE		0x3U
#define XRFDC_DECIM_2G_IQ_DATA_TYPE		0x2U

#define XRFDC_DAC_MAX_WR_FAB_RATE	16U
#define XRFDC_ADC_MAX_RD_FAB_RATE	8U

#define XRFDC_MIN_PHASE_CORR_FACTOR		-26.5
#define XRFDC_MAX_PHASE_CORR_FACTOR		26.5
#define XRFDC_MAX_GAIN_CORR_FACTOR		2.0
#define XRFDC_MIN_GAIN_CORR_FACTOR		0.0

#define XRFDC_FAB_RATE_8	8
#define XRFDC_FAB_RATE_4	4
#define XRFDC_FAB_RATE_2	2
#define XRFDC_FAB_RATE_1	1

#define XRFDC_HSCOM_PWR_STATS_PLL		0xFFC0U
#define XRFDC_HSCOM_PWR_STATS_EXTERNAL	0xF240U

#define XRFDC_CLK_DST_DAC0 0
#define XRFDC_CLK_DST_DAC1 1
#define XRFDC_CLK_DST_DAC2 2
#define XRFDC_CLK_DST_DAC3 3
#define XRFDC_CLK_DST_ADC0 4
#define XRFDC_CLK_DST_ADC1 5
#define XRFDC_CLK_DST_ADC2 6
#define XRFDC_CLK_DST_ADC3 7
#define XRFDC_CLK_DST_INVALID 0xFFU

#define XRFDC_CLK_DISTR_MUX4A_SRC_INT	0x0008U
#define XRFDC_CLK_DISTR_MUX4A_SRC_STH	0x0000U
#define XRFDC_CLK_DISTR_MUX6_SRC_OFF	0x0000U
#define XRFDC_CLK_DISTR_MUX6_SRC_INT	0x0100U
#define XRFDC_CLK_DISTR_MUX6_SRC_NTH	0x0080U
#define XRFDC_CLK_DISTR_MUX7_SRC_OFF	0x0000U
#define XRFDC_CLK_DISTR_MUX7_SRC_STH	0x0200U
#define XRFDC_CLK_DISTR_MUX7_SRC_INT	0x0400U
#define XRFDC_CLK_DISTR_MUX8_SRC_NTH	0x0000U
#define XRFDC_CLK_DISTR_MUX8_SRC_INT	0x8000U
#define XRFDC_CLK_DISTR_MUX9_SRC_NTH	0x4000U
#define XRFDC_CLK_DISTR_MUX9_SRC_INT	0x0000U
#define XRFDC_CLK_DISTR_MUX5A_SRC_PLL	0x0800U
#define XRFDC_CLK_DISTR_MUX5A_SRC_RX	0x0040U
#define XRFDC_CLK_DISTR_OFF		(XRFDC_CLK_DISTR_MUX4A_SRC_INT | \
					XRFDC_CLK_DISTR_MUX6_SRC_OFF | \
					XRFDC_CLK_DISTR_MUX7_SRC_OFF | \
					XRFDC_CLK_DISTR_MUX8_SRC_NTH | \
					XRFDC_CLK_DISTR_MUX9_SRC_INT)
#define XRFDC_CLK_DISTR_LEFTMOST_TILE		0x0000U
#define XRFDC_CLK_DISTR_CONT_LEFT_EVEN		0x8208U
#define  XRFDC_CLK_DISTR_CONT_LEFT_ODD		0x8200U
#define XRFDC_CLK_DISTR_RIGHTMOST_TILE		0x4008
#define XRFDC_CLK_DISTR_CONT_RIGHT_EVEN		0x4080
#define XRFDC_CLK_DISTR_CONT_RIGHT_HWL_ODD	0x4088

#define XRFDC_CLK_DISTR_MUX4A_SRC_CLR	0x0008U
#define XRFDC_CLK_DISTR_MUX6_SRC_CLR	0x0180U
#define XRFDC_CLK_DISTR_MUX7_SRC_CLR	0x0600U
#define XRFDC_CLK_DISTR_MUX8_SRC_CLR	0x8000U
#define XRFDC_CLK_DISTR_MUX9_SRC_CLR	0x4000U

#define XRFDC_DIST_MAX 8

#define XRFDC_NET_CTRL_CLK_REC_PLL			0x1U
#define XRFDC_NET_CTRL_CLK_REC_DIST_T1		0x2U
#define XRFDC_NET_CTRL_CLK_T1_SRC_LOCAL		0x4U
#define XRFDC_NET_CTRL_CLK_T1_SRC_DIST		0x8U
#define XRFDC_NET_CTRL_CLK_INPUT_DIST		0x20U
#define XRFDC_DIST_CTRL_TO_PLL_DIV		0x10U
#define XRFDC_DIST_CTRL_TO_T1			0x20U
#define XRFDC_DIST_CTRL_DIST_SRC_LOCAL		0x40U
#define XRFDC_DIST_CTRL_DIST_SRC_PLL		0x800U
#define XRFDC_DIST_CTRL_CLK_T1_SRC_LOCAL	0x1000U
#define XRFDC_PLLREFDIV_INPUT_OFF		0x20U
#define XRFDC_PLLREFDIV_INPUT_DIST		0x40U
#define XRFDC_PLLREFDIV_INPUT_FABRIC	0x60U
#define XRFDC_PLLOPDIV_INPUT_DIST_LOCAL		0x800U

#define XRFDC_TILE_SOURCE_RX 0U
#define XRFDC_TILE_SOURCE_DIST 1U
#define XRFDC_TILE_SOURCE_FABRIC 2U

#define XRFDC_DIST_OUT_NONE 0U
#define XRFDC_DIST_OUT_RX 1U
#define XRFDC_DIST_OUT_OUTDIV 2U

#define XRFDC_PLL_SOURCE_NONE 0U
#define XRFDC_PLL_SOURCE_RX 1U
#define XRFDC_PLL_SOURCE_OUTDIV 2U

#define XRFDC_PLL_OUTDIV_MODE_1 0x0U
#define XRFDC_PLL_OUTDIV_MODE_2 0x1U
#define XRFDC_PLL_OUTDIV_MODE_3 0x2U
#define XRFDC_PLL_OUTDIV_MODE_N 0x3U

#define XRFDC_PLL_OUTDIV_MODE_3_VAL 0x1U

#define XRFDC_DIVISION_FACTOR_MIN 1

#define XRFDC_DITH_ENABLE 1
#define XRFDC_DITH_DISABLE 0

#define XRFDC_SIGDET_MODE_AVG  	0
#define XRFDC_SIGDET_MODE_RNDM 	1
#define XRFDC_SIGDET_TC_2_0		0
#define XRFDC_SIGDET_TC_2_2		1
#define XRFDC_SIGDET_TC_2_4		2
#define XRFDC_SIGDET_TC_2_8		3
#define XRFDC_SIGDET_TC_2_12	4
#define XRFDC_SIGDET_TC_2_14	5
#define XRFDC_SIGDET_TC_2_16	6
#define XRFDC_SIGDET_TC_2_18	7

#define XRFDC_DISABLED 0
#define XRFDC_ENABLED 1

#define XRFDC_CAL_BLOCK_OCB1 	0
#define XRFDC_CAL_BLOCK_OCB2	1
#define XRFDC_CAL_BLOCK_GCB		2
#define XRFDC_CAL_BLOCK_TSCB	3

#define XRFDC_INV_SYNC_MODE_MAX 2

#define XRFDC_INV_SYNC_EN_MAX 1


#define XRFDC_CTRL_MASK				0x4800
#define XRFDC_EXPORTCTRL_CLKDIST	0x4000
#define XRFDC_PREMIUMCTRL_CLKDIST	0x0800

#define XRFDC_DAC_MODE_7G_NQ1		0U
#define XRFDC_DAC_MODE_7G_NQ2		1U
#define XRFDC_DAC_MODE_10G_IMR		2U
#define XRFDC_DAC_MODE_10G_BYPASS 	3U
#define XRFDC_DAC_MODE_MAX 			XRFDC_DAC_MODE_10G_BYPASS

#define XRFDC_DAC_IMR_MODE_LOWPASS	0U
#define XRFDC_DAC_IMR_MODE_HIGHPASS	1U
#define XRFDC_DAC_IMR_MODE_MAX		XRFDC_DAC_IMR_MODE_HIGHPASS

#define XRFDC_CLOCK_DETECT_CLK		0x1U
#define XRFDC_CLOCK_DETECT_DIST		0x2U
#define XRFDC_CLOCK_DETECT_BOTH		0x3U

#define XRFDC_CAL_UNFREEZE_CALIB	0U
#define XRFDC_CAL_FREEZE_CALIB		1U
#define XRFDC_CAL_FRZ_PIN_ENABLE	0U
#define XRFDC_CAL_FRZ_PIN_DISABLE	1U
/*****************************************************************************/
/**
*
* Execute Read modify Write
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	BaseAddr is address of a block.
* @param	RegAddr is register offset value.
* @param	Mask contains bit mask value.
* @param	Data contains value to be written to register.
*
* @return
*		- None
*
******************************************************************************/
static inline void XRFdc_ClrSetReg(XRFdc *InstancePtr, u32 BaseAddr,
						u32 RegAddr, u16 Mask, u16 Data)
{
	u16 ReadReg;

	ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr, RegAddr);
	ReadReg = (ReadReg & ~Mask) | (Data & Mask);
	XRFdc_WriteReg16(InstancePtr, BaseAddr, RegAddr, ReadReg);
}

/*****************************************************************************/
/**
*
* Execute Read and clear
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	BaseAddr is address of a block.
* @param	RegAddr is register offset value.
* @param	Mask contains bit mask value.
*
* @return
*		- None
*
******************************************************************************/
static inline void XRFdc_ClrReg(XRFdc *InstancePtr, u32 BaseAddr,
						u32 RegAddr, u16 Mask)
{
	u16 ReadReg;

	ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr, RegAddr);
	ReadReg &= ~Mask;
	XRFdc_WriteReg16(InstancePtr, BaseAddr, RegAddr, ReadReg);
}

/*****************************************************************************/
/**
*
* Execute Read and mask with the value
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	BaseAddr is address of a block.
* @param	RegAddr is register offset value.
* @param	Mask contains bit mask value.
*
* @return
*		- None
*
******************************************************************************/
static inline u16 XRFdc_RDReg(XRFdc *InstancePtr, u32 BaseAddr,
						u32 RegAddr, u16 Mask)
{
	u16 ReadReg;

	ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr, RegAddr);
	ReadReg &= Mask;

	return ReadReg;
}

/*****************************************************************************/
/**
*
* Checks whether DAC block is available or not.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- Return 1 if DAC block is available, otherwise 0.
*
******************************************************************************/
static inline u32 XRFdc_IsDACBlockEnabled(XRFdc *InstancePtr, u32 Tile_Id,
												u32 Block_Id)
{
	return InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].
				DACBlock_Analog_Config[Block_Id].BlockAvailable;
}

/*****************************************************************************/
/**
*
* Checks whether ADC block is available or not.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3 in DAC/ADC-2GSPS and 0-1 in ADC-4GSPS.
*
* @return
*		- Return 1 if ADC block is available, otherwise 0.
*
******************************************************************************/
static inline u32 XRFdc_IsADCBlockEnabled(XRFdc *InstancePtr, u32 Tile_Id,
												u32 Block_Id)
{
	u32 IsBlockAvail;

	if (InstancePtr->RFdc_Config.ADCType == XRFDC_ADC_4GSPS) {
		if ((Block_Id == 2U) || (Block_Id == 3U)) {
			IsBlockAvail = 0;
			goto RETURN_PATH;
		}
		if (Block_Id == 1U) {
			Block_Id = 2U;
		}
	}
	IsBlockAvail = InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].
				ADCBlock_Analog_Config[Block_Id].BlockAvailable;
RETURN_PATH:
	return IsBlockAvail;
}

/*****************************************************************************/
/**
*
* Checks whether DAC Digital path is enabled or not.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- Return 1 if DAC digital path is enabled, otherwise 0.
*
******************************************************************************/
static inline u32 XRFdc_IsDACDigitalPathEnabled(XRFdc *InstancePtr,
				u32 Tile_Id, u32 Block_Id)
{
	u32 Status;

	if (InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[Block_Id].
				Mixer_Settings.MixerType == XRFDC_MIXER_TYPE_OFF) {
		Status = 0U;
	} else {
		Status = 1U;
	}

	return Status;
}

/*****************************************************************************/
/**
*
* Checks whether ADC digital path is enabled or not.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3 in DAC/ADC-2GSPS and 0-1 in ADC-4GSPS.
*
* @return
*		- Return 1 if ADC digital path is enabled, otherwise 0.
*
******************************************************************************/
static inline u32 XRFdc_IsADCDigitalPathEnabled(XRFdc *InstancePtr,
							u32 Tile_Id, u32 Block_Id)
{
	u32 IsBlockAvail;

	if (InstancePtr->RFdc_Config.ADCType == XRFDC_ADC_4GSPS) {
		if ((Block_Id == 2U) || (Block_Id == 3U)) {
			IsBlockAvail = 0;
			goto RETURN_PATH;
		}
		if (Block_Id == 1U) {
			Block_Id = 2U;
		}
	}

	if (InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Block_Id].
				Mixer_Settings.MixerType == XRFDC_MIXER_TYPE_OFF) {
		IsBlockAvail = 0;
	} else {
		IsBlockAvail = 1;
	}

RETURN_PATH:
	return IsBlockAvail;
}

/*****************************************************************************/
/**
*
* Checks whether ADC/DAC Digital path is enabled or not.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param    Type is ADC or DAC. 0 for ADC and 1 for DAC.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- XRFDC_SUCCESS if Digital path is enabled.
*       - XRFDC_FAILURE if Digital path is not enabled.
*
******************************************************************************/
static inline u32 XRFdc_CheckDigitalPathEnabled(XRFdc *InstancePtr, u32 Type,
			u32 Tile_Id, u32 Block_Id)
{
	u32 IsBlockAvail;
	u32 Status;

	if ((Type != XRFDC_ADC_TILE) && (Type != XRFDC_DAC_TILE)) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if ((Tile_Id > XRFDC_TILE_ID_MAX) || (Block_Id > XRFDC_BLOCK_ID_MAX)) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (Type == XRFDC_ADC_TILE) {
		IsBlockAvail = XRFdc_IsADCDigitalPathEnabled(InstancePtr, Tile_Id,
						Block_Id);
	} else {
		IsBlockAvail = XRFdc_IsDACDigitalPathEnabled(InstancePtr, Tile_Id,
						Block_Id);
	}
	if (IsBlockAvail == 0U) {
		Status = XRFDC_FAILURE;
	} else {
		Status = XRFDC_SUCCESS;
	}
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Get IP BaseAddress.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
*
* @return
*		- Return IP BaseAddress.
*
******************************************************************************/
static inline u32 XRFdc_Get_IPBaseAddr(XRFdc *InstancePtr)
{
	return (u32)InstancePtr->BaseAddr;
}

/*****************************************************************************/
/**
*
* Get Tile BaseAddress
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
*
* @return
*		- Return Tile BaseAddress.
*
******************************************************************************/
static inline u32 XRFdc_Get_TileBaseAddr(XRFdc *InstancePtr, u32 Type,
								u32 Tile_Id)
{
	u32 BaseAddr;

	if (Type == XRFDC_ADC_TILE) {
		BaseAddr = InstancePtr->BaseAddr + XRFDC_ADC_TILE_DRP_ADDR(Tile_Id);
	} else {
		BaseAddr = InstancePtr->BaseAddr + XRFDC_DAC_TILE_DRP_ADDR(Tile_Id);
	}

	return BaseAddr;
}

/*****************************************************************************/
/**
*
* Get Block BaseAddress
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3 in DAC/ADC-2GSPS and 0-1 in ADC-4GSPS.
*
* @return
*		- Return Block BaseAddress.
*
******************************************************************************/
static inline u32 XRFdc_Get_BlockBaseAddr(XRFdc *InstancePtr, u32 Type,
								u32 Tile_Id, u32 Block_Id)
{
	u32 BaseAddr;

	if (Type == XRFDC_ADC_TILE) {
		if (InstancePtr->RFdc_Config.ADCType == XRFDC_ADC_4GSPS) {
			if (Block_Id == 1U) {
				Block_Id = 2U;
			}
		}
		BaseAddr = InstancePtr->BaseAddr + XRFDC_ADC_TILE_DRP_ADDR(Tile_Id) +
								XRFDC_BLOCK_ADDR_OFFSET(Block_Id);
	} else {
		BaseAddr = InstancePtr->BaseAddr + XRFDC_DAC_TILE_DRP_ADDR(Tile_Id) +
								XRFDC_BLOCK_ADDR_OFFSET(Block_Id);
	}

	return BaseAddr;
}

/*****************************************************************************/
/**
*
* Get Number of DAC Blocks enabled.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
*
* @return
*		- Return number of DAC blocks enabled.
*
******************************************************************************/
static inline u32 XRFdc_GetNoOfDACBlock(XRFdc *InstancePtr, u32 Tile_Id)
{
	return InstancePtr->DAC_Tile[Tile_Id].NumOfDACBlocks;
}

/*****************************************************************************/
/**
*
* Get Number of ADC Blocks enabled.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
*
* @return
*		- Return number of ADC blocks enabled.
*
******************************************************************************/
static inline u32 XRFdc_GetNoOfADCBlocks(XRFdc *InstancePtr, u32 Tile_Id)
{
	return InstancePtr->ADC_Tile[Tile_Id].NumOfADCBlocks;
}

/*****************************************************************************/
/**
*
* Get ADC type is High Speed or Medium Speed.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
*
* @return
*		- Return 1 if ADC type is 4GSPS, otherwise 0.
*
******************************************************************************/

static inline u32 XRFdc_IsHighSpeedADC(XRFdc *InstancePtr, int Tile)
{
	if (InstancePtr->RFdc_Config.ADCTile_Config[Tile].NumSlices == 0) {
		return InstancePtr->ADC4GSPS;
	} else {
		return (InstancePtr->RFdc_Config.ADCTile_Config[Tile].NumSlices == XRFDC_NUM_SLICES_HSADC);
	}
}

/*****************************************************************************/
/**
*
* Get Mixer Input Data Type for ADC/DAC block.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- Return MixerInputDataType of ADC/DAC block.
*
******************************************************************************/
static inline u32 XRFdc_GetDataType(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 Block_Id)
{
	u32 MixerInputDataType;

	if (Type == XRFDC_ADC_TILE) {
		MixerInputDataType = InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].
				ADCBlock_Digital_Config[Block_Id].MixerInputDataType;
	} else {
		MixerInputDataType = InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].
				DACBlock_Digital_Config[Block_Id].MixerInputDataType;
	}

	return MixerInputDataType;
}

/*****************************************************************************/
/**
*
* Get Data Width for ADC/DAC block.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- Return DataWidth of ADC/DAC block.
*
******************************************************************************/
static inline u32 XRFdc_GetDataWidth(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 Block_Id)
{
	u32 DataWidth;

	if (Type == XRFDC_ADC_TILE) {
		DataWidth = InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].
				ADCBlock_Digital_Config[Block_Id].DataWidth;
	} else {
		DataWidth = InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].
				DACBlock_Digital_Config[Block_Id].DataWidth;
	}

	return DataWidth;
}

/*****************************************************************************/
/**
*
* Get Inversesync filter for DAC block.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- Return Inversesync filter for DAC block
*
******************************************************************************/
static inline u32 XRFdc_GetInverseSincFilter(XRFdc *InstancePtr, u32 Tile_Id,
								u32 Block_Id)
{
	return InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].
				DACBlock_Analog_Config[Block_Id].InvSyncEnable;
}

/*****************************************************************************/
/**
*
* Get Mixed mode for DAC block.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- Return mixed mode for DAC block
*
******************************************************************************/
static inline u32 XRFdc_GetMixedMode(XRFdc *InstancePtr, u32 Tile_Id,
								u32 Block_Id)
{
	return InstancePtr->DAC_Tile[Tile_Id].
				DACBlock_Analog_Datapath[Block_Id].MixedMode;
}

/*****************************************************************************/
/**
*
* Get Master Tile for ADC/DAC tiles.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
*
* @return
*		- Return Master Tile for ADC/DAC tiles
*
******************************************************************************/
static inline u32 XRFdc_GetMasterTile(XRFdc *InstancePtr, u32 Type)
{
	u32 MasterTile;

	if (Type == XRFDC_ADC_TILE) {
		MasterTile = InstancePtr->RFdc_Config.MasterADCTile;
	} else {
		MasterTile = InstancePtr->RFdc_Config.MasterDACTile;
	}

	return MasterTile;
}

/*****************************************************************************/
/**
*
* Get Sysref source for ADC/DAC tile.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
*
* @return
*		- Return Sysref source for ADC/DAC tile
*
******************************************************************************/
static inline u32 XRFdc_GetSysRefSource(XRFdc *InstancePtr, u32 Type)
{
	u32 SysRefSource;

	if (Type == XRFDC_ADC_TILE) {
		SysRefSource = InstancePtr->RFdc_Config.ADCSysRefSource;
	} else {
		SysRefSource = InstancePtr->RFdc_Config.DACSysRefSource;
	}

	return SysRefSource;
}

/*****************************************************************************/
/**
*
* Get Fabric Clock frequency.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
*
* @return
*		- Return Fabric Clock frequency for ADC/DAC tile
*
******************************************************************************/
static inline double XRFdc_GetFabClkFreq(XRFdc *InstancePtr, u32 Type,
								u32 Tile_Id)
{
	double FabClkFreq;

	if (Type == XRFDC_ADC_TILE) {
		FabClkFreq = InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].FabClkFreq;
	} else {
		FabClkFreq = InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].FabClkFreq;
	}

	return FabClkFreq;
}

/*****************************************************************************/
/**
*
* Get whether FIFO is enabled or not.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- Return 1 if FIFO is enabled, otherwise 0.
*
******************************************************************************/
static inline u32 XRFdc_IsFifoEnabled(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 Block_Id)
{
	u32 FifoEnable;

	if (Type == XRFDC_ADC_TILE) {
		FifoEnable = InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].
				ADCBlock_Digital_Config[Block_Id].FifoEnable;
	} else {
		FifoEnable = InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].
				DACBlock_Digital_Config[Block_Id].FifoEnable;
	}

	return FifoEnable;
}

/*****************************************************************************/
/**
*
* Get Data Converter connected for digital data path I
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is Digital Data Path number.
*
* @return
*		- Return Data converter Id.
*
******************************************************************************/
static inline int XRFdc_GetConnectedIData(XRFdc *InstancePtr, u32 Type,
				 u32 Tile_Id, u32 Block_Id)
{
	int ConnectedIData;

	if (Type == XRFDC_ADC_TILE) {
		ConnectedIData = InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Block_Id].ConnectedIData;
	} else {
		ConnectedIData = InstancePtr->DAC_Tile[Tile_Id].
				DACBlock_Digital_Datapath[Block_Id].ConnectedIData;
	}

	return ConnectedIData;
}

/*****************************************************************************/
/**
*
* Get Data Converter connected for digital data path Q
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is Digital Data Path number.
*
* @return
*		- Return Data converter Id.
*
******************************************************************************/
static inline int XRFdc_GetConnectedQData(XRFdc *InstancePtr, u32 Type,
				 u32 Tile_Id, u32 Block_Id)
{
	int ConnectedQData;

	if (Type == XRFDC_ADC_TILE) {
		ConnectedQData = InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Block_Id].ConnectedQData;
	} else {
		ConnectedQData = InstancePtr->DAC_Tile[Tile_Id].
				DACBlock_Digital_Datapath[Block_Id].ConnectedQData;
	}

	return ConnectedQData;
}

/*****************************************************************************/
/**
*
* Set Data Converter connected for digital data path I and Q
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is Digital Data Path number.
* @param	ConnectedIData is Converter Id to which DigitalPathI connected.
* @param	ConnectedQData is Converter Id to which DigitalPathQ connected.
*
* @return
*		- None.
*
******************************************************************************/
static inline void XRFdc_SetConnectedIQData(XRFdc *InstancePtr, u32 Type,
			u32 Tile_Id, u32 Block_Id, int ConnectedIData, int ConnectedQData)
{
	if (Type == XRFDC_ADC_TILE) {
		InstancePtr->ADC_Tile[Tile_Id].
			ADCBlock_Digital_Datapath[Block_Id].ConnectedIData = ConnectedIData;
		InstancePtr->ADC_Tile[Tile_Id].
			ADCBlock_Digital_Datapath[Block_Id].ConnectedQData = ConnectedQData;
	} else {
		InstancePtr->DAC_Tile[Tile_Id].
			DACBlock_Digital_Datapath[Block_Id].ConnectedIData = ConnectedIData;
		InstancePtr->DAC_Tile[Tile_Id].
			DACBlock_Digital_Datapath[Block_Id].ConnectedQData = ConnectedQData;
	}
}

/*****************************************************************************/
/**
*
* Get Multiband Config data
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
*
* @return
*		- Return Multiband Configuration.
*
******************************************************************************/
static inline u32 XRFdc_GetMultibandConfig(XRFdc *InstancePtr, u32 Type,
				 u32 Tile_Id)
{
	u32 MultibandConfig;

	if (Type == XRFDC_ADC_TILE) {
		MultibandConfig = InstancePtr->ADC_Tile[Tile_Id].MultibandConfig;
	} else {
		MultibandConfig = InstancePtr->DAC_Tile[Tile_Id].MultibandConfig;
	}

	return MultibandConfig;
}

/*****************************************************************************/
/**
*
* Checks whether ADC/DAC block is enabled or not.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param    Type is ADC or DAC. 0 for ADC and 1 for DAC.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- XRFDC_SUCCESS if block enabled.
*       - XRFDC_FAILURE if Block not enabled.
*
******************************************************************************/
static inline u32 XRFdc_CheckBlockEnabled(XRFdc *InstancePtr, u32 Type,
			u32 Tile_Id, u32 Block_Id)
{
	u32 IsBlockAvail;
	u32 Status;

	if ((Type != XRFDC_ADC_TILE) && (Type != XRFDC_DAC_TILE)) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if ((Tile_Id > XRFDC_TILE_ID_MAX) || (Block_Id > XRFDC_BLOCK_ID_MAX)) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (Type == XRFDC_ADC_TILE) {
		IsBlockAvail = XRFdc_IsADCBlockEnabled(InstancePtr, Tile_Id, Block_Id);
	} else {
		IsBlockAvail = XRFdc_IsDACBlockEnabled(InstancePtr, Tile_Id, Block_Id);
	}
	if (IsBlockAvail == 0U) {
		Status = XRFDC_FAILURE;
	} else {
		Status = XRFDC_SUCCESS;
	}
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Checks whether ADC/DAC tile is enabled or not.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param    Type is ADC or DAC. 0 for ADC and 1 for DAC.
* @param	Tile_Id Valid values are 0-3.
*
* @return
*		- XRFDC_SUCCESS if tile enabled.
*       - XRFDC_FAILURE if tile not enabled.
*
******************************************************************************/
static inline u32 XRFdc_CheckTileEnabled(XRFdc *InstancePtr, u32 Type,
								u32 Tile_Id)
{
	u32 IsTileAvail;
	u32 Status;

	if ((Type != XRFDC_ADC_TILE) && (Type != XRFDC_DAC_TILE)) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (Tile_Id > XRFDC_TILE_ID_MAX) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (Type == XRFDC_ADC_TILE) {
		IsTileAvail = InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].Enable;
	} else {
		IsTileAvail = InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].Enable;
	}
	if (IsTileAvail == 0U) {
		Status = XRFDC_FAILURE;
	} else {
		Status = XRFDC_SUCCESS;
	}
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* Gets ADC/DAC tile maximum sampling rate.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param    Type is ADC or DAC. 0 for ADC and 1 for DAC.
* @param	Tile_Id Valid values are 0-3.
* @param	MaxSampleRatePtr pointer for maximum sample rate.
*
* @return
*		- XRFDC_SUCCESS if found sampling rate.
*       - XRFDC_FAILURE if could not find sampling rate.
*
******************************************************************************/
static inline u32 XRFdc_GetMaxSampleRate(XRFdc *InstancePtr, u32 Type,
								u32 Tile_Id, double *MaxSampleRatePtr)
{
	u32 Status;

	if ((Type != XRFDC_ADC_TILE) && (Type != XRFDC_DAC_TILE)) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (Tile_Id > XRFDC_TILE_ID_MAX) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (Type == XRFDC_ADC_TILE) {
		*MaxSampleRatePtr = InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].MaxSampleRate*1000;
		if (*MaxSampleRatePtr == 0) {
			*MaxSampleRatePtr = XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id)?XRFDC_ADC_4G_SAMPLING_MAX:XRFDC_ADC_2G_SAMPLING_MAX;
		}
	} else {
		*MaxSampleRatePtr = InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].MaxSampleRate*1000;
		if (*MaxSampleRatePtr == 0) {
			*MaxSampleRatePtr = XRFDC_DAC_SAMPLING_MAX;
		}
	}
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* Gets ADC/DAC tile minimum sampling rate.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param    Type is ADC or DAC. 0 for ADC and 1 for DAC.
* @param	Tile_Id Valid values are 0-3.
* @param	MinSampleRatePtr pointer for minimum sample rate.
*
* @return
*		- XRFDC_SUCCESS if found sampling rate.
*       - XRFDC_FAILURE if could not find sampling rate.
*
******************************************************************************/
static inline u32 XRFdc_GetMinSampleRate(XRFdc *InstancePtr, u32 Type,
								u32 Tile_Id, double *MinSampleRatePtr)
{
	u32 Status;

	if ((Type != XRFDC_ADC_TILE) && (Type != XRFDC_DAC_TILE)) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (Tile_Id > XRFDC_TILE_ID_MAX) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (Type == XRFDC_ADC_TILE) {
		*MinSampleRatePtr = XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id)?XRFDC_ADC_4G_SAMPLING_MIN:XRFDC_ADC_2G_SAMPLING_MIN;
	} else {
		*MinSampleRatePtr = XRFDC_DAC_SAMPLING_MIN;
	}
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This API is used to get the driver version.
*
* @param	None
*
* @return
*		Driver version number
*
* @note		None
*
******************************************************************************/
static inline double XRFdc_GetDriverVersion(void)
{
	return 6.0;
}

/************************** Function Prototypes ******************************/

XRFdc_Config *XRFdc_LookupConfig(u16 DeviceId);
u32 XRFdc_CfgInitialize(XRFdc *InstancePtr, XRFdc_Config *ConfigPtr);
u32 XRFdc_StartUp(XRFdc *InstancePtr, u32 Type, int Tile_Id);
u32 XRFdc_Shutdown(XRFdc *InstancePtr, u32 Type, int Tile_Id);
u32 XRFdc_Reset(XRFdc *InstancePtr, u32 Type, int Tile_Id);
u32 XRFdc_GetIPStatus(XRFdc *InstancePtr, XRFdc_IPStatus *IPStatusPtr);
u32 XRFdc_GetBlockStatus(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 Block_Id, XRFdc_BlockStatus *BlockStatusPtr);
u32 XRFdc_SetMixerSettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
			u32 Block_Id, XRFdc_Mixer_Settings *MixerSettingsPtr);
u32 XRFdc_GetMixerSettings(XRFdc *InstancePtr, u32 Type,
				u32 Tile_Id, u32 Block_Id,
				XRFdc_Mixer_Settings *MixerSettingsPtr);
u32 XRFdc_SetQMCSettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 Block_Id, XRFdc_QMC_Settings *QMCSettingsPtr);
u32 XRFdc_GetQMCSettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 Block_Id, XRFdc_QMC_Settings *QMCSettingsPtr);
u32 XRFdc_GetCoarseDelaySettings(XRFdc *InstancePtr, u32 Type,
				u32 Tile_Id, u32 Block_Id,
				XRFdc_CoarseDelay_Settings *CoarseDelaySettingsPtr);
u32 XRFdc_SetCoarseDelaySettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
						u32 Block_Id, XRFdc_CoarseDelay_Settings *CoarseDelaySettingsPtr);
u32 XRFdc_GetInterpolationFactor(XRFdc *InstancePtr, u32 Tile_Id,
				u32 Block_Id, u32 *InterpolationFactorPtr);
u32 XRFdc_GetDecimationFactor(XRFdc *InstancePtr, u32 Tile_Id,
				u32 Block_Id, u32 *DecimationFactorPtr);
u32 XRFdc_GetFabWrVldWords(XRFdc *InstancePtr, u32 Type,
				u32 Tile_Id, u32 Block_Id, u32 *FabricDataRatePtr);
u32 XRFdc_GetFabRdVldWords(XRFdc *InstancePtr, u32 Type,
				u32 Tile_Id, u32 Block_Id, u32 *FabricDataRatePtr);
u32 XRFdc_SetFabRdVldWords(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
								u32 FabricRdVldWords);
u32 XRFdc_SetFabWrVldWords(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
								u32 FabricWrVldWords);
u32 XRFdc_GetThresholdSettings(XRFdc *InstancePtr, u32 Tile_Id,
				u32 Block_Id, XRFdc_Threshold_Settings *ThresholdSettingsPtr);
u32 XRFdc_SetThresholdSettings(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
				XRFdc_Threshold_Settings *ThresholdSettingsPtr);
u32 XRFdc_SetDecoderMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
				u32 DecoderMode);
u32 XRFdc_UpdateEvent(XRFdc *InstancePtr, u32 Type, u32 Tile_Id, u32 Block_Id,
				u32 Event);
u32 XRFdc_GetDecoderMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
				u32 *DecoderModePtr);
u32 XRFdc_ResetNCOPhase(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 Block_Id);
void XRFdc_DumpRegs(XRFdc *InstancePtr, u32 Type, int Tile_Id);
u32 XRFdc_MultiBand(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u8 DigitalDataPathMask, u32 MixerInOutDataType, u32 DataConverterMask);
u32 XRFdc_IntrHandler(u32 Vector, void *XRFdcPtr);
u32 XRFdc_IntrClr(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 Block_Id, u32 IntrMask);
u32 XRFdc_GetIntrStatus(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 Block_Id, u32 *IntrStsPtr);
u32 XRFdc_IntrDisable(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 Block_Id, u32 IntrMask);
u32 XRFdc_IntrEnable(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 Block_Id, u32 IntrMask);
u32 XRFdc_SetThresholdClrMode(XRFdc *InstancePtr, u32 Tile_Id,
				u32 Block_Id, u32 ThresholdToUpdate, u32 ClrMode);
u32 XRFdc_ThresholdStickyClear(XRFdc *InstancePtr, u32 Tile_Id,
					u32 Block_Id, u32 ThresholdToUpdate);
void XRFdc_SetStatusHandler(XRFdc *InstancePtr, void *CallBackRef,
				XRFdc_StatusHandler FunctionPtr);
u32 XRFdc_SetupFIFO(XRFdc *InstancePtr, u32 Type, int Tile_Id, u8 Enable);
u32 XRFdc_GetFIFOStatus(XRFdc *InstancePtr, u32 Type,
				u32 Tile_Id, u8 *EnablePtr);
u32 XRFdc_SetNyquistZone(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
						u32 Block_Id, u32 NyquistZone);
u32 XRFdc_GetNyquistZone(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
						u32 Block_Id, u32 *NyquistZonePtr);
u32 XRFdc_GetOutputCurr(XRFdc *InstancePtr, u32 Tile_Id,
						u32 Block_Id, u32 *OutputCurrPtr);
u32 XRFdc_SetDecimationFactor(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u32 DecimationFactor);
u32 XRFdc_SetInterpolationFactor(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u32 InterpolationFactor);
u32 XRFdc_SetFabClkOutDiv(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u16 FabClkDiv);
u32 XRFdc_SetCalibrationMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
				u8 CalibrationMode);
u32 XRFdc_GetCalibrationMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
				u8 *CalibrationModePtr);
u32 XRFdc_GetClockSource(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 *ClockSourcePtr);
u32 XRFdc_GetPLLLockStatus(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 *LockStatusPtr);
u32 XRFdc_GetPLLConfig(XRFdc *InstancePtr, u32 Type,
					u32 Tile_Id, XRFdc_PLL_Settings *PLLSettings);
u32 XRFdc_DynamicPLLConfig(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u8 Source, double RefClkFreq, double SamplingRate);
u32 XRFdc_SetInvSincFIR(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, u16 Mode);
u32 XRFdc_GetInvSincFIR(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
				u16 *ModePtr);
u32 XRFdc_GetLinkCoupling(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
				u32 *ModePtr);
u32 XRFdc_GetFabClkOutDiv(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u16 *FabClkDivPtr);
u32 XRFdc_SetDither(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, u32 Mode);
u32 XRFdc_GetDither(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, u32 *ModePtr);
u32 XRFdc_SetClkDistribution(XRFdc *InstancePtr, XRFdc_Distribution_Settings
				*DistributionSettingsPtr);
u32 XRFdc_GetClkDistribution(XRFdc *InstancePtr, XRFdc_Distribution_Settings
				*DistributionSettingsPtr);
u32 XRFdc_SetTileClkSettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
						XRFdc_Tile_Clock_Settings *SettingsPtr);
u32 XRFdc_SetDataPathMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
					u32 Mode);
u32 XRFdc_GetDataPathMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
				u32 *ModePtr);
u32 XRFdc_SetIMRPassMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
				u32 Mode);
u32 XRFdc_GetIMRPassMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
					u32 *ModePtr);
u32 XRFdc_SetSignalDetector(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
					XRFdc_Signal_Detector_Settings *SettingsPtr);
u32 XRFdc_GetSignalDetector(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
					XRFdc_Signal_Detector_Settings *SettingsPtr);
u32 XRFdc_DisableCoefficientsOverride(XRFdc *InstancePtr, u32 Tile_Id, u32
						Block_Id, u32 CalibrationBlock);
u32 XRFdc_SetCalCoefficients(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
					u32 CalibrationBlock, XRFdc_Calibration_Coefficients *CoeffPtr);
u32 XRFdc_GetCalCoefficients(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
					u32 CalibrationBlock, XRFdc_Calibration_Coefficients *CoeffPtr);
u32 XRFdc_SetCalFreeze(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
					XRFdc_Cal_Freeze_Settings *CalFreezePtr);
u32 XRFdc_GetCalFreeze(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
					XRFdc_Cal_Freeze_Settings *CalFreezePtr);
#ifndef __BAREMETAL__
s32 XRFdc_GetDeviceNameByDeviceId(char *DevNamePtr, u16 DevId);
#endif

#ifdef __cplusplus
}
#endif

#endif /* RFDC_H_ */
/** @} */
