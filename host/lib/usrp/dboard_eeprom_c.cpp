//
// Copyright 2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/usrp/dboard_eeprom.h>
#include <uhd/error.h>

#include <boost/lexical_cast.hpp>

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

uhd_error uhd_dboard_eeprom_get_revision(
    uhd_dboard_eeprom_handle h,
    int* revision_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *revision_out = boost::lexical_cast<int>(h->dboard_eeprom_cpp.revision);
    )
}

uhd_error uhd_dboard_eeprom_set_revision(
    uhd_dboard_eeprom_handle h,
    int revision
){
    UHD_SAFE_C_SAVE_ERROR(h,
        h->dboard_eeprom_cpp.revision = boost::lexical_cast<std::string>(revision);
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
