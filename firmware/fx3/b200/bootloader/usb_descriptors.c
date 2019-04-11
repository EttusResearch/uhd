//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/* Bootloader's unique config and string descriptors. */

/* Bootloader Full Speed Configuration Descriptor */
const unsigned char bl_fs_config_desc[] =
{
    /* Configuration Descriptor Type */
    0x09,                           /* Descriptor Size */
    0x02,                           /* Configuration Descriptor Type */
    0x20,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - Bus powered */
    0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    0x04,                           /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x02,                           /* Number of end points */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint Descriptor for Producer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x01,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x40,0x00,                      /* Max packet size = 64 bytes */
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */

    /* Endpoint Descriptor for Consumer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x81,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x40,0x00,                      /* Max packet size = 64 bytes */
    0x00                            /* Servicing interval for data transfers : NA for Bulk */
};

/* Bootloader High Speed Configuration Descriptor */
const unsigned char bl_hs_config_desc[] =
{
    0x09,                           /* Descriptor Size */
    0x02,                           /* Configuration Descriptor Type */
    0x20,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - Bus powered */
    0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    0x04,                           /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x02,                           /* Number of end points */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint Descriptor for Producer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x01,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x00,                           /* Max packet size = 512 bytes */
    0x02,
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */

    /* Endpoint Descriptor for Consumer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x81,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x00,                           /* Max packet size = 512 bytes */
    0x02,
    0x00                            /* Servicing interval for data transfers : NA for Bulk */
};

/* Bootloader Super Speed Configuration Descriptor */
const unsigned char bl_ss_config_desc[] =
{
    /* Configuration Descriptor Type */
    0x09,                           /* Descriptor Size */
    0x02,                           /* Configuration Descriptor Type */
    0x2C,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - D6: Self power; D5: Remote Wakeup */
    0x32,                           /* Max power consumption of device (in 8mA unit) : 400mA */

    /* Interface Descriptor */
    0x09,                           /* Descriptor size */
    0x04,                           /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x02,                           /* Number of end points */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint Descriptor for Producer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x01,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */

    /* Super Speed Endpoint Companion Descriptor for Producer EP */
    0x06,                           /* Descriptor size */
    0x30,                           /* SS Endpoint Companion Descriptor Type */
    0x00,                           /* Max no. of packets in a Burst : 0: Burst 1 packet at a time */
    0x00,                           /* Max streams for Bulk EP = 0 (No streams)*/
    0x00,0x00,                      /* Service interval for the EP : NA for Bulk */

    /* Endpoint Descriptor for Consumer EP */
    0x07,                           /* Descriptor size */
    0x05,                           /* Endpoint Descriptor Type */
    0x81,                           /* Endpoint address and description */
    0x02,                           /* Bulk End point Type */
    0x00,0x04,                      /* Max packet size = 1024 bytes */
    0x00,                           /* Servicing interval for data transfers : NA for Bulk */

    /* Super Speed Endpoint Companion Descriptor for Consumer EP */
    0x06,                           /* Descriptor size */
    0x30,                           /* SS Endpoint Companion Descriptor Type */
    0x00,                           /* Max no. of packets in a Burst : 0: Burst 1 packet at a time */
    0x00,                           /* Max streams for Bulk EP = 0 (No streams)*/
    0x00,0x00                       /* Service interval for the EP : NA for Bulk */
};

/* Bootloader Product String Descriptor */
const unsigned char bl_manufacturer_desc[] =
{
    0x10,
    0x03,
    'C', 0x00,
    'y', 0x00,
    'p', 0x00,
    'r', 0x00,
    'e', 0x00,
    's', 0x00,
    's', 0x00
};

/* Bootloader Manufacturer String Descriptor */
const unsigned char bl_product_desc[] =
{
    0x16,
    0x03,
    'W', 0x00,
    'e', 0x00,
    's', 0x00,
    't', 0x00,
    'B', 0x00,
    'r', 0x00,
    'i', 0x00,
    'd', 0x00,
    'g', 0x00,
    'e', 0x00
};

/* Bootloader Device Serial String Descriptor */
const unsigned char bl_dev_serial_desc [] =
{
    0x1A,                           /* bLength */
    0x03,                           /* bDescType */
    '0',0,'0',0,'0',0,'0',0,'0',0,'0',0,
    '0',0,'0',0,'0',0,'4',0,'B',0,'E',0,
    0,0,                            /* long word align */
};

