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
* @file xrfdc_mb.c
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
* 6.0   cog    02/17/18 Initial release/handle alternate bound out.
*
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "mpm/rfdc/xrfdc.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/
static void XRFdc_SetSignalFlow(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u32 Mode, u32 DigitalDataPathId, u32 MixerInOutDataType,
		int ConnectIData, int ConnectQData);
static void XRFdc_MB_R2C_C2R(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u8 NoOfDataPaths, u32 MixerInOutDataType, u32 Mode,
				u32 DataPathIndex[], u32 BlockIndex[]);
static void XRFdc_MB_C2C(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u8 NoOfDataPaths, u32 MixerInOutDataType, u32 Mode,
			u32 DataPathIndex[], u32 BlockIndex[]);
static void XRFdc_SB_R2C_C2R(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u32 MixerInOutDataType, u32 Mode, u32 DataPathIndex[], u32 BlockIndex[]);
static void XRFdc_SB_C2C(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u32 MixerInOutDataType, u32 Mode, u32 DataPathIndex[], u32 BlockIndex[]);
/************************** Function Prototypes ******************************/

/*****************************************************************************/
/**
*
* Static API to setup Singleband configuration for C2C MixerInOutDataType
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param    MixerInOutDataType is mixer data type, valid values are XRFDC_MB_DATATYPE_*
* @param    Mode is connection mode SB/MB_2X/MB_4X.
* @param    DataPathIndex is the array that represents the blocks enabled in
*           DigitalData Path.
* @param    BlockIndex is the array that represents the blocks enabled in
*           Analog Path(Data Converters).
*
* @return
*		- None
*
* @note		Static API for ADC/DAC blocks
*
******************************************************************************/
static void XRFdc_SB_C2C(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u32 MixerInOutDataType, u32 Mode, u32 DataPathIndex[], u32 BlockIndex[])
{
	u32 Block_Id;

	if ((Type == XRFDC_ADC_TILE) && (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1)) {
		/* Update ConnectedIData and ConnectedQData for ADC 4GSPS */
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedIData = BlockIndex[0U];
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedQData = BlockIndex[1U];
		Block_Id = (DataPathIndex[0] == 0U ? 1U : 0U);
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Block_Id].
						ConnectedIData = -1;
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[Block_Id].
						ConnectedQData = -1;

		if (DataPathIndex[0] == XRFDC_BLK_ID1) {
			DataPathIndex[0] = XRFDC_BLK_ID2;
		}
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
				MixerInOutDataType, BlockIndex[0U], BlockIndex[0U]+1U);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0]+1U,
				MixerInOutDataType, BlockIndex[1U]+1U, BlockIndex[1U]+2U);
		Block_Id = (DataPathIndex[0] == XRFDC_BLK_ID2 ? XRFDC_BLK_ID0 :
				XRFDC_BLK_ID2);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, Block_Id,
				MixerInOutDataType, -1, -1);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, Block_Id+1U,
				MixerInOutDataType, -1, -1);
	} else {
		DataPathIndex[1] = BlockIndex[0] + BlockIndex[1] - DataPathIndex[0];
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
				MixerInOutDataType, BlockIndex[0], BlockIndex[1]);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[1],
				MixerInOutDataType, -1, -1);

		/* Update ConnectedIData and ConnectedQData for DAC and ADC 2GSPS */
		if (Type == XRFDC_ADC_TILE) {
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = BlockIndex[0];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = BlockIndex[1];

			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = -1;
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = -1;
		} else {
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = BlockIndex[0];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = BlockIndex[1];

			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = -1;
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = -1;
		}
	}
}

/*****************************************************************************/
/**
*
* Static API to setup Singleband configuration for C2R and R2C MultiBandDataTypes
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param    MixerInOutDataType is mixer data type, valid values are XRFDC_MB_DATATYPE_*
* @param    Mode is connection mode SB/MB_2X/MB_4X.
* @param    DataPathIndex is the array that represents the blocks enabled in
*           DigitalData Path.
* @param    BlockIndex is the array that represents the blocks enabled in
*           Analog Path(Data Converters).
*
* @return
*		- None
*
* @note		Static API for ADC/DAC blocks
*
******************************************************************************/
static void XRFdc_SB_R2C_C2R(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u32 MixerInOutDataType, u32 Mode, u32 DataPathIndex[], u32 BlockIndex[])
{
	if (Type == XRFDC_ADC_TILE) {
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedIData = BlockIndex[0U];
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedQData = -1;
	} else {
		InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedIData = BlockIndex[0U];
		InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedQData = -1;
	}
	if ((Type == XRFDC_ADC_TILE) && (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1)) {
		if (DataPathIndex[0] == XRFDC_BLK_ID1) {
			DataPathIndex[0] = XRFDC_BLK_ID2;
		}
		if (BlockIndex[0] == XRFDC_BLK_ID1) {
			BlockIndex[0] = XRFDC_BLK_ID2;
		}
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0]+1U,
				MixerInOutDataType, BlockIndex[0U]+1U, -1);
	}
	XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
			MixerInOutDataType, BlockIndex[0U], -1);
}

/*****************************************************************************/
/**
*
* Static API to setup Multiband configuration for C2C MixerInOutDataType
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param    MixerInOutDataType is mixer data type, valid values are XRFDC_MB_DATATYPE_*
* @param    Mode is connection mode SB/MB_2X/MB_4X.
* @param    DataPathIndex is the array that represents the blocks enabled in
*           DigitalData Path.
* @param    BlockIndex is the array that represents the blocks enabled in
*           Analog Path(Data Converters).
*
* @return
*		- None
*
* @note		Static API for ADC/DAC blocks
*
******************************************************************************/
static void XRFdc_MB_C2C(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u8 NoOfDataPaths, u32 MixerInOutDataType, u32 Mode,
			u32 DataPathIndex[], u32 BlockIndex[])
{
	if ((Type == XRFDC_ADC_TILE) && (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1)) {
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
				MixerInOutDataType, BlockIndex[0U], BlockIndex[0U]+1U);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0]+1U,
				MixerInOutDataType, BlockIndex[0U]+2U, BlockIndex[0U]+3U);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0]+2U,
				MixerInOutDataType, BlockIndex[0U], BlockIndex[0U]+1U);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0]+3U,
				MixerInOutDataType, BlockIndex[0U]+2U, BlockIndex[0U]+3U);

		/* Update ConnectedIData and ConnectedQData for ADC 4GSPS */
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedIData = BlockIndex[0U];
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedQData = BlockIndex[1U];
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
						ConnectedIData = BlockIndex[0U];
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
						ConnectedQData = BlockIndex[1U];
	} else if (NoOfDataPaths == 2U) {
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
				MixerInOutDataType, BlockIndex[0U], BlockIndex[1U]);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[1],
				MixerInOutDataType, BlockIndex[0U], BlockIndex[1U]);

		/* Update ConnectedIData and ConnectedQData for DAC and ADC 2GSPS */
		if (Type == XRFDC_ADC_TILE) {
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = BlockIndex[1U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = BlockIndex[1U];
		} else {
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = BlockIndex[1U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = BlockIndex[1U];
		}
	}
	if (NoOfDataPaths == 4U) {
		if (Type == XRFDC_ADC_TILE) {
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
					MixerInOutDataType, BlockIndex[0U], BlockIndex[1U]);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[1],
					MixerInOutDataType,  BlockIndex[0U], BlockIndex[1U]);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[2],
					MixerInOutDataType, BlockIndex[0U], BlockIndex[1U]);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[3],
					MixerInOutDataType, BlockIndex[0U], BlockIndex[1U]);

			/* Update ConnectedIData and ConnectedQData for ADC 4GSPS */
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = BlockIndex[1U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = BlockIndex[1U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[2]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[2]].
							ConnectedQData = BlockIndex[1U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[3]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[3]].
							ConnectedQData = BlockIndex[1U];
		} else {
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
					MixerInOutDataType, DataPathIndex[0], DataPathIndex[1U]);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[1],
					MixerInOutDataType, DataPathIndex[0U], DataPathIndex[1U]);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[2],
					MixerInOutDataType, DataPathIndex[2U], DataPathIndex[3U]);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[3],
					MixerInOutDataType, DataPathIndex[2U], DataPathIndex[3U]);

			/* Update ConnectedIData and ConnectedQData for DAC and ADC 2GSPS */
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = BlockIndex[1U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = BlockIndex[1U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[2]].
							ConnectedIData = DataPathIndex[0U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[2]].
							ConnectedQData = DataPathIndex[1U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[3]].
							ConnectedIData = DataPathIndex[0U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[3]].
							ConnectedQData = DataPathIndex[1U];
		}
	}
}

/*****************************************************************************/
/**
*
* Static API to setup Multiband configuration for C2C MixerInOutDataType
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param    MixerInOutDataType is mixer data type, valid values are XRFDC_MB_DATATYPE_*
* @param    Mode is connection mode SB/MB_2X/MB_4X.
* @param    DataPathIndex is the array that represents the blocks enabled in
*           DigitalData Path.
* @param    BlockIndex is the array that represents the blocks enabled in
*           Analog Path(Data Converters).
*
* @return
*		- None
*
* @note		Static API for ADC/DAC blocks
*
******************************************************************************/
static void XRFdc_MB_R2C_C2R(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u8 NoOfDataPaths, u32 MixerInOutDataType, u32 Mode,
				u32 DataPathIndex[], u32 BlockIndex[])
{
	if ((Type == XRFDC_ADC_TILE) && (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1)) {
		/* Update ConnectedIData and ConnectedQData */
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedIData = BlockIndex[0U];
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
						ConnectedQData = -1;
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
						ConnectedIData = BlockIndex[0U];
		InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
						ConnectedQData = -1;
		if (BlockIndex[0] == XRFDC_BLK_ID1) {
			BlockIndex[0] = XRFDC_BLK_ID2;
		}
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
				MixerInOutDataType, BlockIndex[0U], -1);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[1],
				MixerInOutDataType, BlockIndex[0U]+1U, -1);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0]+2U,
				MixerInOutDataType, BlockIndex[0U], -1);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[1]+2U,
				MixerInOutDataType, BlockIndex[0U]+1U, -1);
	} else if (NoOfDataPaths == 2U) {
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
				MixerInOutDataType, BlockIndex[0], -1);
		XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[1],
				MixerInOutDataType, BlockIndex[0], -1);

		/* Update ConnectedIData and ConnectedQData */
		if (Type == XRFDC_ADC_TILE) {
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = -1;
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = -1;
		} else {
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = -1;
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = -1;
		}

	}
	if (NoOfDataPaths == 4U) {
		if (Type == XRFDC_ADC_TILE) {
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
					MixerInOutDataType, BlockIndex[0], -1);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[1],
					MixerInOutDataType, BlockIndex[0], -1);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[2],
					MixerInOutDataType, BlockIndex[0], -1);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[3],
					MixerInOutDataType, BlockIndex[0], -1);

			/* Update ConnectedIData and ConnectedQData */
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = -1;
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = -1;
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[2]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[2]].
							ConnectedQData = -1;
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[3]].
							ConnectedIData = BlockIndex[0U];
			InstancePtr->ADC_Tile[Tile_Id].ADCBlock_Digital_Datapath[DataPathIndex[3]].
							ConnectedQData = -1;

		} else {
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[0],
					MixerInOutDataType, DataPathIndex[0], -1);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[1],
					MixerInOutDataType, DataPathIndex[0], -1);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[2],
					MixerInOutDataType, DataPathIndex[2], -1);
			XRFdc_SetSignalFlow(InstancePtr, Type, Tile_Id, Mode, DataPathIndex[3],
					MixerInOutDataType, DataPathIndex[2], -1);

			/* Update ConnectedIData and ConnectedQData */
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedIData = DataPathIndex[0];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[0]].
							ConnectedQData = -1;
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedIData = DataPathIndex[0];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[1]].
							ConnectedQData = -1;
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[2]].
							ConnectedIData = DataPathIndex[0];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[2]].
							ConnectedQData = -1;
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[3]].
							ConnectedIData = DataPathIndex[0];
			InstancePtr->DAC_Tile[Tile_Id].DACBlock_Digital_Datapath[DataPathIndex[3]].
							ConnectedQData = -1;
		}
	}
}

/*****************************************************************************/
/**
*
* Static API to update mode and MultibandConfig
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param    NoOfDataPaths is number of DataPaths enabled.
* @param    ModePtr is a pointer to connection mode SB/MB_2X/MB_4X.
* @param    DataPathIndex is the array that represents the blocks enabled in
*           DigitalData Path.
*
* @return
*		- None
*
* @note		Static API for ADC/DAC blocks
*
******************************************************************************/
static u32 XRFdc_UpdateMBConfig(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u8 NoOfDataPaths, u32 *ModePtr, u32 DataPathIndex[])
{
	u8 MultibandConfig;
	u32 Status;

	if (Type == XRFDC_ADC_TILE) {
		MultibandConfig = InstancePtr->ADC_Tile[Tile_Id].MultibandConfig;
	} else {
		MultibandConfig = InstancePtr->DAC_Tile[Tile_Id].MultibandConfig;
	}

	if (NoOfDataPaths == 1U) {
		*ModePtr = XRFDC_SINGLEBAND_MODE;
		if (((DataPathIndex[0] == XRFDC_BLK_ID2) ||
				(DataPathIndex[0] == XRFDC_BLK_ID3)) &&
				((MultibandConfig == XRFDC_MB_MODE_2X_BLK01_BLK23) ||
						(MultibandConfig == XRFDC_MB_MODE_4X))) {
			MultibandConfig = XRFDC_MB_MODE_2X_BLK01;
		} else if (((DataPathIndex[0] == XRFDC_BLK_ID0) ||
				(DataPathIndex[0] == XRFDC_BLK_ID1)) &&
				((MultibandConfig == XRFDC_MB_MODE_2X_BLK01_BLK23) ||
						(MultibandConfig == XRFDC_MB_MODE_4X))) {
			MultibandConfig = XRFDC_MB_MODE_2X_BLK23;
		} else if ((MultibandConfig == XRFDC_MB_MODE_2X_BLK01) &&
				((DataPathIndex[0] == XRFDC_BLK_ID0) ||
						(DataPathIndex[0] == XRFDC_BLK_ID1))) {
			MultibandConfig = XRFDC_MB_MODE_SB;
		} else if ((MultibandConfig == XRFDC_MB_MODE_2X_BLK23) &&
				((DataPathIndex[0] == XRFDC_BLK_ID2) ||
						(DataPathIndex[0] == XRFDC_BLK_ID3))) {
			MultibandConfig = XRFDC_MB_MODE_SB;
		}
	} else if (NoOfDataPaths == 2U) {
		*ModePtr = XRFDC_MULTIBAND_MODE_2X;
		if (((MultibandConfig == XRFDC_MB_MODE_2X_BLK01) &&
				(DataPathIndex[0] == XRFDC_BLK_ID2) && (DataPathIndex[1] == XRFDC_BLK_ID3)) ||
				((MultibandConfig == XRFDC_MB_MODE_2X_BLK23) && (DataPathIndex[0] == XRFDC_BLK_ID0) &&
				(DataPathIndex[1] == XRFDC_BLK_ID1)) || (MultibandConfig == XRFDC_MB_MODE_4X)) {
			MultibandConfig = XRFDC_MB_MODE_2X_BLK01_BLK23;
		} else if (((DataPathIndex[0] == XRFDC_BLK_ID2) && (DataPathIndex[1] == XRFDC_BLK_ID3)) &&
				(MultibandConfig == XRFDC_MB_MODE_SB)) {
			MultibandConfig = XRFDC_MB_MODE_2X_BLK23;
		} else if (((DataPathIndex[0] == XRFDC_BLK_ID0) && (DataPathIndex[1] == XRFDC_BLK_ID1)) &&
				(MultibandConfig == XRFDC_MB_MODE_SB)) {
			MultibandConfig = XRFDC_MB_MODE_2X_BLK01;
		}
	} else if (NoOfDataPaths == 4U) {
		*ModePtr = XRFDC_MULTIBAND_MODE_4X;
		MultibandConfig = XRFDC_MB_MODE_4X;
	} else {
		metal_log(METAL_LOG_ERROR, "\n Invalid DigitalDataPathMask "
				"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	/* Update Multiband Config member */
	if (Type == XRFDC_ADC_TILE) {
		InstancePtr->ADC_Tile[Tile_Id].MultibandConfig = MultibandConfig;
	} else {
		InstancePtr->DAC_Tile[Tile_Id].MultibandConfig = MultibandConfig;
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* User-level API to setup multiband configuration.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param    DigitalDataPathMask is the DataPath mask. First 4 bits represent
*           4 data paths, 1 means enabled and 0 means disabled.
* @param    MixerInOutDataType is mixer data type, valid values are XRFDC_MB_DATATYPE_*
* @param    DataConverterMask is block enabled mask (input/output driving
*           blocks). 1 means enabled and 0 means disabled.
*
* @return
*		- XRFDC_SUCCESS if successful.
*       - XRFDC_FAILURE if Block not enabled.
*
* @note		Common API for ADC/DAC blocks
*
******************************************************************************/
u32 XRFdc_MultiBand(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u8 DigitalDataPathMask, u32 MixerInOutDataType, u32 DataConverterMask)
{
	u32 Status;
	u32 Block_Id;
	u8 NoOfDataPaths = 0U;
	u32 BlockIndex[XRFDC_NUM_OF_BLKS4] = {XRFDC_BLK_ID4};
	u32 DataPathIndex[XRFDC_NUM_OF_BLKS4] = {XRFDC_BLK_ID4};
	u32 NoOfDataConverters = 0U;
	u32 Mode = 0x0;
	u32 NoOfBlocks = XRFDC_BLK_ID4;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	if ((DigitalDataPathMask == 0U) || (DigitalDataPathMask > 0xFU)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid DigitalDataPathMask "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	if ((DataConverterMask == 0U) || (DataConverterMask > 0xFU)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid DataConverterMask "
					"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	if ((MixerInOutDataType != XRFDC_MB_DATATYPE_C2C) &&
			(MixerInOutDataType != XRFDC_MB_DATATYPE_R2C) &&
			(MixerInOutDataType != XRFDC_MB_DATATYPE_C2R)) {
		metal_log(METAL_LOG_ERROR, "\n Invalid MixerInOutDataType "
				"value in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	if ((XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) &&
				(Type == XRFDC_ADC_TILE)) {
		NoOfBlocks = XRFDC_BLK_ID2;
	}
	/* Identify DataPathIndex and BlockIndex */
	for (Block_Id = XRFDC_BLK_ID0; Block_Id < NoOfBlocks; Block_Id++) {
		if ((DataConverterMask & (1U << Block_Id)) != 0U) {
			BlockIndex[NoOfDataConverters] = Block_Id;
			NoOfDataConverters += 1U;
			Status = XRFdc_CheckBlockEnabled(InstancePtr, Type, Tile_Id,
					Block_Id);
			if (Status != XRFDC_SUCCESS) {
				metal_log(METAL_LOG_ERROR, "\n Requested block not "
										"available in %s\r\n", __func__);
				goto RETURN_PATH;
			}
		}
		if ((DigitalDataPathMask & (1U << Block_Id)) != 0U) {
			DataPathIndex[NoOfDataPaths] = Block_Id;
			NoOfDataPaths += 1U;
			Status = XRFdc_CheckDigitalPathEnabled(InstancePtr, Type, Tile_Id,
					Block_Id);
			if (Status != XRFDC_SUCCESS) {
				metal_log(METAL_LOG_ERROR, "\n Requested block digital path "
									"not enabled in %s\r\n", __func__);
				goto RETURN_PATH;
			}
		}
	}

	/* rerouting & configuration for alternative bonding. */
	if ((Type == XRFDC_DAC_TILE) && (DataConverterMask & 0x05) && (MixerInOutDataType == XRFDC_MB_DATATYPE_C2C) &&
	    (InstancePtr->RFdc_Config.DACTile_Config[Tile_Id].NumSlices == 2)) {
		BlockIndex[XRFDC_BLK_ID1] = XRFDC_BLK_ID1;
		XRFdc_ClrSetReg(InstancePtr, XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, XRFDC_BLK_ID1),
				XRFDC_DAC_MB_CFG_OFFSET, XRFDC_ALT_BOND_MASK, XRFDC_ENABLED << XRFDC_ALT_BOND_SHIFT);
		XRFdc_ClrSetReg(InstancePtr, XRFDC_BLOCK_BASE(XRFDC_DAC_TILE, Tile_Id, XRFDC_BLK_ID2),
				XRFDC_DAC_MB_CFG_OFFSET, XRFDC_ALT_BOND_MASK, XRFDC_ENABLED << XRFDC_ALT_BOND_SHIFT);
	}

	if (BlockIndex[0] != DataPathIndex[0]) {
		metal_log(METAL_LOG_ERROR, "\n Not a valid MB/SB "
					"combination in %s\r\n", __func__);
		Status = XRFDC_FAILURE;
		goto RETURN_PATH;
	}

	/* UPdate MultibandConfig in driver instance */
	Status = XRFdc_UpdateMBConfig(InstancePtr, Type, Tile_Id, NoOfDataPaths, &Mode,
					DataPathIndex);
	if (Status != XRFDC_SUCCESS) {
		goto RETURN_PATH;
	}

	if ((MixerInOutDataType == XRFDC_MB_DATATYPE_C2C) && (Mode == XRFDC_SINGLEBAND_MODE)) {
		/* Singleband C2C */
		XRFdc_SB_C2C(InstancePtr, Type, Tile_Id, MixerInOutDataType, Mode,
				DataPathIndex, BlockIndex);
	} else if (((MixerInOutDataType == XRFDC_MB_DATATYPE_R2C) ||
			(MixerInOutDataType == XRFDC_MB_DATATYPE_C2R)) && (Mode == XRFDC_SINGLEBAND_MODE)) {
		/* Singleband R2C and C2R */
		XRFdc_SB_R2C_C2R(InstancePtr, Type, Tile_Id, MixerInOutDataType, Mode,
						DataPathIndex, BlockIndex);
	}
	if ((MixerInOutDataType == XRFDC_MB_DATATYPE_C2C) &&
			((Mode == XRFDC_MULTIBAND_MODE_2X) || (Mode == XRFDC_MULTIBAND_MODE_4X))) {
		/* Multiband C2C */
		XRFdc_MB_C2C(InstancePtr, Type, Tile_Id, NoOfDataPaths, MixerInOutDataType, Mode,
							DataPathIndex, BlockIndex);
	} else if (((MixerInOutDataType == XRFDC_MB_DATATYPE_R2C) || (MixerInOutDataType == XRFDC_MB_DATATYPE_C2R)) &&
				((Mode == XRFDC_MULTIBAND_MODE_2X) || (Mode == XRFDC_MULTIBAND_MODE_4X))) {
		/* Multiband C2R and R2C */
		XRFdc_MB_R2C_C2R(InstancePtr, Type, Tile_Id, NoOfDataPaths, MixerInOutDataType,
					Mode, DataPathIndex, BlockIndex);
	}

	Status = XRFDC_SUCCESS;
RETURN_PATH:
	return Status;
}
/*****************************************************************************/
/**
*
* Sets up signal flow configuration.
*
* @param	InstancePtr is a pointer to the XRfdc instance.
* @param	Type is ADC or DAC. 0 for ADC and 1 for DAC
* @param	Tile_Id Valid values are 0-3.
* @param    Mode is connection mode SB/MB_2X/MB_4X.
* @param    DigitalDataPathId for the requested I or Q data.
* @param    MixerInOutDataType is mixer data type, valid values are XRFDC_MB_DATATYPE_*
* @param    ConnectIData is analog blocks that are connected to
*           DigitalDataPath I.
* @param    ConnectQData is analog blocks that are connected to
*           DigitalDataPath Q.
*
* @return   None
*
* @note		static API used internally.
*
******************************************************************************/
static void XRFdc_SetSignalFlow(XRFdc *InstancePtr, u32 Type, u32 Tile_Id,
		u32 Mode, u32 DigitalDataPathId, u32 MixerInOutDataType,
		int ConnectIData, int ConnectQData)
{
	u16 ReadReg;
	u32 BaseAddr;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XRFDC_COMPONENT_IS_READY);

	BaseAddr = XRFDC_BLOCK_BASE(Type, Tile_Id, DigitalDataPathId);
	if (Type == XRFDC_ADC_TILE) {
		/* ADC */
		ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr,
						XRFDC_ADC_SWITCH_MATRX_OFFSET);
		ReadReg &= ~XRFDC_SWITCH_MTRX_MASK;
		if (ConnectIData != -1) {
			ReadReg |= ((u16)ConnectIData) << XRFDC_SEL_CB_TO_MIX0_SHIFT;
		}
		if (ConnectQData != -1) {
			ReadReg |= (u16)ConnectQData;
		}
		if ((MixerInOutDataType == XRFDC_MB_DATATYPE_C2C) &&
				(XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1)) {
			ReadReg |= XRFDC_SEL_CB_TO_QMC_MASK;
		}
		if (XRFdc_IsHighSpeedADC(InstancePtr, Tile_Id) == 1) {
			ReadReg |= XRFDC_SEL_CB_TO_DECI_MASK;
		}

		XRFdc_WriteReg16(InstancePtr, BaseAddr,
						XRFDC_ADC_SWITCH_MATRX_OFFSET, ReadReg);
	} else {
		/* DAC */
		ReadReg = XRFdc_ReadReg16(InstancePtr, BaseAddr,
					XRFDC_DAC_MB_CFG_OFFSET);
		ReadReg &= ~XRFDC_MB_CFG_MASK;
		if (Mode == XRFDC_SINGLEBAND_MODE) {
			if ((u32)ConnectIData == DigitalDataPathId) {
				if (ConnectQData != -1) {
					ReadReg |= XRFDC_SB_C2C_BLK0;
				} else {
					ReadReg |= XRFDC_SB_C2R;
				}
			}
			if ((ConnectIData == -1) && (ConnectQData == -1)) {
				ReadReg |= XRFDC_SB_C2C_BLK1;
			}
		} else {
			if (Mode == XRFDC_MULTIBAND_MODE_4X) {
				ReadReg |= XRFDC_MB_EN_4X_MASK;
			}
			if ((u32)ConnectIData == DigitalDataPathId) {
				if (ConnectQData != -1) {
					ReadReg |= XRFDC_MB_C2C_BLK0;
				} else {
					ReadReg |= XRFDC_MB_C2R_BLK0;
				}
			} else {
				if (ConnectQData != -1) {
					ReadReg |= XRFDC_MB_C2C_BLK1;
				} else {
					ReadReg |= XRFDC_MB_C2R_BLK1;
				}
			}
		}
		XRFdc_WriteReg16(InstancePtr, BaseAddr,
					XRFDC_DAC_MB_CFG_OFFSET, ReadReg);
	}
}

/** @} */