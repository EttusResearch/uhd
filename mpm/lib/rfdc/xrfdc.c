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
* @file xrfdc.c
* @addtogroup xrfdc_v6_0
* @{
*
* Contains the interface functions of the XRFdc driver.
* See xrfdc.h for a detailed description of the device and driver.
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
*                       In BYPASS mode, input datatype can be Real or IQ,
*                       hence checked both while reading the mixer mode.
*              10/17/17 Fixed Set Threshold API Issue.
* 2.3   sk     11/06/17 Fixed PhaseOffset truncation issue.
*                       Provide user configurability for FineMixerScale.
*              11/08/17 Return error for DAC R2C mode and ADC C2R mode.
*              11/20/17 Fixed StartUp, Shutdown and Reset API for Tile_Id -1.
*              11/20/17 Remove unwanted ADC block checks in 4GSPS mode.
* 3.0   sk     12/11/17 Added DDC and DUC support.
*              12/13/17 Add CoarseMixMode field in Mixer_Settings structure.
*              12/15/17 Add support to switch calibration modes.
*              12/15/17 Add support for mixer frequencies > Fs/2 and < -Fs/2.
*       sg     13/01/18 Added PLL and external clock switch support.
*                       Added API to get PLL lock status.
*                       Added API to get clock source.
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
*       sk     03/22/18 Updated PLL settings based on latest IP values.
* 4.0   sk     04/17/18 Corrected Set/Get MixerSettings API description for
*                       FineMixerScale parameter.
*       sk     04/19/18 Enable VCO Auto selection while configuring the clock.
*       sk     04/24/18 Add API to get PLL Configurations.
*       sk     04/24/18 Add API to get the Link Coupling mode.
*       sk     04/28/18 Implement timeouts for PLL Lock, Startup and shutdown.
*       sk     05/30/18 Removed CalibrationMode check for DAC.
*       sk     06/05/18 Updated minimum Ref clock value to 102.40625MHz.
* 5.0   sk     06/25/18 Update DAC min sampling rate to 500MHz and also update
*                       VCO Range, PLL_DIVIDER and PLL_FPDIV ranges.
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
*       sk     09/24/18 Update powerup-state value based on PLL mode in
*                       XRFdc_DynamicPLLConfig() API.
*       sk     10/10/18 Check for DigitalPath enable in XRFdc_GetNyquistZone()
*                       and XRFdc_GetCalibrationMode() APIs for Multiband.
*       sk     10/13/18 Add support to read the REFCLKDIV param from design.
*                       Update XRFdc_SetPLLConfig() API to support range of
*                       REF_CLK_DIV values(1 to 4).
* 5.1   cog    01/29/19 Replace structure reference ADC checks with
*                       function.
*       cog    01/29/19 Added XRFdc_SetDither() and XRFdc_GetDither() APIs.
*       cog    01/29/19 Rename DataType for mixer input to MixerInputDataType
*                       for readability.
*       cog    01/29/19 Refactoring of interpolation and decimation APIs and
*                       changed fabric rate for decimation X8 for non-high speed ADCs.
*       cog    01/29/19 New inline functions to determine max & min sampling rates
*                       rates in PLL range checking.
* 6.0   cog    02/17/19 Added decimation & interpolation modes
*              02/17/19 Added Inverse-Sinc Second Nyquist Zone Support
*       cog    02/17/19 Added new clock Distribution functionality.
*       cog    02/17/19 Refactored to improve delay balancing in clock
*                       distribution.
*       cog    02/17/19 Added delay calculation & metal log messages.
*       cog    02/17/19 Added intratile clock settings.
*       cog    02/17/19 Moved multiband to a new file xrfdc_mb.c
*       cog    02/17/19 Moved clocking functionality to a new file xrfdc_clock.c
*       cog    02/17/19 Added XRFdc_SetIMRPassMode() and XRFdc_SetIMRPassMode() APIs
*       cog    02/17/19 Added XRFdc_SetDACMode() and XRFdc_GetDACMode() APIs
*       cog    02/17/19	Added XRFdc_SetSignalDetector() and XRFdc_GetSignalDetector() APIs.
*       cog    02/17/19 Added XRFdc_DisableCoefficientsOverride(), XRFdc_SetCalCoefficients
*                       and XRFdc_GetCalCoefficients APIs.
*       cog    02/21/19 Added XRFdc_SetCalFreeze() and XRFdc_GetCalFreeze() APIs.
*       cog    04/09/19 Changed Calibrtation coefficient override control register for OCB1.
*       cog    04/15/19 Rename XRFdc_SetDACMode() and XRFdc_GetDACMode() APIs to
*                       XRFdc_SetDataPathMode() and XRFdc_GetDataPathMode() respectively.
*       cog    04/30/19 Made Changes to the bypass calibration functionality to support Gen2
*                       and below.
*
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "mpm/rfdc/xrfdc.h"

/************************** Constant Definitions *****************************/
#define XRFDC_PLL_LOCK_DLY_CNT		1000U

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/
static u32 XRFdc_RestartIPSM(XRFdc *InstancePtr, u32 Type, int Tile_Id,
					u32 Start, u32 End);
static void StubHandler(void *CallBackRefPtr, u32 Type, u32 Tile_Id,
								u32 Block_Id, u32 StatusEvent);
static void XRFdc_ADCInitialize(XRFdc *InstancePtr);
static void XRFdc_DACInitialize(XRFdc *InstancePtr);
static void XRFdc_DACMBConfigInit(XRFdc *InstancePtr, u32 Tile_Id,
				u32 Block_Id);
static void XRFdc_ADCMBConfigInit(XRFdc *InstancePtr, u32 Tile_Id,
				u32 Block_Id);
static void XRFdc_UpdatePLLStruct(XRFdc *InstancePtr, u32 Type, u32 Tile_Id);
static u32 XRFdc_GetADCBlockStatus(XRFdc *InstancePtr, u32 BaseAddr,
				u32 Tile_Id, u32 Block_Id, XRFdc_BlockStatus *BlockStatusPtr);
static u32 XRFdc_GetDACBlockStatus(XRFdc *InstancePtr, u32 BaseAddr,
				u32 Tile_Id, u32 Block_Id, XRFdc_BlockStatus *BlockStatusPtr);
static void XRFdc_DumpHSCOMRegs(XRFdc *InstancePtr, u32 Type, int Tile_Id);
static void XRFdc_DumpDACRegs(XRFdc *InstancePtr, int Tile_Id);
static void XRFdc_DumpADCRegs(XRFdc *InstancePtr, int Tile_Id);
static u32 XRFdc_WaitForRestartClr(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
			u32 BaseAddr, u32 End);

/************************** Function Prototypes ******************************/

/*****************************************************************************/
/**
*
* Initializes a specific XRFdc instance such that the driver is ready to use.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	ConfigPtr is a reference to a structure containing information
*			about xrfdc. This function initializes an InstancePtr object
*			for a specific device specified by the contents of Config.
*
* @return
*		- XRFDC_SUCCESS if successful.
*
* @note		The user needs to first call the XRFdc_LookupConfig() API
*			which returns the Configuration structure pointer which is
*			passed as a parameter to the XRFdc_CfgInitialize() API.
*
******************************************************************************/
u32 XRFdc_CfgInitialize(XRFdc *InstancePtr, XRFdc_Config *ConfigPtr)
{
	u32 Status;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(ConfigPtr != NULL);

	InstancePtr->io = (struct metal_io_region *)
			metal_allocate_memory(sizeof(struct metal_io_region));
	metal_io_init(InstancePtr->io, (void *)(metal_phys_addr_t)ConfigPtr->BaseAddr,
			&ConfigPtr->BaseAddr, XRFDC_REGION_SIZE, (unsigned)(-1), 0, NULL);

	/*
	 * Set the values read from the device config and the base address.
	 */
	InstancePtr->BaseAddr = ConfigPtr->BaseAddr;
	InstancePtr->RFdc_Config = *ConfigPtr;
	InstancePtr->ADC4GSPS = ConfigPtr->ADCType;
	InstancePtr->StatusHandler = StubHandler;

	/* Initialize ADC */
	XRFdc_ADCInitialize(InstancePtr);

	/* Initialize DAC */
	XRFdc_DACInitialize(InstancePtr);

	/*
	 * Indicate the instance is now ready to use and
	 * initialized without error.
	 */
	InstancePtr->IsReady = XRFDC_COMPONENT_IS_READY;

	Status = XRFDC_SUCCESS;
	return Status;
}

/*****************************************************************************/
/**
*
* Initialize ADC Tiles.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
*
* @return
*		- None.
*
* @note		Static API used to initialize ADC Tiles
*
******************************************************************************/
static void XRFdc_ADCInitialize(XRFdc *InstancePtr)
{
	u32 Tile_Id;
	u32 Block_Id;
	u8 MixerType;

	for (Tile_Id = XRFDC_TILE_ID0; Tile_Id < XRFDC_TILE_ID4; Tile_Id++) {
		InstancePtr->ADC_Tile[Tile_Id].NumOfADCBlocks = 0U;
		for (Block_Id = XRFDC_BLK_ID0; Block_Id < XRFDC_BLK_ID4; Block_Id++) {
			if (XRFdc_IsADCBlockEnabled(InstancePtr, Tile_Id, Block_Id) != 0U) {
				InstancePtr->ADC_Tile[Tile_Id].NumOfADCBlocks += 1U;
				InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Analog_Datapath[Block_Id].
					AnalogPathEnabled = XRFDC_ANALOGPATH_ENABLE;
			}
			/* Initialize Data Type */
			if (InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].
				ADCBlock_Analog_Config[Block_Id].MixMode == XRFDC_MIXER_MODE_BYPASS) {
				InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Block_Id].MixerInputDataType =
					InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].ADCBlock_Digital_Config[Block_Id].MixerInputDataType;
			} else {
				InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Block_Id].MixerInputDataType =
						InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].
										ADCBlock_Analog_Config[Block_Id].MixMode;
			}
			/* Initialize MixerType */
			MixerType = InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].
					ADCBlock_Digital_Config[Block_Id].MixerType;
			InstancePtr->ADC_Tile[Tile_Id].
					ADCBlock_Digital_Datapath[Block_Id].Mixer_Settings.MixerType = MixerType;

			InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Block_Id].ConnectedIData = -1;
			InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Block_Id].ConnectedQData = -1;
			InstancePtr->ADC_Tile[Tile_Id].MultibandConfig =
					InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].MultibandConfig;
			if (XRFdc_IsADCDigitalPathEnabled(InstancePtr, Tile_Id, Block_Id) != 0U) {
				InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Block_Id].DigitalPathAvailable =
						XRFDC_DIGITALPATH_ENABLE;
				InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Block_Id].DigitalPathEnabled =
						XRFDC_DIGITALPATH_ENABLE;
				/* Initialize ConnectedI/QData, MB Config */
				XRFdc_ADCMBConfigInit(InstancePtr, Tile_Id, Block_Id);
			}
		}

		/* Initialize PLL Structure */
		XRFdc_UpdatePLLStruct(InstancePtr, XRFDC_ADC_TILE, Tile_Id);
	}
}

/*****************************************************************************/
/**
*
* Initialize DAC Tiles.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
*
* @return
*		- None.
*
* @note		Static API used to initialize DAC Tiles
*
******************************************************************************/
static void XRFdc_DACInitialize(XRFdc *InstancePtr)
{
	u32 Tile_Id;
	u32 Block_Id;
	u8 MixerType;

	for (Tile_Id = XRFDC_TILE_ID0; Tile_Id < XRFDC_TILE_ID4; Tile_Id++) {
		InstancePtr->DAC_Tile[Tile_Id].NumOfDACBlocks = 0U;
		for (Block_Id = XRFDC_BLK_ID0; Block_Id < XRFDC_BLK_ID4; Block_Id++) {
			if (XRFdc_IsDACBlockEnabled(InstancePtr, Tile_Id, Block_Id) != 0U) {
				InstancePtr->DAC_Tile[Tile_Id].NumOfDACBlocks += 1U;
				InstancePtr->DAC_Tile[Tile_Id].DACBlock_Analog_Datapath[Block_Id].AnalogPathEnabled =
						XRFDC_ANALOGPATH_ENABLE;
			}
			/* Initialize Data Type */
			if (InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].
				DACBlock_Analog_Config[Block_Id].MixMode == XRFDC_MIXER_MODE_BYPASS) {
				InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[Block_Id].MixerInputDataType =
					InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].DACBlock_Digital_Config[Block_Id].MixerInputDataType;
			} else {
				InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[Block_Id].MixerInputDataType =
						XRFDC_DATA_TYPE_IQ;
			}
			/* Initialize MixerType */
			MixerType = InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].
					DACBlock_Digital_Config[Block_Id].MixerType;
			InstancePtr->DAC_Tile[Tile_Id].
					DACBlock_Digital_Datapath[Block_Id].Mixer_Settings.MixerType = MixerType;

			InstancePtr->DAC_Tile[Tile_Id].
				DACBlock_Digital_Datapath[Block_Id].ConnectedIData = -1;
			InstancePtr->DAC_Tile[Tile_Id].
				DACBlock_Digital_Datapath[Block_Id].ConnectedQData = -1;
			InstancePtr->DAC_Tile[Tile_Id].MultibandConfig =
						InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].MultibandConfig;
			if (XRFdc_IsDACDigitalPathEnabled(InstancePtr, Tile_Id, Block_Id) != 0U) {
				InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[Block_Id].DigitalPathAvailable =
						XRFDC_DIGITALPATH_ENABLE;
				InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[Block_Id].DigitalPathEnabled =
						XRFDC_DIGITALPATH_ENABLE;
				/* Initialize ConnectedI/QData, MB Config */
				XRFdc_DACMBConfigInit(InstancePtr, Tile_Id, Block_Id);
			}
		}
		/* Initialize PLL Structure */
		XRFdc_UpdatePLLStruct(InstancePtr, XRFDC_DAC_TILE, Tile_Id);
	}
}

/*****************************************************************************/
/**
*
* Initialize Multiband Configuration for DAC Tiles.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3, and -1.
* @param	Block_Id is DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- None.
*
* @note		Static API used to initialize DAC MB Config
*
******************************************************************************/
static void XRFdc_DACMBConfigInit(XRFdc *InstancePtr, u32 Tile_Id,
				u32 Block_Id)
{
	if (InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].
			DACBlock_Analog_Config[Block_Id].MixMode == XRFDC_MIXER_MODE_C2C) {
		/* Mixer Mode is C2C */
		switch (InstancePtr->DAC_Tile[Tile_Id].MultibandConfig) {
			case XRFDC_MB_MODE_4X:
				XRFdc_SetConnectedIQData(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
									Block_Id, XRFDC_BLK_ID0, XRFDC_BLK_ID1);
				break;
			case XRFDC_MB_MODE_2X_BLK01_BLK23:
			case XRFDC_MB_MODE_2X_BLK01:
			case XRFDC_MB_MODE_2X_BLK23:
				if ((Block_Id == XRFDC_BLK_ID0) || (Block_Id == XRFDC_BLK_ID1)) {
					XRFdc_SetConnectedIQData(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
										Block_Id, XRFDC_BLK_ID0, XRFDC_BLK_ID1);
				} else {
					XRFdc_SetConnectedIQData(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
										Block_Id, XRFDC_BLK_ID2, XRFDC_BLK_ID3);
				}
				break;
			default:
				XRFdc_SetConnectedIQData(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
											Block_Id, Block_Id, Block_Id + 1U);
				break;
		}
	} else if (InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].
			DACBlock_Analog_Config[Block_Id].MixMode == 0x0) {
		/* Mixer Mode is C2R */
		switch (InstancePtr->DAC_Tile[Tile_Id].MultibandConfig) {
			case XRFDC_MB_MODE_4X:
				XRFdc_SetConnectedIQData(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
									Block_Id, XRFDC_BLK_ID0, -1);
				break;
			case XRFDC_MB_MODE_2X_BLK01_BLK23:
			case XRFDC_MB_MODE_2X_BLK01:
			case XRFDC_MB_MODE_2X_BLK23:
				if ((Block_Id == XRFDC_BLK_ID0) || (Block_Id == XRFDC_BLK_ID1)) {
					XRFdc_SetConnectedIQData(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
										Block_Id, XRFDC_BLK_ID0, -1);
				} else {
					XRFdc_SetConnectedIQData(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
										Block_Id, XRFDC_BLK_ID2, -1);
				}
				break;
			default:
				XRFdc_SetConnectedIQData(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
										Block_Id, Block_Id, -1);
				break;
		}
	} else {
		/* Mixer Mode is BYPASS */
		XRFdc_SetConnectedIQData(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
										Block_Id, Block_Id, -1);
	}
}
/*****************************************************************************/
/**
*
* Initialize Multiband Configuration for ADC Tiles.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3, and -1.
* @param	Block_Id is ADC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- None.
*
* @note		Static API used to initialize ADC MB Config
*
******************************************************************************/
static void XRFdc_ADCMBConfigInit(XRFdc *InstancePtr, u32 Tile_Id,
				u32 Block_Id)
{
	if (InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].
			ADCBlock_Analog_Config[Block_Id].MixMode == XRFDC_MIXER_MODE_C2C) {
		/* Mixer mode is C2C */
		switch (InstancePtr->ADC_Tile[Tile_Id].MultibandConfig) {
			case XRFDC_MB_MODE_4X:
				XRFdc_SetConnectedIQData(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
									Block_Id, XRFDC_BLK_ID0, XRFDC_BLK_ID1);
				break;
			case XRFDC_MB_MODE_2X_BLK01_BLK23:
			case XRFDC_MB_MODE_2X_BLK01:
			case XRFDC_MB_MODE_2X_BLK23:
				if ((Block_Id == XRFDC_BLK_ID0) || (Block_Id == XRFDC_BLK_ID1)) {
					XRFdc_SetConnectedIQData(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
										Block_Id, XRFDC_BLK_ID0, XRFDC_BLK_ID1);
				} else {
					XRFdc_SetConnectedIQData(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
										Block_Id, XRFDC_BLK_ID2, XRFDC_BLK_ID3);
				}
				break;
			default:
				XRFdc_SetConnectedIQData(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
											Block_Id, Block_Id, Block_Id + 1U);
				break;
		}
	} else if (InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].
			ADCBlock_Analog_Config[Block_Id].MixMode == 0x0) {
		/* Mixer mode is R2C */
		switch (InstancePtr->ADC_Tile[Tile_Id].MultibandConfig) {
			case XRFDC_MB_MODE_4X:
				XRFdc_SetConnectedIQData(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
									Block_Id, XRFDC_BLK_ID0, -1);
				break;
			case XRFDC_MB_MODE_2X_BLK01_BLK23:
			case XRFDC_MB_MODE_2X_BLK01:
			case XRFDC_MB_MODE_2X_BLK23:
				if ((Block_Id == XRFDC_BLK_ID0) || (Block_Id == XRFDC_BLK_ID1)) {
					XRFdc_SetConnectedIQData(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
										Block_Id, XRFDC_BLK_ID0, -1);
				} else {
					XRFdc_SetConnectedIQData(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
										Block_Id, XRFDC_BLK_ID2, -1);
				}
				break;
			default:
				XRFdc_SetConnectedIQData(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
										Block_Id, Block_Id, -1);
				break;
		}
	} else {
		/* Mixer mode is BYPASS */
		XRFdc_SetConnectedIQData(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
												Block_Id, Block_Id, -1);
	}
}

/*****************************************************************************/
/**
*
* This API updates PLL Structure.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC.
* @param	Tile_Id Valid values are 0-3, and -1.
*
* @return
*		- None.
*
* @note		Static API used to initialize PLL Settings for ADC and DAC
*
******************************************************************************/
static void XRFdc_UpdatePLLStruct(XRFdc *InstancePtr, u32 Type, u32 Tile_Id)
{
	if (Type == XRFDC_ADC_TILE) {
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.SampleRate =
				InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].SamplingRate;
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.RefClkFreq =
				InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].RefClkFreq;
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.Enabled =
				InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].PLLEnable;
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.FeedbackDivider =
				InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].FeedbackDiv;
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.OutputDivider =
				InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].OutputDiv;
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.RefClkDivider =
				InstancePtr->RFdc_Config.ADCTile_Config[Tile_Id].RefClkDiv;
	} else {
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.SampleRate =
				InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].SamplingRate;
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.RefClkFreq =
				InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].RefClkFreq;
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.Enabled =
				InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].PLLEnable;
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.FeedbackDivider =
				InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].FeedbackDiv;
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.OutputDivider =
				InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].OutputDiv;
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.RefClkDivider =
				InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].RefClkDiv;
	}
}

/*****************************************************************************/
/**
*
* The API Restarts the requested tile. It can restart a single tile and
* alternatively can restart all the tiles. Existing register settings are not
* lost or altered in the process. It just starts the requested tile(s).
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3, and -1.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if tile is not enabled or available
*
* @note		None
*
******************************************************************************/
u32 XRFdc_StartUp(XRFdc *InstancePtr, u32 Type, int Tile_Id)
{
	u32 Status;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_RestartIPSM(InstancePtr, Type, Tile_Id, XRFDC_SM_STATE1,
					XRFDC_SM_STATE15);
	return Status;

}

/*****************************************************************************/
/**
*
* The API stops the tile as requested. It can also stop all the tiles if
* asked for. It does not clear any of the existing register settings. It just
* stops the requested tile(s).
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3, and -1.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if tile is not enabled or available
*
* @note		None
*
******************************************************************************/
u32 XRFdc_Shutdown(XRFdc *InstancePtr, u32 Type, int Tile_Id)
{
	u32 Status;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_RestartIPSM(InstancePtr, Type, Tile_Id, XRFDC_SM_STATE1,
					XRFDC_SM_STATE1);
	return Status;

}

/*****************************************************************************/
/**
*
* The API resets the requested tile. It can reset all the tiles as well. In
* the process, all existing register settings are cleared and are replaced
* with the settings initially configured (through the GUI).
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3, and -1.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if tile is not enabled or available
*
* @note		None
******************************************************************************/
u32 XRFdc_Reset(XRFdc *InstancePtr, u32 Type, int Tile_Id)
{
	u32 Status;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_RestartIPSM(InstancePtr, Type, Tile_Id, XRFDC_SM_STATE0,
					XRFDC_SM_STATE15);
	return Status;

}

/*****************************************************************************/
/**
*
* This Static API will be used to wait for restart bit clears and also check
* for PLL Lock if clock source is internal PLL.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	End is end state of State Machine.
*
* @return   - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if timeout occurs.
*
* @note		None
*
******************************************************************************/
static u32 XRFdc_WaitForRestartClr(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 BaseAddr, u32 End)
{
	u32 ClkSrc = 0U;
	u32 DelayCount;
	u32 LockStatus = 0U;
	u32 Status;

	/*
	 * Get Tile clock source information
	 */
	if (XRFdc_GetClockSource(InstancePtr, Type, Tile_Id, &ClkSrc)
								!= XRFDC_SUCCESS) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	if ((ClkSrc == XRFDC_INTERNAL_PLL_CLK) && (End == XRFDC_SM_STATE15)) {
		/*
		 * Wait for internal PLL to lock
		 */
		if (XRFdc_GetPLLLockStatus(InstancePtr, Type, Tile_Id,
				&LockStatus) != XRFDC_SUCCESS) {
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		DelayCount = 0U;
		while (LockStatus != XRFDC_PLL_LOCKED) {
			if (DelayCount == XRFDC_PLL_LOCK_DLY_CNT) {
				metal_log(METAL_LOG_ERROR,  "\n PLL Lock timeout "
					"error in %s\r\n", __func__);
				Status = XRFDC_FAILURE;
				goto RETURN_PATH;
			} else {
				/* Wait for 1 msec */
#ifdef __BAREMETAL__
				usleep(1000);
#else
				metal_sleep_usec(1000);
#endif
				DelayCount++;
				(void)XRFdc_GetPLLLockStatus(InstancePtr, Type, Tile_Id,
								&LockStatus);
			}
		}
	}

	/* Wait till restart bit clear */
	DelayCount = 0U;
	while (XRFdc_ReadReg16(InstancePtr, BaseAddr, XRFDC_RESTART_OFFSET) != 0U) {
		if (DelayCount == XRFDC_PLL_LOCK_DLY_CNT) {
			metal_log(METAL_LOG_ERROR,  "\n Failed to clear "
					"the restart bit in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		} else {
			/* Wait for 1 msec */
#ifdef __BAREMETAL__
			usleep(1000);
#else
			metal_sleep_usec(1000);
#endif
			DelayCount++;
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* Restarts a requested the tile and ensures that starts from a defined start
* state and reaches the requested or defined end state.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3, and -1.
* @param	Start is start state of State Machine
* @param	End is end state of State Machine.
*
* @return   - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if tile is not enabled or available
*
* @note		None
*
******************************************************************************/
static u32 XRFdc_RestartIPSM(XRFdc *InstancePtr, u32 Type, int Tile_Id,
						u32 Start, u32 End)
{
	u32 Status;
	u32 BaseAddr;
	u16 NoOfTiles;
	u16 Index;

	/* An input tile if of -1 selects all tiles */
	if (Tile_Id == XRFDC_SELECT_ALL_TILES) {
		NoOfTiles = XRFDC_NUM_OF_TILES4;
		Index = XRFDC_TILE_ID0;
	} else {
		NoOfTiles = Tile_Id + 1;
		Index = Tile_Id;
	}

	for (; Index < NoOfTiles; Index++) {
		BaseAddr = XRFDC_CTRL_STS_BASE(Type, Index);
		Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Index);
		if ((Status != XRFDC_SUCCESS) && (Tile_Id != XRFDC_SELECT_ALL_TILES)) {
			metal_log(METAL_LOG_ERROR, "\n Requested tile%d not "
							"available in %s\r\n", Index, __func__);
			goto RETURN_PATH;
		} else if (Status != XRFDC_SUCCESS) {
			metal_log(METAL_LOG_DEBUG, "\n Tile%d not "
							"available in %s\r\n", Index, __func__);
			continue;
		} else {
			/* Write Start and End states */
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_RESTART_STATE_OFFSET,
				XRFDC_PWR_STATE_MASK, (Start << XRFDC_RSR_START_SHIFT) | End);

			/* Trigger restart */
			XRFdc_WriteReg(InstancePtr, BaseAddr, XRFDC_RESTART_OFFSET,
					XRFDC_RESTART_MASK);

			/* Wait for restart bit clear */
			Status = XRFdc_WaitForRestartClr(InstancePtr, Type, Index,
					BaseAddr, End);
			if (Status != XRFDC_SUCCESS) {
				goto RETURN_PATH;
			}
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* The API returns the IP status.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	IPStatusPtr Pointer to the XRFdc_IPStatus structure through
*           which the status is returned.
*
* @return
*		- XRFDC_SUCCESS if successful.
*
* @note		None.
*
******************************************************************************/
u32 XRFdc_GetIPStatus(XRFdc *InstancePtr, XRFdc_IPStatus *IPStatusPtr)
{
	u32 Tile_Id;
	u32 Block_Id;
	u32 BaseAddr;
	u16 ReadReg;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(IPStatusPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	for (Tile_Id = XRFDC_TILE_ID0; Tile_Id < XRFDC_TILE_ID4; Tile_Id++) {
		IPStatusPtr->ADCTileStatus[Tile_Id].BlockStatusMask = 0x0;
		IPStatusPtr->DACTileStatus[Tile_Id].BlockStatusMask = 0x0;
		for (Block_Id = XRFDC_BLK_ID0; Block_Id < XRFDC_BLK_ID4; Block_Id++) {
			if (XRFdc_IsADCBlockEnabled(InstancePtr, Tile_Id,
								Block_Id) != 0U) {
				IPStatusPtr->ADCTileStatus[Tile_Id].IsEnabled = 1;
				IPStatusPtr->ADCTileStatus[Tile_Id].BlockStatusMask |=
								(1U << Block_Id);
				BaseAddr = XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id);
				ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr,
										XRFDC_STATUS_OFFSET);
				IPStatusPtr->ADCTileStatus[Tile_Id].PowerUpState = (ReadReg &
						XRFDC_PWR_UP_STAT_MASK) >> XRFDC_PWR_UP_STAT_SHIFT;
				IPStatusPtr->ADCTileStatus[Tile_Id].PLLState = (ReadReg &
						XRFDC_PLL_LOCKED_MASK) >> XRFDC_PLL_LOCKED_SHIFT;
				IPStatusPtr->ADCTileStatus[Tile_Id].TileState =
								XRFdc_ReadReg16(InstancePtr, BaseAddr,
												XRFDC_CURRENT_STATE_OFFSET);
			}
			if (XRFdc_IsDACBlockEnabled(InstancePtr, Tile_Id,
								Block_Id) != 0U) {
				IPStatusPtr->DACTileStatus[Tile_Id].IsEnabled = 1;
				IPStatusPtr->DACTileStatus[Tile_Id].BlockStatusMask |=
								(1U << Block_Id);
				BaseAddr = XRFDC_DAC_TILE_CTRL_STATS_ADDR(Tile_Id);
				IPStatusPtr->DACTileStatus[Tile_Id].TileState =
						XRFdc_ReadReg16(InstancePtr, BaseAddr,
										XRFDC_CURRENT_STATE_OFFSET);
				ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr,
										XRFDC_STATUS_OFFSET);
				IPStatusPtr->DACTileStatus[Tile_Id].PowerUpState = (ReadReg &
						XRFDC_PWR_UP_STAT_MASK) >> XRFDC_PWR_UP_STAT_SHIFT;
				IPStatusPtr->DACTileStatus[Tile_Id].PLLState = (ReadReg &
						XRFDC_PLL_LOCKED_MASK) >> XRFDC_PLL_LOCKED_SHIFT;
			}
		}
	}

	/*TODO IP state*/

	return XRFDC_SUCCESS;
}

/*****************************************************************************/
/**
*
* The API returns the requested block status.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3. XRFdc_BlockStatus.
* @param	BlockStatusPtr is Pointer to the XRFdc_BlockStatus structure through
*			which the ADC/DAC block status is returned.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Common API for ADC/DAC blocks.
*
******************************************************************************/
u32 XRFdc_GetBlockStatus(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
					u32 Block_Id, XRFdc_BlockStatus *BlockStatusPtr)
{
	u32 Status;
	u32 Block;
	u16 ReadReg;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(BlockStatusPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, Type, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	Block = Block_Id;
	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Block_Id == XRFDC_BLK_ID1) && (Type == XRFDC_ADC_TILE)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Block_Id);
	if (Type == XRFDC_ADC_TILE) {
		Status = XRFdc_GetADCBlockStatus(InstancePtr, BaseAddr, Tile_Id,
							Block, BlockStatusPtr);
	} else {
		Status = XRFdc_GetDACBlockStatus(InstancePtr, BaseAddr, Tile_Id,
								Block, BlockStatusPtr);
	}
	if (Status != XRFDC_SUCCESS) {
		goto RETURN_PATH;
	}

	ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CLK_EN_OFFSET,
				XRFDC_DAT_CLK_EN_MASK);
	if (ReadReg == XRFDC_DAT_CLK_EN_MASK) {
		BlockStatusPtr->DataPathClocksStatus = 0x1U;
	} else {
		BlockStatusPtr->DataPathClocksStatus = 0x0U;
	}

	Status = XRFDC_SUCCESS;

RETURN_PATH:
	return Status;

}

/*****************************************************************************/
/**
*
* The API returns the requested block status for ADC block
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3. XRFdc_BlockStatus.
* @param	BlockStatus is Pointer to the XRFdc_BlockStatus structure through
*			which the ADC/DAC block status is returned.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Static API for ADC blocks.
*
******************************************************************************/
static u32 XRFdc_GetADCBlockStatus(XRFdc *InstancePtr, u32 BaseAddr,
				u32 Tile_Id, u32 Block_Id, XRFdc_BlockStatus *BlockStatusPtr)
{
	u8 FIFOEnable = 0U;
	u32 DecimationFactor = 0U;
	u8 MixerMode;
	u16 ReadReg;
	u32 Status;

	BlockStatusPtr->SamplingFreq = InstancePtr->ADC_Tile[Tile_Id].
			PLL_Settings.SampleRate;

	/* DigitalDataPathStatus */
	(void)XRFdc_GetFIFOStatus(InstancePtr, XRFDC_ADC_TILE,
				Tile_Id, &FIFOEnable);
	BlockStatusPtr->DigitalDataPathStatus = FIFOEnable;
	(void)XRFdc_GetDecimationFactor(InstancePtr, Tile_Id,
			Block_Id, &DecimationFactor);
	BlockStatusPtr->DigitalDataPathStatus |=
		(DecimationFactor << XRFDC_DIGI_ANALOG_SHIFT4);

	ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_MXR_MODE_OFFSET,
				(XRFDC_EN_I_IQ_MASK | XRFDC_EN_Q_IQ_MASK));
	switch (ReadReg) {
		case XRFDC_MIXER_MODE_C2C_MASK:
			MixerMode = XRFDC_MIXER_MODE_C2C;
			break;
		case XRFDC_MIXER_MODE_R2C_MASK:
			MixerMode = XRFDC_MIXER_MODE_R2C;
			break;
		case XRFDC_MIXER_MODE_OFF_MASK:
			MixerMode = XRFDC_MIXER_MODE_OFF;
			break;
		default:
			metal_log(METAL_LOG_ERROR, "\n Invalid MixerMode "
						"for ADC in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
	}

	BlockStatusPtr->DigitalDataPathStatus |=
			(MixerMode << XRFDC_DIGI_ANALOG_SHIFT8);

	/*
	 * Checking ADC block enable for ADC AnalogPath.
	 * This can be changed later,
	 */
	BlockStatusPtr->AnalogDataPathStatus =
			XRFdc_IsADCBlockEnabled(InstancePtr, Tile_Id, Block_Id);
	BlockStatusPtr->IsFIFOFlagsEnabled = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_ADC_FABRIC_IMR_OFFSET, XRFDC_FAB_IMR_USRDAT_MASK);
	BlockStatusPtr->IsFIFOFlagsAsserted = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_ADC_FABRIC_ISR_OFFSET, XRFDC_FAB_ISR_USRDAT_MASK);

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* The API returns the requested block status for DAC block
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3. XRFdc_BlockStatus.
* @param	BlockStatus is Pointer to the XRFdc_BlockStatus structure through
*			which the DAC block status is returned.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Static API for DAC blocks.
*
******************************************************************************/
static u32 XRFdc_GetDACBlockStatus(XRFdc *InstancePtr, u32 BaseAddr,
				u32 Tile_Id, u32 Block_Id, XRFdc_BlockStatus *BlockStatusPtr)
{
	u32 InterpolationFactor = 0U;
	u32 DecoderMode = 0U;
	u8 MixerMode;
	u16 ReadReg;
	u32 Status;
	u8 FIFOEnable = 0U;

	BlockStatusPtr->SamplingFreq = InstancePtr->DAC_Tile[Tile_Id].
			PLL_Settings.SampleRate;

	/* DigitalDataPathStatus */
	(void)XRFdc_GetFIFOStatus(InstancePtr, XRFDC_DAC_TILE,
			Tile_Id, &FIFOEnable);
	BlockStatusPtr->DigitalDataPathStatus = FIFOEnable;
	(void)XRFdc_GetInterpolationFactor(InstancePtr, Tile_Id,
			Block_Id, &InterpolationFactor);
	BlockStatusPtr->DigitalDataPathStatus |=
		(InterpolationFactor << XRFDC_DIGI_ANALOG_SHIFT4);
	/* Adder Enable */
	ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_DAC_MB_CFG_OFFSET, XRFDC_EN_MB_MASK);
	ReadReg = ReadReg >> XRFDC_EN_MB_SHIFT;
	BlockStatusPtr->DigitalDataPathStatus |=
				(ReadReg << XRFDC_DIGI_ANALOG_SHIFT8);
	ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_MXR_MODE_OFFSET,
				(XRFDC_EN_I_IQ_MASK | XRFDC_EN_Q_IQ_MASK));
	switch (ReadReg) {
		case XRFDC_MIXER_MODE_C2C_MASK:
			MixerMode = XRFDC_MIXER_MODE_C2C;
			break;
		case XRFDC_MIXER_MODE_C2R_MASK:
			MixerMode = XRFDC_MIXER_MODE_C2R;
			break;
		case XRFDC_MIXER_MODE_OFF_MASK:
			MixerMode = XRFDC_MIXER_MODE_OFF;
			break;
		default:
			metal_log(METAL_LOG_ERROR, "\n Invalid MixerMode "
						"for ADC in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
	}

	BlockStatusPtr->DigitalDataPathStatus |=
			(MixerMode << XRFDC_DIGI_ANALOG_SHIFT12);

	/* AnalogDataPathStatus */
	BlockStatusPtr->AnalogDataPathStatus = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_DAC_INVSINC_OFFSET, (InstancePtr->RFdc_Config.IPType < 2)?XRFDC_EN_INVSINC_MASK:XRFDC_MODE_INVSINC_MASK);
	(void)XRFdc_GetDecoderMode(InstancePtr, Tile_Id, Block_Id,
				&DecoderMode);
	BlockStatusPtr->AnalogDataPathStatus |=
		(DecoderMode << XRFDC_DIGI_ANALOG_SHIFT4);

	/* FIFO Flags status */
	BlockStatusPtr->IsFIFOFlagsEnabled = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_DAC_FABRIC_IMR_OFFSET, XRFDC_FAB_IMR_USRDAT_MASK);
	BlockStatusPtr->IsFIFOFlagsAsserted = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_DAC_FABRIC_ISR_OFFSET, XRFDC_FAB_ISR_USRDAT_MASK);

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
* This API is used to update various QMC settings, eg gain, phase, offset etc.
* QMC settings passed are used to update the corresponding
* block level registers. Driver structure is updated with the new values.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	QMCSettingsPtr is Pointer to the XRFdc_QMC_Settings structure
*			in which the QMC settings are passed.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		None
*
******************************************************************************/
u32 XRFdc_SetQMCSettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
					u32 Block_Id, XRFdc_QMC_Settings *QMCSettingsPtr)
{
	u32 Status;
	XRFdc_QMC_Settings *QMCConfigPtr;
	u32 BaseAddr;
	s32 PhaseCorrectionFactor;
	u32 GainCorrectionFactor;
	u32 Index;
	u32 NoOfBlocks;
	u32 Offset;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(QMCSettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, Type, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Type == XRFDC_ADC_TILE)) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
		if (InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Index].
				MixerInputDataType == XRFDC_DATA_TYPE_IQ) {
			Index = Block_Id;
			NoOfBlocks = XRFDC_NUM_OF_BLKS3;
			if (Block_Id == XRFDC_BLK_ID1) {
				NoOfBlocks = XRFDC_NUM_OF_BLKS4;
			}
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks;) {
		if (Type == XRFDC_ADC_TILE) {
			/* ADC */
			QMCConfigPtr = &InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Analog_Datapath[Index].QMC_Settings;
		} else {
			QMCConfigPtr = &InstancePtr->DAC_Tile[Tile_Id].
				DACBlock_Analog_Datapath[Index].QMC_Settings;
		}

		BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Index);

		if ((QMCSettingsPtr->EnableGain != 0U) &&
				(QMCSettingsPtr->EnableGain != 1U)) {
			metal_log(METAL_LOG_ERROR, "\n Invalid QMC gain option "
											"in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		if ((QMCSettingsPtr->EnablePhase != 0U) &&
					(QMCSettingsPtr->EnablePhase != 1U)) {
			metal_log(METAL_LOG_ERROR, "\n Invalid QMC phase option "
										"in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		if ((QMCSettingsPtr->PhaseCorrectionFactor <= XRFDC_MIN_PHASE_CORR_FACTOR) ||
				(QMCSettingsPtr->PhaseCorrectionFactor >= XRFDC_MAX_PHASE_CORR_FACTOR)) {
			metal_log(METAL_LOG_ERROR, "\n Invalid QMC Phase Correction "
								"factor in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		if ((QMCSettingsPtr->GainCorrectionFactor < XRFDC_MIN_GAIN_CORR_FACTOR) ||
				(QMCSettingsPtr->GainCorrectionFactor >= XRFDC_MAX_GAIN_CORR_FACTOR)) {
			metal_log(METAL_LOG_ERROR, "\n Invalid QMC Gain Correction "
								"factor in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}

		if ((QMCSettingsPtr->EventSource > XRFDC_EVNT_SRC_PL) ||
				((QMCSettingsPtr->EventSource == XRFDC_EVNT_SRC_MARKER) &&
							(Type == XRFDC_ADC_TILE))) {
			metal_log(METAL_LOG_ERROR, "\n Invalid event source selection "
											"in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}

		if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
				(Type == XRFDC_ADC_TILE) &&
				((QMCSettingsPtr->EventSource == XRFDC_EVNT_SRC_SLICE) ||
				(QMCSettingsPtr->EventSource ==
						XRFDC_EVNT_SRC_IMMEDIATE))) {
			metal_log(METAL_LOG_ERROR, "\n Invalid Event Source, "
					"event source is not supported in 4GSPS ADC %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}

		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_QMC_CFG_OFFSET,
				XRFDC_QMC_CFG_EN_GAIN_MASK, QMCSettingsPtr->EnableGain);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_QMC_CFG_OFFSET,
				XRFDC_QMC_CFG_EN_PHASE_MASK,
				(QMCSettingsPtr->EnablePhase << XRFDC_QMC_CFG_PHASE_SHIFT));

		/* Phase Correction factor is applicable to ADC/DAC IQ mode only */
		if (((Type == XRFDC_ADC_TILE) &&
			(InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Index].
			MixerInputDataType == XRFDC_DATA_TYPE_IQ)) ||
			((Type == XRFDC_DAC_TILE) &&
			(InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[Index].
					MixerInputDataType == XRFDC_DATA_TYPE_IQ))) {
			PhaseCorrectionFactor =
				((QMCSettingsPtr->PhaseCorrectionFactor / XRFDC_MAX_PHASE_CORR_FACTOR) *
							XRFDC_QMC_PHASE_MULT);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_QMC_PHASE_OFFSET,
					XRFDC_QMC_PHASE_CRCTN_MASK, PhaseCorrectionFactor);
		}

		/* Gain Correction factor */
		GainCorrectionFactor = ((QMCSettingsPtr->GainCorrectionFactor *
								XRFDC_QMC_GAIN_MULT) / XRFDC_MAX_GAIN_CORR_FACTOR);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_QMC_GAIN_OFFSET,
						XRFDC_QMC_GAIN_CRCTN_MASK, GainCorrectionFactor);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_QMC_OFF_OFFSET,
			XRFDC_QMC_OFFST_CRCTN_MASK, QMCSettingsPtr->OffsetCorrectionFactor);

		/* Event Source */
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_QMC_UPDT_OFFSET,
				XRFDC_QMC_UPDT_MODE_MASK, QMCSettingsPtr->EventSource);

		if (QMCSettingsPtr->EventSource == XRFDC_EVNT_SRC_IMMEDIATE) {
			if (Type == XRFDC_ADC_TILE) {
				Offset = XRFDC_ADC_UPDATE_DYN_OFFSET;
			} else {
				Offset = XRFDC_DAC_UPDATE_DYN_OFFSET;
			}
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, Offset,
				XRFDC_UPDT_EVNT_MASK, XRFDC_UPDT_EVNT_QMC_MASK);
		}
		/* Update the instance with new values */
		QMCConfigPtr->EventSource = QMCSettingsPtr->EventSource;
		QMCConfigPtr->PhaseCorrectionFactor =
								QMCSettingsPtr->PhaseCorrectionFactor;
		QMCConfigPtr->GainCorrectionFactor =
								QMCSettingsPtr->GainCorrectionFactor;
		QMCConfigPtr->OffsetCorrectionFactor =
								QMCSettingsPtr->OffsetCorrectionFactor;
		QMCConfigPtr->EnablePhase = QMCSettingsPtr->EnablePhase;
		QMCConfigPtr->EnableGain = QMCSettingsPtr->EnableGain;
		if ((Type == XRFDC_ADC_TILE) &&
			(InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Index].MixerInputDataType ==
				XRFDC_DATA_TYPE_IQ) && (XRFdc_IsHighSpeedADC(InstancePtr,
				Tile_Id) == 1)) {
			Index += XRFDC_BLK_ID2;
		} else {
			Index += XRFDC_BLK_ID1;
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* QMC settings are returned back to the caller through this API.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	QMCSettingsPtr Pointer to the XRFdc_QMC_Settings structure
*			in which the QMC settings are passed.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		None
*
******************************************************************************/
u32 XRFdc_GetQMCSettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
						u32 Block_Id, XRFdc_QMC_Settings *QMCSettingsPtr)
{
	u32 Status;
	u32 BaseAddr;
	s32 PhaseCorrectionFactor;
	u32 GainCorrectionFactor;
	s32 OffsetCorrectionFactor;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(QMCSettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, Type, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Block_Id == XRFDC_BLK_ID1) && (Type == XRFDC_ADC_TILE) &&
				(InstancePtr->ADC_Tile[Tile_Id].
					ADCBlock_Digital_Datapath[Block_Id].MixerInputDataType !=
						XRFDC_DATA_TYPE_IQ)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Block_Id);

	QMCSettingsPtr->EnableGain = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_QMC_CFG_OFFSET, XRFDC_QMC_CFG_EN_GAIN_MASK);
	QMCSettingsPtr->EnablePhase = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_QMC_CFG_OFFSET, XRFDC_QMC_CFG_EN_PHASE_MASK) >>
					XRFDC_QMC_CFG_PHASE_SHIFT;

	/* Phase Correction factor */
	if (((Type == XRFDC_ADC_TILE) &&
		(InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Block_Id].
				MixerInputDataType == XRFDC_DATA_TYPE_IQ)) ||
		((Type == XRFDC_DAC_TILE) &&
		(InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[Block_Id].
				MixerInputDataType == XRFDC_DATA_TYPE_IQ))) {
		PhaseCorrectionFactor = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_QMC_PHASE_OFFSET, XRFDC_QMC_PHASE_CRCTN_MASK);
		PhaseCorrectionFactor = (PhaseCorrectionFactor >>
			11) == 0 ? PhaseCorrectionFactor :
			((-1 ^ 0xFFF) | PhaseCorrectionFactor);
		QMCSettingsPtr->PhaseCorrectionFactor =
			((PhaseCorrectionFactor * XRFDC_MAX_PHASE_CORR_FACTOR) /
				XRFDC_QMC_PHASE_MULT);
	} else {
		QMCSettingsPtr->PhaseCorrectionFactor = 0U;
	}

	/* Gain Correction factor */
	GainCorrectionFactor = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_QMC_GAIN_OFFSET, XRFDC_QMC_GAIN_CRCTN_MASK);
	QMCSettingsPtr->GainCorrectionFactor = ((GainCorrectionFactor *
			XRFDC_MAX_GAIN_CORR_FACTOR) / XRFDC_QMC_GAIN_MULT);
	OffsetCorrectionFactor = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_QMC_OFF_OFFSET, XRFDC_QMC_OFFST_CRCTN_MASK);
	QMCSettingsPtr->OffsetCorrectionFactor =
		(OffsetCorrectionFactor >> 11) == 0 ? OffsetCorrectionFactor :
			((-1 ^ 0xFFF) | OffsetCorrectionFactor);

	/* Event Source */
	QMCSettingsPtr->EventSource = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_QMC_UPDT_OFFSET, XRFDC_QMC_UPDT_MODE_MASK);

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Coarse delay settings passed are used to update the corresponding
* block level registers. Driver structure is updated with the new values.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	CoarseDelaySettingsPtr is Pointer to the XRFdc_CoarseDelay_Settings
*			structure in which the CoarseDelay settings are passed.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		None
*
******************************************************************************/
u32 XRFdc_SetCoarseDelaySettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u32 Block_Id, XRFdc_CoarseDelay_Settings *CoarseDelaySettingsPtr)
{
	u32 Status;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;
	u16 Mask;
	u16 MaxDelay;
	XRFdc_CoarseDelay_Settings *CoarseDelayConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(CoarseDelaySettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Mask = (InstancePtr->RFdc_Config.IPType < 2)?XRFDC_CRSE_DLY_CFG_MASK:XRFDC_CRSE_DLY_CFG_MASK_EXT;
	MaxDelay = (InstancePtr->RFdc_Config.IPType < 2)?XRFDC_CRSE_DLY_MAX:XRFDC_CRSE_DLY_MAX_EXT;
	Status = XRFdc_CheckBlockEnabled(InstancePtr, Type, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Type == XRFDC_ADC_TILE)) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks; Index++) {
		if (Type == XRFDC_ADC_TILE) {
			CoarseDelayConfigPtr = &InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Analog_Datapath[Index].
				CoarseDelay_Settings;
		} else {
			CoarseDelayConfigPtr = &InstancePtr->DAC_Tile[Tile_Id].
				DACBlock_Analog_Datapath[Index].
				CoarseDelay_Settings;
		}

		BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Index);

		if (CoarseDelaySettingsPtr->CoarseDelay > MaxDelay) {
			metal_log(METAL_LOG_ERROR, "\n Requested coarse "
				"delay not valid in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		if ((CoarseDelaySettingsPtr->EventSource > XRFDC_EVNT_SRC_PL) ||
			((CoarseDelaySettingsPtr->EventSource == XRFDC_EVNT_SRC_MARKER) &&
						(Type == XRFDC_ADC_TILE))) {
			metal_log(METAL_LOG_ERROR, "\n Invalid event "
				"source selection in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Type == XRFDC_ADC_TILE) &&
			((CoarseDelaySettingsPtr->EventSource ==
				XRFDC_EVNT_SRC_SLICE) ||
			(CoarseDelaySettingsPtr->EventSource ==
				XRFDC_EVNT_SRC_IMMEDIATE))) {
			Status = XRFDC_FAILURE;
			metal_log(METAL_LOG_ERROR, "\n Invalid Event "
				"Source, event source is not supported in "
				"4GSPS ADC %s\r\n", __func__);
			goto RETURN_PATH;
		}
		if (Type == XRFDC_ADC_TILE) {
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_CRSE_DLY_CFG_OFFSET, Mask, CoarseDelaySettingsPtr->CoarseDelay);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_ADC_CRSE_DLY_UPDT_OFFSET, XRFDC_QMC_UPDT_MODE_MASK,
				CoarseDelaySettingsPtr->EventSource);
			if (CoarseDelaySettingsPtr->EventSource ==
						XRFDC_EVNT_SRC_IMMEDIATE) {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr,
					XRFDC_ADC_UPDATE_DYN_OFFSET, XRFDC_UPDT_EVNT_MASK,
					XRFDC_ADC_UPDT_CRSE_DLY_MASK);
			}
		} else {
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_DAC_CRSE_DLY_CFG_OFFSET, Mask,
				CoarseDelaySettingsPtr->CoarseDelay);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_DAC_CRSE_DLY_UPDT_OFFSET, XRFDC_QMC_UPDT_MODE_MASK,
				CoarseDelaySettingsPtr->EventSource);
			if (CoarseDelaySettingsPtr->EventSource ==
					XRFDC_EVNT_SRC_IMMEDIATE) {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr,
					XRFDC_DAC_UPDATE_DYN_OFFSET, XRFDC_UPDT_EVNT_MASK,
					XRFDC_DAC_UPDT_CRSE_DLY_MASK);
			}
		}
		/* Update the instance with new values */
		CoarseDelayConfigPtr->CoarseDelay =
			CoarseDelaySettingsPtr->CoarseDelay;
		CoarseDelayConfigPtr->EventSource =
			CoarseDelaySettingsPtr->EventSource;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Coarse delay settings are returned back to the caller.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	CoarseDelaySettingsPtr Pointer to the XRFdc_CoarseDelay_Settings
*			structure in which the Coarse Delay settings are passed.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		None.
*
******************************************************************************/
u32 XRFdc_GetCoarseDelaySettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u32 Block_Id, XRFdc_CoarseDelay_Settings *CoarseDelaySettingsPtr)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(CoarseDelaySettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, Type, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Block_Id == XRFDC_BLK_ID1) && (Type == XRFDC_ADC_TILE)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Block_Id);

	if (Type == XRFDC_ADC_TILE) {
		CoarseDelaySettingsPtr->CoarseDelay =
			XRFdc_ReadReg16(InstancePtr, BaseAddr,
						XRFDC_ADC_CRSE_DLY_CFG_OFFSET);
		CoarseDelaySettingsPtr->EventSource = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_ADC_CRSE_DLY_UPDT_OFFSET, XRFDC_QMC_UPDT_MODE_MASK);
	} else {
		CoarseDelaySettingsPtr->CoarseDelay =
			XRFdc_ReadReg16(InstancePtr, BaseAddr,
					XRFDC_DAC_CRSE_DLY_CFG_OFFSET);
		CoarseDelaySettingsPtr->EventSource = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_DAC_CRSE_DLY_UPDT_OFFSET, XRFDC_QMC_UPDT_MODE_MASK);
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This function will trigger the update event for an event.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	Event is for which dynamic update event will trigger.
*			XRFDC_EVENT_* defines the different events.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Common API for ADC/DAC blocks
*
******************************************************************************/
u32 XRFdc_UpdateEvent(XRFdc *InstancePtr, u32 Type, u32 Tile_Id, u32 Block_Id,
								u32 Event)
{
	u32 Status;
	u32 BaseAddr;
	u32 EventSource;
	u32 NoOfBlocks;
	u32 Index;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Index = Block_Id;
	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Type == XRFDC_ADC_TILE)) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
		if ((Event == XRFDC_EVENT_QMC) && (InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Index].MixerInputDataType ==
				XRFDC_DATA_TYPE_IQ)) {
			Index = Block_Id;
			NoOfBlocks = XRFDC_NUM_OF_BLKS3;
			if (Block_Id == XRFDC_BLK_ID1) {
				NoOfBlocks = XRFDC_NUM_OF_BLKS4;
			}
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	if ((Event != XRFDC_EVENT_MIXER) && (Event != XRFDC_EVENT_QMC) &&
				(Event != XRFDC_EVENT_CRSE_DLY)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Event "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	for (; Index < NoOfBlocks;) {
		/* Identify the Event Source */
		BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Index);
		if (Event == XRFDC_EVENT_MIXER) {
			Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, Type,
							Tile_Id, Block_Id);
			EventSource = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_NCO_UPDT_OFFSET, XRFDC_NCO_UPDT_MODE_MASK);
		} else if (Event == XRFDC_EVENT_CRSE_DLY) {
			Status = XRFdc_CheckBlockEnabled(InstancePtr, Type,
						Tile_Id, Block_Id);
			EventSource = XRFdc_RDReg(InstancePtr, BaseAddr,
					(Type == XRFDC_ADC_TILE) ? XRFDC_ADC_CRSE_DLY_UPDT_OFFSET :
					XRFDC_DAC_CRSE_DLY_UPDT_OFFSET, XRFDC_QMC_UPDT_MODE_MASK);
		} else {
			Status = XRFdc_CheckBlockEnabled(InstancePtr, Type,
						Tile_Id, Block_Id);
			EventSource = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_QMC_UPDT_OFFSET, XRFDC_QMC_UPDT_MODE_MASK);
		}
		if (Status != XRFDC_SUCCESS) {
			metal_log(METAL_LOG_ERROR, "\n Requested block not "
									"available in %s\r\n", __func__);
			goto RETURN_PATH;
		}

		if ((EventSource == XRFDC_EVNT_SRC_SYSREF) ||
				(EventSource == XRFDC_EVNT_SRC_PL) ||
				(EventSource == XRFDC_EVNT_SRC_MARKER)) {
			Status = XRFDC_FAILURE;
			metal_log(METAL_LOG_ERROR, "\n Invalid Event "
				"Source, this should be issued external to "
				"the driver %s\r\n", __func__);
			goto RETURN_PATH;
		}
		if (Type == XRFDC_ADC_TILE) {
			if (EventSource == XRFDC_EVNT_SRC_SLICE) {
				XRFdc_WriteReg16(InstancePtr, BaseAddr,
					XRFDC_ADC_UPDATE_DYN_OFFSET, 0x1);
			} else {
				BaseAddr = XRFDC_ADC_TILE_DRP_ADDR(Tile_Id) +
							XRFDC_HSCOM_ADDR;
				XRFdc_WriteReg16(InstancePtr, BaseAddr,
					XRFDC_HSCOM_UPDT_DYN_OFFSET, 0x1);
			}
		} else {
			if (EventSource == XRFDC_EVNT_SRC_SLICE) {
				XRFdc_WriteReg16(InstancePtr, BaseAddr,
					XRFDC_DAC_UPDATE_DYN_OFFSET, 0x1);
			} else {
				BaseAddr = XRFDC_DAC_TILE_DRP_ADDR(Tile_Id) +
							XRFDC_HSCOM_ADDR;
				XRFdc_WriteReg16(InstancePtr, BaseAddr,
					XRFDC_HSCOM_UPDT_DYN_OFFSET, 0x1);
			}
		}
		if ((Event == XRFDC_EVENT_QMC) && (InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Index].MixerInputDataType ==
				XRFDC_DATA_TYPE_IQ) && (XRFdc_IsHighSpeedADC(InstancePtr,
				Tile_Id) == 1) && (Type == XRFDC_ADC_TILE)) {
			Index += XRFDC_BLK_ID2;
		} else {
			Index += XRFDC_BLK_ID1;
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API is to set the decimation factor and also update the FIFO write
* words w.r.t to decimation factor.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	DecimationFactor to be set for DAC block.
*           XRFDC_INTERP_DECIM_* defines the valid values.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only ADC blocks
*
******************************************************************************/
u32 XRFdc_SetDecimationFactor(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
					u32 DecimationFactor)
{
	u32 Status;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;
	u16 FabricRate;
	u8 DataType;
	u32 Factor;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
				Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block digital path not "
								"enabled in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((DecimationFactor != XRFDC_INTERP_DECIM_OFF) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_1X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_2X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_4X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_8X) &&
			((InstancePtr->RFdc_Config.IPType < 2) ||
			((DecimationFactor != XRFDC_INTERP_DECIM_3X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_5X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_6X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_10X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_12X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_16X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_20X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_24X) &&
			(DecimationFactor != XRFDC_INTERP_DECIM_40X)))) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Decimation factor "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		DataType = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_ADC_DECI_CONFIG_OFFSET, XRFDC_DEC_CFG_MASK);

		/* Decimation factor */
		Factor = DecimationFactor;
		if (InstancePtr->RFdc_Config.IPType < 2) {
			if (DecimationFactor == XRFDC_INTERP_DECIM_4X) {
				Factor = 0x3;
			}
			if (DecimationFactor == XRFDC_INTERP_DECIM_8X) {
				Factor = 0x4;
			}
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_DECI_MODE_OFFSET,
				XRFDC_DEC_MOD_MASK, Factor);
		} else {
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_DECI_MODE_OFFSET,
				XRFDC_DEC_MOD_MASK_EXT, Factor);
		}



		/* Fabric rate */
		FabricRate = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_ADC_FABRIC_RATE_OFFSET, XRFDC_ADC_FAB_RATE_WR_MASK);
		if ((DataType == XRFDC_DECIM_2G_IQ_DATA_TYPE) ||
				(DataType == XRFDC_DECIM_4G_DATA_TYPE) ||
				(XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1)) {
			switch (DecimationFactor) {
				case XRFDC_INTERP_DECIM_1X:
					FabricRate = XRFDC_FAB_RATE_8;
					break;
				case XRFDC_INTERP_DECIM_2X:
				case XRFDC_INTERP_DECIM_3X:
					FabricRate = XRFDC_FAB_RATE_4;
					break;
				case XRFDC_INTERP_DECIM_4X:
				case XRFDC_INTERP_DECIM_5X:
				case XRFDC_INTERP_DECIM_6X:
					FabricRate = XRFDC_FAB_RATE_2;
					break;
				case XRFDC_INTERP_DECIM_10X:
				case XRFDC_INTERP_DECIM_12X:
				case XRFDC_INTERP_DECIM_8X:
					FabricRate = XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) ?
							XRFDC_FAB_RATE_1:XRFDC_FAB_RATE_2;
					break;
				case XRFDC_INTERP_DECIM_16X:
				case XRFDC_INTERP_DECIM_20X:
				case XRFDC_INTERP_DECIM_24X:
				case XRFDC_INTERP_DECIM_40X:
					FabricRate = XRFDC_FAB_RATE_1;
					break;
				default:
					metal_log(METAL_LOG_DEBUG, "\n Decimation block "
								"is OFF in %s\r\n", __func__);
					break;
			}
		} else {
			switch (DecimationFactor) {
				case XRFDC_INTERP_DECIM_1X:
					FabricRate = XRFDC_FAB_RATE_4;
					break;
				case XRFDC_INTERP_DECIM_2X:
				case XRFDC_INTERP_DECIM_3X:
					FabricRate = XRFDC_FAB_RATE_2;
					break;
				case XRFDC_INTERP_DECIM_4X:
				case XRFDC_INTERP_DECIM_8X:
				case XRFDC_INTERP_DECIM_5X:
				case XRFDC_INTERP_DECIM_6X:
				case XRFDC_INTERP_DECIM_10X:
				case XRFDC_INTERP_DECIM_12X:
				case XRFDC_INTERP_DECIM_16X:
				case XRFDC_INTERP_DECIM_20X:
				case XRFDC_INTERP_DECIM_24X:
				case XRFDC_INTERP_DECIM_40X:
					FabricRate = XRFDC_FAB_RATE_1;
					break;
				default:
					metal_log(METAL_LOG_DEBUG, "\n Decimation block "
								"is OFF in %s\r\n", __func__);
					break;
			}
		}
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_FABRIC_RATE_OFFSET,
				XRFDC_ADC_FAB_RATE_WR_MASK, FabricRate);
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API is to set the divider for clock fabric out.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	FabClkDiv to be set for a tile.
*           XRFDC_FAB_CLK_* defines the valid divider values.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		ADC and DAC Tiles
*
******************************************************************************/
u32 XRFdc_SetFabClkOutDiv(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u16 FabClkDiv)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Tile_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested tile not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((FabClkDiv != XRFDC_FAB_CLK_DIV1) &&
			(FabClkDiv != XRFDC_FAB_CLK_DIV2) &&
			(FabClkDiv != XRFDC_FAB_CLK_DIV4) &&
			(FabClkDiv != XRFDC_FAB_CLK_DIV8) &&
			(FabClkDiv != XRFDC_FAB_CLK_DIV16)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Fabric clock out "
				"divider value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_DRP_BASE(Type, Tile_Id) + XRFDC_HSCOM_ADDR;

	if ((Type == XRFDC_ADC_TILE) &&
			(FabClkDiv == XRFDC_FAB_CLK_DIV1)) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR, "\n Invalid clock divider "
						"in %s \r\n", __func__);
		goto RETURN_PATH;
	} else {
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_HSCOM_CLK_DIV_OFFSET,
			XRFDC_FAB_CLK_DIV_MASK, FabClkDiv);
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API is to get the divider for clock fabric out.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	FabClkDivPtr is a pointer to get fabric clock for a tile.
*           XRFDC_FAB_CLK_* defines the valid divider values.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		API is applicable for both ADC and DAC Tiles
*
******************************************************************************/
u32 XRFdc_GetFabClkOutDiv(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u16 *FabClkDivPtr)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(FabClkDivPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Tile_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested tile not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_DRP_BASE(Type, Tile_Id) + XRFDC_HSCOM_ADDR;

	*FabClkDivPtr = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_HSCOM_CLK_DIV_OFFSET, XRFDC_FAB_CLK_DIV_MASK);

	if ((*FabClkDivPtr < XRFDC_FAB_CLK_DIV1) ||
			(*FabClkDivPtr > XRFDC_FAB_CLK_DIV16)) {
		*FabClkDivPtr = XRFDC_FAB_CLK_DIV16;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API is to set the interpolation factor and also update the FIFO read
* words w.r.t to interpolation factor.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	InterpolationFactor to be set for DAC block.
*           XRFDC_INTERP_DECIM_* defines the valid values.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only DAC blocks
*
******************************************************************************/
u32 XRFdc_SetInterpolationFactor(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u32 InterpolationFactor)
{
	u32 Status;
	u32 BaseAddr;
	u16 FabricRate;
	u8 DataType;
	u32 Factor;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
				Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block digital path not "
								"enabled in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((InterpolationFactor != XRFDC_INTERP_DECIM_OFF) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_1X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_2X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_4X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_8X) &&
			((InstancePtr->RFdc_Config.IPType < 2) ||
			((InterpolationFactor != XRFDC_INTERP_DECIM_3X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_5X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_6X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_10X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_12X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_16X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_20X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_24X) &&
			(InterpolationFactor != XRFDC_INTERP_DECIM_40X)))) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Interpolation factor "
				"divider value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);

	DataType = XRFdc_ReadReg16(InstancePtr, BaseAddr,
					XRFDC_DAC_ITERP_DATA_OFFSET);
	if ((DataType == XRFDC_ADC_MIXER_MODE_IQ) &&
			(InterpolationFactor ==
				XRFDC_INTERP_DECIM_1X)) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR, "\n Invalid interpolation "
			"factor in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	/* Interpolation factor */
	Factor = InterpolationFactor;

	if (InstancePtr->RFdc_Config.IPType < 2) {
		if (InterpolationFactor == XRFDC_INTERP_DECIM_4X) {
			Factor = 0x3;
		}
		if (InterpolationFactor == XRFDC_INTERP_DECIM_8X) {
			Factor = 0x4;
		}
	}
	if (DataType == XRFDC_ADC_MIXER_MODE_IQ) {
		Factor |= Factor << ((InstancePtr->RFdc_Config.IPType < 2)?XRFDC_INTERP_MODE_Q_SHIFT:XRFDC_INTERP_MODE_Q_SHIFT_EXT);
	}

	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_DAC_INTERP_CTRL_OFFSET,
			(InstancePtr->RFdc_Config.IPType < 2)?XRFDC_INTERP_MODE_MASK:XRFDC_INTERP_MODE_MASK_EXT, Factor);

	/* Fabric rate */
	FabricRate = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_ADC_FABRIC_RATE_OFFSET, XRFDC_FAB_RATE_RD_MASK);
	FabricRate = FabricRate >> XRFDC_FAB_RATE_RD_SHIFT;
	if (DataType == XRFDC_ADC_MIXER_MODE_IQ) {
		switch (InterpolationFactor) {
			case XRFDC_INTERP_DECIM_2X:
			case XRFDC_INTERP_DECIM_3X:
				FabricRate = XRFDC_FAB_RATE_8;
				break;
			case XRFDC_INTERP_DECIM_4X:
				case XRFDC_INTERP_DECIM_5X:
				case XRFDC_INTERP_DECIM_6X:
				FabricRate = XRFDC_FAB_RATE_4;
				break;
			case XRFDC_INTERP_DECIM_8X:
				case XRFDC_INTERP_DECIM_10X:
				case XRFDC_INTERP_DECIM_12X:
				FabricRate = XRFDC_FAB_RATE_2;
				break;
			case XRFDC_INTERP_DECIM_16X:
			case XRFDC_INTERP_DECIM_20X:
			case XRFDC_INTERP_DECIM_24X:
			case XRFDC_INTERP_DECIM_40X:
				FabricRate = XRFDC_FAB_RATE_1;
				break;
			default:
				metal_log(METAL_LOG_DEBUG, "\n Interpolation block "
							"is OFF in %s\r\n", __func__);
				break;
		}
	} else {
		switch (InterpolationFactor) {
			case XRFDC_INTERP_DECIM_1X:
				FabricRate = XRFDC_FAB_RATE_8;
				break;
			case XRFDC_INTERP_DECIM_2X:
			case XRFDC_INTERP_DECIM_3X:
				FabricRate = XRFDC_FAB_RATE_4;
				break;
			case XRFDC_INTERP_DECIM_4X:
				case XRFDC_INTERP_DECIM_5X:
				case XRFDC_INTERP_DECIM_6X:
				FabricRate = XRFDC_FAB_RATE_2;
				break;
			case XRFDC_INTERP_DECIM_8X:
			case XRFDC_INTERP_DECIM_10X:
			case XRFDC_INTERP_DECIM_12X:
			case XRFDC_INTERP_DECIM_16X:
			case XRFDC_INTERP_DECIM_20X:
			case XRFDC_INTERP_DECIM_24X:
			case XRFDC_INTERP_DECIM_40X:
				FabricRate = XRFDC_FAB_RATE_1;
				break;
			default:
				metal_log(METAL_LOG_DEBUG, "\n Interpolation block "
							"is OFF in %s\r\n", __func__);
				break;
		}
	}
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_FABRIC_RATE_OFFSET,
		XRFDC_FAB_RATE_RD_MASK, (FabricRate << XRFDC_FAB_RATE_RD_SHIFT));

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Interpolation factor are returned back to the caller.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	InterpolationFactorPtr Pointer to return the interpolation factor
*			for DAC blocks.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only for DAC blocks
*
******************************************************************************/
u32 XRFdc_GetInterpolationFactor(XRFdc *InstancePtr, u32 Tile_Id,
							u32 Block_Id, u32 *InterpolationFactorPtr)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InterpolationFactorPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
				Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block digital path not "
								"enabled in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);

	if (InstancePtr->RFdc_Config.IPType < 2) {
		*InterpolationFactorPtr = XRFdc_RDReg(InstancePtr, BaseAddr,
		XRFDC_DAC_INTERP_CTRL_OFFSET, XRFDC_INTERP_MODE_I_MASK);
		if (*InterpolationFactorPtr == 0x3U) {
			*InterpolationFactorPtr = XRFDC_INTERP_DECIM_4X;
		} else if (*InterpolationFactorPtr == 0x4U) {
			*InterpolationFactorPtr = XRFDC_INTERP_DECIM_8X;
		}
	} else {
		*InterpolationFactorPtr = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_DAC_INTERP_CTRL_OFFSET, XRFDC_INTERP_MODE_I_MASK_EXT);
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Decimation factor are returned back to the caller.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	DecimationFactorPtr Pointer to return the Decimation factor
*			for DAC blocks.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_GetDecimationFactor(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u32 *DecimationFactorPtr)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(DecimationFactorPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
				Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block digital path not "
								"enabled in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
				(Block_Id == XRFDC_BLK_ID1)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Block_Id);



	if (InstancePtr->RFdc_Config.IPType < 2) {
		*DecimationFactorPtr = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_ADC_DECI_MODE_OFFSET, XRFDC_DEC_MOD_MASK);
		if (*DecimationFactorPtr == 0x3U) {
			*DecimationFactorPtr = XRFDC_INTERP_DECIM_4X;
		} else if (*DecimationFactorPtr == 0x4U) {
			*DecimationFactorPtr = XRFDC_INTERP_DECIM_8X;
		}
	} else {
		*DecimationFactorPtr = XRFdc_RDReg(InstancePtr, BaseAddr,
			XRFDC_ADC_DECI_MODE_OFFSET, XRFDC_DEC_MOD_MASK_EXT);
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Fabric data rate for the requested DAC block is set by writing to the
* corresponding register. The function writes the number of valid write words
* for the requested DAC block.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	FabricWrVldWords is write fabric rate to be set for DAC block.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only for DAC blocks
*
******************************************************************************/
u32 XRFdc_SetFabWrVldWords(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
								u32 FabricWrVldWords)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
				Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block digital path not "
								"enabled in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if (FabricWrVldWords > XRFDC_DAC_MAX_WR_FAB_RATE) {
		metal_log(METAL_LOG_ERROR, "\n Requested write valid words "
					"is Invalid in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_FABRIC_RATE_OFFSET,
		XRFDC_DAC_FAB_RATE_WR_MASK, FabricWrVldWords);

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;

}

/*****************************************************************************/
/**
*
* Fabric data rate for the requested ADC block is set by writing to the
* corresponding register. The function writes the number of valid read words
* for the requested ADC block.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC block number inside the tile. Valid values
*			are 0-3.
* @param	FabricRdVldWords is Read fabric rate to be set for ADC block.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_SetFabRdVldWords(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
								u32 FabricRdVldWords)
{
	u32 Status;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
				Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block digital path not "
								"enabled in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if (FabricRdVldWords > XRFDC_ADC_MAX_RD_FAB_RATE) {
		metal_log(METAL_LOG_ERROR, "\n Requested read "
			"valid words is Invalid in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr,
			XRFDC_ADC_FABRIC_RATE_OFFSET, XRFDC_FAB_RATE_RD_MASK,
			(FabricRdVldWords << XRFDC_FAB_RATE_RD_SHIFT));
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;

}

/*****************************************************************************/
/**
*
* This API returns the the number of fabric write valid words requested
* for the block.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	FabricWrVldWordsPtr Pointer to return the fabric data rate for
*			DAC block
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		ADC/DAC blocks
*
******************************************************************************/
u32 XRFdc_GetFabWrVldWords(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
					u32 Block_Id, u32 *FabricWrVldWordsPtr)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(FabricWrVldWordsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, Type, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block digital path not "
								"enabled in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Block_Id == XRFDC_BLK_ID1) && (Type == XRFDC_ADC_TILE)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Block_Id);

	*FabricWrVldWordsPtr = XRFdc_ReadReg16(InstancePtr, BaseAddr,
					XRFDC_ADC_FABRIC_RATE_OFFSET);
	if (Type == XRFDC_ADC_TILE) {
		*FabricWrVldWordsPtr &= XRFDC_ADC_FAB_RATE_WR_MASK;
	} else {
		*FabricWrVldWordsPtr &= XRFDC_DAC_FAB_RATE_WR_MASK;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API returns the the number of fabric read valid words requested
* for the block.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	FabricRdVldWordsPtr Pointer to return the fabric data rate for
*			ADC/DAC block
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		ADC/DAC blocks
*
******************************************************************************/
u32 XRFdc_GetFabRdVldWords(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
					u32 Block_Id, u32 *FabricRdVldWordsPtr)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(FabricRdVldWordsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, Type, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block digital path not "
								"enabled in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Block_Id == XRFDC_BLK_ID1) && (Type == XRFDC_ADC_TILE)) {
		Block_Id = XRFDC_BLK_ID2;
	}
	BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Block_Id);

	*FabricRdVldWordsPtr = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_ADC_FABRIC_RATE_OFFSET, XRFDC_FAB_RATE_RD_MASK);
	*FabricRdVldWordsPtr = (*FabricRdVldWordsPtr) >> XRFDC_FAB_RATE_RD_SHIFT;

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API is to clear the Sticky bit in threshold config registers.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	ThresholdToUpdate Select which Threshold (Threshold0 or
*           Threshold1 or both) to update.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only ADC blocks
*
******************************************************************************/
u32 XRFdc_ThresholdStickyClear(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
													u32 ThresholdToUpdate)
{
	u32 Status;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
					Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((ThresholdToUpdate != XRFDC_UPDATE_THRESHOLD_0) &&
		(ThresholdToUpdate != XRFDC_UPDATE_THRESHOLD_1) &&
		(ThresholdToUpdate != XRFDC_UPDATE_THRESHOLD_BOTH)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid ThresholdToUpdate "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
		if (InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Index].MixerInputDataType ==
				XRFDC_DATA_TYPE_IQ) {
			Index = Block_Id;
			NoOfBlocks = XRFDC_NUM_OF_BLKS3;
			if (Block_Id == XRFDC_BLK_ID1) {
				NoOfBlocks = XRFDC_NUM_OF_BLKS4;
			}
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks;) {
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		/* Update for Threshold0 */
		if ((ThresholdToUpdate == XRFDC_UPDATE_THRESHOLD_0) ||
			(ThresholdToUpdate == XRFDC_UPDATE_THRESHOLD_BOTH)) {
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD0_CFG_OFFSET, XRFDC_TRSHD0_STIKY_CLR_MASK,
				XRFDC_TRSHD0_STIKY_CLR_MASK);
		}
		/* Update for Threshold1 */
		if ((ThresholdToUpdate == XRFDC_UPDATE_THRESHOLD_1) ||
			(ThresholdToUpdate == XRFDC_UPDATE_THRESHOLD_BOTH)) {
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD1_CFG_OFFSET, XRFDC_TRSHD1_STIKY_CLR_MASK,
				XRFDC_TRSHD1_STIKY_CLR_MASK);
		}

		if ((InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Index].MixerInputDataType ==
				XRFDC_DATA_TYPE_IQ) && (XRFdc_IsHighSpeedADC(InstancePtr,
				Tile_Id) == 1)) {
			Index += XRFDC_BLK_ID2;
		} else {
			Index += XRFDC_BLK_ID1;
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API sets the threshold clear mode. The clear mode can be through
* explicit DRP access (manual) or auto clear (QMC gain update event).
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADCC block number inside the tile. Valid values
*			are 0-3.
* @param	ThresholdToUpdate Select which Threshold (Threshold0 or
*           Threshold1 or both) to update.
* @param	ClrMode can be DRP access (manual) or auto clear (QMC gain
*           update event).
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only ADC blocks
*
******************************************************************************/
u32 XRFdc_SetThresholdClrMode(XRFdc *InstancePtr, u32 Tile_Id,
							u32 Block_Id, u32 ThresholdToUpdate, u32 ClrMode)
{
	u32 Status;
	u16 ReadReg;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
					Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((ThresholdToUpdate != XRFDC_UPDATE_THRESHOLD_0) &&
			(ThresholdToUpdate != XRFDC_UPDATE_THRESHOLD_1) &&
			(ThresholdToUpdate != XRFDC_UPDATE_THRESHOLD_BOTH)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid ThresholdToUpdate "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	if ((ClrMode != XRFDC_THRESHOLD_CLRMD_MANUAL_CLR) &&
			(ClrMode != XRFDC_THRESHOLD_CLRMD_AUTO_CLR)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Clear mode "
				"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
		if (InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Index].MixerInputDataType ==
				XRFDC_DATA_TYPE_IQ) {
			Index = Block_Id;
			NoOfBlocks = XRFDC_NUM_OF_BLKS3;
			if (Block_Id == XRFDC_BLK_ID1) {
				NoOfBlocks = XRFDC_NUM_OF_BLKS4;
			}
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks;) {
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		/* Update for Threshold0 */
		if ((ThresholdToUpdate == XRFDC_UPDATE_THRESHOLD_0) ||
			(ThresholdToUpdate == XRFDC_UPDATE_THRESHOLD_BOTH)) {
			ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr,
						XRFDC_ADC_TRSHD0_CFG_OFFSET);
			if (ClrMode == XRFDC_THRESHOLD_CLRMD_MANUAL_CLR) {
				ReadReg &= ~XRFDC_TRSHD0_CLR_MOD_MASK;
			} else {
				ReadReg |= XRFDC_TRSHD0_CLR_MOD_MASK;
			}
			XRFdc_WriteReg16(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD0_CFG_OFFSET, ReadReg);
		}
		/* Update for Threshold1 */
		if ((ThresholdToUpdate == XRFDC_UPDATE_THRESHOLD_1) ||
			(ThresholdToUpdate == XRFDC_UPDATE_THRESHOLD_BOTH)) {
			ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr,
					XRFDC_ADC_TRSHD1_CFG_OFFSET);
			if (ClrMode == XRFDC_THRESHOLD_CLRMD_MANUAL_CLR) {
				ReadReg &= ~XRFDC_TRSHD1_CLR_MOD_MASK;
			} else {
				ReadReg |= XRFDC_TRSHD1_CLR_MOD_MASK;
			}
			XRFdc_WriteReg16(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD1_CFG_OFFSET, ReadReg);
		}
		if ((InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Index].MixerInputDataType ==
				XRFDC_DATA_TYPE_IQ) && (XRFdc_IsHighSpeedADC(InstancePtr,
				Tile_Id) == 1)) {
			Index += XRFDC_BLK_ID2;
		} else {
			Index += XRFDC_BLK_ID1;
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Threshold settings are updated into the relevant registers. Driver structure
* is updated with the new values. There can be two threshold settings:
* threshold0 and threshold1. Both of them are independent of each other.
* The function returns the requested threshold (which can be threshold0,
* threshold1, or both.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	ThresholdSettingsPtr Pointer through which the register settings for
*			thresholds are passed to the API.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only ADC blocks
*
******************************************************************************/
u32 XRFdc_SetThresholdSettings(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						XRFdc_Threshold_Settings *ThresholdSettingsPtr)
{
	u32 Status;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;
	XRFdc_Threshold_Settings *ThresholdConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(ThresholdSettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
					Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
		if (InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Index].MixerInputDataType ==
				XRFDC_DATA_TYPE_IQ) {
			Index = Block_Id;
			NoOfBlocks = XRFDC_NUM_OF_BLKS3;
			if (Block_Id == XRFDC_BLK_ID1) {
				NoOfBlocks = XRFDC_NUM_OF_BLKS4;
			}
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks;) {
		ThresholdConfigPtr = &InstancePtr->ADC_Tile[Tile_Id].
			ADCBlock_Analog_Datapath[Index].Threshold_Settings;
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);

		if ((ThresholdSettingsPtr->UpdateThreshold != XRFDC_UPDATE_THRESHOLD_0) &&
			(ThresholdSettingsPtr->UpdateThreshold != XRFDC_UPDATE_THRESHOLD_1) &&
			(ThresholdSettingsPtr->UpdateThreshold != XRFDC_UPDATE_THRESHOLD_BOTH)) {
			metal_log(METAL_LOG_ERROR, "\n Invalid UpdateThreshold "
				"value in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		if (((ThresholdSettingsPtr->UpdateThreshold ==
				XRFDC_UPDATE_THRESHOLD_0) ||
			(ThresholdSettingsPtr->UpdateThreshold ==
				XRFDC_UPDATE_THRESHOLD_BOTH)) &&
			(ThresholdSettingsPtr->ThresholdMode[0] > XRFDC_TRSHD_HYSTERISIS)) {
			metal_log(METAL_LOG_ERROR, "\n Requested threshold "
				"mode for threshold0 is invalid "
				"in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		if (((ThresholdSettingsPtr->UpdateThreshold ==
				XRFDC_UPDATE_THRESHOLD_1) ||
			(ThresholdSettingsPtr->UpdateThreshold ==
				XRFDC_UPDATE_THRESHOLD_BOTH)) &&
			(ThresholdSettingsPtr->ThresholdMode[1] > XRFDC_TRSHD_HYSTERISIS)) {
			metal_log(METAL_LOG_ERROR, "\n Requested threshold "
					"mode for threshold1 is invalid "
					"in %s\r\n", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}

		/* Update for Threshold0 */
		if ((ThresholdSettingsPtr->UpdateThreshold ==
				XRFDC_UPDATE_THRESHOLD_0) ||
			(ThresholdSettingsPtr->UpdateThreshold ==
				XRFDC_UPDATE_THRESHOLD_BOTH)) {
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TRSHD0_CFG_OFFSET,
				XRFDC_TRSHD0_EN_MOD_MASK,
				ThresholdSettingsPtr->ThresholdMode[0]);
			XRFdc_WriteReg16(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD0_AVG_LO_OFFSET,
				(u16)ThresholdSettingsPtr->ThresholdAvgVal[0]);
			XRFdc_WriteReg16(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD0_AVG_UP_OFFSET,
				(u16)(ThresholdSettingsPtr->ThresholdAvgVal[0] >>
						XRFDC_TRSHD0_AVG_UPP_SHIFT));
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD0_UNDER_OFFSET, XRFDC_TRSHD0_UNDER_MASK,
						ThresholdSettingsPtr->ThresholdUnderVal[0]);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD0_OVER_OFFSET, XRFDC_TRSHD0_OVER_MASK,
						ThresholdSettingsPtr->ThresholdOverVal[0]);

			ThresholdConfigPtr->ThresholdMode[0] =
					ThresholdSettingsPtr->ThresholdMode[0];
			ThresholdConfigPtr->ThresholdAvgVal[0] =
					ThresholdSettingsPtr->ThresholdAvgVal[0];
			ThresholdConfigPtr->ThresholdUnderVal[0] =
				ThresholdSettingsPtr->ThresholdUnderVal[0];
			ThresholdConfigPtr->ThresholdOverVal[0] =
				ThresholdSettingsPtr->ThresholdOverVal[0];
		}

		/* Update for Threshold1 */
		if ((ThresholdSettingsPtr->UpdateThreshold ==
				XRFDC_UPDATE_THRESHOLD_1) ||
			(ThresholdSettingsPtr->UpdateThreshold ==
				XRFDC_UPDATE_THRESHOLD_BOTH)) {
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD1_CFG_OFFSET, XRFDC_TRSHD1_EN_MOD_MASK,
						ThresholdSettingsPtr->ThresholdMode[1]);
			XRFdc_WriteReg16(InstancePtr, BaseAddr,
					XRFDC_ADC_TRSHD1_AVG_LO_OFFSET,
				(u16)ThresholdSettingsPtr->ThresholdAvgVal[1]);
			XRFdc_WriteReg16(InstancePtr, BaseAddr,
					XRFDC_ADC_TRSHD1_AVG_UP_OFFSET,
				(u16)(ThresholdSettingsPtr->ThresholdAvgVal[1] >>
						XRFDC_TRSHD1_AVG_UPP_SHIFT));
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD1_UNDER_OFFSET, XRFDC_TRSHD1_UNDER_MASK,
					ThresholdSettingsPtr->ThresholdUnderVal[1]);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr,
				XRFDC_ADC_TRSHD1_OVER_OFFSET, XRFDC_TRSHD1_OVER_MASK,
				ThresholdSettingsPtr->ThresholdOverVal[1]);

			ThresholdConfigPtr->ThresholdMode[1] =
					ThresholdSettingsPtr->ThresholdMode[1];
			ThresholdConfigPtr->ThresholdAvgVal[1] =
					ThresholdSettingsPtr->ThresholdAvgVal[1];
			ThresholdConfigPtr->ThresholdUnderVal[1] =
				ThresholdSettingsPtr->ThresholdUnderVal[1];
			ThresholdConfigPtr->ThresholdOverVal[1] =
				ThresholdSettingsPtr->ThresholdOverVal[1];
		}
		if ((InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Digital_Datapath[Index].MixerInputDataType ==
				XRFDC_DATA_TYPE_IQ) && (XRFdc_IsHighSpeedADC(InstancePtr,
				Tile_Id) ==	1)) {
			Index += XRFDC_BLK_ID2;
		} else {
			Index += XRFDC_BLK_ID1;
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Threshold settings are read from the corresponding registers and are passed
* back to the caller. There can be two threshold settings:
* threshold0 and threshold1. Both of them are independent of each other.
* The function returns the requested threshold (which can be threshold0,
* threshold1, or both.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param	ThresholdSettingsPtr Pointer through which the register settings
*			for thresholds are passed back..
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_GetThresholdSettings(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						XRFdc_Threshold_Settings *ThresholdSettingsPtr)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(ThresholdSettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
					Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Block_Id == XRFDC_BLK_ID1) && (InstancePtr->ADC_Tile[Tile_Id].
					ADCBlock_Digital_Datapath[Block_Id].MixerInputDataType !=
						XRFDC_DATA_TYPE_IQ)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Block_Id);

	/* Threshold mode */
	ThresholdSettingsPtr->UpdateThreshold = XRFDC_UPDATE_THRESHOLD_BOTH;
	ThresholdSettingsPtr->ThresholdMode[0] = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_ADC_TRSHD0_CFG_OFFSET, XRFDC_TRSHD0_EN_MOD_MASK);
	ThresholdSettingsPtr->ThresholdMode[1] = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_ADC_TRSHD1_CFG_OFFSET, XRFDC_TRSHD1_EN_MOD_MASK);

	/* Threshold Average Value */
	ThresholdSettingsPtr->ThresholdAvgVal[0] = XRFdc_ReadReg16(InstancePtr,
			BaseAddr, XRFDC_ADC_TRSHD0_AVG_LO_OFFSET);
	ThresholdSettingsPtr->ThresholdAvgVal[0] |= (XRFdc_ReadReg16(InstancePtr,
			BaseAddr, XRFDC_ADC_TRSHD0_AVG_UP_OFFSET) <<
					XRFDC_TRSHD0_AVG_UPP_SHIFT);
	ThresholdSettingsPtr->ThresholdAvgVal[1] = XRFdc_ReadReg16(InstancePtr,
			BaseAddr, XRFDC_ADC_TRSHD1_AVG_LO_OFFSET);
	ThresholdSettingsPtr->ThresholdAvgVal[1] |= (XRFdc_ReadReg16(InstancePtr,
			BaseAddr, XRFDC_ADC_TRSHD1_AVG_UP_OFFSET) <<
					XRFDC_TRSHD1_AVG_UPP_SHIFT);

	/* Threshold Under Value */
	ThresholdSettingsPtr->ThresholdUnderVal[0] = XRFdc_RDReg(InstancePtr,
			BaseAddr, XRFDC_ADC_TRSHD0_UNDER_OFFSET, XRFDC_TRSHD0_UNDER_MASK);
	ThresholdSettingsPtr->ThresholdUnderVal[1] = XRFdc_RDReg(InstancePtr,
			BaseAddr, XRFDC_ADC_TRSHD1_UNDER_OFFSET, XRFDC_TRSHD1_UNDER_MASK);

	/* Threshold Over Value */
	ThresholdSettingsPtr->ThresholdOverVal[0] = XRFdc_RDReg(InstancePtr,
			BaseAddr, XRFDC_ADC_TRSHD0_OVER_OFFSET, XRFDC_TRSHD0_OVER_MASK);
	ThresholdSettingsPtr->ThresholdOverVal[1] = XRFdc_RDReg(InstancePtr,
			BaseAddr, XRFDC_ADC_TRSHD1_OVER_OFFSET, XRFDC_TRSHD1_OVER_MASK);

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Decoder mode is updated into the relevant registers. Driver structure is
* updated with the new values.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is DAC block number inside the tile. Valid values
*			are 0-3.
* @param	DecoderMode Valid values are 1 (Maximum SNR, for non-
*			randomized decoder), 2 (Maximum Linearity, for randomized decoder)
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only DAC blocks
*
******************************************************************************/
u32 XRFdc_SetDecoderMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
								u32 DecoderMode)
{
	u32 Status;
	u32 *DecoderModeConfigPtr;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
					Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	DecoderModeConfigPtr = &InstancePtr->DAC_Tile[Tile_Id].
			DACBlock_Analog_Datapath[Block_Id].DecoderMode;
	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);

	if ((DecoderMode != XRFDC_DECODER_MAX_SNR_MODE) &&
		(DecoderMode != XRFDC_DECODER_MAX_LINEARITY_MODE)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid decoder mode "
						"in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_DAC_DECODER_CTRL_OFFSET,
					XRFDC_DEC_CTRL_MODE_MASK, DecoderMode);
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_DAC_DECODER_CLK_OFFSET,
					 XRFDC_DEC_CTRL_MODE_MASK, DecoderMode);
	*DecoderModeConfigPtr = DecoderMode;

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Decoder mode is read and returned back.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is DAC block number inside the tile. Valid values
*			are 0-3.
* @param	DecoderModePtr Valid values are 1 (Maximum SNR, for non-randomized
*			decoder), 2 (Maximum Linearity, for randomized decoder)
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only for DAC blocks
*
******************************************************************************/
u32 XRFdc_GetDecoderMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
								u32 *DecoderModePtr)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(DecoderModePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
					Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);
	*DecoderModePtr = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_DAC_DECODER_CTRL_OFFSET, XRFDC_DEC_CTRL_MODE_MASK);

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Resets the NCO phase of the current block phase accumulator.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		None
*
******************************************************************************/
u32 XRFdc_ResetNCOPhase(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 Block_Id)
{
	u32 Status;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, Type, Tile_Id,
						Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block digital path not "
								"enabled in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Type == XRFDC_ADC_TILE)) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Index);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_NCO_RST_OFFSET,
					XRFDC_NCO_PHASE_RST_MASK, XRFDC_NCO_PHASE_RST_MASK);
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;

}
/*****************************************************************************/
/**
*
* Enable and Disable the ADC/DAC FIFO.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param    Tile_Id Valid values are 0-3.
* @param    Enable valid values are 1 (FIFO enable) and 0 (FIFO Disable)
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Common API for ADC/DAC blocks
*
******************************************************************************/
u32 XRFdc_SetupFIFO(XRFdc *InstancePtr, u32 Type, int Tile_Id, u8 Enable)
{
	u32 Status;
	u32 BaseAddr;
	u16 NoOfTiles;
	u16 Index;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if ((Enable != 0U) && (Enable != 1U)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid enable "
						"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	/* An input tile if of -1 selects all tiles */
	if (Tile_Id == XRFDC_SELECT_ALL_TILES) {
		NoOfTiles = XRFDC_NUM_OF_TILES4;
		Index = XRFDC_TILE_ID0;
	} else {
		NoOfTiles = Tile_Id + 1;
		Index = Tile_Id;
	}

	for (; Index < NoOfTiles; Index++) {
		BaseAddr = XRFDC_CTRL_STS_BASE(Type, Index);

		Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Index);
		if ((Status != XRFDC_SUCCESS) && (Tile_Id != XRFDC_SELECT_ALL_TILES)) {
			metal_log(METAL_LOG_ERROR, "\n Requested tile%d not "
							"available in %s\r\n", Index, __func__);
			goto RETURN_PATH;
		} else if (Status != XRFDC_SUCCESS) {
			metal_log(METAL_LOG_DEBUG, "\n Tile%d not "
							"available in %s\r\n", Index, __func__);
			continue;
		} else {
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_FIFO_ENABLE,
					XRFDC_FIFO_EN_MASK, (!Enable));
		}
	}
	Status = XRFDC_SUCCESS;

RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Current status of ADC/DAC FIFO.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param    Tile_Id Valid values are 0-3.
* @param    EnablePtr valid values are 1 (FIFO enable) and 0 (FIFO Disable)
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Common API for ADC/DAC blocks
*
******************************************************************************/
u32 XRFdc_GetFIFOStatus(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
					u8 *EnablePtr)
{
	u32 Status;
	u32 BaseAddr;
	u32 ReadReg;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(EnablePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Tile_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested tile not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_CTRL_STS_BASE(Type, Tile_Id);

	ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_FIFO_ENABLE, XRFDC_FIFO_EN_MASK);
	*EnablePtr = (!ReadReg);

	Status = XRFDC_SUCCESS;

RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Get Output Current for DAC block.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param    OutputCurrPtr pointer to return the output current.
*
* @return
*		- Return Output Current for DAC block
*
******************************************************************************/
u32 XRFdc_GetOutputCurr(XRFdc *InstancePtr, u32 Tile_Id,
								u32 Block_Id, u32 *OutputCurrPtr)
{
	u32 Status;
	u32 BaseAddr;
	u16 ReadReg_Cfg2;
	u16 ReadReg_Cfg3;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(OutputCurrPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
								Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);

	ReadReg_Cfg2 = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_ADC_DAC_MC_CFG2_OFFSET, XRFDC_DAC_MC_CFG2_OPCSCAS_MASK);
	ReadReg_Cfg3 = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_DAC_MC_CFG3_OFFSET, XRFDC_DAC_MC_CFG3_CSGAIN_MASK);
	if ((ReadReg_Cfg2 == XRFDC_DAC_MC_CFG2_OPCSCAS_32MA) &&
			(ReadReg_Cfg3 == XRFDC_DAC_MC_CFG3_CSGAIN_32MA)) {
		*OutputCurrPtr = XRFDC_OUTPUT_CURRENT_32MA;
	} else if ((ReadReg_Cfg2 == XRFDC_DAC_MC_CFG2_OPCSCAS_20MA) &&
			(ReadReg_Cfg3 == XRFDC_DAC_MC_CFG3_CSGAIN_20MA)) {
		*OutputCurrPtr = XRFDC_OUTPUT_CURRENT_20MA;
	} else if ((ReadReg_Cfg2 == 0x0) && (ReadReg_Cfg3 == 0x0)) {
		*OutputCurrPtr = 0x0;
	} else {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR, "\n Invalid output "
					"current value %s\r\n", __func__);
		goto RETURN_PATH;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Set the Nyquist zone.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param    Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param    NyquistZone valid values are 1 (Odd),2 (Even).
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Common API for ADC/DAC blocks
*
******************************************************************************/
u32 XRFdc_SetNyquistZone(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 Block_Id, u32 NyquistZone)
{
	u32 Status;
	u16 ReadReg;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;
	u8 CalibrationMode = 0U;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, Type, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((NyquistZone != XRFDC_ODD_NYQUIST_ZONE) &&
			(NyquistZone != XRFDC_EVEN_NYQUIST_ZONE)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid NyquistZone "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Type == XRFDC_ADC_TILE)) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Index);
		if (Type == XRFDC_ADC_TILE) {
			/* Identify calibration mode */
			Status = XRFdc_GetCalibrationMode(InstancePtr,
				Tile_Id, Block_Id, &CalibrationMode);
			if (Status != XRFDC_SUCCESS) {
				return XRFDC_FAILURE;
			}

			if (CalibrationMode == XRFDC_CALIB_MODE1) {
				if (NyquistZone ==
					XRFDC_ODD_NYQUIST_ZONE) {
					NyquistZone =
						XRFDC_EVEN_NYQUIST_ZONE;
				} else {
					NyquistZone =
						XRFDC_ODD_NYQUIST_ZONE;
				}
			}
			ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr,
					XRFDC_ADC_TI_TISK_CRL0_OFFSET);
			if ((NyquistZone % 2U) == 0U) {
				ReadReg |= XRFDC_TI_TISK_ZONE_MASK;
			} else {
				ReadReg &= ~XRFDC_TI_TISK_ZONE_MASK;
			}

			XRFdc_WriteReg16(InstancePtr, BaseAddr,
				XRFDC_ADC_TI_TISK_CRL0_OFFSET, ReadReg);
			InstancePtr->ADC_Tile[Tile_Id].
				ADCBlock_Analog_Datapath[Index].
					NyquistZone = NyquistZone;
		} else {
			ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr,
						XRFDC_DAC_MC_CFG0_OFFSET);
			if ((NyquistZone % 2U) == 0U) {
				ReadReg |= XRFDC_MC_CFG0_MIX_MODE_MASK;
			} else {
				ReadReg &= ~XRFDC_MC_CFG0_MIX_MODE_MASK;
			}

			XRFdc_WriteReg16(InstancePtr, BaseAddr,
					XRFDC_DAC_MC_CFG0_OFFSET, ReadReg);
			InstancePtr->DAC_Tile[Tile_Id].
				DACBlock_Analog_Datapath[Index].
						NyquistZone = NyquistZone;
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Get the Nyquist zone.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param    Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param    NyquistZonePtr Pointer to return the Nyquist zone.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Common API for ADC/DAC blocks
*
******************************************************************************/
u32 XRFdc_GetNyquistZone(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 Block_Id, u32 *NyquistZonePtr)
{
	u32 Status;
	u16 ReadReg;
	u32 BaseAddr;
	u32 Block;
	u8 CalibrationMode = 0U;
	u8 MultibandConfig;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(NyquistZonePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if (Type == XRFDC_ADC_TILE) {
		MultibandConfig = InstancePtr->ADC_Tile[Tile_Id].MultibandConfig;
	} else {
		MultibandConfig = InstancePtr->DAC_Tile[Tile_Id].MultibandConfig;
	}

	if (MultibandConfig != XRFDC_MB_MODE_SB) {
		Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, Type,
					Tile_Id, Block_Id);
	} else {
		Status = XRFdc_CheckBlockEnabled(InstancePtr, Type,
					Tile_Id, Block_Id);
	}
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	Block = Block_Id;
	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Block_Id == XRFDC_BLK_ID1) && (Type == XRFDC_ADC_TILE)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, Block_Id);

	if (Type == XRFDC_ADC_TILE) {
		/* Identify calibration mode */
		Status = XRFdc_GetCalibrationMode(InstancePtr, Tile_Id,
					Block, &CalibrationMode);
		if (Status != XRFDC_SUCCESS) {
			return XRFDC_FAILURE;
		}
		ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_ADC_TI_TISK_CRL0_OFFSET, XRFDC_TI_TISK_ZONE_MASK);
		*NyquistZonePtr = (ReadReg >> XRFDC_TISK_ZONE_SHIFT);
	} else {
		ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_DAC_MC_CFG0_OFFSET, XRFDC_MC_CFG0_MIX_MODE_MASK);
		*NyquistZonePtr = (ReadReg >> XRFDC_MC_CFG0_MIX_MODE_SHIFT);
	}
	if (*NyquistZonePtr == 0U) {
		*NyquistZonePtr = XRFDC_ODD_NYQUIST_ZONE;
	} else {
		*NyquistZonePtr = XRFDC_EVEN_NYQUIST_ZONE;
	}

	if ((Type == XRFDC_ADC_TILE) &&
			(CalibrationMode == XRFDC_CALIB_MODE1)) {
		if (*NyquistZonePtr == XRFDC_EVEN_NYQUIST_ZONE) {
			*NyquistZonePtr = XRFDC_ODD_NYQUIST_ZONE;
		} else {
			*NyquistZonePtr = XRFDC_EVEN_NYQUIST_ZONE;
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API is to set the DAC Datapath mode.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param    Mode valid values are 0-3.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled / out of range.
*
* @note		Only for DAC blocks
*
******************************************************************************/
u32 XRFdc_SetDataPathMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u32 Mode)
{
	u32 Status = XRFDC_SUCCESS;
	u32 BaseAddr;
	u32 NyquistZone;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		return Status;
	}

	if (Mode > XRFDC_DAC_MODE_MAX) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Mode "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		return Status;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_DAC_DATAPATH_OFFSET,
			XRFDC_DATAPATH_MODE_MASK, Mode);

	NyquistZone = (Mode == XRFDC_DAC_MODE_7G_NQ2) ?
			XRFDC_EVEN_NYQUIST_ZONE : XRFDC_ODD_NYQUIST_ZONE;
	XRFdc_SetNyquistZone(InstancePtr, XRFDC_DAC_TILE, Tile_Id, Block_Id, NyquistZone);
	return Status;
}

/*****************************************************************************/
/**
*
* This API is to get the DAC Datapath mode.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param    ModePtr pointer used to return value.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Only for DAC blocks
*
******************************************************************************/
u32 XRFdc_GetDataPathMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u32 *ModePtr)
{
	u32 Status = XRFDC_SUCCESS;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);
	Xil_AssertNonvoid(ModePtr != NULL);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		return Status;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);
	*ModePtr = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_DAC_DATAPATH_OFFSET,
			XRFDC_DATAPATH_MODE_MASK);
	return Status;
}

/*****************************************************************************/
/**
*
* This API is to set the DAC Image Reject Filter Pass mode.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param    Mode valid values are 0 (for low pass) 1 (for high pass).
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled / bad parameter passed
*
* @note		Only for DAC blocks
*
******************************************************************************/
u32 XRFdc_SetIMRPassMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u32 Mode)
{
	u32 Status = XRFDC_SUCCESS;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		return Status;
	}

	if (Mode > XRFDC_DAC_IMR_MODE_MAX) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Mode "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		return Status;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_DAC_DATAPATH_OFFSET,
			XRFDC_DATAPATH_IMR_MASK, Mode << 2);

	return Status;
}

/*****************************************************************************/
/**
*
* This API is to get the DAC Image Reject Filter Pass mode.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param     ModePtr pointer used to return value.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Only for DAC blocks
*
******************************************************************************/
u32 XRFdc_GetIMRPassMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u32 *ModePtr)
{
	u32 Status = XRFDC_SUCCESS;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);
	Xil_AssertNonvoid(ModePtr != NULL);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		return Status;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);
	*ModePtr = (XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_DAC_DATAPATH_OFFSET,
			XRFDC_DATAPATH_IMR_MASK)) >> 2;
	return Status;
}
/*****************************************************************************/
/**
*
* This API is to set the Calibration mode.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param    CalibrationMode valid values are 1 and 2.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_SetCalibrationMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u8 CalibrationMode)
{
	u32 Status;
	u16 ReadReg;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;
	XRFdc_Mixer_Settings Mixer_Settings = {0};
	u32 NyquistZone = 0U;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if ((CalibrationMode != XRFDC_CALIB_MODE1) &&
			(CalibrationMode != XRFDC_CALIB_MODE2)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Calibration mode "
					"value in %s\r\n", __func__);
		return XRFDC_FAILURE;
	}

	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	/* Get Mixer Configurations */
	Status = XRFdc_GetMixerSettings(InstancePtr, XRFDC_ADC_TILE,
					Tile_Id, Block_Id, &Mixer_Settings);
	if (Status != XRFDC_SUCCESS) {
		return XRFDC_FAILURE;
	}

	/* Get Nyquist Zone */
	Status = XRFdc_GetNyquistZone(InstancePtr, XRFDC_ADC_TILE,
					Tile_Id, Block_Id, &NyquistZone);
	if (Status != XRFDC_SUCCESS) {
		return XRFDC_FAILURE;
	}

	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_ADC_TILE_DRP_ADDR(Tile_Id) +
					XRFDC_BLOCK_ADDR_OFFSET(Index);
		ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr,
						XRFDC_ADC_TI_DCB_CRL0_OFFSET);
		ReadReg &= ~XRFDC_TI_DCB_MODE_MASK;
		if (CalibrationMode == XRFDC_CALIB_MODE1) {
			if (((Index % 2U) != 0U) &&
				(XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1)) {
				ReadReg |= XRFDC_TI_DCB_MODE1_4GSPS;
			} else if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 0) {
				ReadReg |= XRFDC_TI_DCB_MODE1_2GSPS;
			}
		}
		XRFdc_WriteReg16(InstancePtr, BaseAddr,
					XRFDC_ADC_TI_DCB_CRL0_OFFSET, ReadReg);
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Analog_Datapath[Index].
					CalibrationMode = CalibrationMode;
	}

	/* Set Nyquist Zone */
	Status = XRFdc_SetNyquistZone(InstancePtr, XRFDC_ADC_TILE,
					Tile_Id, Block_Id, NyquistZone);
	if (Status != XRFDC_SUCCESS) {
		return XRFDC_FAILURE;
	}

	/* Set Mixer Configurations */
	Status = XRFdc_SetMixerSettings(InstancePtr, XRFDC_ADC_TILE,
					Tile_Id, Block_Id, &Mixer_Settings);
	if (Status != XRFDC_SUCCESS) {
		return XRFDC_FAILURE;
	}

	Status = XRFDC_SUCCESS;

	return Status;
}

/*****************************************************************************/
/**
*
* This API is to get the Calibration mode.
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param    Tile_Id Valid values are 0-3.
* @param	Block_Id is ADC/DAC block number inside the tile. Valid values
*			are 0-3.
* @param    CalibrationModePtr pointer to get the calibration mode.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_GetCalibrationMode(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
						u8 *CalibrationModePtr)
{
	u32 Status;
	u16 ReadReg;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(CalibrationModePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if (InstancePtr->ADC_Tile[Tile_Id].MultibandConfig != XRFDC_MB_MODE_SB) {
		Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, XRFDC_ADC_TILE,
					Tile_Id, Block_Id);
	} else {
		Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE,
					Tile_Id, Block_Id);
	}
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		if (Block_Id == XRFDC_BLK_ID1) {
			Block_Id = XRFDC_BLK_ID3;
		}
		if (Block_Id == XRFDC_BLK_ID0) {
			Block_Id = XRFDC_BLK_ID1;
		}
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Block_Id);
	ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_ADC_TI_DCB_CRL0_OFFSET, XRFDC_TI_DCB_MODE_MASK);
	if (ReadReg != 0U) {
		*CalibrationModePtr = XRFDC_CALIB_MODE1;
	} else {
		*CalibrationModePtr = XRFDC_CALIB_MODE2;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API is used to set the mode for the Inverse-Sinc filter.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is DAC block number inside the tile. Valid values
*			are 0-3.
* @param	Mode valid values are 0(disable),  1(1st Nyquist zone)
			and 2(2nd Nyquist zone).
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled/invalid mode.
*
* @note		Only DAC blocks
*
******************************************************************************/
u32 XRFdc_SetInvSincFIR(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
								u16 Mode)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if (Mode > ((InstancePtr->RFdc_Config.IPType < 2)?XRFDC_INV_SYNC_EN_MAX:XRFDC_INV_SYNC_MODE_MAX)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid mode "
						"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
					Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_DAC_INVSINC_OFFSET,
					(InstancePtr->RFdc_Config.IPType < 2)?XRFDC_EN_INVSINC_MASK:XRFDC_MODE_INVSINC_MASK, Mode);
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This API is used to get the Inverse-Sinc filter mode.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3.
* @param	Block_Id is DAC block number inside the tile. Valid values
*			are 0-3.
* @param	ModePtr is a pointer to get the inv-sinc status. valid values
*           are 0(disable),  1(1st Nyquist zone) and 2(2nd Nyquist zone).
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Only DAC blocks
*
******************************************************************************/
u32 XRFdc_GetInvSincFIR(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
								u16 *ModePtr)
{
	u32 Status;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);
	Xil_AssertNonvoid(ModePtr != NULL);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_DAC_TILE, Tile_Id,
					Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, Block_Id);
	*ModePtr = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_DAC_INVSINC_OFFSET, (InstancePtr->RFdc_Config.IPType < 2)?XRFDC_EN_INVSINC_MASK:XRFDC_MODE_INVSINC_MASK);
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* Static API to dump ADC registers.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3, and -1.
*
* @return
*			None
*
* @note		None
*
******************************************************************************/
static void XRFdc_DumpADCRegs(XRFdc *InstancePtr, int Tile_Id)
{
	u32 BlockId;
	u32 Block;
	u32 IsBlockAvail;
	u32 Offset;
	u32 BaseAddr;
	u32 ReadReg = 0U;

	for (BlockId = XRFDC_BLK_ID0; BlockId < XRFDC_BLK_ID4; BlockId++) {
		Block = BlockId;
		if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
			if (BlockId == XRFDC_BLK_ID1) {
				Block = XRFDC_BLK_ID0;
			}
			if ((BlockId == XRFDC_BLK_ID3) || (BlockId == XRFDC_BLK_ID2)) {
				Block = XRFDC_BLK_ID1;
			}
		}
		IsBlockAvail = XRFdc_IsADCBlockEnabled(InstancePtr, Tile_Id,
						Block);
		if (IsBlockAvail == 0U) {
			IsBlockAvail = XRFdc_IsADCDigitalPathEnabled(InstancePtr, Tile_Id,
											Block);
			if (IsBlockAvail == 0U) {
				continue;
			}
		}
		metal_log(METAL_LOG_DEBUG, "\n ADC%d%d:: \r\n", Tile_Id, BlockId);
		BaseAddr = XRFDC_ADC_TILE_DRP_ADDR(Tile_Id) +
				XRFDC_BLOCK_ADDR_OFFSET(BlockId);
		for (Offset = 0x0U; Offset <= 0x284U; Offset += 0x4U) {
			if ((Offset >= 0x24U && Offset <= 0x2CU) ||
					(Offset >= 0x48U && Offset <= 0x7CU) ||
					(Offset >= 0xACU && Offset <= 0xC4U) ||
					(Offset >= 0x114U && Offset <= 0x13CU) ||
					(Offset >= 0x188U && Offset <= 0x194U) ||
					(Offset >= 0x1B8U && Offset <= 0x1BCU) ||
					(Offset >= 0x1D8U && Offset <= 0x1FCU) ||
					(Offset >= 0x240U && Offset <= 0x27CU)) {
				continue;
			}
			ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr, Offset);
			metal_log(METAL_LOG_DEBUG,
			"\n offset = 0x%x and Value = 0x%x \t",
				Offset, ReadReg);
		}
	}
	(void)ReadReg;
}

/*****************************************************************************/
/**
*
* Static API to dump DAC registers.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id Valid values are 0-3, and -1.
*
* @return
*			None
*
* @note		None
*
******************************************************************************/
static void XRFdc_DumpDACRegs(XRFdc *InstancePtr, int Tile_Id)
{
	u32 BlockId;
	u32 IsBlockAvail;
	u32 Offset;
	u32 BaseAddr;
	u32 ReadReg = 0U;

	for (BlockId = XRFDC_BLK_ID0; BlockId < XRFDC_BLK_ID4; BlockId++) {
		IsBlockAvail = XRFdc_IsDACBlockEnabled(InstancePtr, Tile_Id,
									BlockId);
		if (IsBlockAvail == 0U) {
			IsBlockAvail = XRFdc_IsDACDigitalPathEnabled(InstancePtr, Tile_Id,
							BlockId);
			if (IsBlockAvail == 0U) {
				continue;
			}
		}
		metal_log(METAL_LOG_DEBUG, "\n DAC%d%d:: \r\n", Tile_Id, BlockId);
		BaseAddr = XRFDC_DAC_TILE_DRP_ADDR(Tile_Id) +
				XRFDC_BLOCK_ADDR_OFFSET(BlockId);
		for (Offset = 0x0U; Offset <= 0x24CU; Offset += 0x4U) {
			if ((Offset >= 0x28U && Offset <= 0x34U) ||
					(Offset >= 0x48U && Offset <= 0x7CU) ||
					(Offset >= 0xA8U && Offset <= 0xBCU) ||
					(Offset >= 0xE4U && Offset <= 0xFCU) ||
					(Offset >= 0x16CU && Offset <= 0x17CU) ||
					(Offset >= 0x198U && Offset <= 0x1BCU) ||
					(Offset >= 0x1ECU && Offset <= 0x1FCU) ||
					(Offset >= 0x204U && Offset <= 0x23CU)) {
				continue;
			}
			ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr, Offset);
			metal_log(METAL_LOG_DEBUG,
			"\n offset = 0x%x and Value = 0x%x \t",
			Offset, ReadReg);
		}
	}
	(void)ReadReg;
}

/*****************************************************************************/
/**
*
* Static API to dump HSCOM registers.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3, and -1.
*
* @return
*			None
*
* @note		None
*
******************************************************************************/
static void XRFdc_DumpHSCOMRegs(XRFdc *InstancePtr, u32 Type, int Tile_Id)
{
	u32 Offset;
	u32 BaseAddr;
	u32 ReadReg = 0U;

	if (Type == XRFDC_ADC_TILE) {
		metal_log(METAL_LOG_DEBUG, "\n ADC%d HSCOM:: \r\n", Tile_Id);
		BaseAddr = XRFDC_ADC_TILE_DRP_ADDR(Tile_Id) + XRFDC_HSCOM_ADDR;

	} else {
		metal_log(METAL_LOG_DEBUG, "\n DAC%d HSCOM:: \r\n", Tile_Id);
		BaseAddr = XRFDC_DAC_TILE_DRP_ADDR(Tile_Id) + XRFDC_HSCOM_ADDR;
	}
	metal_log(METAL_LOG_DEBUG, "\n Offset\tValue \r\n");
	for (Offset = 0x0U; Offset <= 0x148U; Offset += 0x4U) {
		if ((Offset >= 0x60U && Offset <= 0x88U) ||
			(Offset == 0xBCU) ||
			(Offset >= 0xC4U && Offset <= 0xFCU) ||
			(Offset >= 0x110U && Offset <= 0x11CU) ||
			(Offset >= 0x12CU && Offset <= 0x13CU)) {
			continue;
		}
		ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr, Offset);
		metal_log(METAL_LOG_DEBUG, "\n 0x%x \t 0x%x \t",
					Offset, ReadReg);
	}
	(void)ReadReg;
}

/*****************************************************************************/
/**
*
* This Prints the offset of the register along with the content. This API is
* meant to be used for debug purposes. It prints to the console the contents
* of registers for the passed Tile_Id. If -1 is passed, it prints the contents
* of the registers for all the tiles for the respective ADC or DAC
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3, and -1.
*
* @return
*			None
*
* @note		None
*
******************************************************************************/
void XRFdc_DumpRegs(XRFdc *InstancePtr, u32 Type, int Tile_Id)
{
	u16 NoOfTiles;
	u16 Index;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if (Tile_Id == XRFDC_SELECT_ALL_TILES) {
		NoOfTiles = XRFDC_NUM_OF_TILES4;
	} else {
		NoOfTiles = XRFDC_NUM_OF_TILES1;
	}
	for (Index = XRFDC_TILE_ID0; Index < NoOfTiles; Index++) {
		if (NoOfTiles == XRFDC_NUM_OF_TILES4) {
			Tile_Id = Index;
		}
		if (Type == XRFDC_ADC_TILE) {
			XRFdc_DumpADCRegs(InstancePtr, Tile_Id);
		} else {
			XRFdc_DumpDACRegs(InstancePtr, Tile_Id);
		}
		XRFdc_DumpHSCOMRegs(InstancePtr, Type, Tile_Id);
	}
}

/*****************************************************************************/
/**
*
* This is a stub for the status callback. The stub is here in case the upper
* layers forget to set the handler.
*
* @param	CallBackRefPtr is a pointer to the upper layer callback reference.
* @param	Type indicates ADC/DAC.
* @param	Tile_Id indicates Tile number (0-3).
* @param	Block_Id indicates Block number (0-3).
* @param	StatusEvent indicates one or more interrupt occurred.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void StubHandler(void *CallBackRefPtr, u32 Type, u32 Tile_Id,
								u32 Block_Id, u32 StatusEvent)
{
	(void) ((void *)CallBackRefPtr);
	(void) Type;
	(void) Tile_Id;
	(void) Block_Id;
	(void) StatusEvent;

	Xil_AssertVoidAlways();

}
/*****************************************************************************/
/**
*
* This function is used to get the Link Coupling mode.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for 2G, 0-1 for 4G).
* @param	ModePtr pointer to get link coupling mode.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_GetLinkCoupling(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
							u32 *ModePtr)
{
	u32 Status;
	u16 ReadReg;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(ModePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
								Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Block_Id == XRFDC_BLK_ID1)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Block_Id);

	ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_ADC_RXPR_MC_CFG0_OFFSET, XRFDC_RX_MC_CFG0_CM_MASK);
	if (ReadReg != 0U) {
		*ModePtr = XRFDC_LINK_COUPLING_AC;
	} else {
		*ModePtr = XRFDC_LINK_COUPLING_DC;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This function is used to set the IM3 Dither mode.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for LS, 0-1 for HS).
* @param	Mode 0: Disable
*				 1: Enable
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_SetDither(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
							u32 Mode)
{
	u32 Status;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
								Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if (Mode > XRFDC_DITH_ENABLE) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR, "\n Invalid Dither Mode "
								"in %s\r\n", __func__);
		goto RETURN_PATH;
	}
	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr,
			XRFDC_ADC_DAC_MC_CFG0_OFFSET, XRFDC_RX_MC_CFG0_IM3_DITH_MASK,
			(Mode << XRFDC_RX_MC_CFG0_IM3_DITH_SHIFT));
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This function is used to get the IM3 Dither mode.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for LS, 0-1 for HS).
* @param	ModePtr pointer to get link coupling mode.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_GetDither(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id,
							u32 *ModePtr)
{
	u32 Status;
	u16 ReadReg;
	u32 BaseAddr;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(ModePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id,
								Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested block not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
			(Block_Id == XRFDC_BLK_ID1)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Block_Id);

	ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr,
					XRFDC_ADC_DAC_MC_CFG0_OFFSET, XRFDC_RX_MC_CFG0_IM3_DITH_MASK);
	if (ReadReg != 0U) {
		*ModePtr = XRFDC_DITH_ENABLE;
	} else {
		*ModePtr = XRFDC_DITH_DISABLE;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This function is used to set the ADC Signal Detector Settings.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for LS, 0-1 for HS).
* @param    SettingsPtr pointer to the XRFdc_Signal_Detector_Settings structure
*           to set the signal detector configurations
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled, or invaid values.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_SetSignalDetector(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, XRFdc_Signal_Detector_Settings *SettingsPtr)
{
	u32 Status;
	u32 BaseAddr;
	u32 Index;
	u32 NoOfBlocks;
	u16 SignalDetCtrlReg = 0;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(SettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if (InstancePtr->RFdc_Config.IPType < 2) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Requested fuctionality not "
			  "available for this IP in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id, Block_Id);

	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Requested block not "
			  "available in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	if (SettingsPtr->Mode > XRFDC_SIGDET_MODE_RNDM) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Signal Detector "
			  "Mode in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	if (SettingsPtr->EnableIntegrator > XRFDC_ENABLED) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Signal Detector "
			  "Integrator Enable in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	if (SettingsPtr->HysteresisEnable > XRFDC_ENABLED) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Signal Detector "
			  "Hysteresis Enable in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	if (SettingsPtr->Flush > XRFDC_ENABLED) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Signal Detector "
			  "Flush Option in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	if (SettingsPtr->TimeConstant > XRFDC_SIGDET_TC_2_18) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Signal Detector "
			  "Time Constant in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	SignalDetCtrlReg |= SettingsPtr->EnableIntegrator << XRFDC_ADC_SIG_DETECT_INTG_SHIFT;
	SignalDetCtrlReg |= SettingsPtr->Flush << XRFDC_ADC_SIG_DETECT_FLUSH_SHIFT;
	SignalDetCtrlReg |= SettingsPtr->TimeConstant << XRFDC_ADC_SIG_DETECT_TCONST_SHIFT;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		SignalDetCtrlReg |= ((SettingsPtr->Mode << 1) | 1) << XRFDC_ADC_SIG_DETECT_MODE_WRITE_SHIFT;
	} else {
		SignalDetCtrlReg |= (SettingsPtr->Mode << 1) << XRFDC_ADC_SIG_DETECT_MODE_WRITE_SHIFT;
	}
	SignalDetCtrlReg |= SettingsPtr->HysteresisEnable << XRFDC_ADC_SIG_DETECT_HYST_SHIFT;

	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_SIG_DETECT_CTRL_OFFSET, XRFDC_ADC_SIG_DETECT_MASK,
				SignalDetCtrlReg);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_SIG_DETECT_THRESHOLD0_LEVEL_OFFSET,
				XRFDC_ADC_SIG_DETECT_THRESH_MASK, SettingsPtr->HighThreshold);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_SIG_DETECT_THRESHOLD1_LEVEL_OFFSET,
				XRFDC_ADC_SIG_DETECT_THRESH_MASK, SettingsPtr->LowThreshold);
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This function is used to get the ADC Signal Detector Settings.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for LS, 0-1 for HS).
* @param    SettingsPtr pointer to the XRFdc_Signal_Detector_Settings structure
*           to get the signal detector configurations
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_GetSignalDetector(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, XRFdc_Signal_Detector_Settings *SettingsPtr)
{
	u32 Status;
	u32 BaseAddr;
	u16 SignalDetCtrlReg = 0;
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(SettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if (InstancePtr->RFdc_Config.IPType < 2) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Requested functionality not "
			  "available for this IP in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id, Block_Id);

	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Requested block not "
			  "available in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) && (Block_Id == XRFDC_BLK_ID1)) {
		Block_Id = XRFDC_BLK_ID2;
	}

	BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Block_Id);

	SignalDetCtrlReg =
		XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_ADC_SIG_DETECT_CTRL_OFFSET, XRFDC_ADC_SIG_DETECT_MASK);
	SettingsPtr->EnableIntegrator =
		(SignalDetCtrlReg & XRFDC_ADC_SIG_DETECT_INTG_MASK) >> XRFDC_ADC_SIG_DETECT_INTG_SHIFT;
	SettingsPtr->Flush = (SignalDetCtrlReg & XRFDC_ADC_SIG_DETECT_FLUSH_MASK) >> XRFDC_ADC_SIG_DETECT_FLUSH_SHIFT;
	SettingsPtr->TimeConstant =
		(SignalDetCtrlReg & XRFDC_ADC_SIG_DETECT_TCONST_MASK) >> XRFDC_ADC_SIG_DETECT_TCONST_SHIFT;
	SettingsPtr->Mode = (SignalDetCtrlReg & XRFDC_ADC_SIG_DETECT_MODE_MASK) >> XRFDC_ADC_SIG_DETECT_MODE_READ_SHIFT;

	SettingsPtr->HysteresisEnable =
		(SignalDetCtrlReg & XRFDC_ADC_SIG_DETECT_HYST_MASK) >> XRFDC_ADC_SIG_DETECT_HYST_SHIFT;
	SettingsPtr->HighThreshold = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_ADC_SIG_DETECT_THRESHOLD0_LEVEL_OFFSET,
						 XRFDC_ADC_SIG_DETECT_THRESH_MASK);
	SettingsPtr->LowThreshold = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_ADC_SIG_DETECT_THRESHOLD1_LEVEL_OFFSET,
						XRFDC_ADC_SIG_DETECT_THRESH_MASK);
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This function is used to disable Calibration Coefficients override.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for LS, 0-1 for HS).
* @param	CalibrationBlock indicates the calibration block.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if error occurs.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_DisableCoefficientsOverride(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, u32 CalibrationBlock)
{
	u32 BaseAddr;
	u32 Status;
	u32 Index;
	u32 NoOfBlocks;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Requested block not "
			  "available in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	if ((InstancePtr->RFdc_Config.IPType < 2) && (CalibrationBlock == XRFDC_CAL_BLOCK_OCB1)) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Requested functionality not "
			  "available for this IP in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		switch (CalibrationBlock) {
		case XRFDC_CAL_BLOCK_OCB1:
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL1_OFFSET, XRFDC_CAL_OCB_EN_MASK,
					XRFDC_DISABLED);
			break;
		case XRFDC_CAL_BLOCK_OCB2:
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL3_OFFSET, XRFDC_CAL_OCB_EN_MASK,
					XRFDC_DISABLED);
			break;
		case XRFDC_CAL_BLOCK_GCB:
			if (InstancePtr->RFdc_Config.IPType < 2) {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL1_OFFSET,
						XRFDC_CAL_GCB_ENFL_MASK, XRFDC_CAL_GCB_ACEN_MASK);
				/*Clear IP Override Coeffs*/
				XRFdc_ClrSetReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
						XRFDC_CAL_GCB_COEFF0_FAB(Index), XRFDC_CAL_GCB_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
						XRFDC_CAL_GCB_COEFF1_FAB(Index), XRFDC_CAL_GCB_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
						XRFDC_CAL_GCB_COEFF2_FAB(Index), XRFDC_CAL_GCB_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
						XRFDC_CAL_GCB_COEFF3_FAB(Index), XRFDC_CAL_GCB_MASK, XRFDC_DISABLED);
			} else {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL2_OFFSET,
						XRFDC_CAL_GCB_EN_MASK, XRFDC_DISABLED);
			}
			break;
		case XRFDC_CAL_BLOCK_TSCB:
			if (InstancePtr->RFdc_Config.IPType < 2) {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF0_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF1_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF2_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF3_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF4_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF5_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF6_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF7_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
			} else {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF0,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF1,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF2,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF3,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF4,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF5,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF6,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF7,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_DISABLED);
			}
			break;
		default:
			Status = XRFDC_FAILURE;
			metal_log(METAL_LOG_ERROR,
				  "\n Invalid Calibration "
				  "Mode in %s\r\n",
				  __func__);
			goto RETURN_PATH;
		}
	}
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This function is used to set the ADC Calibration Coefficients.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for LS, 0-1 for HS).
* @param	CalibrationBlock indicates the block to be written to.
* @param    CoeffPtr is pointer to the XRFdc_Calibration_Coefficients structure
*           to set the calibration coefficients.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if error occurs.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_SetCalCoefficients(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, u32 CalibrationBlock,
			     XRFdc_Calibration_Coefficients *CoeffPtr)
{
	u32 BaseAddr;
	u32 Status;
	u32 Index;
	u32 NoOfBlocks;
	u32 HighSpeed;
	u32 Shift;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(CoeffPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if ((InstancePtr->RFdc_Config.IPType < 2) && (CalibrationBlock == XRFDC_CAL_BLOCK_OCB1)) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Requested functionality not "
			  "available for this IP in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	if (CalibrationBlock == XRFDC_CAL_BLOCK_GCB) {
		if ((CoeffPtr->Coeff0 | CoeffPtr->Coeff1 | CoeffPtr->Coeff2 | CoeffPtr->Coeff3) &
		    ~(XRFDC_CAL_GCB_MASK | (XRFDC_CAL_GCB_MASK << XRFDC_CAL_SLICE_SHIFT))) {
			Status = XRFDC_FAILURE;
			metal_log(METAL_LOG_ERROR,
				  "\n Bad Coefficient "
				  "available for this IP in %s\r\n",
				  __func__);
			goto RETURN_PATH;
		}
	}

	if (CalibrationBlock == XRFDC_CAL_BLOCK_TSCB) {
		if ((CoeffPtr->Coeff0 | CoeffPtr->Coeff1 | CoeffPtr->Coeff2 | CoeffPtr->Coeff3 | CoeffPtr->Coeff4 |
		     CoeffPtr->Coeff5 | CoeffPtr->Coeff6 | CoeffPtr->Coeff7) &
		    ~(XRFDC_CAL_TSCB_MASK | (XRFDC_CAL_TSCB_MASK << XRFDC_CAL_SLICE_SHIFT))) {
			Status = XRFDC_FAILURE;
			metal_log(METAL_LOG_ERROR,
				  "\n Bad Coefficient "
				  "available for this IP in %s\r\n",
				  __func__);
			goto RETURN_PATH;
		}
	}
	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Requested block not "
			  "available in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	Index = Block_Id;
	HighSpeed = XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id);
	if (HighSpeed == XRFDC_ENABLED) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}
	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		Shift = HighSpeed ? XRFDC_CAL_SLICE_SHIFT * (Index % 2) : 0;
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		switch (CalibrationBlock) {
		case XRFDC_CAL_BLOCK_OCB1:
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL1_OFFSET, XRFDC_CAL_OCB_EN_MASK,
					XRFDC_ENABLED);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB1_OFFSET_COEFF0, XRFDC_CAL_OCB_MASK,
					CoeffPtr->Coeff0 >> Shift);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB1_OFFSET_COEFF1, XRFDC_CAL_OCB_MASK,
					CoeffPtr->Coeff1 >> Shift);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB1_OFFSET_COEFF2, XRFDC_CAL_OCB_MASK,
					CoeffPtr->Coeff2 >> Shift);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB1_OFFSET_COEFF3, XRFDC_CAL_OCB_MASK,
					CoeffPtr->Coeff3 >> Shift);
			break;
		case XRFDC_CAL_BLOCK_OCB2:
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL3_OFFSET, XRFDC_CAL_OCB_EN_MASK,
					XRFDC_ENABLED);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB2_OFFSET_COEFF0, XRFDC_CAL_OCB_MASK,
					CoeffPtr->Coeff0 >> Shift);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB2_OFFSET_COEFF1, XRFDC_CAL_OCB_MASK,
					CoeffPtr->Coeff1 >> Shift);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB2_OFFSET_COEFF2, XRFDC_CAL_OCB_MASK,
					CoeffPtr->Coeff2 >> Shift);
			XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB2_OFFSET_COEFF3, XRFDC_CAL_OCB_MASK,
					CoeffPtr->Coeff3 >> Shift);
			break;
		case XRFDC_CAL_BLOCK_GCB:

			if (InstancePtr->RFdc_Config.IPType < 2) {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL1_OFFSET,
						XRFDC_CAL_GCB_ACEN_MASK, XRFDC_DISABLED);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL1_OFFSET,
						XRFDC_CAL_GCB_FLSH_MASK, XRFDC_ENABLED << XRFDC_CAL_GCB_FLSH_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
						XRFDC_CAL_GCB_COEFF0_FAB(Index), XRFDC_CAL_GCB_MASK,
						CoeffPtr->Coeff0 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
						XRFDC_CAL_GCB_COEFF1_FAB(Index), XRFDC_CAL_GCB_MASK,
						CoeffPtr->Coeff1 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
						XRFDC_CAL_GCB_COEFF2_FAB(Index), XRFDC_CAL_GCB_MASK,
						CoeffPtr->Coeff2 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
						XRFDC_CAL_GCB_COEFF3_FAB(Index), XRFDC_CAL_GCB_MASK,
						CoeffPtr->Coeff3 >> Shift);
			} else {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL2_OFFSET,
						XRFDC_CAL_GCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_GCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF0, XRFDC_CAL_GCB_MASK,
						CoeffPtr->Coeff0 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF1, XRFDC_CAL_GCB_MASK,
						CoeffPtr->Coeff1 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF2, XRFDC_CAL_GCB_MASK,
						CoeffPtr->Coeff2 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF3, XRFDC_CAL_GCB_MASK,
						CoeffPtr->Coeff3 >> Shift);
			}
			break;
		case XRFDC_CAL_BLOCK_TSCB:
			if (InstancePtr->RFdc_Config.IPType < 2) {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF0_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF1_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF2_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF3_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF4_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF5_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF6_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF7_ALT,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF0_ALT,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff0 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF1_ALT,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff1);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF2_ALT,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff2 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF3_ALT,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff3 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF4_ALT,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff4 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF5_ALT,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff5 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF6_ALT,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff6 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF7_ALT,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff7 >> Shift);
			} else {
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF0,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF1,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF2,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF3,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF4,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF5,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF6,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF7,
						XRFDC_CAL_TSCB_EN_MASK, XRFDC_ENABLED << XRFDC_CAL_TSCB_EN_SHIFT);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF0,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff0 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF1,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff1 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF2,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff2 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF3,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff3 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF4,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff4 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF5,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff5 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF6,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff6 >> Shift);
				XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF7,
						XRFDC_CAL_TSCB_MASK, CoeffPtr->Coeff7 >> Shift);
			}
			break;
		default:
			Status = XRFDC_FAILURE;
			metal_log(METAL_LOG_ERROR,
				  "\n Invalid Calibration "
				  "Mode in %s\r\n",
				  __func__);
			goto RETURN_PATH;
		}
	}
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This function is used to get the ADC Calibration Coefficients.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for LS, 0-1 for HS).
* @param	CalibrationBlock indicates the block to be read from
* @param    CoeffPtr is pointer to the XRFdc_Calibration_Coefficients structure
*           to get the calibration coefficients.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if error occurs.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_GetCalCoefficients(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, u32 CalibrationBlock,
			     XRFdc_Calibration_Coefficients *CoeffPtr)
{
	u32 BaseAddr;
	u32 Status;
	u32 Index;
	u32 HighSpeed;
	u32 Shift;
	u32 NoOfBlocks;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(CoeffPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Requested block not "
			  "available in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	memset(CoeffPtr, 0, sizeof(XRFdc_Calibration_Coefficients));
	Index = Block_Id;
	HighSpeed = XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id);
	if (HighSpeed == XRFDC_ENABLED) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}
	for (; Index < NoOfBlocks; Index++) {
		BaseAddr = XRFDC_BLOCK_BASE(XRFDC_ADC_TILE, Tile_Id, Index);
		Shift = HighSpeed ? XRFDC_CAL_SLICE_SHIFT * (Index % 2) : 0;
		switch (CalibrationBlock) {
		case XRFDC_CAL_BLOCK_OCB1:
			CoeffPtr->Coeff0 |=
				XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB1_OFFSET_COEFF0, XRFDC_CAL_OCB_MASK)
				<< Shift;
			CoeffPtr->Coeff1 |=
				XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB1_OFFSET_COEFF1, XRFDC_CAL_OCB_MASK)
				<< Shift;
			CoeffPtr->Coeff2 |=
				XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB1_OFFSET_COEFF2, XRFDC_CAL_OCB_MASK)
				<< Shift;
			CoeffPtr->Coeff3 |=
				XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB1_OFFSET_COEFF3, XRFDC_CAL_OCB_MASK)
				<< Shift;
			break;
		case XRFDC_CAL_BLOCK_OCB2:
			CoeffPtr->Coeff0 |=
				XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB2_OFFSET_COEFF0, XRFDC_CAL_OCB_MASK)
				<< Shift;
			CoeffPtr->Coeff1 |=
				XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB2_OFFSET_COEFF1, XRFDC_CAL_OCB_MASK)
				<< Shift;
			CoeffPtr->Coeff2 |=
				XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB2_OFFSET_COEFF2, XRFDC_CAL_OCB_MASK)
				<< Shift;
			CoeffPtr->Coeff3 |=
				XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_OCB2_OFFSET_COEFF3, XRFDC_CAL_OCB_MASK)
				<< Shift;
			break;
		case XRFDC_CAL_BLOCK_GCB:
			if (InstancePtr->RFdc_Config.IPType < 2) {
				if (XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_ADC_TI_DCB_CRL1_OFFSET,
						XRFDC_CAL_GCB_FLSH_MASK) == XRFDC_DISABLED) {
					CoeffPtr->Coeff0 |=
						(XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF0_ALT,
							     XRFDC_CAL_GCB_FAB_MASK) >>
						 4)
						<< Shift;
					CoeffPtr->Coeff1 |=
						(XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF1_ALT,
							     XRFDC_CAL_GCB_FAB_MASK) >>
						 4)
						<< Shift;
					CoeffPtr->Coeff2 |=
						(XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF2_ALT,
							     XRFDC_CAL_GCB_FAB_MASK) >>
						 4)
						<< Shift;
					CoeffPtr->Coeff3 |=
						(XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF3_ALT,
							     XRFDC_CAL_GCB_FAB_MASK) >>
						 4)
						<< Shift;
				} else {
					CoeffPtr->Coeff0 |=
						XRFdc_RDReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
							    XRFDC_CAL_GCB_COEFF0_FAB(Block_Id), XRFDC_CAL_GCB_MASK)
						<< Shift;
					CoeffPtr->Coeff1 |=
						XRFdc_RDReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
							    XRFDC_CAL_GCB_COEFF1_FAB(Block_Id), XRFDC_CAL_GCB_MASK)
						<< Shift;
					CoeffPtr->Coeff2 |=
						XRFdc_RDReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
							    XRFDC_CAL_GCB_COEFF2_FAB(Block_Id), XRFDC_CAL_GCB_MASK)
						<< Shift;
					CoeffPtr->Coeff3 |=
						XRFdc_RDReg(InstancePtr, XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id),
							    XRFDC_CAL_GCB_COEFF3_FAB(Block_Id), XRFDC_CAL_GCB_MASK)
						<< Shift;
				}
			} else {
				CoeffPtr->Coeff0 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF0,
								XRFDC_CAL_GCB_MASK)
						    << Shift;
				CoeffPtr->Coeff1 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF1,
								XRFDC_CAL_GCB_MASK)
						    << Shift;
				CoeffPtr->Coeff2 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF2,
								XRFDC_CAL_GCB_MASK)
						    << Shift;
				CoeffPtr->Coeff3 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_GCB_OFFSET_COEFF3,
								XRFDC_CAL_GCB_MASK)
						    << Shift;
			}
			break;
		case XRFDC_CAL_BLOCK_TSCB:
			if (InstancePtr->RFdc_Config.IPType < 2) {
				CoeffPtr->Coeff0 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF0_ALT,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff1 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF1_ALT,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff2 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF2_ALT,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff3 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF3_ALT,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff4 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF4_ALT,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff5 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF5_ALT,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff6 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF6_ALT,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff7 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF7_ALT,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
			} else {
				CoeffPtr->Coeff0 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF0,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff1 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF1,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff2 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF2,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff3 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF3,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff4 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF4,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff5 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF5,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff6 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF6,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
				CoeffPtr->Coeff7 |= XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CAL_TSCB_OFFSET_COEFF7,
								XRFDC_CAL_TSCB_MASK)
						    << Shift;
			}
			break;
		default:
			Status = XRFDC_FAILURE;
			metal_log(METAL_LOG_ERROR,
				  "\n Invalid Calibration "
				  "Mode in %s\r\n",
				  __func__);
			goto RETURN_PATH;
		}
	}
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This function is used to set calibration freeze settings.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for LS, 0-1 for HS).
* @param	CalFreezePtr pointer to the settings to be applied.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if error occurs.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_SetCalFreeze(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, XRFdc_Cal_Freeze_Settings *CalFreezePtr)
{
	u32 BaseAddr;
	u32 Status;
	u32 Index;
	u32 NoOfBlocks;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(CalFreezePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if (CalFreezePtr->FreezeCalibration > XRFDC_CAL_FREEZE_CALIB) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid FreezeCalibration "
			  "option in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	if (CalFreezePtr->DisableFreezePin > XRFDC_CAL_FRZ_PIN_DISABLE) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid DisableFreezePin "
			  "option in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Requested block not "
			  "available in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id);

	Index = Block_Id;
	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		NoOfBlocks = XRFDC_NUM_OF_BLKS2;
		if (Block_Id == XRFDC_BLK_ID1) {
			Index = XRFDC_BLK_ID2;
			NoOfBlocks = XRFDC_NUM_OF_BLKS4;
		}
	} else {
		NoOfBlocks = Block_Id + 1U;
	}

	for (; Index < NoOfBlocks; Index++) {
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CONV_CAL_STGS(Index), XRFDC_CAL_FREEZE_PIN_MASK,
				CalFreezePtr->DisableFreezePin << XRFDC_CAL_FREEZE_PIN_SHIFT);
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CONV_CAL_STGS(Index), XRFDC_CAL_FREEZE_CAL_MASK,
				CalFreezePtr->FreezeCalibration << XRFDC_CAL_FREEZE_CAL_SHIFT);
	}
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This function is used to get calibration freeze settings and status.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Tile_Id indicates Tile number (0-3).
* @param    Block_Id indicates Block number(0-3 for LS, 0-1 for HS).
* @param	CalFreezePtr pointer to be filled the settings/status.
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if error occurs.
*
* @note		Only for ADC blocks
*
******************************************************************************/
u32 XRFdc_GetCalFreeze(XRFdc *InstancePtr, u32 Tile_Id, u32 Block_Id, XRFdc_Cal_Freeze_Settings *CalFreezePtr)
{
	u32 BaseAddr;
	u32 Status;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(CalFreezePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckBlockEnabled(InstancePtr, XRFDC_ADC_TILE, Tile_Id, Block_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Requested block not "
			  "available in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id);

	if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
		if (Block_Id == XRFDC_BLK_ID1) {
			Block_Id = XRFDC_BLK_ID2;
		}
	}
	CalFreezePtr->CalFrozen =
		XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CONV_CAL_STGS(Block_Id), XRFDC_CAL_FREEZE_STS_MASK) >>
		XRFDC_CAL_FREEZE_STS_SHIFT;
	CalFreezePtr->DisableFreezePin =
		XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CONV_CAL_STGS(Block_Id), XRFDC_CAL_FREEZE_PIN_MASK) >>
		XRFDC_CAL_FREEZE_PIN_SHIFT;
	CalFreezePtr->FreezeCalibration =
		XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_CONV_CAL_STGS(Block_Id), XRFDC_CAL_FREEZE_CAL_MASK) >>
		XRFDC_CAL_FREEZE_CAL_SHIFT;

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/** @} */
