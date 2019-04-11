//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef _B200_USB_DESCRIPTORS_H
#define _B200_USB_DESCRIPTORS_H

#include "cyu3externcstart.h"

/* Descriptor definitions for USB enumerations. */
extern uint8_t b200_usb_fs_config_desc[];
extern uint8_t b200_usb_hs_config_desc[];
extern uint8_t b200_usb_ss_config_desc[];
extern uint8_t b200_dev_serial_desc[];

#include "cyu3externcend.h"

#endif /* _B200_USB_DESCRIPTORS_H */
