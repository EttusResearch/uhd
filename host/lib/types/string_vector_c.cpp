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

#include <uhd/types/string_vector.h>

#include <string.h>

uhd_error uhd_string_vector_make(
    uhd_string_vector_handle *h
){
    UHD_SAFE_C(
        (*h) = new uhd_string_vector_t;
    )
}

uhd_error uhd_string_vector_free(
    uhd_string_vector_handle *h
){
    UHD_SAFE_C(
        delete (*h);
        (*h) = NULL;
    )
}

uhd_error uhd_string_vector_push_back(
    uhd_string_vector_handle *h,
    const char* value
){
    UHD_SAFE_C_SAVE_ERROR((*h),
        (*h)->string_vector_cpp.push_back(value);
    )
}

uhd_error uhd_string_vector_at(
    uhd_string_vector_handle h,
    size_t index,
    char* value_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        memset(value_out, '\0', strbuffer_len);

        const std::string& value_cpp = h->string_vector_cpp.at(index);
        strncpy(value_out, value_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_string_vector_size(
    uhd_string_vector_handle h,
    size_t *size_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *size_out = h->string_vector_cpp.size();
    )
}

uhd_error uhd_string_vector_last_error(
    uhd_string_vector_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}
