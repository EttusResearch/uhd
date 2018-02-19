//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/dboard_eeprom.h>
#include <uhd/error.h>
#include <boost/format.hpp>

#include <string.h>

uhd_error uhd_dboard_eeprom_make(
    uhd_dboard_eeprom_handle* h
){
    UHD_SAFE_C(
        *h = new uhd_dboard_eeprom_t;
    )
}

uhd_error uhd_dboard_eeprom_free(
    uhd_dboard_eeprom_handle* h
){
    UHD_SAFE_C(
        delete *h;
        *h = NULL;
    )
}

uhd_error uhd_dboard_eeprom_get_id(
    uhd_dboard_eeprom_handle h,
    char* id_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string dboard_id_cpp = h->dboard_eeprom_cpp.id.to_string();
        strncpy(id_out, dboard_id_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_dboard_eeprom_set_id(
    uhd_dboard_eeprom_handle h,
    const char* id
){
    UHD_SAFE_C_SAVE_ERROR(h,
        h->dboard_eeprom_cpp.id = uhd::usrp::dboard_id_t::from_string(id);
    )
}

uhd_error uhd_dboard_eeprom_get_serial(
    uhd_dboard_eeprom_handle h,
    char* id_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string dboard_serial_cpp = h->dboard_eeprom_cpp.serial;
        strncpy(id_out, dboard_serial_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_dboard_eeprom_set_serial(
    uhd_dboard_eeprom_handle h,
    const char* serial
){
    UHD_SAFE_C_SAVE_ERROR(h,
        h->dboard_eeprom_cpp.serial = serial;
    )
}

//! Convert a string into an int. If that doesn't work, craft our own exception
// instead of using the Boost exception. We need to put this separate from the
// caller function because of macro expansion.
int _convert_rev_with_exception(const std::string &rev_str)
{
    try {
        return std::stoi(rev_str);
    } catch (const std::invalid_argument &) {
        throw uhd::lookup_error(str(
            boost::format("Error retrieving revision from string `%s`")
            % rev_str
        ));
    } catch (const std::out_of_range &) {
        throw uhd::lookup_error(str(
            boost::format("Error retrieving revision from string `%s`")
            % rev_str
        ));
    }
}

uhd_error uhd_dboard_eeprom_get_revision(
    uhd_dboard_eeprom_handle h,
    int* revision_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *revision_out = \
            _convert_rev_with_exception(h->dboard_eeprom_cpp.revision);
    )
}

uhd_error uhd_dboard_eeprom_set_revision(
    uhd_dboard_eeprom_handle h,
    int revision
){
    UHD_SAFE_C_SAVE_ERROR(h,
        h->dboard_eeprom_cpp.revision = std::to_string(revision);
    )
}

uhd_error uhd_dboard_eeprom_last_error(
    uhd_dboard_eeprom_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}
