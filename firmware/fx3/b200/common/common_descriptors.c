//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/* This file contains all descriptors that are common between the bootloader
   and firmware code.  Non-const descriptors are modified at runtime based on
   information read from the EEPROM. */

/* Standard Device Descriptor for USB 2.0 */
unsigned char common_usb2_dev_desc[] =
{
    0x12,                           /* Descriptor size */
    0x01,/* CY_U3P_USB_DEVICE_DESCR    Device descriptor type */
    0x10,0x02,                      /* USB 2.10 */
    0xFF,                           /* Device class */
    0x00,                           /* Device sub-class */
    0x00,                           /* Device protocol */
    0x40,                           /* Maxpacket size for EP0 : 64 bytes */
    0xB4,0x04,                      /* Vendor ID */
    0xF0,0x00,                      /* Product ID */
    0x00,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x03,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

/* Standard Device Descriptor for USB 3.0 */
unsigned char common_usb3_dev_desc[] =
{
    0x12,                           /* Descriptor size */
    0x01,/* CY_U3P_USB_DEVICE_DESCR    Device descriptor type */
    0x00,0x03,                      /* USB 3.0 */
    0xFF,                           /* Device class */
    0x00,                           /* Device sub-class */
    0x00,                           /* Device protocol */
    0x09,                           /* Maxpacket size for EP0 : 2^9 */
    0xB4,0x04,                      /* Vendor ID */
    0xF0,0x00,                      /* Product ID */
    0x00,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x03,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

/* Standard Device Qualifier Descriptor */
const unsigned char common_dev_qual_desc[] =
{
    0x0A,                           /* Descriptor size */
    0x06,/* CY_U3P_USB_DEVQUAL_DESCR   Device qualifier descriptor type */
    0x00,0x02,                      /* USB 2.0 */
    0xFF,                           /* Device class */
    0x00,                           /* Device sub-class */
    0x00,                           /* Device protocol */
    0x40,                           /* Maxpacket size for EP0 : 64 bytes */
    0x01,                           /* Number of configurations */
    0x00                            /* Reserved */
};

/* Standard Binary Device Object Store Descriptor */
const unsigned char common_usb_bos_desc[] =
{
    0x05,                           /* Descriptor size */
    0x0F,/* CY_U3P_BOS_DESCR           Device descriptor type */
    0x16,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of device capability descriptors */

    /* USB 2.0 extension */
    0x07,                           /* Descriptor size */
    0x10,/* CY_U3P_DEVICE_CAPB_DESCR   Device capability type descriptor */
    0x02,/* CY_U3P_USB2_EXTN_CAPB_TYPE USB 2.0 extension capability type */
    0x02,0x00,0x00,0x00,            /* Supported device level features: LPM support  */

    /* SuperSpeed device capability */
    0x0A,                           /* Descriptor size */
    0x10,/* CY_U3P_DEVICE_CAPB_DESCR   Device capability type descriptor */
    0x03,/* CY_U3P_SS_USB_CAPB_TYPE    SuperSpeed device capability type */
    0x00,                           /* Supported device level features  */
    0x0E,0x00,                      /* Speeds supported by the device : SS, HS and FS */
    0x03,                           /* Functionality support */
    0x00,                           /* U1 Device Exit latency */
    0x00,0x00                       /* U2 Device Exit latency */
};

/* Standard Language ID String Descriptor */
const unsigned char common_string_lang_id_desc[] =
{
    0x04,                           /* Descriptor Size */
    0x03,/* CY_U3P_USB_STRING_DESCR    Device Descriptor Type */
    0x09,0x04                       /* Language ID supported */
};


/* Ettus Manufacturer String Descriptor */
const unsigned char common_ettus_manufacturer_desc[] =
{
    0x26,                           /* Descriptor Size */
    0x03,/* CY_U3P_USB_STRING_DESCR    Device Descriptor Type */
    'E',0x00,
    't',0x00,
    't',0x00,
    'u',0x00,
    's',0x00,
    ' ',0x00,
    'R',0x00,
    'e',0x00,
    's',0x00,
    'e',0x00,
    'a',0x00,
    'r',0x00,
    'c',0x00,
    'h',0x00,
    ' ',0x00,
    'L',0x00,
    'L',0x00,
    'C',0x00
};

/* NI Manufacturer String Descriptor */
const unsigned char common_ni_manufacturer_desc[] =
{
    0x36,                           /* Descriptor Size */
    0x03,/* CY_U3P_USB_STRING_DESCR    Device Descriptor Type */
    'N',0x00,
    'a',0x00,
    't',0x00,
    'i',0x00,
    'o',0x00,
    'n',0x00,
    'a',0x00,
    'l',0x00,
    ' ',0x00,
    'I',0x00,
    'n',0x00,
    's',0x00,
    't',0x00,
    'r',0x00,
    'u',0x00,
    'm',0x00,
    'e',0x00,
    'n',0x00,
    't',0x00,
    's',0x00,
    ' ',0x00,
    'C',0x00,
    'o',0x00,
    'r',0x00,
    'p',0x00,
    '.',0x00
};


/* Ettus Product String Descriptor */
const unsigned char common_b200_product_desc[] =
{
    0x14,                           /* Descriptor Size */
    0x03,/* CY_U3P_USB_STRING_DESCR    Device Descriptor Type */
    'U',0x00,
    'S',0x00,
    'R',0x00,
    'P',0x00,
    ' ',0x00,
    'B',0x00,
    '2',0x00,
    '0',0x00,
    '0',0x00
};

/* NI-USRP 2900 Product String Descriptor */
const unsigned char common_niusrp_2900_product_desc[] =
{
    0x1A,                           /* Descriptor Size */
    0x03,/* CY_U3P_USB_STRING_DESCR    Device Descriptor Type */
    'N',0x00,
    'I',0x00,
    ' ',0x00,
    'U',0x00,
    'S',0x00,
    'R',0x00,
    'P',0x00,
    '-',0x00,
    '2',0x00,
    '9',0x00,
    '0',0x00,
    '0',0x00
};

/* NI-USRP 2901 Product String Descriptor */
const unsigned char common_niusrp_2901_product_desc[] =
{
    0x1A,                           /* Descriptor Size */
    0x03,/* CY_U3P_USB_STRING_DESCR    Device Descriptor Type */
    'N',0x00,
    'I',0x00,
    ' ',0x00,
    'U',0x00,
    'S',0x00,
    'R',0x00,
    'P',0x00,
    '-',0x00,
    '2',0x00,
    '9',0x00,
    '0',0x00,
    '1',0x00
};

/* Unknown Product String Descriptor */
const unsigned char common_unknown_desc[] =
{
    0x10,                           /* Descriptor Size */
    0x03,/* CY_U3P_USB_STRING_DESCR    Device Descriptor Type */
    'U',0x00,
    'n',0x00,
    'k',0x00,
    'n',0x00,
    'o',0x00,
    'w',0x00,
    'n',0x00
};

unsigned char common_dev_serial_desc[] =
{
    0x14,                           /* Descriptor Size */
    0x03,/* CY_U3P_USB_STRING_DESCR    Device Descriptor Type */
    '0', 0x00,
    '0', 0x00,
    '0', 0x00,
    '0', 0x00,
    '0', 0x00,
    '0', 0x00,
    '0', 0x00,
    '0', 0x00,
    '0', 0x00
};
