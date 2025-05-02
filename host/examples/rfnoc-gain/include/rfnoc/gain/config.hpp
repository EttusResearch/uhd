//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>

// See uhd/config.hpp for more information on these macros
#ifdef RFNOC_GAIN_STATIC_LIB
#    define RFNOC_GAIN_API
#    define RFNOC_GAIN_API_HEADER
#else
#    ifdef RFNOC_GAIN_DLL_EXPORTS
#        define RFNOC_GAIN_API        UHD_EXPORT
#        define RFNOC_GAIN_API_HEADER UHD_EXPORT_HEADER
#    else
#        define RFNOC_GAIN_API        UHD_IMPORT
#        define RFNOC_GAIN_API_HEADER UHD_IMPORT_HEADER
#    endif // RFNOC_GAIN_DLL_EXPORTS
#endif // RFNOC_GAIN_STATIC_LIB
