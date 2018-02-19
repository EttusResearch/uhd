//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_DBOARD_EEPROM_H
#define INCLUDED_UHD_USRP_DBOARD_EEPROM_H

#include <uhd/config.h>
#include <uhd/error.h>

#ifdef __cplusplus
#include <uhd/usrp/dboard_eeprom.hpp>
#include <string>

struct uhd_dboard_eeprom_t {
    uhd::usrp::dboard_eeprom_t dboard_eeprom_cpp;
    std::string last_error;
};

extern "C" {
#else
struct uhd_dboard_eeprom_t;
#endif

//! A C-level interface for interacting with a daughterboard EEPROM
/*!
 * See uhd::usrp::dboard_eeprom_t for more details.
 *
 * NOTE: Using a handle before passing it into uhd_dboard_eeprom_make() will
 * result in undefined behavior.
 */
typedef struct uhd_dboard_eeprom_t* uhd_dboard_eeprom_handle;

//! Create handle for a USRP daughterboard EEPROM
UHD_API uhd_error uhd_dboard_eeprom_make(
    uhd_dboard_eeprom_handle* h
);

//! Safely destroy the given handle
/*!
 * NOTE: Using a handle after passing it into this function will result in
 * a segmentation fault.
 */
UHD_API uhd_error uhd_dboard_eeprom_free(
    uhd_dboard_eeprom_handle* h
);

//! Get the ID associated with the given daughterboard as a string hex representation
UHD_API uhd_error uhd_dboard_eeprom_get_id(
    uhd_dboard_eeprom_handle h,
    char* id_out,
    size_t strbuffer_len
);

//! Set the daughterboard ID using a string hex representation
UHD_API uhd_error uhd_dboard_eeprom_set_id(
    uhd_dboard_eeprom_handle h,
    const char* id
);

//! Get the daughterboard's serial
UHD_API uhd_error uhd_dboard_eeprom_get_serial(
    uhd_dboard_eeprom_handle h,
    char* serial_out,
    size_t strbuffer_len
);

//! Set the daughterboard's serial
UHD_API uhd_error uhd_dboard_eeprom_set_serial(
    uhd_dboard_eeprom_handle h,
    const char* serial
);

/*! Get the daughterboard's revision
 *
 * The revision doesn't always have to be present, in which case this function
 * will return an error.
 */
UHD_API uhd_error uhd_dboard_eeprom_get_revision(
    uhd_dboard_eeprom_handle h,
    int* revision_out
);

//! Set the daughterboard's revision
UHD_API uhd_error uhd_dboard_eeprom_set_revision(
    uhd_dboard_eeprom_handle h,
    int revision
);

//! Get the last error reported by the handle
UHD_API uhd_error uhd_dboard_eeprom_last_error(
    uhd_dboard_eeprom_handle h,
    char* error_out,
    size_t strbuffer_len
);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_UHD_USRP_DBOARD_EEPROM_H */
