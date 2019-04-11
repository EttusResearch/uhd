//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "common_helpers.h"
#include "common_const.h"
#include "common_descriptors.h"

#define UNREAD 0xFFFF

/* Check the rev, magic value, and eeprom compatibility field to determine if
   our eeprom map is up-to-date with what is written on the device */
int eeprom_is_readable(eeprom_read_t read_fn) {
    /* Only check this once, then store that info */
    static int is_readable = UNREAD;

    if (is_readable != UNREAD) {
        return is_readable;
    }

    int rev = get_rev(read_fn);

    if (rev == EEPROM_REV_UNRECOGNIZED)
    {
        is_readable = 0;
        return is_readable;
    }


    /* If rev is 0, there's no further checks available */
    if (rev == 0)
    {
        is_readable = 1;
        return is_readable;
    }

    /* For rev 1, check the EEPROM magic value and compat number */

    if (rev == 1) {
        {
            uint8_t buffer[EEPROM_MAGIC_LENGTH];
            read_fn(EEPROM_REV1_MAGIC_ADDR, buffer, EEPROM_MAGIC_LENGTH);
            if ((buffer[1] << 8 | buffer[0]) != EEPROM_EXPECTED_MAGIC) {
                is_readable = 0;
                return is_readable;
            }
        }
        {
            uint8_t buffer[EEPROM_COMPAT_LENGTH];
            read_fn(EEPROM_REV1_COMPAT_ADDR, buffer, EEPROM_COMPAT_LENGTH);
            if ((buffer[1] << 8 | buffer[0]) > EEPROM_EXPECTED_COMPAT) {
                is_readable = 0;
                return is_readable;
            }
        }
        is_readable = 1;
        return is_readable;
    }

    /* some other unrecognized rev */
    is_readable = 0;
    return is_readable;
}

/* Read the EEPROM layout revision number from EEPROM using the function
   specified */
int get_rev(eeprom_read_t read_fn)
{
    /* Only check the rev once, then store that info */
    static int rev = UNREAD;

    if (rev != UNREAD) {
        return rev;
    }

    uint8_t buffer[EEPROM_SIGNATURE_LENGTH];
    read_fn(EEPROM_SIGNATURE_ADDR, buffer, EEPROM_SIGNATURE_LENGTH);

    const uint32_t signature = buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8
                               | buffer[0];
    if (signature == EEPROM_REV0_SIGNATURE) {
        rev = 0;
    } else if (signature == EEPROM_REV1_OR_GREATER_SIGNATURE) {
        rev = 1;
    } else {
        rev = EEPROM_REV_UNRECOGNIZED;
    }
    return rev;
}



/* Read the vendor ID from EEPROM using the function specified*/
uint16_t get_vid(eeprom_read_t read_fn)
{
    static uint16_t vid = UNREAD;

    if (vid != UNREAD) {
        return vid;
    }

    if (!eeprom_is_readable(read_fn)) {
        vid = VID_CYPRESS;
        return vid;
    }

    // eeprom_is_readable guarantees rev is valid
    int rev = get_rev(read_fn);

    const uint16_t addr = (rev == 0) ? EEPROM_REV0_VID_ADDR : EEPROM_REV1_VID_ADDR;

    uint8_t buffer[EEPROM_VID_LENGTH];
    read_fn(addr, buffer, EEPROM_VID_LENGTH);
    vid = buffer[1] << 8 | buffer[0];
    return vid;
}

/* Read the product ID from EEPROM using the function specified*/
uint16_t get_pid(eeprom_read_t read_fn)
{
    static uint16_t pid = UNREAD;

    if (pid != UNREAD) {
        return pid;
    }

    if (!eeprom_is_readable(read_fn)) {
        pid = PID_CYPRESS_DEFAULT;
        return pid;
    }

    // eeprom_is_readable guarantees rev is valid
    int rev = get_rev(read_fn);

    const uint16_t addr = (rev == 0) ? EEPROM_REV0_PID_ADDR : EEPROM_REV1_PID_ADDR;

    uint8_t buffer[EEPROM_PID_LENGTH];
    read_fn(addr, buffer, EEPROM_PID_LENGTH);
    pid = buffer[1] << 8 | buffer[0];
    return pid;
}

/* Read the vendor ID from EEPROM using the function specified */
const uint8_t* get_serial_string_descriptor(eeprom_read_t read_fn)
{
    static uint8_t* serial_string_descriptor = 0;
    if (serial_string_descriptor) {
        return serial_string_descriptor;
    }

    /* All code paths will eventually return this value, but some will modify
       it beforehand */
    serial_string_descriptor = common_dev_serial_desc;

    if (!eeprom_is_readable(read_fn)) {
        return serial_string_descriptor;
    }

    // eeprom_is_readable guarantees rev is valid
    int rev = get_rev(read_fn);

    const uint16_t addr = (rev == 0) ? EEPROM_REV0_SERIAL_ADDR : EEPROM_REV1_SERIAL_ADDR;

    uint8_t buffer[EEPROM_SERIAL_LENGTH];
    read_fn(addr, buffer, EEPROM_SERIAL_LENGTH);
    int i;
    for (i = 0; i < EEPROM_SERIAL_LENGTH; ++i) {
        common_dev_serial_desc[2 + i * 2] = buffer[i];
    }
    return serial_string_descriptor;
}

/* Return the string descriptor based on the VID given */
const uint8_t* get_manufacturer_string_descriptor(uint16_t vid)
{
    if (vid == VID_ETTUS_RESEARCH) {
        return common_ettus_manufacturer_desc;
    } else if (vid == VID_NATIONAL_INSTRUMENTS) {
        return common_ni_manufacturer_desc;
    } else {
        return common_unknown_desc;
    }
}

/* Return the string descriptor based on the PID given */
const uint8_t* get_product_string_descriptor(uint16_t pid)
{
    if (pid == PID_ETTUS_B200_B210) {
        return common_b200_product_desc;
    } else if (pid == PID_NI_USRP_2900) {
        return common_niusrp_2900_product_desc;
    } else if (pid == PID_NI_USRP_2901) {
        return common_niusrp_2901_product_desc;
    } else {
        return common_unknown_desc;
    }
}
