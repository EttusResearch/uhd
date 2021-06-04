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
* @file xrfdc_sinit.c
* @addtogroup rfdc_v6_0
* @{
*
* The implementation of the XRFdc component's static initialization
* functionality.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---    -------- -----------------------------------------------
* 1.0   sk     05/16/17 Initial release
* 5.0   mus    08/17/18 Updated XRFdc_LookupConfig to make use of device
*                       tree instead of xrfdc_g.c, to obtain
*                       XRFdc_Config for provided device id.It is being
*                       achieved through "param-list" property in RFDC
*                       device node, it will be having 1:1 mapping with
*                       the XRFdc_Config structure. Said changes
*                       have been done, to remove the xparameters.h
*                       dependency from RFDC Linux user space driver.
*
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#define METAL_INTERNAL
#include "mpm/rfdc/xrfdc.h"
#ifdef __BAREMETAL__
#include "xparameters.h"
#else
#include <dirent.h>
#include <arpa/inet.h>
#endif
/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/
#ifdef __BAREMETAL__
extern XRFdc_Config XRFdc_ConfigTable[];
#else
static XRFdc_Config *XRFdc_ConfigTablePtr=NULL;
#endif

#ifndef __BAREMETAL__
/*****************************************************************************/
/**
*
* Compare two strings in the reversed order.This function compares only
* the last "Count" number of characters of Str1Ptr and Str2Ptr.
*
* @param	Str1Ptr is base address of first string
* @param	Str2Ptr is base address of second string
* @param	Count is number of last characters  to be compared between
*			Str1Ptr and Str2Ptr
*
* @return
*			0 if last "Count" number of bytes matches between Str1Ptr and
*			Str2Ptr, else differnce in unmatched character.
*
*@note		None.
*
******************************************************************************/
static s32 XRFdc_Strrncmp(const char *Str1Ptr, const char *Str2Ptr, size_t Count)
{
	u16 Len1 = strlen(Str1Ptr);
	u16 Len2 = strlen(Str2Ptr);
	u8 Diff;

	for (; Len1 && Len2; Len1--, Len2--) {
		if ((Diff = Str1Ptr[Len1 - 1] - Str2Ptr[Len2 - 1]) != 0) {
			return Diff;
		}
		if (--Count == 0) {
			return 0;
		}
	}

	return (Len1 - Len2);
}

/*****************************************************************************/
/**
*
* Traverse "/sys/bus/platform/device" directory, to find RFDC device entry,
* corresponding to provided device id. If device entry corresponding to said
* device id is found, store it in output buffer DevNamePtr.
*
* @param	DevNamePtr is base address of char array, where device name
*			will be stored
* @param	DevId contains the ID of the device to look up the
*			RFDC device name entry in "/sys/bus/platform/device"
*
* @return
 *			- XRFDC_SUCCESS if successful.
 *			- XRFDC_FAILURE if device entry not found for given device id.
 *
 *@note		None.
*
******************************************************************************/
s32 XRFdc_GetDeviceNameByDeviceId(char *DevNamePtr, u16 DevId)
{
	s32 Status = XRFDC_FAILURE;
	u32 Data = 0;
	char CompatibleString[NAME_MAX];
	struct metal_device *DevicePtr;
	DIR *DirPtr;
	struct dirent *DirentPtr;
	char Len = strlen(XRFDC_COMPATIBLE_STRING);
	char SignLen = strlen(XRFDC_SIGNATURE);

	DirPtr = opendir(XRFDC_PLATFORM_DEVICE_DIR);
	if (DirPtr) {
		while ((DirentPtr = readdir(DirPtr)) != NULL) {
			if (XRFdc_Strrncmp(DirentPtr->d_name,
				XRFDC_SIGNATURE, SignLen) == 0) {
				Status = metal_device_open("platform",DirentPtr->d_name,
						 &DevicePtr);
				if (Status) {
					metal_log(METAL_LOG_ERROR,
							"\n Failed to open device %s", DirentPtr->d_name);
					continue;
				}
				Status = metal_linux_get_device_property(DevicePtr,
							XRFDC_COMPATIBLE_PROPERTY, CompatibleString ,
							Len);
				if (Status < 0) {
					metal_log(METAL_LOG_ERROR,
							"\n Failed to read device tree property");
				} else if (strncmp(CompatibleString, \
							XRFDC_COMPATIBLE_STRING, Len) == 0) {
					Status = metal_linux_get_device_property(DevicePtr,
								XRFDC_CONFIG_DATA_PROPERTY,
								&Data, XRFDC_DEVICE_ID_SIZE);
					if (Status < 0) {
						metal_log(METAL_LOG_ERROR,
							"\n Failed to read device tree property");
					} else if ( Data == DevId ) {
						strcpy(DevNamePtr, DirentPtr->d_name);
						Status = XRFDC_SUCCESS;
						metal_device_close(DevicePtr);
						break;
					}
				}
				metal_device_close(DevicePtr);
			}
		}
		closedir(DirPtr);
	}
	return Status;
}
#endif
/*****************************************************************************/
/**
*
* Looks up the device configuration based on the unique device ID. A table
* contains the configuration info for each device in the system.
*
* @param	DeviceId contains the ID of the device to look up the
*		configuration for.
*
* @return
*
* A pointer to the configuration found or NULL if the specified device ID was
* not found. See xrfdc.h for the definition of XRFdc_Config.
*
* @note		None.
*
******************************************************************************/
XRFdc_Config *XRFdc_LookupConfig(u16 DeviceId)
{
	XRFdc_Config *CfgPtr = NULL;
#ifndef __BAREMETAL__
	s32 Status=0;
	u32 NumInstances;
	struct metal_device *Deviceptr;
	char DeviceName[NAME_MAX];

	Status = XRFdc_GetDeviceNameByDeviceId(DeviceName, DeviceId);
	if (Status != XRFDC_SUCCESS) {
		metal_log(METAL_LOG_ERROR, "\n Invalid device id %d", DeviceId);
		goto RETURN_PATH2;
	}

	Status = metal_device_open(XRFDC_BUS_NAME, DeviceName, &Deviceptr);
	if (Status) {
		metal_log(METAL_LOG_ERROR, "\n Failed to open device %s", DeviceName);
		goto RETURN_PATH2;
	}

	if (XRFdc_ConfigTablePtr == NULL) {
		Status = metal_linux_get_device_property(Deviceptr,
					XRFDC_NUM_INSTANCES_PROPERTY,
					&NumInstances, XRFDC_NUM_INST_SIZE);
		if (Status < 0) {
			metal_log(METAL_LOG_ERROR,
					"\n Failed to read device tree property %s",
					XRFDC_NUM_INSTANCES_PROPERTY);
			goto RETURN_PATH1;
		}
		XRFdc_ConfigTablePtr = (XRFdc_Config*) malloc(ntohl(NumInstances) * \
								XRFDC_CONFIG_DATA_SIZE);
		if (XRFdc_ConfigTablePtr == NULL) {
			metal_log(METAL_LOG_ERROR,
					"\n Failed to allocate memory for XRFdc_ConfigTablePtr");
			goto RETURN_PATH1;
		}
	}
	Status = metal_linux_get_device_property(Deviceptr,
						XRFDC_CONFIG_DATA_PROPERTY,
						&XRFdc_ConfigTablePtr[DeviceId],
						XRFDC_CONFIG_DATA_SIZE);
	if (Status == XRFDC_SUCCESS) {
		CfgPtr = &XRFdc_ConfigTablePtr[DeviceId];
	} else {
		metal_log(METAL_LOG_ERROR,
			"\n Failed to read device tree property %s",
			XRFDC_CONFIG_DATA_PROPERTY);

	}
RETURN_PATH1:
	metal_device_close(Deviceptr);
RETURN_PATH2:
#else
	u32 Index;

	for (Index = 0U; Index < (u32)XPAR_XRFDC_NUM_INSTANCES; Index++) {
		if (XRFdc_ConfigTable[Index].DeviceId == DeviceId) {
			CfgPtr = &XRFdc_ConfigTable[Index];
			break;
		}
	}
#endif
	return (XRFdc_Config *)CfgPtr;
}
/** @} */
