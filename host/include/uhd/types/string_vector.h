/*
 * Copyright 2015 Ettus Research
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_UHD_TYPES_STRING_VECTOR_H
#define INCLUDED_UHD_TYPES_STRING_VECTOR_H

#include <uhd/config.h>
#include <uhd/error.h>

#include <stdlib.h>

#ifdef __cplusplus
#include <string>
#include <vector>

struct uhd_string_vector_t {
    std::vector<std::string> string_vector_cpp;
    std::string last_error;
};

extern "C" {
#else
//! C-level read-only interface for interacting with a string vector
struct uhd_string_vector_t;
#endif

typedef struct uhd_string_vector_t uhd_string_vector_t;

typedef uhd_string_vector_t* uhd_string_vector_handle;

//! Instantiate a string_vector handle.
UHD_API uhd_error uhd_string_vector_make(
    uhd_string_vector_handle *h
);

//! Safely destroy a string_vector handle.
UHD_API uhd_error uhd_string_vector_free(
    uhd_string_vector_handle *h
);

//! Add a string to the list
UHD_API uhd_error uhd_string_vector_push_back(
    uhd_string_vector_handle *h,
    const char* value
);

//! Get the string at the given index
UHD_API uhd_error uhd_string_vector_at(
    uhd_string_vector_handle h,
    size_t index,
    char* value_out,
    size_t strbuffer_len
);

//! Get the number of strings in this list
UHD_API uhd_error uhd_string_vector_size(
    uhd_string_vector_handle h,
    size_t *size_out
);

//! Get the last error reported by the underlying object
UHD_API uhd_error uhd_string_vector_last_error(
    uhd_string_vector_handle h,
    char* error_out,
    size_t strbuffer_len
);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_UHD_TYPES_STRING_VECTOR_H */
