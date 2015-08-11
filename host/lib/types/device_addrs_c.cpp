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

#include <uhd/types/device_addrs.h>

uhd_error uhd_device_addrs_make(
    uhd_device_addrs_handle *h
){
    UHD_SAFE_C(
        (*h) = new uhd_device_addrs_t;
    )
}

uhd_error uhd_device_addrs_free(
    uhd_device_addrs_handle *h
){
    UHD_SAFE_C(
        delete (*h);
        (*h) = NULL;
    )
}

uhd_error uhd_device_addrs_push_back(
    uhd_device_addrs_handle h,
    const char* value
){
    UHD_SAFE_C_SAVE_ERROR(h,
        h->device_addrs_cpp.push_back(uhd::device_addr_t(value));
    )
}

uhd_error uhd_device_addrs_at(
    uhd_device_addrs_handle h,
    size_t index,
    char* value_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        memset(value_out, '\0', strbuffer_len);

        std::string value_cpp = h->device_addrs_cpp.at(index).to_string();
        strncpy(value_out, value_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_device_addrs_size(
    uhd_device_addrs_handle h,
    size_t *size_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *size_out = h->device_addrs_cpp.size();
    )
}

uhd_error uhd_device_addrs_last_error(
    uhd_device_addrs_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}
