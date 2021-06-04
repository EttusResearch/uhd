/******************************************************************************
*
* Copyright (C) 2019 Xilinx, Inc.  All rights reserved.
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
* @file xrfdc_clock.c
* @addtogroup xrfdc_v6_0
* @{
*
* Contains the interface functions of the Mixer Settings in XRFdc driver.
* See xrfdc.h for a detailed description of the device and driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---    -------- -----------------------------------------------
* 6.0   cog    02/17/19 Initial release.
*       cog    03/12/19 Invert clock detection bits to support IP change.
*       cog    03/12/19 Fix bug where incorrect FS, RefClk and were output
*                       divider were being returned.
*       cog    04/09/19 Discriminate between Gen 3 IP and lower for checking
*                       if internal PLL is enabled.
*       cog    04/09/19 Fixed issue where tile was not autostarting after PLL
*                       rate change.
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "mpm/rfdc/xrfdc.h"

/************************** Constant Definitions *****************************/
static u32 PllTuningMatrix[8][4][2] = {
		{{0x7F8A, 0x3FFF}, {0x7F9C, 0x3FFF}, {0x7FE2, 0x3FFF} },
		{{0x7FE9, 0xFFFF}, {0x7F8E, 0xFFFF}, {0x7F9C, 0xFFFF} },
		{{0x7F95, 0xFFFF}, {0x7F8E, 0xFFFF}, { 0x7F9A, 0xFFFF}, {0x7F8C, 0xFFFF} },
		{{0x7F95, 0x3FFF}, {0x7FEE, 0x3FFF}, { 0x7F9A, 0xFFFF}, {0x7F9C, 0xFFFF} },
		{{0x7F95, 0x3FFF}, {0x7FEE, 0x3FFF}, { 0x7F9A, 0xFFFF}, {0x7F9C, 0xFFFF} },
		{{0x7F95, 0xFFFF}, {0x7F8E, 0xFFFF}, { 0x7FEA, 0xFFFF}, {0x7F9C, 0xFFFF} },
		{{0x7FE9, 0xFFFF}, {0x7F8E, 0xFFFF}, { 0x7F9A, 0xFFFF}, {0x7F9C, 0xFFFF} },
		{{0x7FEC, 0xFFFF}, {0x7FEE, 0x3FFF}, { 0x7F9C, 0xFFFF} }
};


/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/
static u32 XRFdc_CheckClkDistValid(XRFdc *InstancePtr, XRFdc_Distribution_Settings *DistributionSettingsPtr);
static u32 XRFdc_SetPLLConfig(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		double RefClkFreq, double SamplingRate);

/************************** Function Prototypes ******************************/

/*****************************************************************************/
/**
*
* This function is used to set the clock settings
*
* @param    InstancePtr is a pointer to the XRfdc instance.
* @param	Type indicates ADC/DAC.
* @param	Tile_Id indicates Tile number (0-3).
* @param    SettingsPtr pointer to set the clock settings
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if no valid distribution found.
*
******************************************************************************/
u32 XRFdc_SetTileClkSettings(XRFdc *InstancePtr, u32 Type, u32 Tile_Id, XRFdc_Tile_Clock_Settings *SettingsPtr)
{
	u32 Status;
	u32 BaseAddr;
	u16 PLLSource;
	u16 NetworkCtrlReg;
	u16 DistCtrlReg;
	u16 PLLRefDivReg;
	u16 PLLOpDivReg;
	u32 TileIndex;
	u16 DivideMode = 0;
	u16 DivideValue = 0;

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

	TileIndex = (Type == XRFDC_DAC_TILE) ? Tile_Id : Tile_Id + XRFDC_CLK_DST_ADC0;
	Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Tile_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Requested Tile not "
			  "available in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	if (SettingsPtr->SourceTile > XRFDC_CLK_DST_ADC3) {
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Parameter Value "
			  "for Source in %s\r\n",
			  __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (SettingsPtr->DistributedClock > XRFDC_DIST_OUT_OUTDIV) {
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Parameter Value "
			  "for Distribution Out in %s\r\n",
			  __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (SettingsPtr->PLLEnable > 1) {
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Parameter Value "
			  "for PLLEnable in %s\r\n",
			  __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if ((SettingsPtr->PLLEnable == XRFDC_ENABLED) && (SettingsPtr->DivisionFactor < 1)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Configuration in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if ((SettingsPtr->DistributedClock == XRFDC_DIST_OUT_OUTDIV) &&
	    ((SettingsPtr->DivisionFactor < 2) && (SettingsPtr->PLLEnable == XRFDC_DISABLED))) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Configuration in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if ((SettingsPtr->SourceTile != TileIndex) && (SettingsPtr->DistributedClock != XRFDC_DIST_OUT_NONE)) {
		metal_log(METAL_LOG_ERROR, "\n Cannot Redistribute Clock in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	/*configure PLL & divider or just divider*/
	if (SettingsPtr->PLLEnable == XRFDC_ENABLED) {
		metal_log(METAL_LOG_WARNING,
			  "\n Output divider settings may "
			  "be overridden in %s\r\n",
			  __func__);
		PLLSource = XRFDC_INTERNAL_PLL_CLK;
		Status = XRFdc_DynamicPLLConfig(InstancePtr, Type, Tile_Id, PLLSource,
						SettingsPtr->PLLSettings.RefClkFreq,
						SettingsPtr->PLLSettings.SampleRate);
		SettingsPtr->DivisionFactor = SettingsPtr->PLLSettings.OutputDivider;
		if (Status != XRFDC_SUCCESS) {
			metal_log(METAL_LOG_ERROR,
				  "\n Could not set up PLL "
				  "in %s\r\n",
				  __func__);
			goto RETURN_PATH;
		}
	} else if (SettingsPtr->DivisionFactor > 1) {
		if (SettingsPtr->DivisionFactor == 2U) {
			DivideMode = XRFDC_PLL_OUTDIV_MODE_2;
		} else if (SettingsPtr->DivisionFactor == 3U) {
			DivideMode = XRFDC_PLL_OUTDIV_MODE_3;
			DivideValue = XRFDC_PLL_OUTDIV_MODE_3_VAL;
		} else if (SettingsPtr->DivisionFactor >= 4U) {
			DivideMode = XRFDC_PLL_OUTDIV_MODE_N;
			DivideValue = ((SettingsPtr->DivisionFactor - 4U) >> 1);
		}
		XRFdc_ClrSetReg(InstancePtr, XRFDC_DRP_BASE(Type, Tile_Id) + XRFDC_HSCOM_ADDR, XRFDC_PLL_DIVIDER0,
				XRFDC_PLL_DIVIDER0_MASK, ((DivideMode << XRFDC_PLL_DIVIDER0_SHIFT) | DivideValue));
	}
	DistCtrlReg = 0;
	PLLRefDivReg = 0;
	PLLOpDivReg = 0;
	NetworkCtrlReg = 0;
	if (SettingsPtr->SourceTile == TileIndex) {
		if (SettingsPtr->DistributedClock == XRFDC_DIST_OUT_NONE) {
			if (SettingsPtr->PLLEnable == XRFDC_DISABLED) {
				PLLRefDivReg |= XRFDC_PLLREFDIV_INPUT_OFF;
				NetworkCtrlReg |= XRFDC_NET_CTRL_CLK_REC_DIST_T1;
				if (SettingsPtr->DivisionFactor < 2) {
					/*
					T1 from Self
					No PLL
					Do Not Use PLL Output Divider
					Do Not Distribute
					*/
					NetworkCtrlReg |= XRFDC_NET_CTRL_CLK_T1_SRC_LOCAL;
					DistCtrlReg |= XRFDC_DIST_CTRL_CLK_T1_SRC_LOCAL;
				} else {
					/*
					T1 from Self
					No PLL
					Do Not Distribute
					*/
					PLLOpDivReg |= XRFDC_PLLOPDIV_INPUT_DIST_LOCAL;
				}
			} else {
				/*
				T1 from Self
				PLL
				Use PLL Output Divider
				Do Not Distribute
				*/
				NetworkCtrlReg |= XRFDC_NET_CTRL_CLK_REC_PLL;
			}
		} else {
			NetworkCtrlReg |= XRFDC_NET_CTRL_CLK_T1_SRC_DIST;
			DistCtrlReg |= XRFDC_DIST_CTRL_TO_T1;
			if (SettingsPtr->PLLEnable == XRFDC_DISABLED) {
				NetworkCtrlReg |= XRFDC_NET_CTRL_CLK_REC_DIST_T1;
				PLLRefDivReg |= XRFDC_PLLREFDIV_INPUT_OFF;
				if (SettingsPtr->DivisionFactor < 2) {
					/*
					T1 From Distribution
					No PLL
					Do Not Use PLL Output Divider
					Send to Distribution
					*/
					DistCtrlReg |= XRFDC_DIST_CTRL_DIST_SRC_LOCAL;
				} else {
					/*
					T1 From Distribution
					No PLL
					Use PLL Output Divider
					Send to Distribution
					*/
					PLLOpDivReg |= XRFDC_PLLOPDIV_INPUT_DIST_LOCAL;
					DistCtrlReg |= (SettingsPtr->DistributedClock == XRFDC_DIST_OUT_RX) ?
							       XRFDC_DIST_CTRL_DIST_SRC_LOCAL :
							       XRFDC_DIST_CTRL_DIST_SRC_PLL;
				}

			} else {
				/*
				T1 From Distribution
				PLL
				Use PLL Output Divider
				Send to Distribution
				*/
				DistCtrlReg |= (SettingsPtr->DistributedClock == XRFDC_DIST_OUT_RX) ?
						       XRFDC_DIST_CTRL_DIST_SRC_LOCAL :
						       XRFDC_DIST_CTRL_DIST_SRC_PLL;
				NetworkCtrlReg |= XRFDC_NET_CTRL_CLK_REC_PLL;
			}
		}
	} else {
		if (SettingsPtr->PLLEnable == XRFDC_DISABLED) {
			PLLRefDivReg |= XRFDC_PLLREFDIV_INPUT_OFF;
			if (SettingsPtr->DivisionFactor > 1) {
				/*
				Source From Distribution
				No PLL
				Use PLL Output Divider
				Do Not Distribute
				*/

				PLLOpDivReg |= XRFDC_PLLOPDIV_INPUT_DIST_LOCAL;
				NetworkCtrlReg |= XRFDC_NET_CTRL_CLK_INPUT_DIST;
				DistCtrlReg |= XRFDC_DIST_CTRL_TO_PLL_DIV;
			} else {
				/*
				Source From Distribution
				No PLL
				Do Not Use PLL Output Divider
				Do Not Distribute
				*/
				NetworkCtrlReg |= XRFDC_NET_CTRL_CLK_T1_SRC_DIST;
				DistCtrlReg |= XRFDC_DIST_CTRL_TO_T1;
			}
		} else {
			/*
			Source From Distribution
			PLL
			Use PLL Output Divider
			Do Not Distribute
			*/
			PLLRefDivReg |= XRFDC_PLLREFDIV_INPUT_DIST;
			DistCtrlReg |= XRFDC_DIST_CTRL_TO_PLL_DIV;
		}
	}

	/*Write to Registers*/
	if (Type == XRFDC_ADC_TILE) {
		BaseAddr = XRFDC_ADC_TILE_DRP_ADDR(Tile_Id);
	} else {
		BaseAddr = XRFDC_DAC_TILE_DRP_ADDR(Tile_Id);
	}
	BaseAddr += XRFDC_HSCOM_ADDR;
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK_ALT, DistCtrlReg);
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CLK_NETWORK_CTRL1, XRFDC_HSCOM_NETWORK_CTRL1_MASK, NetworkCtrlReg);
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_PLL_REFDIV, XRFDC_PLL_REFDIV_MASK, PLLRefDivReg);
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_PLL_DIVIDER0, XRFDC_PLL_DIVIDER0_ALT_MASK, PLLOpDivReg);
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This function is used to check the distribution chosen is valid
*
* @param        InstancePtr is a pointer to the XRfdc instance.
* @param        DistributionSettingsPtr pointer to the distribution settings struct
*
* @return
*       - XRFDC_SUCCESS if Valid.
*       - XRFDC_FAILURE if Not Valid.
*
******************************************************************************/
static u32 XRFdc_CheckClkDistValid(XRFdc *InstancePtr, XRFdc_Distribution_Settings *DistributionSettingsPtr)
{
	u32 Status;
	u8 CurrentTile;
	u8 *Source;
	u8 Sources[8] = { DistributionSettingsPtr->DAC[0].SourceTile, DistributionSettingsPtr->DAC[1].SourceTile,
			  DistributionSettingsPtr->DAC[2].SourceTile, DistributionSettingsPtr->DAC[3].SourceTile,
			  DistributionSettingsPtr->ADC[0].SourceTile, DistributionSettingsPtr->ADC[1].SourceTile,
			  DistributionSettingsPtr->ADC[2].SourceTile, DistributionSettingsPtr->ADC[3].SourceTile };
	u8 LowBoundary;
	u16 EFuse;
	XRFdc_Distribution *DistributionPtr;

	/*init for first distribution*/
	DistributionPtr = DistributionSettingsPtr->DistributionStatus;
	Source = Sources;
	LowBoundary = DistributionSettingsPtr->DAC[0].SourceTile;
	DistributionPtr->DistributionSource = DistributionSettingsPtr->DAC[0].SourceTile;
	DistributionPtr->Enabled = XRFDC_ENABLED;
	DistributionPtr->LowerBound = 0;

	for (CurrentTile = 0; CurrentTile < XRFDC_DIST_MAX; CurrentTile++, Source++) {
		if (*Source >= XRFDC_DIST_MAX) {
			Status = XRFDC_FAILURE; /*out of range*/
			metal_log(METAL_LOG_ERROR,
				  "\n Invalid Source "
				  "value in %s - Out of Range\r\n",
				  __func__);
			goto RETURN_PATH;
		}
		if (*Source < LowBoundary) {
			Status = XRFDC_FAILURE; /*SW: no hopovers*/
			metal_log(METAL_LOG_ERROR,
				  "\n Invalid Configuration "
				  "Hopping Over Not Allowed in %s\r\n",
				  __func__);
			goto RETURN_PATH;
		}
		if (Sources[*Source] != *Source) { /*SW: check source is a distributer*/
			Status = XRFDC_FAILURE;
			metal_log(METAL_LOG_ERROR,
				  "\n Invalid Source "
				  "Sourcing from Tile that is not Distributing in %s\r\n",
				  __func__);
			goto RETURN_PATH;
		}
		if ((*Source == XRFDC_CLK_DST_DAC0) &&
		    (InstancePtr->RFdc_Config.DACTile_Config[XRFDC_CLK_DST_DAC0].NumSlices == 2)) { /*HW: only 2 clks*/
			Status = XRFDC_FAILURE;
			metal_log(METAL_LOG_ERROR,
				  "\n Invalid Source "
				  "Sourcing from Tile Without Clock in %s\r\n",
				  __func__);
			goto RETURN_PATH;
		}
		if ((*Source == XRFDC_CLK_DST_DAC2) &&
		    (InstancePtr->RFdc_Config.DACTile_Config[XRFDC_CLK_DST_DAC2].NumSlices == 2)) { /*HW: only 2 clks*/
			metal_log(METAL_LOG_ERROR,
				  "\n Invalid Source "
				  "Sourcing from Tile Without Clock in %s\r\n",
				  __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		if ((CurrentTile < XRFDC_CLK_DST_ADC0) &&
		    (*Source > XRFDC_CLK_DST_DAC3)) { /*Cut between ADC0 MUX8 && DAC3 STH*/
			metal_log(METAL_LOG_ERROR,
				  "\n Invalid Configuration "
				  "DAC Cannot Source from ADC in %s\r\n",
				  __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
		if (CurrentTile < XRFDC_CLK_DST_ADC0) { /*DAC*/
			EFuse = XRFdc_ReadReg16(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_DAC_TILE, CurrentTile) + XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_EFUSE_2_OFFSET);
		} else { /*ADC*/
			EFuse = XRFdc_ReadReg16(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_ADC_TILE, (CurrentTile - XRFDC_CLK_DST_ADC0)) +
							XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_EFUSE_2_OFFSET);
		}
		/*if PKG <2*/

		if ((DistributionSettingsPtr->DAC[0].SourceTile != DistributionSettingsPtr->DAC[1].SourceTile) ||
		    (DistributionSettingsPtr->DAC[0].SourceTile != DistributionSettingsPtr->DAC[2].SourceTile) ||
		    (DistributionSettingsPtr->DAC[0].SourceTile != DistributionSettingsPtr->DAC[3].SourceTile) ||
		    (DistributionSettingsPtr->DAC[0].SourceTile != DistributionSettingsPtr->ADC[0].SourceTile) ||
		    (DistributionSettingsPtr->DAC[0].SourceTile != DistributionSettingsPtr->ADC[1].SourceTile) ||
		    (DistributionSettingsPtr->DAC[0].SourceTile != DistributionSettingsPtr->ADC[2].SourceTile) ||
		    (DistributionSettingsPtr->DAC[0].SourceTile != DistributionSettingsPtr->ADC[3].SourceTile) ||
		    (DistributionSettingsPtr->DAC[0].PLLEnable != XRFDC_ENABLED) ||
		    (DistributionSettingsPtr->DAC[1].PLLEnable != XRFDC_ENABLED) ||
		    (DistributionSettingsPtr->DAC[2].PLLEnable != XRFDC_ENABLED) ||
		    (DistributionSettingsPtr->DAC[3].PLLEnable != XRFDC_ENABLED) ||
		    (DistributionSettingsPtr->ADC[0].PLLEnable != XRFDC_ENABLED) ||
		    (DistributionSettingsPtr->ADC[1].PLLEnable != XRFDC_ENABLED) ||
		    (DistributionSettingsPtr->ADC[2].PLLEnable != XRFDC_ENABLED) ||
		    (DistributionSettingsPtr->ADC[3].PLLEnable != XRFDC_ENABLED)) { /*special case that is allowed.*/

			/*if PKG <2*/
			if (EFuse & XRFDC_PREMIUMCTRL_CLKDIST) {
				if ((CurrentTile > XRFDC_CLK_DST_ADC1) &&
				    (*Source != CurrentTile)) { /*E: no dist past adc1*/
					Status = XRFDC_FAILURE;
					metal_log(METAL_LOG_ERROR,
						  "\n Invalid Configuration "
						  "- Licensing - Not Premium in %s\r\n",
						  __func__);
					goto RETURN_PATH;
				}
			}
			/*if PKG <1*/
			if ((EFuse & XRFDC_EXPORTCTRL_CLKDIST) == XRFDC_EXPORTCTRL_CLKDIST) {
				if ((CurrentTile > XRFDC_CLK_DST_DAC3) && (*Source != CurrentTile)) { /*E: No ADC Dist*/
					Status = XRFDC_FAILURE;
					metal_log(METAL_LOG_ERROR,
						  "\n Invalid Configuration "
						  "- Licensing in %s\r\n",
						  __func__);
					goto RETURN_PATH;
				}
				if ((CurrentTile == XRFDC_CLK_DST_DAC0) &&
				    (*Source != XRFDC_CLK_DST_DAC1)) { /*E: DAC0 must source from DAC1*/
					Status = XRFDC_FAILURE;
					metal_log(METAL_LOG_ERROR,
						  "\n Invalid Configuration "
						  "- Licensing in - Export Control %s\r\n",
						  __func__);
					goto RETURN_PATH;
				}
				if ((CurrentTile == XRFDC_CLK_DST_DAC2) &&
				    (*Source != XRFDC_CLK_DST_DAC3)) { /*E: DAC2 must source from DAC3*/
					Status = XRFDC_FAILURE;
					metal_log(METAL_LOG_ERROR,
						  "\n Invalid Configuration "
						  "- Licensing in %s\r\n",
						  __func__);
					goto RETURN_PATH;
				}
			}
		}

		if (*Source != DistributionPtr->DistributionSource) { /*i.e. if new distribution*/
			DistributionPtr->UpperBound = CurrentTile - 1;
			DistributionPtr++;
			DistributionPtr->Enabled = XRFDC_ENABLED;
			LowBoundary = *Source;
			DistributionPtr->DistributionSource = *Source;
			DistributionPtr->LowerBound = CurrentTile;
		}
	}
	DistributionPtr->UpperBound = CurrentTile - 1;
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	if (Status == XRFDC_FAILURE) {
		memset(DistributionSettingsPtr, 0, sizeof(XRFdc_Distribution_Settings));
	}
	return Status;
}
/*****************************************************************************/
/**
*
* This function is used to set the clock distribution
*
* @param        InstancePtr is a pointer to the XRfdc instance.
* @param        DistributionSettingsPtr pointer to the distribution settings struct
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if could not set distribution.
*
******************************************************************************/
u32 XRFdc_SetClkDistribution(XRFdc *InstancePtr, XRFdc_Distribution_Settings *DistributionSettingsPtr)
{
	u32 Status;
	u8 DelayLeft;
	u8 DelayRight;
	s8 Delay;
	s8 ClkDetItr;
	u8 *Delays[8];
	u8 DelayOutSourceLeft;
	u8 DelayOutSourceRight;
	XRFdc_Distribution *Distribution;
	u8 DistributionCount;
	u16 Reg;
	u16 ClkDetectReg;
	u8 FeedBackForInputRight = 0;
	u8 FeedBackForInputLeft = 0;
	u8 Tile;
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(DistributionSettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if (InstancePtr->RFdc_Config.IPType < 2) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			"\n Requested fuctionality not "
			"available for this IP in %s\r\n",
			__func__);
		goto RETURN_PATH;
	}

	Status = XRFdc_CheckClkDistValid(InstancePtr, DistributionSettingsPtr);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Distribution "
			  "in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	Delays[0] = &DistributionSettingsPtr->DAC[0].Delay;
	Delays[1] = &DistributionSettingsPtr->DAC[1].Delay;
	Delays[2] = &DistributionSettingsPtr->DAC[2].Delay;
	Delays[3] = &DistributionSettingsPtr->DAC[3].Delay;
	Delays[4] = &DistributionSettingsPtr->ADC[0].Delay;
	Delays[5] = &DistributionSettingsPtr->ADC[1].Delay;
	Delays[6] = &DistributionSettingsPtr->ADC[2].Delay;
	Delays[7] = &DistributionSettingsPtr->ADC[3].Delay;
	Status = XRFdc_Shutdown(InstancePtr, XRFDC_ADC_TILE, -1);
	if (Status != XRFDC_SUCCESS) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	Status = XRFdc_Shutdown(InstancePtr, XRFDC_DAC_TILE, -1);
	if (Status != XRFDC_SUCCESS) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	for (Distribution = DistributionSettingsPtr->DistributionStatus, DistributionCount = 0;
	     DistributionCount < XRFDC_DIST_MAX; Distribution++, DistributionCount++) {
		if (Distribution->Enabled == XRFDC_DISABLED) {
			break;
		}
		DelayLeft = (-Distribution->LowerBound + Distribution->DistributionSource);
		DelayRight = (Distribution->UpperBound - Distribution->DistributionSource);
		DelayOutSourceLeft = 0;
		DelayOutSourceRight = 0;
		Distribution->MaxDelay = 0;
		Distribution->MinDelay = 255;
		Distribution->IsDelayBalanced = 0;
		if ((DelayLeft == 0) && (DelayRight == 0)) { /*self contained*/
			Reg = XRFDC_CLK_DISTR_OFF;
		} else {
			Reg = XRFDC_CLK_DISTR_MUX9_SRC_INT;
			if (DelayLeft == 0) {
				Reg |= XRFDC_CLK_DISTR_MUX8_SRC_NTH;
			} else {
				Reg |= XRFDC_CLK_DISTR_MUX8_SRC_INT;
			}
			if (((Distribution->DistributionSource == XRFDC_CLK_DST_DAC3) ||
			     (Distribution->DistributionSource == XRFDC_CLK_DST_ADC3)) &&
			    ((DelayLeft > 1) || (DelayRight > 1))) /*cases for no FB from tile to right*/
			{
				Reg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT | XRFDC_CLK_DISTR_MUX6_SRC_INT |
				       XRFDC_CLK_DISTR_MUX7_SRC_INT;
				FeedBackForInputRight = 0;
				FeedBackForInputLeft = 0;
			} else {
				if (DelayLeft > 1) {
					Reg |= XRFDC_CLK_DISTR_MUX4A_SRC_STH | XRFDC_CLK_DISTR_MUX6_SRC_NTH |
					       XRFDC_CLK_DISTR_MUX7_SRC_INT;
					DelayOutSourceRight = 2;
					FeedBackForInputRight = 0;
					FeedBackForInputLeft = 1;
				} else {
					Reg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT;
					FeedBackForInputRight = 1;
					FeedBackForInputLeft = 0;
					if ((DelayRight > 1) &&
					    (Distribution->DistributionSource != XRFDC_CLK_DST_DAC2)) {
						Reg |= XRFDC_CLK_DISTR_MUX7_SRC_STH;
						DelayOutSourceLeft = 2;
					} else {
						Reg |= XRFDC_CLK_DISTR_MUX7_SRC_INT;
					}
					if (DelayRight == 0) {
						Reg |= XRFDC_CLK_DISTR_MUX6_SRC_OFF;
					} else {
						Reg |= XRFDC_CLK_DISTR_MUX6_SRC_INT;
					}
				}
			}
		}

		*Delays[Distribution->DistributionSource] =
			(Reg == XRFDC_CLK_DISTR_OFF) ? 0 : DelayOutSourceLeft + DelayOutSourceRight + 2;
		Distribution->MaxDelay = MAX(Distribution->MaxDelay, (*Delays[Distribution->DistributionSource]));
		Distribution->MinDelay = MIN(Distribution->MinDelay, (*Delays[Distribution->DistributionSource]));

		/* setup clk detect register */
		ClkDetectReg = (XRFDC_CLOCK_DETECT_CLK <<
			((XRFDC_CLK_DST_ADC3 - Distribution->DistributionSource) << 1));

		if ((Distribution->DistributionSource) < XRFDC_CLK_DST_ADC0) { /*DAC*/
			XRFdc_ClrSetReg(InstancePtr,
					XRFDC_DRP_BASE(XRFDC_DAC_TILE, (Distribution->DistributionSource)) +
					XRFDC_HSCOM_ADDR,
					XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
			XRFdc_ClrSetReg(InstancePtr,
					XRFDC_CTRL_STS_BASE(XRFDC_DAC_TILE, (Distribution->DistributionSource)),
					XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
		} else { /*ADC*/
			XRFdc_ClrSetReg(InstancePtr,
					XRFDC_DRP_BASE(XRFDC_ADC_TILE,
					(Distribution->DistributionSource - XRFDC_CLK_DST_ADC0)) +
					XRFDC_HSCOM_ADDR,
					XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
			XRFdc_ClrSetReg(InstancePtr,
					XRFDC_CTRL_STS_BASE(XRFDC_ADC_TILE,
					(Distribution->DistributionSource - XRFDC_CLK_DST_ADC0)),
					XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
		}
		/*Leftmost tile*/
		if (DelayLeft) {
			*Delays[Distribution->LowerBound] = DelayOutSourceLeft + (DelayLeft << 1);
			Distribution->MaxDelay = MAX(Distribution->MaxDelay, (*Delays[Distribution->LowerBound]));
			Distribution->MinDelay = MIN(Distribution->MinDelay, (*Delays[Distribution->LowerBound]));
			Reg = XRFDC_CLK_DISTR_MUX4A_SRC_STH | XRFDC_CLK_DISTR_MUX6_SRC_OFF |
			      XRFDC_CLK_DISTR_MUX7_SRC_OFF | XRFDC_CLK_DISTR_MUX8_SRC_NTH |
			      XRFDC_CLK_DISTR_MUX9_SRC_INT;

			/* setup clk detect register */
			ClkDetectReg = (XRFDC_CLOCK_DETECT_BOTH <<
				((XRFDC_CLK_DST_ADC3 - Distribution->DistributionSource) << 1));
			for (ClkDetItr = DelayLeft - 1; ClkDetItr > 0; ClkDetItr--) {
				ClkDetectReg |= (XRFDC_CLOCK_DETECT_DIST <<
					((XRFDC_CLK_DST_ADC3 - (Distribution->DistributionSource - ClkDetItr)) << 1));
			}

			if ((Distribution->DistributionSource - DelayLeft) < XRFDC_CLK_DST_ADC0) { /*DAC*/
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_DAC_TILE,
							(Distribution->DistributionSource - DelayLeft)) +
							XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_CTRL_STS_BASE(XRFDC_DAC_TILE, (Distribution->DistributionSource - DelayLeft)),
						XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
			} else { /*ADC*/
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_ADC_TILE, (Distribution->DistributionSource -
						DelayLeft - XRFDC_CLK_DST_ADC0)) +
						XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_CTRL_STS_BASE(XRFDC_ADC_TILE,
						(Distribution->DistributionSource - DelayLeft - XRFDC_CLK_DST_ADC0)),
						XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
			}
		}
		/*Rest of tiles left of Distribution->DistributionSource*/
		for (Delay = 1; Delay < DelayLeft; Delay++) {
			Reg = XRFDC_CLK_DISTR_MUX6_SRC_OFF | XRFDC_CLK_DISTR_MUX7_SRC_STH |
			      XRFDC_CLK_DISTR_MUX8_SRC_INT | XRFDC_CLK_DISTR_MUX9_SRC_INT;
			if (FeedBackForInputLeft == 0) {
				Reg |= XRFDC_CLK_DISTR_MUX4A_SRC_STH;
			} else {
				Reg |= XRFDC_CLK_DISTR_MUX4A_SRC_INT;
			}
			*Delays[Distribution->DistributionSource - Delay] =
				DelayOutSourceLeft + ((Delay + FeedBackForInputLeft) << 1);
			Distribution->MaxDelay =
				MAX(Distribution->MaxDelay, (*Delays[Distribution->DistributionSource - Delay]));
			Distribution->MinDelay =
				MIN(Distribution->MinDelay, (*Delays[Distribution->DistributionSource - Delay]));
			FeedBackForInputLeft = !FeedBackForInputLeft;

			/* setup clk detect register */
			ClkDetectReg = (XRFDC_CLOCK_DETECT_BOTH <<
				((XRFDC_CLK_DST_ADC3 - Distribution->DistributionSource) << 1));
			for (ClkDetItr = Delay - 1; ClkDetItr > 0; ClkDetItr--) {
				ClkDetectReg |= (XRFDC_CLOCK_DETECT_DIST <<
					((XRFDC_CLK_DST_ADC3 - (Distribution->DistributionSource - ClkDetItr)) << 1));
			}

			if ((Distribution->DistributionSource - Delay) < XRFDC_CLK_DST_ADC0) { /*DAC*/
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_DAC_TILE,
							       (Distribution->DistributionSource - Delay)) +
							XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_CTRL_STS_BASE(XRFDC_DAC_TILE, (Distribution->DistributionSource - Delay)),
						XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
			} else { /*ADC*/
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_ADC_TILE, (Distribution->DistributionSource -
						Delay - XRFDC_CLK_DST_ADC0)) +
						XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_CTRL_STS_BASE(XRFDC_ADC_TILE,
						(Distribution->DistributionSource - Delay - XRFDC_CLK_DST_ADC0)),
						XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
			}
		}
		/*Rightmost tile*/
		if (DelayRight) {
			Reg = XRFDC_CLK_DISTR_MUX4A_SRC_INT | XRFDC_CLK_DISTR_MUX6_SRC_OFF |
			      XRFDC_CLK_DISTR_MUX7_SRC_OFF | XRFDC_CLK_DISTR_MUX8_SRC_NTH |
			      XRFDC_CLK_DISTR_MUX9_SRC_NTH;
			*Delays[Distribution->UpperBound] = DelayOutSourceRight + (DelayRight << 1);
			Distribution->MaxDelay = MAX(Distribution->MaxDelay, (*Delays[Distribution->UpperBound]));
			Distribution->MinDelay = MIN(Distribution->MinDelay, (*Delays[Distribution->UpperBound]));

			/* setup clk detect register */
			ClkDetectReg = (XRFDC_CLOCK_DETECT_BOTH <<
				((XRFDC_CLK_DST_ADC3 - Distribution->DistributionSource) << 1));
			for (ClkDetItr = DelayRight - 1; ClkDetItr > 0; ClkDetItr--) {
				ClkDetectReg |= (XRFDC_CLOCK_DETECT_DIST <<
					((XRFDC_CLK_DST_ADC3 - (Distribution->DistributionSource + ClkDetItr)) << 1));
			}

			if ((Distribution->DistributionSource + DelayRight) < XRFDC_CLK_DST_ADC0) { /*DAC*/
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_DAC_TILE,
							       (Distribution->DistributionSource + DelayRight)) +
							XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_CTRL_STS_BASE(XRFDC_DAC_TILE, (Distribution->DistributionSource + DelayRight)),
						XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
			} else { /*ADC*/
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_ADC_TILE, (Distribution->DistributionSource +
						DelayRight - XRFDC_CLK_DST_ADC0)) +
						XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_CTRL_STS_BASE(XRFDC_ADC_TILE,
						(Distribution->DistributionSource + DelayRight - XRFDC_CLK_DST_ADC0)),
						XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
			}
		}
		/*rest of tiles to right*/
		for (Delay = 1; Delay < DelayRight; Delay++) {
			if (((Delay + Distribution->DistributionSource) == 3) || (FeedBackForInputRight == 0)) {
				FeedBackForInputRight = 0;
				Reg = XRFDC_CLK_DISTR_MUX4A_SRC_INT;
				*Delays[Distribution->DistributionSource + Delay] = DelayOutSourceRight + (Delay << 1);
			} else {
				Reg = XRFDC_CLK_DISTR_MUX4A_SRC_STH;
				*Delays[Distribution->DistributionSource + Delay] =
					DelayOutSourceRight + ((Delay + 1) << 1);
			}
			Distribution->MaxDelay =
				MAX(Distribution->MaxDelay, (*Delays[Distribution->DistributionSource + Delay]));
			Distribution->MinDelay =
				MIN(Distribution->MinDelay, (*Delays[Distribution->DistributionSource + Delay]));
			FeedBackForInputRight = !FeedBackForInputRight;
			Reg |= XRFDC_CLK_DISTR_MUX6_SRC_NTH | XRFDC_CLK_DISTR_MUX7_SRC_OFF |
			       XRFDC_CLK_DISTR_MUX8_SRC_NTH | XRFDC_CLK_DISTR_MUX9_SRC_NTH;

			/* setup clk detect register */
			ClkDetectReg = (XRFDC_CLOCK_DETECT_BOTH <<
				((XRFDC_CLK_DST_ADC3 - Distribution->DistributionSource) << 1));
			for (ClkDetItr = Delay - 1; ClkDetItr > 0; ClkDetItr--) {
				ClkDetectReg |= (XRFDC_CLOCK_DETECT_DIST <<
					((XRFDC_CLK_DST_ADC3 - (Distribution->DistributionSource + ClkDetItr)) << 1));
			}

			if ((Distribution->DistributionSource + Delay) < XRFDC_CLK_DST_ADC0) { /*DAC*/
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_DAC_TILE,
						(Distribution->DistributionSource + Delay)) +
						XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_CTRL_STS_BASE(XRFDC_DAC_TILE, (Distribution->DistributionSource + Delay)),
						XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
			} else { /*ADC*/
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_DRP_BASE(XRFDC_ADC_TILE, (Distribution->DistributionSource +
						Delay - XRFDC_CLK_DST_ADC0)) +
						XRFDC_HSCOM_ADDR,
						XRFDC_HSCOM_CLK_DSTR_OFFSET, XRFDC_HSCOM_CLK_DSTR_MASK, Reg);
				XRFdc_ClrSetReg(InstancePtr,
						XRFDC_CTRL_STS_BASE(XRFDC_ADC_TILE,
						(Distribution->DistributionSource + Delay - XRFDC_CLK_DST_ADC0)),
						XRFDC_CLOCK_DETECT_OFFSET, XRFDC_CLOCK_DETECT_MASK, ClkDetectReg);
			}
		}
		Distribution->IsDelayBalanced = (Distribution->MaxDelay == Distribution->MinDelay) ? 1 : 0;
	}
	for (Tile = 0; Tile < XRFDC_NUM_OF_TILES4; Tile++) {
		XRFdc_SetTileClkSettings(InstancePtr, XRFDC_ADC_TILE, Tile, &DistributionSettingsPtr->ADC[Tile]);
		XRFdc_SetTileClkSettings(InstancePtr, XRFDC_DAC_TILE, Tile, &DistributionSettingsPtr->DAC[Tile]);
	}
	Status = XRFdc_StartUp(InstancePtr, XRFDC_ADC_TILE, -1);
	if (Status != XRFDC_SUCCESS) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	Status = XRFdc_StartUp(InstancePtr, XRFDC_DAC_TILE, -1);
	if (Status != XRFDC_SUCCESS) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This function is used to get the clock distribution
*
* @param        InstancePtr is a pointer to the XRfdc instance.
* @param        DistributionSettingsPtr pointer to get the distribution settings
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if no valid distribution found.
*
******************************************************************************/
u32 XRFdc_GetClkDistribution(XRFdc *InstancePtr, XRFdc_Distribution_Settings *DistributionSettingsPtr)
{
	u32 Status;
	u16 ReadReg;
	u8 *Tile[8];
	u8 CurrentTile;
	s8 AdjacentTile;
	u8 DelayOutSourceLeft;
	u8 DelayOutSourceRight;
	u8 *Delays[8];
	XRFdc_Distribution *Distribution;
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(DistributionSettingsPtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if (InstancePtr->RFdc_Config.IPType < 2) {
		Status = XRFDC_FAILURE;
		metal_log(METAL_LOG_ERROR,
			  "\n Requested fuctionality not "
			  "available for this IP in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	Distribution = DistributionSettingsPtr->DistributionStatus;
	Delays[0] = &DistributionSettingsPtr->DAC[0].Delay;
	Delays[1] = &DistributionSettingsPtr->DAC[1].Delay;
	Delays[2] = &DistributionSettingsPtr->DAC[2].Delay;
	Delays[3] = &DistributionSettingsPtr->DAC[3].Delay;
	Delays[4] = &DistributionSettingsPtr->ADC[0].Delay;
	Delays[5] = &DistributionSettingsPtr->ADC[1].Delay;
	Delays[6] = &DistributionSettingsPtr->ADC[2].Delay;
	Delays[7] = &DistributionSettingsPtr->ADC[3].Delay;
	Tile[0] = &DistributionSettingsPtr->DAC[0].SourceTile;
	Tile[1] = &DistributionSettingsPtr->DAC[1].SourceTile;
	Tile[2] = &DistributionSettingsPtr->DAC[2].SourceTile;
	Tile[3] = &DistributionSettingsPtr->DAC[3].SourceTile;
	Tile[4] = &DistributionSettingsPtr->ADC[0].SourceTile;
	Tile[5] = &DistributionSettingsPtr->ADC[1].SourceTile;
	Tile[6] = &DistributionSettingsPtr->ADC[2].SourceTile;
	Tile[7] = &DistributionSettingsPtr->ADC[3].SourceTile;
	memset(DistributionSettingsPtr, XRFDC_CLK_DST_INVALID, sizeof(XRFdc_Distribution_Settings));

	for (CurrentTile = 0; CurrentTile < XRFDC_DIST_MAX; CurrentTile++) {
		DelayOutSourceLeft = 0;
		DelayOutSourceRight = 0;
		if (*Tile[CurrentTile] != XRFDC_CLK_DST_INVALID) {
			continue;
		}

		if (CurrentTile < XRFDC_CLK_DST_ADC0) { /*DAC*/
			ReadReg = XRFdc_ReadReg16(InstancePtr,
						  XRFDC_DRP_BASE(XRFDC_DAC_TILE, CurrentTile) + XRFDC_HSCOM_ADDR,
						  XRFDC_HSCOM_CLK_DSTR_OFFSET) &
				  XRFDC_HSCOM_CLK_DSTR_MASK;
		} else { /*ADC*/
			ReadReg = XRFdc_ReadReg16(InstancePtr,
						  XRFDC_DRP_BASE(XRFDC_ADC_TILE, (CurrentTile - XRFDC_CLK_DST_ADC0)) +
							  XRFDC_HSCOM_ADDR,
						  XRFDC_HSCOM_CLK_DSTR_OFFSET) &
				  XRFDC_HSCOM_CLK_DSTR_MASK;
		}

		if (ReadReg == XRFDC_CLK_DISTR_OFF) { /*it is its own source no dist*/
			*Tile[CurrentTile] = CurrentTile;
			*Delays[CurrentTile] = 0;
			Distribution->MaxDelay = 0;
			Distribution->MinDelay = 0;
			Distribution->IsDelayBalanced = 1;
			Distribution++;
		} else if (ReadReg & (XRFDC_CLK_DISTR_MUX6_SRC_INT |
				      XRFDC_CLK_DISTR_MUX7_SRC_INT)) { /*it is its own source, distributes its clk*/
			Distribution->MaxDelay = 0;
			Distribution->MinDelay = 255;
			Distribution->IsDelayBalanced = 0;
			*Tile[CurrentTile] = CurrentTile;
			if (ReadReg & XRFDC_CLK_DISTR_MUX7_SRC_STH) {
				DelayOutSourceLeft = 2;
			} else if (ReadReg & XRFDC_CLK_DISTR_MUX6_SRC_NTH) {
				DelayOutSourceRight = 2;
			}
			*Delays[CurrentTile] = DelayOutSourceLeft + DelayOutSourceRight + 2;
			Distribution->MaxDelay = MAX(Distribution->MaxDelay, (*Delays[CurrentTile]));
			Distribution->MinDelay = MIN(Distribution->MinDelay, (*Delays[CurrentTile]));
			/*work right*/
			for (AdjacentTile = CurrentTile + 1; AdjacentTile <= XRFDC_CLK_DST_ADC3; AdjacentTile++) {
				if (AdjacentTile < XRFDC_CLK_DST_ADC0) { /*DAC*/
					ReadReg = XRFdc_ReadReg16(InstancePtr,
								  XRFDC_DRP_BASE(XRFDC_DAC_TILE, AdjacentTile) +
									  XRFDC_HSCOM_ADDR,
								  XRFDC_HSCOM_CLK_DSTR_OFFSET) &
						  XRFDC_HSCOM_CLK_DSTR_MASK;
				} else { /*ADC*/
					ReadReg = XRFdc_ReadReg16(InstancePtr,
								  XRFDC_DRP_BASE(XRFDC_ADC_TILE,
										 (AdjacentTile - XRFDC_CLK_DST_ADC0)) +
									  XRFDC_HSCOM_ADDR,
								  XRFDC_HSCOM_CLK_DSTR_OFFSET) &
						  XRFDC_HSCOM_CLK_DSTR_MASK;
				}
				if ((ReadReg == XRFDC_CLK_DISTR_CONT_RIGHT_EVEN) &&
				    (AdjacentTile != XRFDC_CLK_DST_DAC3)) {
					*Tile[AdjacentTile] = CurrentTile;
					*Delays[AdjacentTile] =
						DelayOutSourceRight + ((AdjacentTile - CurrentTile) << 1) + 2;
					Distribution->MaxDelay = MAX(Distribution->MaxDelay, (*Delays[AdjacentTile]));
					Distribution->MinDelay = MIN(Distribution->MinDelay, (*Delays[AdjacentTile]));
				} else if (ReadReg == XRFDC_CLK_DISTR_CONT_RIGHT_HWL_ODD) {
					*Tile[AdjacentTile] = CurrentTile;
					*Delays[AdjacentTile] =
						DelayOutSourceRight + ((AdjacentTile - CurrentTile) << 1);
					Distribution->MaxDelay = MAX(Distribution->MaxDelay, (*Delays[AdjacentTile]));
					Distribution->MinDelay = MIN(Distribution->MinDelay, (*Delays[AdjacentTile]));
				} else if (ReadReg == XRFDC_CLK_DISTR_RIGHTMOST_TILE) {
					*Tile[AdjacentTile] = CurrentTile;
					*Delays[AdjacentTile] =
						DelayOutSourceRight + ((AdjacentTile - CurrentTile) << 1);
					Distribution->MaxDelay = MAX(Distribution->MaxDelay, (*Delays[AdjacentTile]));
					Distribution->MinDelay = MIN(Distribution->MinDelay, (*Delays[AdjacentTile]));
					break;
				} else {
					break;
				}
			}
			/*work left*/
			for (AdjacentTile = CurrentTile - 1; AdjacentTile >= XRFDC_CLK_DST_DAC0; AdjacentTile--) {
				if (*Tile[AdjacentTile] != XRFDC_CLK_DST_INVALID) {
					break;
				}
				if (AdjacentTile < XRFDC_CLK_DST_ADC0) { /*DAC*/
					ReadReg = XRFdc_ReadReg16(InstancePtr,
								  XRFDC_DRP_BASE(XRFDC_DAC_TILE, AdjacentTile) +
									  XRFDC_HSCOM_ADDR,
								  XRFDC_HSCOM_CLK_DSTR_OFFSET) &
						  XRFDC_HSCOM_CLK_DSTR_MASK;
				} else { /*ADC*/
					ReadReg = XRFdc_ReadReg16(InstancePtr,
								  XRFDC_DRP_BASE(XRFDC_ADC_TILE,
										 (AdjacentTile - XRFDC_CLK_DST_ADC0)) +
									  XRFDC_HSCOM_ADDR,
								  XRFDC_HSCOM_CLK_DSTR_OFFSET) &
						  XRFDC_HSCOM_CLK_DSTR_MASK;
				}
				if (ReadReg == XRFDC_CLK_DISTR_LEFTMOST_TILE) {
					*Tile[AdjacentTile] = CurrentTile;
					*Delays[AdjacentTile] =
						DelayOutSourceLeft + ((CurrentTile - AdjacentTile) << 1);
					break;
				} else if (ReadReg == XRFDC_CLK_DISTR_CONT_LEFT_EVEN) {
					*Tile[AdjacentTile] = CurrentTile;
					*Delays[AdjacentTile] =
						DelayOutSourceLeft + ((CurrentTile - AdjacentTile) << 1) + 2;
				} else if (ReadReg == XRFDC_CLK_DISTR_CONT_LEFT_ODD) {
					*Delays[AdjacentTile] =
						DelayOutSourceLeft + ((CurrentTile - AdjacentTile) << 1);
					*Tile[AdjacentTile] = CurrentTile;
				} else {
					break;
				}
				Distribution->MaxDelay = MAX(Distribution->MaxDelay, (*Delays[AdjacentTile]));
				Distribution->MinDelay = MIN(Distribution->MinDelay, (*Delays[AdjacentTile]));
			}
			Distribution->IsDelayBalanced = (Distribution->MaxDelay == Distribution->MinDelay) ? 1 : 0;
			Distribution++;
		}
	}
	Status = XRFdc_CheckClkDistValid(InstancePtr, DistributionSettingsPtr);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Invalid Distribution "
			  "in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* This function gets Clock source
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type indicates ADC/DAC.
* @param	Tile_Id indicates Tile number (0-3).
* @param	ClockSourcePtr Pointer to return the clock source
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		None.
*
******************************************************************************/
u32 XRFdc_GetClockSource(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
								u32 *ClockSourcePtr)
{
	u32 BaseAddr;
	u32 Status;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(ClockSourcePtr != NULL);

	Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Tile_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested tile not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_DRP_BASE(Type, Tile_Id) + XRFDC_HSCOM_ADDR;

	*ClockSourcePtr = XRFdc_RDReg(InstancePtr, BaseAddr,
				XRFDC_CLK_NETWORK_CTRL1, XRFDC_CLK_NETWORK_CTRL1_USE_PLL_MASK);

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This function gets PLL lock status
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type indicates ADC/DAC.
* @param	Tile_Id indicates Tile number (0-3).
* @param	LockStatusPtr Pointer to return the PLL lock status
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		None.
*
******************************************************************************/
u32 XRFdc_GetPLLLockStatus(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
							u32 *LockStatusPtr)
{
	u32 BaseAddr;
	u32 ReadReg;
	u32 ClkSrc = 0U;
	u32 Status;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(LockStatusPtr != NULL);

	/*
	 * Get Tile clock source information
	 */
	if (XRFdc_GetClockSource(InstancePtr, Type, Tile_Id, &ClkSrc)
								!= XRFDC_SUCCESS) {
			metal_log(METAL_LOG_ERROR, "\n Get clock source request Tile %d "
						"failed in %s\r\n", Tile_Id, __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	if (ClkSrc == XRFDC_EXTERNAL_CLK) {
		metal_log(METAL_LOG_DEBUG,  "\n Requested Tile %d "
					"uses external clock source in %s\r\n", Tile_Id, __func__);
		*LockStatusPtr = XRFDC_PLL_LOCKED;
	} else {
		if (Type == XRFDC_ADC_TILE) {
			BaseAddr = XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id);
		} else {
			BaseAddr = XRFDC_DAC_TILE_CTRL_STATS_ADDR(Tile_Id);
		}

		ReadReg = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_STATUS_OFFSET,
				XRFDC_PLL_LOCKED_MASK);
		if (ReadReg != 0U) {
			*LockStatusPtr = XRFDC_PLL_LOCKED;
		} else {
			*LockStatusPtr = XRFDC_PLL_UNLOCKED;
		}
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}

/*****************************************************************************/
/**
*
* This function used for configuring the internal PLL registers
* based on reference clock and sampling rate
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type indicates ADC/DAC.
* @param	Tile_Id indicates Tile number (0-3).
* @param	RefClkFreq Reference Clock Frequency MHz(50MHz - 1.2GHz)
* @param	SamplingRate Sampling Rate in MHz(0.5- 4 GHz)
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		None.
*
******************************************************************************/
static u32 XRFdc_SetPLLConfig(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		double RefClkFreq, double SamplingRate)
{
	u32 BaseAddr;
	u32 Status;
	u32 FeedbackDiv;
	u32 OutputDiv;
	double CalcSamplingRate;
	double PllFreq;
	double SamplingError;
	u32 Best_FeedbackDiv = 0x0U;
	u32 Best_OutputDiv = 0x2U;
	double Best_Error = 0xFFFFFFFFU;
	u32 DivideMode = 0x0U;
	u32 DivideValue = 0x0U;
	u32 PllFreqIndex = 0x0U;
	u32 FbDivIndex = 0x0U;
	u32 RefClkDiv = 0x1;
	u16 ReadReg;

	if (Type == XRFDC_ADC_TILE) {
		BaseAddr = XRFDC_ADC_TILE_DRP_ADDR(Tile_Id);
	} else {
		BaseAddr = XRFDC_DAC_TILE_DRP_ADDR(Tile_Id);
	}

	BaseAddr += XRFDC_HSCOM_ADDR;

	ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr, XRFDC_PLL_REFDIV);
	if (ReadReg & XRFDC_REFCLK_DIV_1_MASK) {
		RefClkDiv = XRFDC_REF_CLK_DIV_1;
	} else {
		switch (ReadReg & XRFDC_REFCLK_DIV_MASK) {
			case XRFDC_REFCLK_DIV_2_MASK:
				RefClkDiv = XRFDC_REF_CLK_DIV_2;
				break;
			case XRFDC_REFCLK_DIV_3_MASK:
				RefClkDiv = XRFDC_REF_CLK_DIV_3;
				break;
			case XRFDC_REFCLK_DIV_4_MASK:
				RefClkDiv = XRFDC_REF_CLK_DIV_4;
				break;
			default:
				/*
				 * IP currently supporting 1 to 4 divider values. This
				 * error condition might change in future based on IP update.
				 */
				metal_log(METAL_LOG_ERROR,  "\n Unsupported Reference "
						"clock Divider value in %s\r\n", __func__);
				return XRFDC_FAILURE;
		}
	}

	RefClkFreq /= RefClkDiv;

	/*
	 * Sweep valid integer values of FeedbackDiv(N) and record a list
	 * of values that fall in the valid VCO range 8.5GHz - 12.8GHz
	 */
	for (FeedbackDiv = PLL_FPDIV_MIN; FeedbackDiv <= PLL_FPDIV_MAX;
			FeedbackDiv++) {

		PllFreq = FeedbackDiv * RefClkFreq;

		if ((PllFreq >= VCO_RANGE_MIN) && (PllFreq <= VCO_RANGE_MAX)) {
			/*
			 * Sweep values of OutputDiv(M) to find the output frequency
			 * that best matches the user requested value
			 */

			for (OutputDiv = PLL_DIVIDER_MIN; OutputDiv <= PLL_DIVIDER_MAX;
					OutputDiv += 2U) {

				CalcSamplingRate = (PllFreq / OutputDiv);

				if (SamplingRate > CalcSamplingRate) {
					SamplingError = SamplingRate - CalcSamplingRate;
				} else {
					SamplingError = CalcSamplingRate - SamplingRate;
				}

				if (Best_Error > SamplingError) {
					Best_FeedbackDiv = FeedbackDiv;
					Best_OutputDiv = OutputDiv;
					Best_Error = SamplingError;
				}
			}

			OutputDiv = 3U;
			CalcSamplingRate = (PllFreq / OutputDiv);

			if (SamplingRate > CalcSamplingRate) {
				SamplingError = SamplingRate - CalcSamplingRate;
			} else {
				SamplingError = CalcSamplingRate - SamplingRate;
			}

			if (Best_Error > SamplingError) {
				Best_FeedbackDiv = FeedbackDiv;
				Best_OutputDiv = OutputDiv;
				Best_Error = SamplingError;
			}
		}

		/*
		 * PLL Static configuration
		 */
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_SDM_CFG0, 0x80U);
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_SDM_SEED0, 0x111U);
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_SDM_SEED1, 0x11U);
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_VREG, 0x45U);
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_VCO0, 0x5800U);
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_VCO1, 0x08U);

		/*
		 * Set Feedback divisor value
		 */
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_FPDIV,
				Best_FeedbackDiv - 2U);

		/*
		 * Set Output divisor value
		 */
		if (Best_OutputDiv == 2U) {
			DivideMode = 0x1U;
		} else if (Best_OutputDiv == 3U) {
			DivideMode = 0x2U;
			DivideValue = 0x1U;
		} else if (Best_OutputDiv >= 4U) {
			DivideMode = 0x3U;
			DivideValue = ((Best_OutputDiv - 4U)/2U);
		}

		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_PLL_DIVIDER0,
			XRFDC_PLL_DIVIDER0_MASK, ((DivideMode << XRFDC_PLL_DIVIDER0_SHIFT) | DivideValue));

		/*
		 * Enable fine sweep
		 */
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_CRS2, XRFDC_PLL_CRS2_VAL);

		/*
		 * Set default PLL spare inputs LSB
		 */
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_SPARE0, 0x507U);

		/*
		 * Set PLL spare inputs MSB
		 */
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_SPARE1, 0x0U);

		PllFreq = RefClkFreq * Best_FeedbackDiv;

		if (PllFreq < 9400U) {
			PllFreqIndex = 0U;
			FbDivIndex = 2U;
			if (Best_FeedbackDiv < 21U) {
				FbDivIndex = 0U;
			} else if (Best_FeedbackDiv < 30U) {
				FbDivIndex = 1U;
			}
		} else if (PllFreq < 10070U) {
			PllFreqIndex = 1U;
			FbDivIndex = 2U;
			if (Best_FeedbackDiv < 18U) {
				FbDivIndex = 0U;
			} else if (Best_FeedbackDiv < 30U) {
				FbDivIndex = 1U;
			}
		} else if (PllFreq < 10690U) {
			PllFreqIndex = 2U;
			FbDivIndex = 3U;
			if (Best_FeedbackDiv < 18U) {
				FbDivIndex = 0U;
			} else if (Best_FeedbackDiv < 25U) {
				FbDivIndex = 1U;
			} else if (Best_FeedbackDiv < 35U) {
				FbDivIndex = 2U;
			}
		} else if (PllFreq < 10990U) {
			PllFreqIndex = 3U;
			FbDivIndex = 3U;
			if (Best_FeedbackDiv < 19U) {
				FbDivIndex = 0U;
			} else if (Best_FeedbackDiv < 27U) {
				FbDivIndex = 1U;
			} else if (Best_FeedbackDiv < 38U) {
				FbDivIndex = 2U;
			}
		} else if (PllFreq < 11430U) {
			PllFreqIndex = 4U;
			FbDivIndex = 3U;
			if (Best_FeedbackDiv < 19U) {
				FbDivIndex = 0U;
			} else if (Best_FeedbackDiv < 27U) {
				FbDivIndex = 1U;
			} else if (Best_FeedbackDiv < 38U) {
				FbDivIndex = 2U;
			}
		} else if (PllFreq < 12040U) {
			PllFreqIndex = 5U;
			FbDivIndex = 3U;
			if (Best_FeedbackDiv < 20U) {
				FbDivIndex = 0U;
			} else if (Best_FeedbackDiv < 28U) {
				FbDivIndex = 1U;
			} else if (Best_FeedbackDiv < 40U) {
				FbDivIndex = 2U;
			}
		} else if (PllFreq < 12530U) {
			PllFreqIndex = 6U;
			FbDivIndex = 3U;
			if (Best_FeedbackDiv < 23U) {
				FbDivIndex = 0U;
			} else if (Best_FeedbackDiv < 30U) {
				FbDivIndex = 1U;
			} else if (Best_FeedbackDiv < 42U) {
				FbDivIndex = 2U;
			}
		} else if (PllFreq < 20000U) {
			PllFreqIndex = 7U;
			FbDivIndex = 2U;
			if (Best_FeedbackDiv < 20U) {
				FbDivIndex = 0U;
				/*
				 * Set PLL spare inputs LSB
				 */
				XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_SPARE0, 0x577);
			} else if (Best_FeedbackDiv < 39U) {
				FbDivIndex = 1U;
			}
		}

		/*
		 * Enable automatic selection of the VCO, this will work with the
		 * IP version 2.0.1 and above and using older version of IP is
		 * not likely to work.
		 */
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_PLL_CRS1,
				XRFDC_PLL_VCO_SEL_AUTO_MASK, XRFDC_PLL_VCO_SEL_AUTO_MASK);

		/*
		 * PLL bits for loop filters LSB
		 */
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_LPF0,
				PllTuningMatrix[PllFreqIndex][FbDivIndex][0]);

		/*
		 * PLL bits for loop filters MSB
		 */
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_LPF1, XRFDC_PLL_LPF1_VAL);

		/*
		 * Set PLL bits for charge pumps
		 */
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_PLL_CHARGEPUMP,
				PllTuningMatrix[PllFreqIndex][FbDivIndex][1]);
	}

	CalcSamplingRate = (Best_FeedbackDiv * RefClkFreq) / Best_OutputDiv;
	CalcSamplingRate /= XRFDC_MILLI;

	if (Type == XRFDC_ADC_TILE) {
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.SampleRate =
							CalcSamplingRate;
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.RefClkDivider = RefClkDiv;
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.FeedbackDivider = Best_FeedbackDiv;
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.OutputDivider = Best_OutputDiv;
	} else {
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.SampleRate =
					CalcSamplingRate;
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.RefClkDivider = RefClkDiv;
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.FeedbackDivider = Best_FeedbackDiv;
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.OutputDivider = Best_OutputDiv;
	}

	Status = XRFDC_SUCCESS;

	return Status;
}
/*****************************************************************************/
/**
*
* This API is used to get the PLL Configurations.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type represents ADC or DAC.
* @param	Tile_Id Valid values are 0-3.
* @param	PLLSettings pointer to the XRFdc_PLL_Settings structure to get
*           the PLL configurations
*
* @return   None
*
******************************************************************************/
u32 XRFdc_GetPLLConfig(XRFdc *InstancePtr, u32 Type,
					u32 Tile_Id, XRFdc_PLL_Settings *PLLSettings)
{
	u32 Status;
	u32 BaseAddr;
	u16 ReadReg;
	double RefClkFreq;
	double SampleRate;
	u32 FeedbackDivider;
	u8 OutputDivider;
	u32 RefClkDivider;
	u32 Enabled;
	u8 DivideMode;
	u32 PLLFreq;
	u32 PLLFS;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);
	Xil_AssertNonvoid(PLLSettings != NULL);

	Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Tile_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested tile not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_CTRL_STS_BASE(Type, Tile_Id);
	PLLFreq = XRFdc_ReadReg(InstancePtr, BaseAddr, XRFDC_PLL_FREQ);

	RefClkFreq = ((double)PLLFreq)/1000;
	PLLFS = XRFdc_ReadReg(InstancePtr, BaseAddr, XRFDC_PLL_FS);
	SampleRate = ((double)PLLFS)/1000000;
	if (PLLFS == 0) {
		/*This code is here to support the old IPs.*/
		if (Type == XRFDC_ADC_TILE) {
			PLLSettings->Enabled =
					InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.Enabled;
			PLLSettings->FeedbackDivider =
					InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.FeedbackDivider;
			PLLSettings->OutputDivider =
					InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.OutputDivider;
			PLLSettings->RefClkDivider =
					InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.RefClkDivider;
			PLLSettings->RefClkFreq =
					InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.RefClkFreq;
			PLLSettings->SampleRate =
					InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.SampleRate;
			Status = XRFDC_SUCCESS;
			goto RETURN_PATH;
		} else {
			PLLSettings->Enabled =
					InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.Enabled;
			PLLSettings->FeedbackDivider =
					InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.FeedbackDivider;
			PLLSettings->OutputDivider =
					InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.OutputDivider;
			PLLSettings->RefClkDivider =
					InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.RefClkDivider;
			PLLSettings->RefClkFreq =
					InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.RefClkFreq;
			PLLSettings->SampleRate =
					InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.SampleRate;
			Status = XRFDC_SUCCESS;
			goto RETURN_PATH;
		}
	} else {
		if (Type == XRFDC_ADC_TILE) {
			BaseAddr = XRFDC_ADC_TILE_DRP_ADDR(Tile_Id);
		} else {
			BaseAddr = XRFDC_DAC_TILE_DRP_ADDR(Tile_Id);
		}

		BaseAddr += XRFDC_HSCOM_ADDR;

		FeedbackDivider = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_PLL_FPDIV, 0x00FF) + 2;

		ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr, XRFDC_PLL_REFDIV);
		if (ReadReg & XRFDC_REFCLK_DIV_1_MASK) {
			RefClkDivider = XRFDC_REF_CLK_DIV_1;
		} else {
			switch (ReadReg & XRFDC_REFCLK_DIV_MASK) {
				case XRFDC_REFCLK_DIV_2_MASK:
					RefClkDivider = XRFDC_REF_CLK_DIV_2;
					break;
				case XRFDC_REFCLK_DIV_3_MASK:
					RefClkDivider = XRFDC_REF_CLK_DIV_3;
					break;
				case XRFDC_REFCLK_DIV_4_MASK:
					RefClkDivider = XRFDC_REF_CLK_DIV_4;
					break;
				default:
					/*
					 * IP currently supporting 1 to 4 divider values. This
					 * error condition might change in future based on IP update.
					 */
					metal_log(METAL_LOG_ERROR,  "\n Unsupported Reference "
							"clock Divider value in %s\r\n", __func__);
					Status = XRFDC_FAILURE;
					goto RETURN_PATH;
			}
		}
		if (InstancePtr->RFdc_Config.IPType < 2) {
			if (XRFdc_GetClockSource(InstancePtr, Type, Tile_Id, &Enabled)
								!= XRFDC_SUCCESS) {
				Status = XRFDC_FAILURE;
				goto RETURN_PATH;
			}
		} else {
			Enabled = (ReadReg & XRFDC_PLLREFDIV_INPUT_OFF)?XRFDC_DISABLED:XRFDC_ENABLED;
		}

		ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr, XRFDC_PLL_DIVIDER0);
		DivideMode = (ReadReg & XRFDC_PLL_DIVIDER0_MODE_MASK) >> XRFDC_PLL_DIVIDER0_SHIFT;

		switch(DivideMode) {
			case XRFDC_PLL_OUTDIV_MODE_1:
				OutputDivider = 1;
				break;
			case XRFDC_PLL_OUTDIV_MODE_2:
				OutputDivider = 2;
				break;
			case XRFDC_PLL_OUTDIV_MODE_3:
				OutputDivider = 3;
				break;
			case XRFDC_PLL_OUTDIV_MODE_N:
				OutputDivider = ((ReadReg & XRFDC_PLL_DIVIDER0_VALUE_MASK) + 2) << 1;
				break;
			default:
				metal_log(METAL_LOG_ERROR,  "\n Unsupported Output "
							"clock Divider value in %s\r\n", __func__);
					Status = XRFDC_FAILURE;
					goto RETURN_PATH;
				break;
		}
		PLLSettings->Enabled = Enabled;
		PLLSettings->FeedbackDivider = FeedbackDivider;
		PLLSettings->OutputDivider = OutputDivider;
		PLLSettings->RefClkDivider = RefClkDivider;
		PLLSettings->RefClkFreq = RefClkFreq;
		PLLSettings->SampleRate = SampleRate;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:

	return Status;
}

/*****************************************************************************/
/**
*
* This function used for dynamically switch between internal PLL and
* external clcok source and configuring the internal PLL
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type indicates ADC/DAC
* @param	Tile_Id indicates Tile number (0-3)
* @param	Source Clock source internal PLL or external clock source
* @param	RefClkFreq Reference Clock Frequency in MHz(102.40625MHz - 1.2GHz)
* @param	SamplingRate Sampling Rate in MHz(0.1- 6.554GHz for DAC and
*           0.5/1.0 - 2.058/4.116GHz for ADC based on the device package).
*
* @return
*       - XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Tile not enabled.
*
* @note		This API enables automatic selection of the VCO which will work in
*           IP version 2.0.1 and above. Using older version of IP this API is
*           not likely to work.
*
******************************************************************************/
u32 XRFdc_DynamicPLLConfig(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u8 Source, double RefClkFreq, double SamplingRate)
{
	u32 ClkSrc = 0U;
	u32 Status;
	u32 BaseAddr;
	u32 PLLEnable = 0x0;
	u32 InitialPowerUpState;
	double MaxSampleRate;
	double MinSampleRate;
	u32 PLLFreq;
	u32 PLLFS;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if ((Source != XRFDC_INTERNAL_PLL_CLK) &&
			(Source != XRFDC_EXTERNAL_CLK)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid Source "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Tile_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Requested tile not "
								"available in %s\r\n", __func__);
		goto RETURN_PATH;
	}

	/*
	 * Get Tile clock source information
	 */
	if (XRFdc_GetClockSource(InstancePtr, Type, Tile_Id, &ClkSrc)
								!= XRFDC_SUCCESS) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	if (XRFdc_GetMaxSampleRate(InstancePtr, Type, Tile_Id, &MaxSampleRate) != XRFDC_SUCCESS) {
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}
	if (XRFdc_GetMinSampleRate(InstancePtr, Type, Tile_Id, &MinSampleRate) != XRFDC_SUCCESS) {
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
	}
	if ((SamplingRate < MinSampleRate) ||
		(SamplingRate > MaxSampleRate)) {
		metal_log(METAL_LOG_ERROR,  "\n Invalid sampling "
				"rate value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_CTRL_STS_BASE(Type, Tile_Id);

	if (Source == XRFDC_INTERNAL_PLL_CLK) {
		if ((RefClkFreq < XRFDC_REFFREQ_MIN) ||
				(RefClkFreq > XRFDC_REFFREQ_MAX)) {
			metal_log(METAL_LOG_ERROR,  "\n Input reference clock "
					"frequency does not respect the specifications "
					"for internal PLL usage. Please use a different "
					"frequency or bypass the internal PLL", __func__);
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
	}

	PLLFreq = (u32)(RefClkFreq*1000);
	PLLFS = (u32)(SamplingRate*1000);
	XRFdc_WriteReg(InstancePtr, BaseAddr, XRFDC_PLL_FREQ, PLLFreq);
	XRFdc_WriteReg(InstancePtr, BaseAddr, XRFDC_PLL_FS, PLLFS);

	if ((Source != XRFDC_INTERNAL_PLL_CLK) &&
			(ClkSrc != XRFDC_INTERNAL_PLL_CLK)) {
		metal_log(METAL_LOG_DEBUG,  "\n Requested Tile %d "
					"uses external clock source in %s\r\n", Tile_Id, __func__);
		if (Type == XRFDC_ADC_TILE) {
			InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.SampleRate =
						(double)(SamplingRate/1000);
			InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.RefClkFreq = RefClkFreq;
		} else {
			InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.SampleRate =
						(double)(SamplingRate/1000);
			InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.RefClkFreq = RefClkFreq;
		}
		Status = XRFDC_SUCCESS;
		goto RETURN_PATH;
	}

	if (Type == XRFDC_ADC_TILE) {
		BaseAddr = XRFDC_ADC_TILE_CTRL_STATS_ADDR(Tile_Id);
		InitialPowerUpState = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_STATUS_OFFSET, XRFDC_PWR_UP_STAT_MASK) >> XRFDC_PWR_UP_STAT_SHIFT;
		BaseAddr = XRFDC_ADC_TILE_DRP_ADDR(Tile_Id) + XRFDC_HSCOM_ADDR;
	} else {
		BaseAddr = XRFDC_DAC_TILE_CTRL_STATS_ADDR(Tile_Id);
		InitialPowerUpState = XRFdc_RDReg(InstancePtr, BaseAddr, XRFDC_STATUS_OFFSET, XRFDC_PWR_UP_STAT_MASK) >> XRFDC_PWR_UP_STAT_SHIFT;
		BaseAddr = XRFDC_DAC_TILE_DRP_ADDR(Tile_Id) + XRFDC_HSCOM_ADDR;
	}

	/*
	 * Stop the ADC or DAC tile by putting tile in reset state if not stopped already
	 */
	if (InitialPowerUpState != XRFDC_DISABLED) {
		Status = XRFdc_Shutdown(InstancePtr, Type, Tile_Id) ;
		if (Status != XRFDC_SUCCESS) {
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
	}
	if (Source == XRFDC_INTERNAL_PLL_CLK) {
		PLLEnable = 0x1;
		/*
		 * Configure the PLL
		 */
		if (XRFdc_SetPLLConfig(InstancePtr, Type, Tile_Id, RefClkFreq,
				SamplingRate) != XRFDC_SUCCESS) {
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}

		XRFdc_ClrSetReg(InstancePtr, BaseAddr,
			XRFDC_CLK_NETWORK_CTRL1, XRFDC_CLK_NETWORK_CTRL1_USE_PLL_MASK,
			XRFDC_CLK_NETWORK_CTRL1_USE_PLL_MASK);
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_HSCOM_PWR_STATE_OFFSET,
				XRFDC_HSCOM_PWR_STATS_PLL);
	} else {
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_CLK_NETWORK_CTRL1,
				XRFDC_CLK_NETWORK_CTRL1_USE_PLL_MASK, 0x0);
		XRFdc_WriteReg16(InstancePtr, BaseAddr, XRFDC_HSCOM_PWR_STATE_OFFSET,
				XRFDC_HSCOM_PWR_STATS_EXTERNAL);
		SamplingRate /= XRFDC_MILLI;

		if (Type == XRFDC_ADC_TILE) {
			InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.SampleRate =
					SamplingRate;
			InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.RefClkDivider = 0x0U;
			InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.FeedbackDivider = 0x0U;
			InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.OutputDivider = 0x0U;
		} else {
			InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.SampleRate =
					SamplingRate;
			InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.RefClkDivider = 0x0U;
			InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.FeedbackDivider = 0x0U;
			InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.OutputDivider = 0x0U;
		}
	}

	/*
	 * Re-start the ADC or DAC tile if tile was shut down in this function
	 */
	if (InitialPowerUpState != XRFDC_DISABLED) {
		Status = XRFdc_StartUp(InstancePtr, Type, Tile_Id) ;
		if (Status != XRFDC_SUCCESS) {
			Status = XRFDC_FAILURE;
			goto RETURN_PATH;
		}
	}

	if (Type == XRFDC_ADC_TILE) {
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.RefClkFreq = RefClkFreq;
		InstancePtr->ADC_Tile[Tile_Id].PLL_Settings.Enabled = PLLEnable;
	} else {
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.RefClkFreq = RefClkFreq;
		InstancePtr->DAC_Tile[Tile_Id].PLL_Settings.Enabled = PLLEnable;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/** @} */
