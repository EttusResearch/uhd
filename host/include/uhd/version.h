//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.h>
#include <uhd/error.h>

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Get the ABI compatibility string for this build of the library
UHD_API uhd_error uhd_get_abi_string(char* abi_string_out, size_t buffer_len);

//! Get the version string (dotted version number + build info)
UHD_API uhd_error uhd_get_version_string(char* version_out, size_t buffer_len);

#ifdef __cplusplus
}
#endif
