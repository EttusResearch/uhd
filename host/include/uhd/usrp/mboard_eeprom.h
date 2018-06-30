//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_MBOARD_EEPROM_H
#define INCLUDED_UHD_USRP_MBOARD_EEPROM_H

#include <uhd/config.h>
#include <uhd/error.h>

#ifdef __cplusplus
#include <uhd/usrp/mboard_eeprom.hpp>
#include <string>

struct uhd_mboard_eeprom_t {
    uhd::usrp::mboard_eeprom_t mboard_eeprom_cpp;
    std::string last_error;
};

extern "C" {
#else
struct uhd_mboard_eeprom_t;
#endif

//! A C-level interface for interacting with a USRP motherboard's EEPROM
/*!
 * See uhd::usrp::mboard_eeprom_t for more details.
 *
 * NOTE: Using a handle before passing it into uhd_mboard_eeprom_make() will
 * result in undefined behavior.
 */
typedef struct uhd_mboard_eeprom_t* uhd_mboard_eeprom_handle;

//! Create a handle for working with a USRP motherboard EEPROM
UHD_API uhd_error uhd_mboard_eeprom_make(
    uhd_mboard_eeprom_handle* h
);

//! Free a USRP motherboard EEPROM handle
/*!
 * NOTE: Using a handle after passing it into this function will result in
 * a segmentation fault.
 */
UHD_API uhd_error uhd_mboard_eeprom_free(
    uhd_mboard_eeprom_handle* h
);

//! Get the value associated with the given EEPROM key
UHD_API uhd_error uhd_mboard_eeprom_get_value(
    uhd_mboard_eeprom_handle h,
    const char* key,
    char* value_out,
    size_t strbuffer_len
);

//! Set the value for the given EEPROM key
UHD_API uhd_error uhd_mboard_eeprom_set_value(
    uhd_mboard_eeprom_handle h,
    const char* key,
    const char* value
);

//! Get the last error recorded by the handle
UHD_API uhd_error uhd_mboard_eeprom_last_error(
    uhd_mboard_eeprom_handle h,
    char* error_out,
    size_t strbuffer_len
);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_UHD_USRP_MBOARD_EEPROM_H */
