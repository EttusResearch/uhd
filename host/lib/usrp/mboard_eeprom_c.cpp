//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/mboard_eeprom.h>

#include <uhd/exception.hpp>

#include <string.h>

uhd_error uhd_mboard_eeprom_make(
    uhd_mboard_eeprom_handle* h
){
    UHD_SAFE_C(
        *h = new uhd_mboard_eeprom_t;
    )
}

uhd_error uhd_mboard_eeprom_free(
    uhd_mboard_eeprom_handle* h
){
    UHD_SAFE_C(
        delete *h;
        *h = NULL;
    )
}

uhd_error uhd_mboard_eeprom_get_value(
    uhd_mboard_eeprom_handle h,
    const char* key,
    char* value_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string value_cpp = h->mboard_eeprom_cpp.get(key);
        strncpy(value_out, value_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_mboard_eeprom_set_value(
    uhd_mboard_eeprom_handle h,
    const char* key,
    const char* value
){
    UHD_SAFE_C_SAVE_ERROR(h,
        h->mboard_eeprom_cpp[key] = value;
    )
}

uhd_error uhd_mboard_eeprom_last_error(
    uhd_mboard_eeprom_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}
