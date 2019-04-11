//
// Copyright 2013-2014 Ettus Research LLC
//

#include "b200_i2c.h"

#include "cyu3i2c.h"

/* I2c initialization for EEPROM programming. */
void CyFxI2cInit (uint16_t pageLen) {
    CyU3PI2cConfig_t i2cConfig;

    /* Initialize and configure the I2C master module. */
    CyU3PI2cInit ();

    /* Start the I2C master block. The bit rate is set at 100KHz.
     * The data transfer is done via DMA. */
    CyU3PMemSet ((uint8_t *)&i2cConfig, 0, sizeof(i2cConfig));
    i2cConfig.bitRate    = CY_FX_USBI2C_I2C_BITRATE;
    i2cConfig.busTimeout = 0xFFFFFFFF;
    i2cConfig.dmaTimeout = 0xFFFF;
    i2cConfig.isDma      = CyFalse;

    CyU3PI2cSetConfig (&i2cConfig, NULL);
    glI2cPageSize = pageLen;
}

/* I2C read / write for programmer application. */
void CyFxUsbI2cTransfer (
        uint16_t  byteAddress,
        uint8_t   devAddr,
        uint16_t  byteCount,
        uint8_t  *buffer,
        CyBool_t  isRead)
{
    CyU3PI2cPreamble_t preamble;
    uint16_t pageCount = (byteCount / glI2cPageSize);
    uint16_t resCount = glI2cPageSize;

    if (byteCount == 0) {
        return;
    }

    if ((byteCount % glI2cPageSize) != 0) {
        pageCount ++;
        resCount = byteCount % glI2cPageSize;
    }

    while (pageCount != 0) {
        if (isRead) {
            /* Update the preamble information. */
            preamble.length    = 4;
            preamble.buffer[0] = devAddr;
            preamble.buffer[1] = (uint8_t)(byteAddress >> 8);
            preamble.buffer[2] = (uint8_t)(byteAddress & 0xFF);
            preamble.buffer[3] = (devAddr | 0x01);
            preamble.ctrlMask  = 0x0004;

            CyU3PI2cReceiveBytes (&preamble, buffer, (pageCount == 1) ? resCount : glI2cPageSize, 0);
        } else {
            /* Write. Update the preamble information. */
            preamble.length    = 3;
            preamble.buffer[0] = devAddr;
            preamble.buffer[1] = (uint8_t)(byteAddress >> 8);
            preamble.buffer[2] = (uint8_t)(byteAddress & 0xFF);
            preamble.ctrlMask  = 0x0000;

            CyU3PI2cTransmitBytes (&preamble, buffer, (pageCount == 1) ? resCount : glI2cPageSize, 0);
            /* Wait for the write to complete. */
            preamble.length = 1;
            CyU3PI2cWaitForAck(&preamble, 200);
        }

        /* An additional delay seems to be required after receiving an ACK. */
        CyU3PThreadSleep (1);

        /* Update the parameters */
        byteAddress  += glI2cPageSize;
        buffer += glI2cPageSize;
        pageCount --;
    }
}
