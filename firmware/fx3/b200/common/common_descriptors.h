//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef _COMMON_DESCRIPTORS_H
#define _COMMON_DESCRIPTORS_H

/* This file contains all descriptors that are common between the bootloader
   and firmware code.  Non-const descriptors are modified at runtime based on
   information read from the EEPROM. */

/* Standard Device Descriptor for USB 2.0 */
extern unsigned char common_usb2_dev_desc[];

/* Standard Device Descriptor for USB 3.0 */
extern unsigned char common_usb3_dev_desc[];

/* Standard Device Qualifier Descriptor */
extern const unsigned char common_dev_qual_desc[];

/* Standard Binary Device Object Store Descriptor */
extern const unsigned char common_usb_bos_desc[];

/* Standard Language ID String Descriptor */
extern const unsigned char common_string_lang_id_desc[];

/* Ettus Manufacturer String Descriptor */
extern const unsigned char common_ettus_manufacturer_desc[];

/* NI Manufacturer String Descriptor */
extern const unsigned char common_ni_manufacturer_desc[];

/* Ettus Product String Descriptor */
extern const unsigned char common_b200_product_desc[];

/* NI-USRP 2900 Product String Descriptor */
extern const unsigned char common_niusrp_2900_product_desc[];

/* NI-USRP 2901 Product String Descriptor */
extern const unsigned char common_niusrp_2901_product_desc[];

/* Unknown Product String Descriptor */
extern const unsigned char common_unknown_desc[];

/* Common Serial String Descriptor */
extern unsigned char common_dev_serial_desc[];

#endif