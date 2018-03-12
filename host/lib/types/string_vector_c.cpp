//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
