/******************************************************************************
*
* Copyright (C) 2018-2019 Xilinx, Inc.  All rights reserved.
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
* @file xrfdc_mts.c
* @addtogroup xrfdc_v6_0
* @{
*
* Contains the multi tile sync functions of the XRFdc driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---    -------- -----------------------------------------------
* 3.1   jm     01/24/18 Initial release
* 3.2   jm     03/12/18 Fixed DAC latency calculation.
*       jm     03/12/18 Added support for reloading DTC scans.
*       jm     03/12/18 Add option to configure sysref capture after MTS.
* 4.0   sk     04/09/18 Added API to enable/disable the sysref.
*       rk     04/17/18 Adjust calculated latency by sysref period, where doing
*                       so results in closer alignment to the target latency.
* 5.0   sk     08/03/18 Fixed MISRAC warnings.
*       sk     08/03/18 Check for Block0 enable for tiles participating in MTS.
*       sk     08/24/18 Reorganize the code to improve readability and
*                       optimization.
* 5.1   cog    01/29/19 Replace structure reference ADC checks with
*                       function.
* 6.0   cog    02/17/19 Added XRFdc_GetMTSEnable API.
*
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "mpm/rfdc/xrfdc_mts.h"

/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

static void XRFdc_MTS_Sysref_TRx(XRFdc *InstancePtr, u32 Enable);
static void XRFdc_MTS_Sysref_Ctrl(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
			u32 Is_PLL, u32 Enable_Cap, u32 Enable_Div_Reset);
static u32 XRFdc_MTS_Sysref_Dist(XRFdc *InstancePtr, int Num_DAC);
static u32 XRFdc_MTS_Sysref_Count(XRFdc *InstancePtr, u32 Type, u32 Count_Val);
static u32 XRFdc_MTS_Dtc_Scan(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
					XRFdc_MTS_DTC_Settings *SettingsPtr);
static u32 XRFdc_MTS_Dtc_Code(XRFdc *InstancePtr, u32 Type, u32 BaseAddr,
		u32 SRCtrlAddr, u32 DTCAddr, u16 SRctl, u16 SRclr_m, u32 Code);
static u32 XRFdc_MTS_Dtc_Calc(u32 Type, u32 Tile_Id,
				XRFdc_MTS_DTC_Settings *SettingsPtr, u8 *FlagsPtr);
static void XRFdc_MTS_Dtc_Flag_Debug(u8 *FlagsPtr, u32 Type, u32 Tile_Id,
						u32 Target, u32 Picked);
static void XRFdc_MTS_FIFOCtrl(XRFdc *InstancePtr, u32 Type, u32 FIFO_Mode,
							u32 Tiles_To_Clear);
static u32 XRFdc_MTS_GetMarker(XRFdc *InstancePtr, u32 Type, u32 Tiles,
				XRFdc_MTS_Marker *MarkersPtr, int Marker_Delay);
static void XRFdc_MTS_Marker_Read(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 FIFO_Id, u32 *CountPtr, u32 *LocPtr, u32 *DonePtr);
static u32 XRFdc_MTS_Latency(XRFdc *InstancePtr, u32 Type,
	XRFdc_MultiConverter_Sync_Config *ConfigPtr, XRFdc_MTS_Marker *MarkersPtr);

/*****************************************************************************/
/**
*
* This API enables the master tile sysref Tx/Rx
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Enable the master tile sysref for Tx/Rx, valid values are 0 and 1.
*
* @return
*		- None.
*
* @note		None
*
******************************************************************************/
static void XRFdc_MTS_Sysref_TRx(XRFdc *InstancePtr, u32 Enable)
{
	u32 BaseAddr;
	u32 Data;

	BaseAddr = XRFDC_DRP_BASE(XRFDC_DAC_TILE, 0) + XRFDC_HSCOM_ADDR;
	Data = (Enable != 0U) ? 0xFFFFU : 0U;

	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_MTS_SRCAP_T1,
				XRFDC_MTS_SRCAP_EN_TRX_M, Data);
}

/*****************************************************************************/
/**
*
* This API Control SysRef Capture Settings
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Is_PLL Valid values are 0 and 1.
* @param	Enable_Cap Valid values are 0 and 1.
* @param	Enable_Div_Reset Valid values are 0 and 1.
*
* @return
*		- None.
*
* @note		None
*
******************************************************************************/
static void XRFdc_MTS_Sysref_Ctrl(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
			u32 Is_PLL, u32 Enable_Cap, u32 Enable_Div_Reset)
{
	u32 BaseAddr;
	u16 RegData;

	RegData = 0U;
	BaseAddr = XRFDC_DRP_BASE(Type, Tile_Id) + XRFDC_HSCOM_ADDR;

	/* Write some bits to ensure sysref is in the right mode */
	XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_MTS_SRCAP_T1,
			XRFDC_MTS_SRCAP_INIT_M, 0U);

	if (Is_PLL != 0U) {
		/* PLL Cap */
		RegData = (Enable_Cap != 0U) ? XRFDC_MTS_SRCAP_PLL_M : 0U;
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_MTS_SRCAP_PLL,
				XRFDC_MTS_SRCAP_PLL_M, RegData);
	} else {
		/* Analog Cap disable */
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_MTS_SRCAP_T1,
				XRFDC_MTS_SRCAP_T1_EN, 0U);

		/* Analog Divider */
		RegData  = (Enable_Div_Reset != 0U) ? 0U : XRFDC_MTS_SRCAP_T1_RST;
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_MTS_SRCAP_T1,
				XRFDC_MTS_SRCAP_T1_RST, RegData);

		/* Digital Divider */
		RegData  = (Enable_Div_Reset != 0U) ? 0U : XRFDC_MTS_SRCAP_DIG_M;
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_MTS_SRCAP_DIG,
				XRFDC_MTS_SRCAP_DIG_M, RegData);

		/* Set SysRef Cap Clear */
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_MTS_SRCAP_T1,
		XRFDC_MTS_SRCLR_T1_M, XRFDC_MTS_SRCLR_T1_M);

		/* Analog Cap enable */
		RegData  = (Enable_Cap != 0U) ? XRFDC_MTS_SRCAP_T1_EN : 0U;
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_MTS_SRCAP_T1,
				XRFDC_MTS_SRCAP_T1_EN, RegData);

		/* Unset SysRef Cap Clear */
		XRFdc_ClrSetReg(InstancePtr, BaseAddr, XRFDC_MTS_SRCAP_T1,
		XRFDC_MTS_SRCLR_T1_M, 0U);
	}
}

/*****************************************************************************/
/**
*
* This API Update SysRef Distribution between tiles
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Num_DAC is number of DAC tiles
*
* @return
*		- XRFDC_MTS_OK if successful.
*		- XRFDC_MTS_NOT_SUPPORTED
*
* @note		None
*
******************************************************************************/
static u32 XRFdc_MTS_Sysref_Dist(XRFdc *InstancePtr, int Num_DAC)
{

	if (Num_DAC < 0) {
		/* Auto-detect. Only 2 types Supported - 2GSPS ADCs, 4GSPS ADCs */
		if (XRFdc_IsHighSpeedADC(InstancePtr,0) != 0U) {
			Num_DAC = 2;
		} else {
			Num_DAC = 4;
		}
	}

	if (Num_DAC == XRFDC_NUM_OF_TILES2) {
		/* 2 DACs, 4ADCs */
		XRFdc_WriteReg16(InstancePtr, XRFDC_DAC_TILE_DRP_ADDR(0U),
					XRFDC_MTS_SRDIST, 0xC980U);
		XRFdc_WriteReg16(InstancePtr, XRFDC_DAC_TILE_DRP_ADDR(1U),
					XRFDC_MTS_SRDIST, 0x0100U);
		XRFdc_WriteReg16(InstancePtr, XRFDC_ADC_TILE_DRP_ADDR(3U),
					XRFDC_MTS_SRDIST, 0x1700U);
	} else if (Num_DAC == XRFDC_NUM_OF_TILES4) {
		/* 4 DACs, 4ADCs */
		XRFdc_WriteReg16(InstancePtr, XRFDC_DAC_TILE_DRP_ADDR(0U),
						XRFDC_MTS_SRDIST, 0xCA80U);
		XRFdc_WriteReg16(InstancePtr, XRFDC_DAC_TILE_DRP_ADDR(1U),
						XRFDC_MTS_SRDIST, 0x2400U);
		XRFdc_WriteReg16(InstancePtr, XRFDC_DAC_TILE_DRP_ADDR(2U),
						XRFDC_MTS_SRDIST, 0x0980U);
		XRFdc_WriteReg16(InstancePtr, XRFDC_DAC_TILE_DRP_ADDR(3U),
						XRFDC_MTS_SRDIST, 0x0100U);
		XRFdc_WriteReg16(InstancePtr, XRFDC_ADC_TILE_DRP_ADDR(3U),
					XRFDC_MTS_SRDIST, 0x0700U);
	} else {
		return XRFDC_MTS_NOT_SUPPORTED;
	}

	XRFdc_WriteReg16(InstancePtr, XRFDC_ADC_TILE_DRP_ADDR(0U),
						XRFDC_MTS_SRDIST, 0x0280U);
	XRFdc_WriteReg16(InstancePtr, XRFDC_ADC_TILE_DRP_ADDR(1U),
						XRFDC_MTS_SRDIST, 0x0600U);
	XRFdc_WriteReg16(InstancePtr, XRFDC_ADC_TILE_DRP_ADDR(2U),
						XRFDC_MTS_SRDIST, 0x8880U);

	return XRFDC_MTS_OK;
}

/*****************************************************************************/
/**
*
* This API Wait for a number of sysref's to be captured
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Count_Val to wait for a number of sysref's to be captured.
*
* @return
*		- XRFDC_MTS_OK if successful.
*		- XRFDC_MTS_TIMEOUT if timeout occurs.
*
* @note		None
*
******************************************************************************/
static u32 XRFdc_MTS_Sysref_Count(XRFdc *InstancePtr, u32 Type, u32 Count_Val)
{
	u32 RegData;
	u32 Timeout;
	u32 Shift;

	RegData = (Type == XRFDC_DAC_TILE) ? 0x2U : 0x1U;
	Shift   = (Type == XRFDC_DAC_TILE) ? 8U : 0U;

	/* Start counter */
	XRFdc_WriteReg(InstancePtr, 0U, XRFDC_MTS_SRCOUNT_CTRL, RegData);

	/* Check counter with timeout in case sysref is not active */
	Timeout = 0U;
	while (Timeout < XRFDC_MTS_SRCOUNT_TIMEOUT) {
		RegData = XRFdc_ReadReg(InstancePtr, 0U, XRFDC_MTS_SRCOUNT_VAL);
		RegData = ((RegData >> Shift) & XRFDC_MTS_SRCOUNT_M);
		if (RegData >= Count_Val) {
			break;
		}
		Timeout++;
	}

	if (Timeout >= XRFDC_MTS_SRCOUNT_TIMEOUT) {
		metal_log(METAL_LOG_ERROR,
			"PL SysRef Timeout - PL SysRef not active: %d\n in %s\n",
			Timeout, __func__);
		return XRFDC_MTS_TIMEOUT;
	}

	return XRFDC_MTS_OK;
}

/*****************************************************************************/
/**
*
* This API print the DTC scan results
*
*
* @param	FlagsPtr is for internal usage.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	Target is for internal usage.
* @param	Picked is for internal usage.
*
* @return	None
*
* @note		None
*
******************************************************************************/
static void XRFdc_MTS_Dtc_Flag_Debug(u8 *FlagsPtr, u32 Type, u32 Tile_Id,
							u32 Target, u32 Picked)
{
	u32 Index;
	char buf[XRFDC_MTS_NUM_DTC+1];

	for (Index = 0U; Index < XRFDC_MTS_NUM_DTC; Index++) {
		if (Index == Picked) {
			buf[Index] = '*';
		} else if (Index == Target) {
			buf[Index] = '#';
		} else {
			buf[Index] = '0' + FlagsPtr[Index];
		}
	}
	buf[XRFDC_MTS_NUM_DTC] = '\0';
	metal_log(METAL_LOG_INFO, "%s%d: %s\n",
		(Type == XRFDC_DAC_TILE) ? "DAC" : "ADC", Tile_Id, buf);

	(void)buf;
	(void)Type;
	(void)Tile_Id;

}

/*****************************************************************************/
/**
*
* This API Calculate the best DTC code to use
*
*
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	SettingsPtr dtc settings structure.
* @param	FlagsPtr is for internal usage.
*
* @return
* 		- XRFDC_MTS_OK if successful.
*		- XRFDC_MTS_NOT_SUPPORTED if MTS is not supported.
*
* @note		None
*
******************************************************************************/
static u32 XRFdc_MTS_Dtc_Calc(u32 Type, u32 Tile_Id,
				XRFdc_MTS_DTC_Settings *SettingsPtr, u8 *FlagsPtr)
{
	u32 Index, Status, Num_Found;
	int Last, Current_Gap, Max_Overlap, Overlap_Cnt;
	int Min_Gap, Max_Gap, Diff, Min_Diff, Min_Range, Val, Target;
	u8 Min_Gap_Allowed;
	int Codes[XRFDC_MTS_MAX_CODE] = {0};

	Min_Gap_Allowed = (SettingsPtr->IsPLL != 0U) ? XRFDC_MTS_MIN_GAP_PLL :
							XRFDC_MTS_MIN_GAP_T1;
	Status = XRFDC_MTS_OK;

	/* Scan the flags and find candidate DTC codes */
	Num_Found = 0U;
	Max_Gap = 0;
	Min_Gap = XRFDC_MTS_NUM_DTC;
	Max_Overlap = 0;
	Overlap_Cnt = 0;
	Last = -1;
	FlagsPtr[XRFDC_MTS_NUM_DTC] = 1;
	for (Index = 0U; Index <= XRFDC_MTS_NUM_DTC; Index++) {
		Current_Gap = Index-Last;
		if (FlagsPtr[Index] != 0) {
			if (Current_Gap > Min_Gap_Allowed) {
				Codes[Num_Found] = Last + (Current_Gap / 2);
				Num_Found++;
				/* Record max/min gaps */
				Current_Gap--;
				if (Current_Gap > Max_Gap) {
					Max_Gap = Current_Gap;
				}
				if (Current_Gap < Min_Gap) {
					Min_Gap = Current_Gap;
				}
			}
			Last = Index;
		}
		/* check for the longest run of overlapping codes */
		if (FlagsPtr[Index] == 3U) {
			Overlap_Cnt++;
			if (Overlap_Cnt > Max_Overlap) {
				Max_Overlap = Overlap_Cnt;
			}
		} else {
			Overlap_Cnt = 0;
		}
	}

	/* Record some stats */
	SettingsPtr->Num_Windows[Tile_Id] = Num_Found;
	SettingsPtr->Max_Gap[Tile_Id]     = Max_Gap;
	SettingsPtr->Min_Gap[Tile_Id]     = Min_Gap;
	SettingsPtr->Max_Overlap[Tile_Id] = Max_Overlap;

	/* Calculate the best code */
	if (SettingsPtr->Scan_Mode == XRFDC_MTS_SCAN_INIT) {
		/* Initial scan */
		if (Tile_Id == SettingsPtr->RefTile) {
			/* RefTile: Get the code closest to the target */
			Target   = XRFDC_MTS_REF_TARGET;
			SettingsPtr->Target[Tile_Id] = XRFDC_MTS_REF_TARGET;
			Min_Diff = XRFDC_MTS_NUM_DTC;
			/* scan all codes to find the closest */
			for (Index = 0U; Index < Num_Found; Index++) {
				Diff = abs(Target - Codes[Index]);
				if (Diff < Min_Diff) {
					Min_Diff = Diff;
					SettingsPtr->DTC_Code[Tile_Id] = Codes[Index];
				}
				metal_log(METAL_LOG_DEBUG,
					"Target %d, DTC Code %d, Diff %d, Min %d\n", Target,
					Codes[Index], Diff, Min_Diff);
			}
			/* set the reference code as the target for the other tiles */
			for (Index = 0U; Index < 4U; Index++) {
				if (Index != Tile_Id) {
					SettingsPtr->Target[Index] = SettingsPtr->DTC_Code[Tile_Id];
				}
			}
			metal_log(METAL_LOG_DEBUG,
					"RefTile (%d): DTC Code Target %d, Picked %d\n", Tile_Id,
					Target, SettingsPtr->DTC_Code[Tile_Id]);

		} else {
			/*
			 *  Other Tiles: Get the code that minimises the total range of codes
			 *  compute the range of the existing dtc codes
			 */
			Max_Gap = 0;
			Min_Gap = XRFDC_MTS_NUM_DTC;
			for (Index = XRFDC_TILE_ID0; Index < XRFDC_TILE_ID4; Index++) {
				Val = SettingsPtr->DTC_Code[Index];
				if ((Val != -1) && (Val > Max_Gap)) {
					Max_Gap = Val;
				}
				if ((Val != -1) && (Val < Min_Gap)) {
					Min_Gap = Val;
				}
			}
			metal_log(METAL_LOG_DEBUG,
					"Tile (%d): Max/Min %d/%d, Range %d\n", Tile_Id, Max_Gap,
					Min_Gap, Max_Gap-Min_Gap);
			Min_Range = XRFDC_MTS_NUM_DTC;
			for (Index = 0U; Index < Num_Found; Index++) {
				Val = Codes[Index];
				Diff = Max_Gap - Min_Gap;
				if (Val < Min_Gap) {
					Diff = Max_Gap - Val;
				}
				if (Val > Max_Gap) {
					Diff = Val - Min_Gap;
				}
				if (Diff <= Min_Range) {
					Min_Range = Diff;
					SettingsPtr->DTC_Code[Tile_Id] = Val;
				}
				metal_log(METAL_LOG_DEBUG,
					"Tile (%d): Code %d, New-Range: %d, Min-Range: %d\n",
					Tile_Id, Val, Diff, Min_Range);
			}
			metal_log(METAL_LOG_DEBUG,
					"Tile (%d): Code %d, Range Prev %d, New %d\n", Tile_Id,
					SettingsPtr->DTC_Code[Tile_Id], Max_Gap-Min_Gap, Min_Range);
		}
	} else {
		/* Reload the results of an initial scan to seed a new scan */
		if (Tile_Id == SettingsPtr->RefTile) {
			/* RefTile: Get code closest to the target */
			Target = SettingsPtr->Target[Tile_Id];
		} else {
			Target = SettingsPtr->DTC_Code[SettingsPtr->RefTile] +
				SettingsPtr->Target[Tile_Id] - SettingsPtr->Target[SettingsPtr->RefTile];
		}
		Min_Diff = XRFDC_MTS_NUM_DTC;
		/* scan all codes to find the closest */
		for (Index = 0U; Index < Num_Found; Index++) {
			Diff = abs(Target - Codes[Index]);
			if (Diff < Min_Diff) {
				Min_Diff = Diff;
				SettingsPtr->DTC_Code[Tile_Id] = Codes[Index];
			}
			metal_log(METAL_LOG_DEBUG,
				"Reload Target %d, DTC Code %d, Diff %d, Min %d\n", Target,
				Codes[Index], Diff, Min_Diff);
		}
	}

	/* Print some debug info */
	XRFdc_MTS_Dtc_Flag_Debug(FlagsPtr, Type, Tile_Id, SettingsPtr->Target[Tile_Id],
						SettingsPtr->DTC_Code[Tile_Id]);

	return Status;
}

/*****************************************************************************/
/**
*
* This API Set a DTC code and wait for it to be updated. Return early/late
* flags, if set
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	BaseAddr is for internal usage.
* @param	SRCtrlAddr is for internal usage.
* @param	DTCAddr is for internal usage.
* @param	SRctl is for internal usage.
* @param	SRclr_m is for internal usage.
* @param	Code is for internal usage.
*
* @return
* 		- XRFDC_MTS_OK if successful.
*		- XRFDC_MTS_TIMEOUT if timeout occurs.
*
* @note		None
*
******************************************************************************/
static u32 XRFdc_MTS_Dtc_Code(XRFdc *InstancePtr, u32 Type, u32 BaseAddr,
		u32 SRCtrlAddr, u32 DTCAddr, u16 SRctl, u16 SRclr_m, u32 Code)
{
	u32 Status;

	/* set the DTC code */
	XRFdc_WriteReg16(InstancePtr, BaseAddr, DTCAddr, Code);

	/* set sysref cap clear */
	XRFdc_WriteReg16(InstancePtr, BaseAddr, SRCtrlAddr, SRctl | SRclr_m);

	/* unset sysref cap clear */
	XRFdc_WriteReg16(InstancePtr, BaseAddr, SRCtrlAddr, SRctl);

	Status = XRFdc_MTS_Sysref_Count(InstancePtr, Type, XRFDC_MTS_DTC_COUNT);

	return Status;
}

/*****************************************************************************/
/**
*
* This API Scan the DTC codes and determine the optimal capture code for
* both PLL and T1 cases
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	SettingsPtr dtc settings structure.
*
* @return
* 		- XRFDC_MTS_OK if successful.
*		- XRFDC_MTS_TIMEOUT if timeout occurs.
*
* @note		None
*
******************************************************************************/
static u32 XRFdc_MTS_Dtc_Scan (XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
					XRFdc_MTS_DTC_Settings *SettingsPtr)
{
	u32 Status;
	u32 BaseAddr;
	u32 SRCtrlAddr;
	u32 DTCAddr;
	u8 Flags[XRFDC_MTS_NUM_DTC+1];
	u16 SRctl;
	u16 SRclr_m;
	u16 Flag_s;
	u32 Index;

	BaseAddr = XRFDC_DRP_BASE(Type, Tile_Id) + XRFDC_HSCOM_ADDR;
	Status = XRFDC_MTS_OK;

	/*  Enable SysRef Capture and Disable Divide Reset */
	XRFdc_MTS_Sysref_Ctrl(InstancePtr, Type, Tile_Id, SettingsPtr->IsPLL, 1, 0);
	SRCtrlAddr = (SettingsPtr->IsPLL != 0U) ? XRFDC_MTS_SRCAP_PLL : XRFDC_MTS_SRCAP_T1;
	DTCAddr = (SettingsPtr->IsPLL != 0U) ? XRFDC_MTS_SRDTC_PLL : XRFDC_MTS_SRDTC_T1;
	SRclr_m = (SettingsPtr->IsPLL != 0U) ? XRFDC_MTS_SRCLR_PLL_M : XRFDC_MTS_SRCLR_T1_M;
	Flag_s = (SettingsPtr->IsPLL != 0U) ? XRFDC_MTS_SRFLAG_PLL : XRFDC_MTS_SRFLAG_T1;

	SRctl = XRFdc_ReadReg16(InstancePtr, BaseAddr, SRCtrlAddr) & ~SRclr_m;

	for (Index = 0U; Index < XRFDC_MTS_NUM_DTC; Index++) {
		Flags[Index] = 0U;
	}
	for (Index = 0U; (Index < XRFDC_MTS_NUM_DTC) && (Status == XRFDC_MTS_OK); Index++) {
		Status  |= XRFdc_MTS_Dtc_Code(InstancePtr, Type, BaseAddr,
					SRCtrlAddr, DTCAddr, SRctl, SRclr_m, Index);
		Flags[Index] = (XRFdc_ReadReg16(InstancePtr, BaseAddr, XRFDC_MTS_SRFLAG) >>
					Flag_s) & 0x3U;
	}

	/* Calculate the best DTC code */
	(void)XRFdc_MTS_Dtc_Calc(Type, Tile_Id, SettingsPtr, Flags);

	/* Program the calculated code */
	if (SettingsPtr->DTC_Code[Tile_Id] == -1) {
		metal_log(METAL_LOG_ERROR,
		"Unable to capture analog SysRef safely on %s tile %d\n"
			, (Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", Tile_Id);
		Status |= XRFDC_MTS_DTC_INVALID;
	} else {
		(void)XRFdc_MTS_Dtc_Code(InstancePtr, Type, BaseAddr, SRCtrlAddr, DTCAddr,
				SRctl, SRclr_m, SettingsPtr->DTC_Code[Tile_Id]);
	}

	if (SettingsPtr->IsPLL != 0U) {
		/* PLL - Disable SysRef Capture */
		XRFdc_MTS_Sysref_Ctrl(InstancePtr, Type, Tile_Id, 1, 0, 0);
	} else {
		/* T1 - Reset Dividers */
		XRFdc_MTS_Sysref_Ctrl(InstancePtr, Type, Tile_Id, 0, 1, 1);
		Status |= XRFdc_MTS_Sysref_Count(InstancePtr, Type,
						XRFDC_MTS_DTC_COUNT);
		XRFdc_MTS_Sysref_Ctrl(InstancePtr, Type, Tile_Id, 0, 1, 0);
	}

	return Status;
}

/*****************************************************************************/
/**
*
* This API Control the FIFO enable for the group. If Tiles_to_clear has bits
* set, the FIFOs of those tiles will have their FIFO flags cleared.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	FIFO_Mode is fifo mode.
* @param	Tiles_To_Clear bits set, FIFO flags will be cleared for those tiles.
*
* @return	None
*
* @note		None
*
******************************************************************************/
static void XRFdc_MTS_FIFOCtrl (XRFdc *InstancePtr, u32 Type, u32 FIFO_Mode,
							u32 Tiles_To_Clear)
{
	u32 RegAddr;
	u32 BaseAddr;
	u32 Tile_Id;
	u32 Block_Id;

	/* Clear the FIFO Flags */
	RegAddr = (Type == XRFDC_ADC_TILE) ? XRFDC_ADC_FABRIC_ISR_OFFSET :
						XRFDC_DAC_FABRIC_ISR_OFFSET;
	for (Tile_Id = XRFDC_TILE_ID0; Tile_Id < XRFDC_TILE_ID4; Tile_Id++) {
		if (((1U << Tile_Id) & Tiles_To_Clear) != 0U) {
			for (Block_Id = XRFDC_BLK_ID0; Block_Id < XRFDC_BLK_ID4; Block_Id++) {
				BaseAddr = XRFDC_DRP_BASE(Type, Tile_Id) +
						XRFDC_BLOCK_ADDR_OFFSET(Block_Id);
				XRFdc_WriteReg16(InstancePtr, BaseAddr,	RegAddr,
						XRFDC_IXR_FIFOUSRDAT_MASK);
			}
		}
	}

	/* Enable the FIFOs */
	RegAddr = (Type == XRFDC_ADC_TILE) ? XRFDC_MTS_FIFO_CTRL_ADC :
						XRFDC_MTS_FIFO_CTRL_DAC;
	XRFdc_WriteReg(InstancePtr, 0, RegAddr, FIFO_Mode);
}

/*****************************************************************************/
/**
*
* This API Read-back the marker data for an ADC or DAC
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param	FIFO_Id is FIFO number.
* @param	Count is for internal usage.
* @param	Loc is for internal usage.
* @param	Done is for internal usage.
*
* @return
* 		- None.
*
* @note		None
*
******************************************************************************/
static void XRFdc_MTS_Marker_Read(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
				u32 FIFO_Id, u32 *CountPtr, u32 *LocPtr, u32 *DonePtr)
{
	u32 BaseAddr;
	u32 RegData = 0x0;

	if (Type == XRFDC_ADC_TILE) {
		BaseAddr = XRFDC_DRP_BASE(Type, Tile_Id) - 0x2000;
		RegData  = XRFdc_ReadReg(InstancePtr, BaseAddr,
				XRFDC_MTS_ADC_MARKER_CNT+(FIFO_Id << 2));
		*CountPtr = XRFDC_MTS_FIELD(RegData, XRFDC_MTS_AMARK_CNT_M, 0);
		*LocPtr = XRFDC_MTS_FIELD(RegData, XRFDC_MTS_AMARK_LOC_M,
						XRFDC_MTS_AMARK_LOC_S);
		*DonePtr = XRFDC_MTS_FIELD(RegData, XRFDC_MTS_AMARK_DONE_M,
						XRFDC_MTS_AMARK_DONE_S);
	} else {
		BaseAddr = XRFDC_DRP_BASE(Type, Tile_Id) +
					XRFDC_BLOCK_ADDR_OFFSET(FIFO_Id);
		*CountPtr = XRFdc_ReadReg(InstancePtr, BaseAddr,
						XRFDC_MTS_DAC_MARKER_CNT);
		*LocPtr = XRFdc_ReadReg(InstancePtr, BaseAddr,
						XRFDC_MTS_DAC_MARKER_LOC);
		*DonePtr = 1;
	}
	metal_log(METAL_LOG_DEBUG,
		"Marker Read Tile %d, FIFO %d - %08X = %04X: count=%d, loc=%d,"
		"done=%d\n", Tile_Id, FIFO_Id, BaseAddr, RegData, *CountPtr,
		*LocPtr, *DonePtr);
}

/*****************************************************************************/
/**
*
* This API Run the marker counter and read the results
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tiles is tiles to get marker
* @param	MarkersPtr mts marker structure.
* @param	Marker_Delay is marker delay.
*
* @return
* 		- XRFDC_MTS_OK if successful.
* 		- XRFDC_MTS_TIMEOUT if timeout occurs.
* 		- XRFDC_MTS_MARKER_RUN
* 		- XRFDC_MTS_MARKER_MISM
* 		-
*
* @note		None
*
******************************************************************************/
static u32 XRFdc_MTS_GetMarker(XRFdc *InstancePtr, u32 Type, u32 Tiles,
				XRFdc_MTS_Marker *MarkersPtr, int Marker_Delay)
{
	u32 Done;
	u32 Count;
	u32 Loc;
	u32 Tile_Id;
	u32 Block_Id;
	u32 Status;

	Status = XRFDC_MTS_OK;
	if (Type == XRFDC_ADC_TILE) {
		/* Reset marker counter */
		XRFdc_WriteReg(InstancePtr, 0, XRFDC_MTS_ADC_MARKER, 1);
		XRFdc_WriteReg(InstancePtr, 0, XRFDC_MTS_ADC_MARKER, 0);
	} else {
		/*
		 * SysRef Capture should be still active from the DTC Scan
		 * but set it anyway to be sure
		 */
		for (Tile_Id = XRFDC_TILE_ID0; Tile_Id < XRFDC_TILE_ID4; Tile_Id++) {
			if (((1U << Tile_Id) & Tiles) != 0U) {
				XRFdc_MTS_Sysref_Ctrl(InstancePtr, XRFDC_DAC_TILE,
						Tile_Id, 0, 1, 0);
			}
		}

		/* Set marker delay */
		XRFdc_WriteReg(InstancePtr, 0, XRFDC_MTS_DAC_MARKER_CTRL,
							Marker_Delay);
	}

	/* Allow the marker counter to run */
	Status |= XRFdc_MTS_Sysref_Count(InstancePtr, Type,
							XRFDC_MTS_MARKER_COUNT);

	/* Read master FIFO (FIFO0 in each Tile) */
	for (Tile_Id = XRFDC_TILE_ID0; Tile_Id < XRFDC_TILE_ID4; Tile_Id++) {
		if (((1U << Tile_Id) & Tiles) != 0U) {
			if (Type == XRFDC_DAC_TILE) {
				/* Disable SysRef Capture before reading it */
				XRFdc_MTS_Sysref_Ctrl(InstancePtr, XRFDC_DAC_TILE,
									Tile_Id, 0, 0, 0);
				Status |= XRFdc_MTS_Sysref_Count(InstancePtr, Type,
								XRFDC_MTS_MARKER_COUNT);
			}

			XRFdc_MTS_Marker_Read(InstancePtr, Type, Tile_Id, 0, &Count,
								&Loc, &Done);
			MarkersPtr->Count[Tile_Id] = Count;
			MarkersPtr->Loc[Tile_Id]   = Loc;
			metal_log(METAL_LOG_INFO,
				"%s%d: Marker: - %d, %d\n", (Type == XRFDC_DAC_TILE) ?
				"DAC":"ADC", Tile_Id, MarkersPtr->Count[Tile_Id], MarkersPtr->Loc[Tile_Id]);

			if ((!Done) != 0U) {
				metal_log(METAL_LOG_ERROR, "Analog SysRef timeout,"
						"SysRef not detected on %s tile %d\n",
						(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", Tile_Id);
				Status |= XRFDC_MTS_MARKER_RUN;
			}

			/*
			 * Check all enabled FIFOs agree with the master FIFO.
			 * This is optional.
			 */
			for (Block_Id = XRFDC_BLK_ID0; Block_Id < XRFDC_BLK_ID4; Block_Id++) {
				if (XRFdc_IsFifoEnabled(InstancePtr, Type, Tile_Id, Block_Id) != 0U) {
					XRFdc_MTS_Marker_Read(InstancePtr, Type, Tile_Id, Block_Id,
								&Count, &Loc, &Done);
					if ((MarkersPtr->Count[Tile_Id] != Count) ||
								(MarkersPtr->Loc[Tile_Id] != Loc)) {
						metal_log(METAL_LOG_DEBUG,
							"Tile %d, FIFO %d Marker != Expected: %d, %d  vs"
							"%d, %d\n", Tile_Id, Block_Id, MarkersPtr->Count[Tile_Id],
							MarkersPtr->Loc[Tile_Id], Count, Loc);
						metal_log(METAL_LOG_ERROR,
							"SysRef capture mismatch on %s tile %d,"
							" PL SysRef may not have been"
							" captured synchronously\n",
							(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", Block_Id);
						Status |= XRFDC_MTS_MARKER_MISM;

					}
				}
			}
		}
	}

	return Status;
}

/*****************************************************************************/
/**
*
* This API Calculate the absoulte/relative latencies
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	ConfigPtr is mts config structure.
* @param	MarkersPtr is mts marker structure.
*
* @return
* 		- XRFDC_MTS_OK if successful.
* 		- XRFDC_MTS_DELAY_OVER
* 		- XRFDC_MTS_TARGET_LOW
* 		-
*
* @note     Latency calculation will use Sysref frequency counters
*           logic which will work with IP version 2.0.1 and above.
*
******************************************************************************/
static u32 XRFdc_MTS_Latency(XRFdc *InstancePtr, u32 Type,
	XRFdc_MultiConverter_Sync_Config *ConfigPtr, XRFdc_MTS_Marker *MarkersPtr)
{
	u32 Status, Fifo, Index, BaseAddr, RegAddr;
	int Count_W, Loc_W, Latency, Offset, Max_Latency, Target, Delta;
	int I_Part, F_Part, SysRefT1Period, LatencyDiff, LatencyOffset;
	u32 RegData, SysRefFreqCntrDone;
	int Target_Latency = -1;
	int LatencyOffsetDiff;
	u32 Factor = 1U;
	u32 Write_Words = 0U;
	u32 Read_Words = 1U;

	Status = XRFDC_MTS_OK;
	if (Type == XRFDC_ADC_TILE) {
		(void)XRFdc_GetDecimationFactor(InstancePtr, ConfigPtr->RefTile, 0, &Factor);
	} else {
		(void)XRFdc_GetInterpolationFactor(InstancePtr, ConfigPtr->RefTile, 0, &Factor);
		(void)XRFdc_GetFabWrVldWords(InstancePtr, Type, ConfigPtr->RefTile, 0, &Write_Words);
	}
	(void)XRFdc_GetFabRdVldWords(InstancePtr, Type, ConfigPtr->RefTile, 0, &Read_Words);
	Count_W = Read_Words * Factor;
	Loc_W = Factor;

	metal_log(METAL_LOG_DEBUG,
			"Count_W %d, loc_W %d\n", Count_W, Loc_W);

	/* Find the individual latencies */
	Max_Latency = 0;

	/* Determine relative SysRef frequency */
	RegData = XRFdc_ReadReg(InstancePtr, 0, XRFDC_MTS_SRFREQ_VAL);
	if (Type == XRFDC_ADC_TILE) {
		/* ADC SysRef frequency information contained in lower 16 bits */
		RegData = RegData & 0XFFFFU;
	} else {
		/* DAC SysRef frequency information contained in upper 16 bits */
		RegData = (RegData >> 16U) & 0XFFFFU;
	}

	/*
	 * Ensure SysRef frequency counter has completed.
	 * Sysref frequency counters logic will work with IP version
	 * 2.0.1 and above.
	 */
	SysRefFreqCntrDone = RegData & 0x1U;
	if (SysRefFreqCntrDone == 0U) {
		metal_log(METAL_LOG_ERROR, "Error : %s SysRef frequency counter not yet done\n",
			(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC");
		Status |= XRFDC_MTS_SYSREF_FREQ_NDONE;
		/* Set SysRef period in terms of T1's will not be used */
		SysRefT1Period = 0;
	} else {
		SysRefT1Period = (RegData >> 1) * Count_W;
		if (Type == XRFDC_DAC_TILE) {
			/*
			 * DAC marker counter is on the tile clock domain so need
			 * to update SysRef period accordingly
			 */
			SysRefT1Period = (SysRefT1Period * Write_Words) / Read_Words;
		}
		metal_log(METAL_LOG_INFO, "SysRef period in terms of %s T1s = %d\n",
			(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", SysRefT1Period);
	}

	/* Work out the latencies */
	for (Index = XRFDC_TILE_ID0; Index < XRFDC_TILE_ID4; Index++) {
		if (((1U << Index) & ConfigPtr->Tiles) != 0U) {
			Latency = (MarkersPtr->Count[Index] * Count_W) + (MarkersPtr->Loc[Index] * Loc_W);
			/* Set marker counter target on first tile */
			if (Target_Latency < 0) {
				Target_Latency = ConfigPtr->Target_Latency;
				if (Target_Latency < 0) {
					Target_Latency = Latency;
				}
				metal_log(METAL_LOG_INFO, "%s target latency = %d\n",
					(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", Target_Latency);
			}

			/*
			 * Adjust reported counter values if offsetting by a SysRef
			 * period reduces distance between current and target latencies
			 */
			LatencyDiff = Target_Latency - Latency;
			LatencyOffset = (LatencyDiff > 0) ? (Latency + SysRefT1Period) :
					(Latency - SysRefT1Period);
			LatencyOffsetDiff = Target_Latency - LatencyOffset;
			if (abs(LatencyDiff) > abs(LatencyOffsetDiff)) {
				Latency = LatencyOffset;
				metal_log(METAL_LOG_INFO, "%s%d latency offset by a SysRef period to %d\n",
					(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", Index, Latency);
			}
			ConfigPtr->Latency[Index] = Latency;
			if (Latency > Max_Latency) {
				Max_Latency = Latency;
			}
			metal_log(METAL_LOG_DEBUG, "Tile %d, latency %d, max %d\n",
					Index, Latency, Max_Latency);
		}
	}

	/*
	 * Adjust the latencies to meet the target. Choose max, if it
	 * is not supplied by the user.
	 */
	Target = (ConfigPtr->Target_Latency < 0) ? Max_Latency :
							ConfigPtr->Target_Latency;

	if (Target < Max_Latency) {
		/* Cannot correct for -ve latencies, so default to aligning */
		Target = Max_Latency;
		metal_log(METAL_LOG_ERROR, "Error : %s alignment target latency of %d < minimum possible %d\n",
				(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", Target, Max_Latency);
		Status |= XRFDC_MTS_TARGET_LOW;
	}

	for (Index = XRFDC_TILE_ID0; Index < XRFDC_TILE_ID4; Index++) {
		if (((1U << Index) & ConfigPtr->Tiles) != 0U) {
			Delta = Target - ConfigPtr->Latency[Index];
			if (Delta < 0) {
				Delta = 0;
			}
			I_Part = Delta / Factor;
			F_Part = Delta % Factor;
			Offset = I_Part;
			if (F_Part > (int)(Factor / 2U)) {
				Offset++;
			}
			metal_log(METAL_LOG_DEBUG,
				"Target %d, Tile %d, delta %d, i/f_part %d/%d, offset %d\n",
				Target, Index, Delta, I_Part, F_Part, Offset * Factor);

			/* check for excessive delay correction values */
			if (Offset > (int)XRFDC_MTS_DELAY_MAX) {
				Offset  = (int)XRFDC_MTS_DELAY_MAX;
				metal_log(METAL_LOG_ERROR,
						"Alignment correction delay %d"
						" required exceeds maximum for %s Tile %d\n",
						Offset, (Type == XRFDC_ADC_TILE) ? "ADC" : "DAC",
								XRFDC_MTS_DELAY_MAX, Index);
				Status |= XRFDC_MTS_DELAY_OVER;
			}

			/* Adjust the latency, write the same value to each FIFO */
			BaseAddr = XRFDC_DRP_BASE(Type, Index) - 0x2000;
			for (Fifo = XRFDC_BLK_ID0; Fifo < XRFDC_BLK_ID4; Fifo++) {
				RegAddr  = XRFDC_MTS_DELAY_CTRL + (Fifo << 2);
				RegData  = XRFdc_ReadReg(InstancePtr, BaseAddr, RegAddr);
				RegData  = XRFDC_MTS_RMW(RegData, XRFDC_MTS_DELAY_VAL_M,
										Offset);
				XRFdc_WriteReg(InstancePtr, BaseAddr, RegAddr, RegData);
			}

			/* Report the total latency for this tile */
			ConfigPtr->Latency[Index] = ConfigPtr->Latency[Index] + (Offset * Factor);
			ConfigPtr->Offset[Index]  = Offset;

			/* Set the Final SysRef Capture Enable state */
			XRFdc_MTS_Sysref_Ctrl(InstancePtr, Type, Index, 0, ConfigPtr->SysRef_Enable, 0);
		}
	}

	return Status;
}

/*****************************************************************************/
/**
*
* This API is used to enable/disable the sysref.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	DACSyncConfigPtr is pointer to DAC Multi-Tile Sync config structure.
* @param	ADCSyncConfigPtr is pointer to ADC Multi-Tile Sync config structure.
* @param	SysRefEnable valid values are 0(disable) and 1(enable).
*
* @return
* 		- XRFDC_MTS_OK if successful.
*
* @note		None
*
******************************************************************************/
u32 XRFdc_MTS_Sysref_Config(XRFdc *InstancePtr,
			XRFdc_MultiConverter_Sync_Config *DACSyncConfigPtr,
	XRFdc_MultiConverter_Sync_Config *ADCSyncConfigPtr, u32 SysRefEnable)
{
	u32 Tile;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(DACSyncConfigPtr != NULL);
	Xil_AssertNonvoid(ADCSyncConfigPtr != NULL);

	/* Enable/disable SysRef Capture on all DACs participating in MTS */
	for (Tile = XRFDC_TILE_ID0; Tile < XRFDC_TILE_ID4; Tile++) {
		if (((1U << Tile) & DACSyncConfigPtr->Tiles) != 0U) {
			XRFdc_MTS_Sysref_Ctrl(InstancePtr,
				XRFDC_DAC_TILE, Tile, 0, SysRefEnable, 0);
		}
	}

	/* Enable/Disable SysRef Capture on all ADCs participating in MTS */
	for (Tile = XRFDC_TILE_ID0; Tile < XRFDC_TILE_ID4; Tile++) {
		if (((1U << Tile) & ADCSyncConfigPtr->Tiles) != 0U) {
			XRFdc_MTS_Sysref_Ctrl(InstancePtr,
				XRFDC_ADC_TILE, Tile, 0, SysRefEnable, 0);
		}
	}

	/* Enable/Disable SysRef TRX */
	XRFdc_MTS_Sysref_TRx(InstancePtr, SysRefEnable);

	return XRFDC_MTS_OK;
}

/*****************************************************************************/
/**
*
* This API Initializes the multi-tile sync config structures.
* Optionally allows target codes to be provided for the Pll/T1
* analog sysref capture
*
* @param	ConfigPtr pointer to Multi-tile sync config structure.
* @param	PLL_CodesPtr pointer to PLL analog sysref capture.
* @param	T1_CodesPtr pointer to T1 analog sysref capture.
*
* @return	None
*
* @note		None
*
******************************************************************************/
void XRFdc_MultiConverter_Init(XRFdc_MultiConverter_Sync_Config *ConfigPtr,
					int *PLL_CodesPtr, int *T1_CodesPtr)
{
	u32 Index;

	Xil_AssertVoid(ConfigPtr != NULL);

	ConfigPtr->RefTile = 0U;
	ConfigPtr->DTC_Set_PLL.Scan_Mode = (PLL_CodesPtr == NULL) ?
			XRFDC_MTS_SCAN_INIT : XRFDC_MTS_SCAN_RELOAD;
	ConfigPtr->DTC_Set_T1.Scan_Mode = (T1_CodesPtr == NULL) ?
			XRFDC_MTS_SCAN_INIT : XRFDC_MTS_SCAN_RELOAD;
	ConfigPtr->DTC_Set_PLL.IsPLL = 1U;
	ConfigPtr->DTC_Set_T1.IsPLL = 0U;
	ConfigPtr->Target_Latency = -1;
	ConfigPtr->Marker_Delay = 15;
	ConfigPtr->SysRef_Enable = 1; /* By default enable Sysref capture after MTS */

	/* Initialize variables per tile */
	for (Index = XRFDC_TILE_ID0; Index < XRFDC_TILE_ID4; Index++) {
		if (PLL_CodesPtr != NULL) {
			ConfigPtr->DTC_Set_PLL.Target[Index] = PLL_CodesPtr[Index];
		} else {
			ConfigPtr->DTC_Set_PLL.Target[Index] = 0;
		}
		if (T1_CodesPtr  != NULL) {
			ConfigPtr->DTC_Set_T1.Target[Index] = T1_CodesPtr[Index];
		} else {
			ConfigPtr->DTC_Set_T1.Target[Index] = 0;
		}

		ConfigPtr->DTC_Set_PLL.DTC_Code[Index] = -1;
		ConfigPtr->DTC_Set_T1.DTC_Code[Index] = -1;
	}

}

/*****************************************************************************/
/**
*
* This is the top level API which will be used for Multi-tile
* Synchronization.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	ConfigPtr Multi-tile sync config structure.
*
* @return
* 		- XRFDC_MTS_OK if successful.
*		- XRFDC_MTS_TIMEOUT if timeout occurs.
* 		- XRFDC_MTS_MARKER_RUN
* 		- XRFDC_MTS_MARKER_MISM
* 		- XRFDC_MTS_NOT_SUPPORTED if MTS is not supported.
*
* @note		None
*
******************************************************************************/
u32 XRFdc_MultiConverter_Sync(XRFdc *InstancePtr, u32 Type,
				XRFdc_MultiConverter_Sync_Config *ConfigPtr)
{
	u32 Status;
	u32 Index;
	u32 RegData;
	XRFdc_IPStatus IPStatus = {0};
	XRFdc_MTS_Marker Markers = {0U};
	u32 BaseAddr;
	u32 TileState;
	u32 BlockStatus;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(ConfigPtr != NULL);

	Status = XRFDC_MTS_OK;

	(void)XRFdc_GetIPStatus(InstancePtr, &IPStatus);
	for (Index = XRFDC_TILE_ID0; Index < XRFDC_TILE_ID4; Index++) {
		if ((ConfigPtr->Tiles & (1U << Index)) != 0U) {
			TileState = (Type == XRFDC_DAC_TILE) ?
				IPStatus.DACTileStatus[Index].TileState :
				IPStatus.ADCTileStatus[Index].TileState ;
			if (TileState != 0xFU) {
				metal_log(METAL_LOG_ERROR,
				"%s tile %d in Multi-Tile group not started\n",
				(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", Index);

				Status |= XRFDC_MTS_IP_NOT_READY;
			}
			BaseAddr = XRFDC_DRP_BASE(Type, Index) - XRFDC_TILE_DRP_OFFSET;
			RegData  = XRFdc_ReadReg(InstancePtr, BaseAddr, XRFDC_MTS_DLY_ALIGNER);
			if (RegData == 0U) {
				metal_log(METAL_LOG_ERROR, "%s tile %d is not enabled for MTS, check IP configuration\n",
					(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", Index);
				Status |= XRFDC_MTS_NOT_ENABLED;
			}

			BlockStatus = XRFdc_CheckBlockEnabled(InstancePtr, Type, Index, 0x0U);
			if (BlockStatus != 0U) {
				metal_log(METAL_LOG_ERROR, "%s%d block0 is not enabled, check IP configuration\n",
					(Type == XRFDC_ADC_TILE) ? "ADC" : "DAC", Index);
				Status |= XRFDC_MTS_NOT_SUPPORTED;
			}
		}
	}

	if (Status != XRFDC_MTS_OK) {
		return Status;
	}

	/* Disable the FIFOs */
	XRFdc_MTS_FIFOCtrl(InstancePtr, Type, XRFDC_MTS_FIFO_DISABLE, 0);

	/* Enable SysRef Rx */
	XRFdc_MTS_Sysref_TRx(InstancePtr, 1);

	/* Update distribution */
	Status |= XRFdc_MTS_Sysref_Dist(InstancePtr, -1);

	/* Scan DTCs for each tile */
	for (Index = XRFDC_TILE_ID0; Index < XRFDC_TILE_ID4; Index++) {
		if ((ConfigPtr->Tiles & (1U << Index)) != 0U) {
			/* Run DTC Scan for T1/PLL */
			BaseAddr = XRFDC_DRP_BASE(Type, Index) + XRFDC_HSCOM_ADDR;
			RegData  = XRFdc_ReadReg16(InstancePtr, BaseAddr,
								XRFDC_MTS_CLKSTAT);
			if ((RegData & XRFDC_MTS_PLLEN_M) != 0U) {
				/* DTC Scan PLL */
				if (Index == 0U) {
					metal_log(METAL_LOG_INFO, "\nDTC Scan PLL\n", 0);
				}
				ConfigPtr->DTC_Set_PLL.RefTile = ConfigPtr->RefTile;
				Status |= XRFdc_MTS_Dtc_Scan(InstancePtr, Type, Index,
							&ConfigPtr->DTC_Set_PLL);
			}
		}
	}

	/* Scan DTCs for each tile T1 */
	metal_log(METAL_LOG_INFO, "\nDTC Scan T1\n", 0);
	for (Index = XRFDC_TILE_ID0; Index < XRFDC_TILE_ID4; Index++) {
		if ((ConfigPtr->Tiles & (1U << Index)) != 0U) {
			ConfigPtr->DTC_Set_T1 .RefTile = ConfigPtr->RefTile;
			Status |= XRFdc_MTS_Dtc_Scan(InstancePtr, Type, Index,
						&ConfigPtr->DTC_Set_T1);
		}
	}

	/* Enable FIFOs */
	XRFdc_MTS_FIFOCtrl(InstancePtr, Type, XRFDC_MTS_FIFO_ENABLE,
							ConfigPtr->Tiles);

	/* Measure latency */
	Status |= XRFdc_MTS_GetMarker(InstancePtr, Type, ConfigPtr->Tiles,
				&Markers, ConfigPtr->Marker_Delay);

	/* Calculate latency difference and adjust for it */
	Status |= XRFdc_MTS_Latency(InstancePtr, Type, ConfigPtr, &Markers);

	return Status;
}
/*****************************************************************************/
/**
*
* This is the top level API which will be used to check if Multi-tile
* is enabled.
*
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC.
* @param	Tile_Id indicates Tile number (0-3).
* @param	EnablePtr to be filled with the enable state.
*
* @return
* 		- XRFDC_SUCCESS if successful.
*		- XRFDC_SUCCESS if error occurs.
*
* @note		None
*
******************************************************************************/
u32 XRFdc_GetMTSEnable(XRFdc *InstancePtr, u32 Type,u32 Tile_Id, u32 *EnablePtr)
{
	u32 RegData;
	u32 BaseAddr;
	u32 Status;
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(EnablePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	Status = XRFdc_CheckTileEnabled(InstancePtr, Type, Tile_Id);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR,
			  "\n Requested Tile not "
			  "available in %s\r\n",
			  __func__);
		goto RETURN_PATH;
	}

	BaseAddr = XRFDC_DRP_BASE(Type, Tile_Id) - XRFDC_TILE_DRP_OFFSET;
	RegData  = XRFdc_ReadReg(InstancePtr, BaseAddr, XRFDC_MTS_DLY_ALIGNER);
	if (RegData == 0) {
		*EnablePtr = 0;
	} else {
		*EnablePtr = 1;
	}
	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/** @} */
