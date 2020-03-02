//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/version.h>
#include <uhd/version.hpp>
#include <string.h>

uhd_error uhd_get_abi_string(char* abi_string_out, size_t buffer_len)
{
    UHD_SAFE_C(const std::string cpp_abi_string = uhd::get_abi_string();

               memset(abi_string_out, 0, buffer_len);
               strncpy(abi_string_out, cpp_abi_string.c_str(), buffer_len);)
}

uhd_error uhd_get_version_string(char* version_out, size_t buffer_len)
{
    UHD_SAFE_C(const std::string cpp_version = uhd::get_version_string();

               memset(version_out, 0, buffer_len);
               strncpy(version_out, cpp_version.c_str(), buffer_len);)
}
