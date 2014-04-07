//
// Copyright 2013-2014 Ettus Research LLC
//

#ifndef _B200_I2C_H
#define _B200_I2C_H

#include "cyu3externcstart.h"

#include "cyu3usbconst.h"
#include "cyu3types.h"

/* Following two definitions made in b200_main.h for consistency. */
/* define B200_VREQ_EEPROM_WRITE          (uint8_t)(0xBA) */
/* define B200_VREQ_EEPROM_READ           (uint8_t)(0xBB) */

static uint16_t glI2cPageSize = 0x40;   /* I2C Page size to be used for transfers. */

/* This application uses EEPROM as the slave I2C device. The I2C EEPROM
 * part number used is 24LC256. The capacity of the EEPROM is 256K bits */
#define CY_FX_USBI2C_I2C_MAX_CAPACITY   (32 * 1024) /* Capacity in bytes */

/* The following constant is defined based on the page size that the I2C
 * device support. 24LC256 support 64 byte page write access. */
#define CY_FX_USBI2C_I2C_PAGE_SIZE      (64)

/* I2C Data rate */
#define CY_FX_USBI2C_I2C_BITRATE        (100000)

/* Give a timeout value of 5s for any programming. */
#define CY_FX_USB_I2C_TIMEOUT           (5000)

/* Function forward-declerations. */
void CyFxI2cInit (uint16_t pageLen);
void CyFxUsbI2cTransfer (uint16_t byteAddress, uint8_t devAddr,
                         uint16_t byteCount, uint8_t  *buffer, CyBool_t isRead);

#include "cyu3externcend.h"

#endif /* _B200_I2C_H */
