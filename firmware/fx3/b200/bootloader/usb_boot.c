//
// Copyright 2011-2012 Cypress Semiconductor Corporation
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "cyfx3usb.h"
#include "cyfx3device.h"
#include "cyfx3utils.h"
#include "cyfx3i2c.h"

#include "common_helpers.h"
#include "common_descriptors.h"
#include "usb_descriptors.h"

typedef enum
{
    eStall = 0,     /* Send STALL */
    eDataIn,        /* Data IN Stage */
    eDataOut,       /* Data Out Stage */
    eStatus         /* Status Stage */
} eUsbStage;

typedef int (*PFI)();

#define USB_SETUP_DIR               (0x80) /* 0x80 = To Host */
#define USB_SETUP_MASK              (0x60) /* Used to mask off request type */
#define USB_STANDARD_REQUEST        (0x00) /* Standard Request */
#define USB_VENDOR_REQUEST          (0x40) /* Vendor Request */
#define USB_REQ_MASK                (0x3F) /* USB Request mask */
#define USB_REQ_DEV                 (0)    /* Device Request */
#define USB_REQ_INTERFACE           (1)    /* Interface Request */
#define USB_REQ_ENDPOINT            (2)    /* Endpoint Request */
#define USB_SET_INTERFACE           (11)
#define USB_SC_SET_SEL              (0x30) /* Set system exit latency. */
#define USB_SC_SET_ISOC_DELAY       (0x31)

#define SELF_POWERED  (0x01)
#define REMOTE_WAKEUP (0x02)
#define U1_ENABLE     (0x04)
#define U2_ENABLE     (0x08)
#define LTM_ENABLE    (0x10)

uint8_t glUsbState = 0;
uint8_t gConfig = 0;      /* Variable to hold the config info. */
uint8_t gAltSetting = 0;  /* Variable to hold the interface info. */
uint8_t gUsbDevStatus = 0;
uint8_t glCheckForDisconnect = 0;
uint8_t glInCompliance = 0;

/* 4KB of buffer area used for control endpoint transfers. */
#define gpUSBData                   (uint8_t*)(0x40077000)
#define USB_DATA_BUF_SIZE           (1024*4)

CyU3PUsbDescrPtrs   *gpUsbDescPtr; /* Pointer to the USB Descriptors */
CyFx3BootUsbEp0Pkt_t gEP0;

void myMemCopy(uint8_t* d, uint8_t* s, int32_t cnt)
{
    int32_t i;
    for (i = 0; i < cnt; i++) {
        *d++ = *s++;
    }
}

void myMemSet(uint8_t* d, uint8_t c, int32_t cnt)
{
    int32_t i;
    for (i = 0; i < cnt; i++) {
        *d++ = c;
    }
}

/************************************************
 * Request handlers
 ***********************************************/

/* Function to handle the GET_STATUS Standard request. */
int getStatus (void)
{
    uint16_t data = 0;

    switch (gEP0.bmReqType & USB_REQ_MASK)
    {
        case USB_REQ_DEV:
            if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
            {
                data = (gpUsbDescPtr->usbSSConfigDesc_p[7] & 0x40) ? 1 : 0;
                data |= gUsbDevStatus;
            }
            else
            {
                data = (gpUsbDescPtr->usbHSConfigDesc_p[7] & 0x40) ? 1 : 0;
                data |= gUsbDevStatus;
            }
            break;

        case USB_REQ_INTERFACE:
            if (!gConfig)
            {
                return eStall;
            }
            break;

        case USB_REQ_ENDPOINT:
            if (CyFx3BootUsbGetEpCfg (gEP0.bIdx0, 0, (CyBool_t *)&data) != 0)
            {
                return eStall;
            }
            break;
        default:
            return eStall;
    }

    *(uint16_t*)gEP0.pData = data;
    return eDataIn;
}

/* Function to handle the GET_DESCRIPTOR Standard request */
int getDescriptor (void)
{
    uint32_t len = 0;
    uint8_t *p = 0;
    uint8_t *cfg_p = 0;
    uint8_t usbSpeed;

    usbSpeed = CyFx3BootUsbGetSpeed();

    gpUsbDescPtr->usbHSConfigDesc_p[1] = CY_U3P_USB_CONFIG_DESCR;
    gpUsbDescPtr->usbFSConfigDesc_p[1] = CY_U3P_USB_CONFIG_DESCR;

    if (usbSpeed == CY_FX3_BOOT_HIGH_SPEED)
    {
        cfg_p = (uint8_t*)gpUsbDescPtr->usbHSConfigDesc_p;
        len = ((gpUsbDescPtr->usbHSConfigDesc_p[3] << 8) | gpUsbDescPtr->usbHSConfigDesc_p[2]);
    }
    else if (usbSpeed == CY_FX3_BOOT_SUPER_SPEED)
    {
        cfg_p = (uint8_t*)gpUsbDescPtr->usbSSConfigDesc_p;
        len = ((gpUsbDescPtr->usbSSConfigDesc_p[3] << 8) | gpUsbDescPtr->usbSSConfigDesc_p[2]);
    }
    else if (usbSpeed == CY_FX3_BOOT_FULL_SPEED)
    {
        cfg_p = (uint8_t*)gpUsbDescPtr->usbFSConfigDesc_p;
        len = ((gpUsbDescPtr->usbFSConfigDesc_p[3] << 8) | gpUsbDescPtr->usbFSConfigDesc_p[2]);
    }

    switch (gEP0.bVal1)
    {
        case CY_U3P_USB_DEVICE_DESCR:
            {
                if ((usbSpeed == CY_FX3_BOOT_HIGH_SPEED) || (usbSpeed == CY_FX3_BOOT_FULL_SPEED))
                {
                    p = (uint8_t*)gpUsbDescPtr->usbDevDesc_p;
                    len = gpUsbDescPtr->usbDevDesc_p[0];
                }
                else if (usbSpeed == CY_FX3_BOOT_SUPER_SPEED)
                {
                    p = (uint8_t*)gpUsbDescPtr->usbSSDevDesc_p;
                    len = gpUsbDescPtr->usbSSDevDesc_p[0];
                }
                break;
            }
        case CY_U3P_BOS_DESCR:
            {
                p = (uint8_t *)gpUsbDescPtr->usbSSBOSDesc_p;
                len = (gpUsbDescPtr->usbSSBOSDesc_p[3] << 8) | gpUsbDescPtr->usbSSBOSDesc_p[2];
                break;
            }
        case CY_U3P_USB_CONFIG_DESCR:
            {
                p = cfg_p;
                break;
            }
        case CY_U3P_USB_DEVQUAL_DESCR:
            {
                if ((usbSpeed == CY_FX3_BOOT_HIGH_SPEED)  || (usbSpeed == CY_FX3_BOOT_FULL_SPEED))
                {
                    p = (uint8_t*)gpUsbDescPtr->usbDevQualDesc_p;
                    len = gpUsbDescPtr->usbDevQualDesc_p[0];
                    break;
                }
                return eStall;
            }
        case CY_U3P_USB_STRING_DESCR:
            {
                /* Ensure that we do not index past the limit of the array. */
                if (gEP0.bVal0 < CY_FX3_USB_MAX_STRING_DESC_INDEX)
                {
                    p = (uint8_t*)gpUsbDescPtr->usbStringDesc_p[gEP0.bVal0];
                    if (p != 0)
                        len = p[0];
                }
                else
                    return eStall;
                break;
            }
        case CY_U3P_USB_OTHERSPEED_DESCR:
            {
                if (usbSpeed == CY_FX3_BOOT_HIGH_SPEED)
                {
                    gpUsbDescPtr->usbFSConfigDesc_p[1] = CY_U3P_USB_OTHERSPEED_DESCR;
                    p = (uint8_t*)gpUsbDescPtr->usbFSConfigDesc_p;

                    len = ((gpUsbDescPtr->usbFSConfigDesc_p[3] < 8) | gpUsbDescPtr->usbFSConfigDesc_p[2]);

                    if (len > gEP0.wLen)
                    {
                        len = gEP0.wLen;
                    }
                }
                else if (usbSpeed == CY_FX3_BOOT_FULL_SPEED)
                {
                    gpUsbDescPtr->usbHSConfigDesc_p[1] = CY_U3P_USB_OTHERSPEED_DESCR;
                    p = (uint8_t*)gpUsbDescPtr->usbHSConfigDesc_p;
                    len = ((gpUsbDescPtr->usbHSConfigDesc_p[3] < 8) | gpUsbDescPtr->usbHSConfigDesc_p[2]);

                    if (len > gEP0.wLen)
                    {
                        len = gEP0.wLen;
                    }
                }
            }
            break;
        default:
            {
                return eStall;
            }
    }

    if (p != 0)
    {
        myMemCopy (gpUSBData, p, len);
        if (gEP0.wLen > len)
        {
            gEP0.wLen = len;
        }

        return eDataIn;
    }
    else
        /* Stall EP0 if the descriptor sought is not available. */
        return eStall;
}

/* Function to handle the SET_CONFIG Standard request */
int setConfig (void)
{
    uint8_t usbSpeed = 0;
    uint32_t retVal  = 0;
    CyFx3BootUsbEpConfig_t epCfg;

    if ((gEP0.bVal0 == 0) || (gEP0.bVal0 == 1))
    {
        glUsbState = gEP0.bVal0;
        gConfig = gEP0.bVal0;

        /* Get the Bus speed */
        usbSpeed = CyFx3BootUsbGetSpeed();

        epCfg.pcktSize = 512;
        /* Based on the Bus Speed configure the endpoint packet size */
        if (usbSpeed == CY_FX3_BOOT_HIGH_SPEED)
        {
            epCfg.pcktSize = 512;
        }
        if (usbSpeed == CY_FX3_BOOT_SUPER_SPEED)
        {
            epCfg.pcktSize = 1024;
        }
        if (usbSpeed == CY_FX3_BOOT_FULL_SPEED)
        {
            epCfg.pcktSize = 64;
        }

        /* Producer Endpoint configuration */
        epCfg.enable = 1;
        epCfg.epType = CY_FX3_BOOT_USB_EP_BULK;
        epCfg.burstLen = 1;
        epCfg.streams = 0;
        epCfg.isoPkts = 0;

        /* Configure the Endpoint */
        retVal = CyFx3BootUsbSetEpConfig(0x01, &epCfg);
        if (retVal != 0)
        {
            /* TODO: Error Handling */
            return eStall;
        }

        /* Consumer Endpoint configuration */
        epCfg.enable = 1;
        epCfg.epType = CY_FX3_BOOT_USB_EP_BULK;
        epCfg.burstLen = 1;
        epCfg.streams = 0;
        epCfg.isoPkts = 0;

        /* Configure the Endpoint */
        retVal = CyFx3BootUsbSetEpConfig (0x81, &epCfg);
        if (retVal != 0)
        {
            /* TODO: Error Handling */
            return eStall;
        }

        return eStatus;
    }

    return eStall;
}

/* Function to handle the GET_INTERFACE Standard request */
int getInterface (void)
{
    if (gConfig == 0)
    {
        return eStall;
    }

    gEP0.pData = (uint8_t *)&gAltSetting;
    return eDataIn;
}

/* Function to handle the SET_INTERFACE Standard request */
int setInterface (void)
{
    gAltSetting = gEP0.bVal0;
    return eStatus;
}

/* This function returns stall for not supported requests. */
int stall (void)
{
    return eStall;
}

/* Function to handle the SET_ADDRESS Standard request. */
int setAddress (void)
{
    return eStatus;
}

/* Function to handle the CLEAR_FEATURE Standard request. */
int clearFeature (void)
{
    /* All of the actual handling for the CLEAR_FEATURE request is done in the API.
       We only need to update the device status flags here.
     */
    if (CyFx3BootUsbSetClrFeature (0, (CyBool_t)glUsbState, &gEP0) != 0)
    {
        return eStall;
    }

    if (gEP0.bmReqType == USB_REQ_DEV)
    {
        /* Update the device status flags as required. */
        if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
        {
            switch (gEP0.bVal0)
            {
            case 48:
                gUsbDevStatus &= ~U1_ENABLE;
                break;
            case 49:
                gUsbDevStatus &= ~U2_ENABLE;
                break;
            case 50:
                gUsbDevStatus &= ~LTM_ENABLE;
                break;
            default:
                break;
            }
        }
        else
        {
            if (gEP0.bVal0 == 1)
            {
                gUsbDevStatus &= ~REMOTE_WAKEUP;
            }
        }
    }

    return eStatus;
}

/* Function to handle the SET_FEATURE Standard request. */
int setFeature (void)
{
    /* All of the actual handling for the SET_FEATURE command is done in the API.
     * We only need to update the device status flags here.
     */
    if (CyFx3BootUsbSetClrFeature (1, (CyBool_t)glUsbState, &gEP0) != 0)
    {
        return eStall;
    }

    if (gEP0.bmReqType == USB_REQ_DEV)
    {
        /* Update the device status flags as required. */
        if (CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED)
        {
            switch (gEP0.bVal0)
            {
            case 48:
                gUsbDevStatus |= U1_ENABLE;
                break;
            case 49:
                gUsbDevStatus |= U2_ENABLE;
                break;
            case 50:
                gUsbDevStatus |= LTM_ENABLE;
                break;
            default:
                break;
            }
        }
        else
        {
            if (gEP0.bVal0 == 1)
            {
                gUsbDevStatus |= REMOTE_WAKEUP;
            }
        }
    }
    return eStatus;
}

/* Function to handle the GET_CONFIG Standard request. */
int getConfig (void)
{
    gEP0.pData = (uint8_t *)&gConfig;
    return eDataIn;
}

const PFI chapter9_cmds[] =
{
    getStatus,        /* USB_GET_STATUS         0 */
    clearFeature,     /* USB_CLEAR_FEATURE      1 */
    stall,            /* Reserved               2 */
    setFeature,       /* USB_SET_FEATURE        3 */
    stall,            /* Reserved               4 */
    setAddress,       /* USB_SET_ADDRESS        5 */
    getDescriptor,    /* USB_GET_DESCRIPTOR     6 */
    stall,            /* USB_SET_DESCRIPTOR     7 */
    getConfig,        /* USB_GET_CONFIGURATION  8 */
    setConfig,        /* USB_SET_CONFIGURATION  9 */
    getInterface,     /* USB_GET_INTERFACE     10 */
    setInterface,     /* USB_SET_INTERFACE     11 */
};

/* This function validates the addresses being written to/read from
   Return Value:
    0 - Address is valid
   -1 - Address is not valid
*/
int checkAddress (uint32_t address, uint32_t len)
{
    if (address & 3)
    {
        /* expect long word boundary */
        return -1;
    }

    len += address;

    if ((address >= CY_FX3_BOOT_SYSMEM_BASE1) && (len <= CY_FX3_BOOT_SYSMEM_END))
    {
        return 0;
    }

    if (len <= CY_FX3_BOOT_ITCM_END)
    {
        return 0;
    }

    return -1;
}

/* Function to handle the vendor commands. */
void vendorCmdHandler (void)
{
    int stage;
    int status;
    uint32_t address  = ((gEP0.bIdx1 << 24) | (gEP0.bIdx0 << 16) | (gEP0.bVal1 << 8) | (gEP0.bVal0));
    uint16_t len  = gEP0.wLen;
    uint16_t bReq = gEP0.bReq;
    uint16_t dir  = gEP0.bmReqType & USB_SETUP_DIR;

    if (len > USB_DATA_BUF_SIZE)
    {
        CyFx3BootUsbStall (0, CyTrue, CyFalse);
        return;
    }

    if (dir)
    {
        stage = eDataIn;
    }
    else
    {
        stage = eDataOut;
    }

    /* Vendor Command 0xA0 handling */
    if (bReq == 0xA0)
    {
	/* Note: This is a command issued by the CyControl Application to detect the legacy products.
	   As we are an FX3 device we stall the endpoint to indicate that this is not a legacy device.
	 */
	if (address == 0xE600)
	{
	    /* Stall the Endpoint */
	    CyFx3BootUsbStall (0, CyTrue, CyFalse);
	    return;
	}

        status = checkAddress (address, len);
        if (len == 0)
        {
            /* Mask the USB Interrupts and Disconnect the USB Phy. */
            CyFx3BootUsbConnect (CyFalse, CyTrue);
            /* Transfer to Program Entry */
            CyFx3BootJumpToProgramEntry (address);
            return;
        }

        if (status < 0)
        {
            /* Stall the endpoint */
            CyFx3BootUsbStall (0, CyTrue, CyFalse);
            return;
        }

        /* Validate the SYSMEM address being accessed */
        if ((address >= CY_FX3_BOOT_SYSMEM_BASE1) && (address < CY_FX3_BOOT_SYSMEM_END))
        {
            gEP0.pData = (uint8_t*)address;
        }

        CyFx3BootUsbAckSetup ();

        if (eDataIn == stage)
        {
            if ((address + gEP0.wLen) <= CY_FX3_BOOT_ITCM_END)
            {
                myMemCopy(gEP0.pData, (uint8_t *)address , len);
            }

            status = CyFx3BootUsbDmaXferData (0x80, (uint32_t)gEP0.pData, gEP0.wLen, CY_FX3_BOOT_WAIT_FOREVER);
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                /* USB DMA Transfer failed. Stall the Endpoint. */
                CyFx3BootUsbStall (0, CyTrue, CyFalse);
                return;
            }
        }
        else if (stage == eDataOut)
        {
            status = CyFx3BootUsbDmaXferData (0x00, (uint32_t)gEP0.pData, gEP0.wLen, CY_FX3_BOOT_WAIT_FOREVER);
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                /* USB DMA Transfer failed. Stall the Endpoint. */
                CyFx3BootUsbStall (0, CyTrue, CyFalse);
                return;
            }

            /* Validate ITCM Memory */
            if ((address + gEP0.wLen) <= CY_FX3_BOOT_ITCM_END)
            {
                /* Avoid writing to the interrupt table. */
                if (address < 0xFF) {
                    gEP0.pData += 0xFF-address;
                    gEP0.wLen -= 0xFF-address;
                    address = 0xFF;
                }
                myMemCopy((uint8_t *)address, gEP0.pData, gEP0.wLen);
            }
        }
        return;
    }

    /* Version request */
    if (dir && bReq == 0xB8) {
        uint8_t version_info[] = {1, 21, 0, 0, 0, 0};

        CyFx3BootUsbAckSetup ();
        status = CyFx3BootUsbDmaXferData (0x80, (uint32_t)version_info,
                sizeof(version_info), CY_FX3_BOOT_WAIT_FOREVER);
        if (status != CY_FX3_BOOT_SUCCESS) {
            /* USB DMA Transfer failed. Stall the Endpoint. */
            CyFx3BootUsbStall (0, CyTrue, CyFalse);
        }

        return;
    }

    /* Stall the Endpoint */
    CyFx3BootUsbStall (0, CyTrue, CyFalse);
    return;
}

/* Setup Data handler */
void setupDataHandler (uint32_t setupDat0, uint32_t setupDat1)
{
    uint32_t *p;
    int status = eStall;

    p = (uint32_t*)&gEP0;
    p[0] = setupDat0;
    p[1] = setupDat1;

    gEP0.pData = gpUSBData;

    switch (gEP0.bmReqType & USB_SETUP_MASK)
    {
        case USB_STANDARD_REQUEST:
            if (gEP0.bReq <= USB_SET_INTERFACE)
            {
                status = (*chapter9_cmds[gEP0.bReq])();
            }
            else
            {
                if (gEP0.bReq == USB_SC_SET_SEL)
                {
                    if ((CyFx3BootUsbGetSpeed () == CY_FX3_BOOT_SUPER_SPEED) && (gEP0.bIdx0 == 0) &&
                            (gEP0.bIdx1 == 0) && (gEP0.bVal0 == 0) && (gEP0.bVal1 == 0) && (gEP0.wLen == 6))
                    {
                        gEP0.wLen = 32;
                        status = eDataOut;
                    }
                    else
                    {
                        status = eStall;
                    }
                }
                else if (gEP0.bReq == USB_SC_SET_ISOC_DELAY)
                {
                    status = eStatus;
                    if ((CyFx3BootUsbGetSpeed () != CY_FX3_BOOT_SUPER_SPEED) || (gEP0.bIdx0 != 0) ||
                            (gEP0.bIdx1 != 0) || (gEP0.wLen != 0))
                    {
                        status = eStall;
                    }
                }
                else
                {
                    status = eStall;
                }
            }
            break;

        case USB_VENDOR_REQUEST:
            vendorCmdHandler();
            return;
    }

    switch (status)
    {
        case eDataIn:
            CyFx3BootUsbAckSetup ();
            status = CyFx3BootUsbDmaXferData (0x80, (uint32_t)gEP0.pData, gEP0.wLen, 1000);
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                CyFx3BootUsbStall (0, CyTrue, CyFalse);
            }
            break;
        case eDataOut:
            CyFx3BootUsbAckSetup ();
            status = CyFx3BootUsbDmaXferData (0x00, (uint32_t)gEP0.pData, gEP0.wLen, CY_FX3_BOOT_WAIT_FOREVER);
            if (status != CY_FX3_BOOT_SUCCESS)
            {
                CyFx3BootUsbStall (0, CyTrue, CyFalse);
            }
            break;
        case eStatus:
            CyFx3BootUsbAckSetup ();
            break;
        default:
            CyFx3BootUsbStall (0, CyTrue, CyFalse);
            return;
    }

    return;
}

/* USB Event Handler. This function is invoked from the USB ISR and as such MUST not be
   blocked.
*/
void usbEventCallback (CyFx3BootUsbEventType_t event)
{
    if (event == CY_FX3_BOOT_USB_RESET)
    {
        gConfig              = 0;
        gAltSetting          = 0;
        gUsbDevStatus        = 0;
        glUsbState           = 0;
        glInCompliance       = 0;
    }

    if ((event == CY_FX3_BOOT_USB_CONNECT) ||
        (event == CY_FX3_BOOT_USB_DISCONNECT))
    {
        glUsbState    = 0;
        gUsbDevStatus = 0;
    }

    if (event == CY_FX3_BOOT_USB_IN_SS_DISCONNECT)
    {
        glCheckForDisconnect = CyTrue;
    }

    if (event == CY_FX3_BOOT_USB_COMPLIANCE)
    {
        glInCompliance = CyTrue;
    }

    return;
}

/* Function passed to common EEPROM functions to access the EEPROM */
void eeprom_read(uint16_t addr, uint8_t* buffer, uint8_t length)
{
    CyFx3BootI2cPreamble_t preamble;
    preamble.length    = 4;
    preamble.buffer[0] = 0xA0;
    preamble.buffer[1] = addr >> 8;
    preamble.buffer[2] = addr & 0xFF;
    preamble.buffer[3] = 0xA1;
    preamble.ctrlMask  = 0x0004;
    CyFx3BootI2cReceiveBytes(&preamble, buffer, length, 0);
}

/* USB initialization */
void usbBoot()
{
    /* Reset globals */
    gConfig              = 0;
    gAltSetting          = 0;
    gUsbDevStatus        = 0;
    glUsbState           = 0;
    glCheckForDisconnect = 0;
    glInCompliance       = 0;

    /* Init */
    CyFx3BootUsbStart (CyFalse, usbEventCallback);

    /* Run USB circuitry off of host power, not internal power. */
    CyFx3BootUsbVBattEnable (CyFalse);

    /* Register callbacks */
    CyFx3BootRegisterSetupCallback (setupDataHandler);

    /* Enable I2C for EEPROM access */
    CyFx3BootI2cInit();
    CyFx3BootI2cConfig_t i2cCfg = {
        .bitRate    = 100000,
        .isDma      = CyFalse,
        .busTimeout = 0xFFFFFFFF,
        .dmaTimeout = 0xFFFF};
    CyFx3BootI2cSetConfig(&i2cCfg);

    /* Retrieve VID and PID from EEPROM */
    const uint16_t vid = get_vid(&eeprom_read);
    const uint16_t pid = get_pid(&eeprom_read);

    /* Power down I2C */
    CyFx3BootI2cDeInit();

    /* Populate device descriptors with IDs */
    common_usb2_dev_desc[8]  = vid & 0xFF;
    common_usb2_dev_desc[9]  = vid >> 8;
    common_usb2_dev_desc[10] = pid & 0xFF;
    common_usb2_dev_desc[11] = pid >> 8;

    common_usb3_dev_desc[8]  = vid & 0xFF;
    common_usb3_dev_desc[9]  = vid >> 8;
    common_usb3_dev_desc[10] = pid & 0xFF;
    common_usb3_dev_desc[11] = pid >> 8;

    /* Copy all descriptors */
    CyFx3BootUsbSetDesc(
        CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t*)common_usb2_dev_desc);
    CyFx3BootUsbSetDesc(
        CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t*)common_usb3_dev_desc);

    CyFx3BootUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, 0, (uint8_t*)common_dev_qual_desc);

    CyFx3BootUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t*)bl_hs_config_desc);
    CyFx3BootUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t*)bl_fs_config_desc);
    CyFx3BootUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t*)bl_ss_config_desc);

    CyFx3BootUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, 0, (uint8_t*)common_usb_bos_desc);

    CyFx3BootUsbSetDesc(
        CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t*)common_string_lang_id_desc);

    CyFx3BootUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t*)bl_manufacturer_desc);
    CyFx3BootUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t*)bl_product_desc);
    CyFx3BootUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 3, (uint8_t*)bl_dev_serial_desc);

    gpUsbDescPtr = CyFx3BootUsbGetDesc();

    /* Connect! */
    CyFx3BootUsbConnect (CyTrue, CyTrue);
}

