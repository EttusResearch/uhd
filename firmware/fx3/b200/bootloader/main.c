//
// Copyright 2011-2012 Cypress Semiconductor Corporation
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "cyfx3usb.h"
#include "cyfx3device.h"

extern void usbBoot (void);
extern uint8_t glCheckForDisconnect;
extern uint8_t glInCompliance;

int main (void)
{
    CyFx3BootErrorCode_t status;
    CyFx3BootIoMatrixConfig_t ioCfg = {
        .isDQ32Bit       = CyFalse,
        .useUart         = CyFalse,
        .useI2C          = CyTrue,
        .useI2S          = CyFalse,
        .useSpi          = CyFalse,
        .gpioSimpleEn[0] = 0,
        .gpioSimpleEn[1] = 0
    };

    CyFx3BootDeviceInit(CyTrue);

    status = CyFx3BootDeviceConfigureIOMatrix(&ioCfg);

    if (status != CY_FX3_BOOT_SUCCESS)
    {
        return status;
    }

    usbBoot();

    while (1)
    {
        if (glCheckForDisconnect)
        {
            CyFx3BootUsbCheckUsb3Disconnect();
            glCheckForDisconnect = 0;
        }

        if (glInCompliance)
        {
            CyFx3BootUsbSendCompliancePatterns();
            glInCompliance = 0;
        }
    }
    return 0;
}

