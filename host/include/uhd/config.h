//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#ifdef _MSC_VER
// Bring in "and", "or", and "not"
#include <iso646.h>

// Define ssize_t
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
# include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif /* _SSIZE_T_DEFINED */

#endif /* _MSC_VER */

// Define cross-platform macros
#if defined(_MSC_VER)
    #define UHD_EXPORT         __declspec(dllexport)
    #define UHD_IMPORT         __declspec(dllimport)
    #define UHD_EXPORT_HEADER
    #define UHD_IMPORT_HEADER
    #define UHD_INLINE         __forceinline
    #define UHD_DEPRECATED     __declspec(deprecated)
    #define UHD_ALIGNED(x)     __declspec(align(x))
    #define UHD_UNUSED(x)      x
#elif defined(__MINGW32__)
    #define UHD_EXPORT         __declspec(dllexport)
    #define UHD_IMPORT         __declspec(dllimport)
    #define UHD_EXPORT_HEADER
    #define UHD_IMPORT_HEADER
    #define UHD_INLINE         inline
    #define UHD_DEPRECATED     __declspec(deprecated)
    #define UHD_ALIGNED(x)     __declspec(align(x))
    #define UHD_UNUSED(x)      x __attribute__((unused))
#elif defined(__clang__)
    #define UHD_EXPORT         __attribute__((visibility("default")))
    #define UHD_IMPORT         __attribute__((visibility("default")))
    #define UHD_EXPORT_HEADER  __attribute__((visibility("default")))
    #define UHD_IMPORT_HEADER  __attribute__((visibility("default")))
    #define UHD_INLINE         inline __attribute__((always_inline))
    #define UHD_DEPRECATED     __attribute__((deprecated))
    #define UHD_ALIGNED(x)     __attribute__((aligned(x)))
    #define UHD_UNUSED(x)      x __attribute__((unused))
#elif defined(__GNUC__) && (__GNUC__ >= 4)
    #define UHD_EXPORT         __attribute__((visibility("default")))
    #define UHD_IMPORT         __attribute__((visibility("default")))
    #define UHD_EXPORT_HEADER  __attribute__((visibility("default")))
    #define UHD_IMPORT_HEADER  __attribute__((visibility("default")))
    #define UHD_INLINE         inline __attribute__((always_inline))
    #define UHD_DEPRECATED     __attribute__((deprecated))
    #define UHD_ALIGNED(x)     __attribute__((aligned(x)))
    #define UHD_UNUSED(x)      x __attribute__((unused))
#else
    #define UHD_EXPORT
    #define UHD_IMPORT
    #define UHD_EXPORT_HEADER
    #define UHD_IMPORT_HEADER
    #define UHD_INLINE         inline
    #define UHD_DEPRECATED
    #define UHD_ALIGNED(x)
    #define UHD_UNUSED(x)      x
#endif

// Define API declaration macro
//
// UHD_API should be used for classes/structs that
// have a direct cpp implementations that get directly
// built into a so/dylib/dll.
//
// UHD_API_HEADER should be used for classes/structs
// that are implemented in header only like hpp/ipp.
#ifdef UHD_STATIC_LIB
    #define UHD_API
    #define UHD_API_HEADER
#else
    #ifdef UHD_DLL_EXPORTS
        #define UHD_API UHD_EXPORT
        #define UHD_API_HEADER UHD_EXPORT_HEADER
    #else
        #define UHD_API UHD_IMPORT
        #define UHD_API_HEADER UHD_IMPORT_HEADER
    #endif // UHD_DLL_EXPORTS
#endif // UHD_STATIC_LIB

// Platform defines for conditional code:
// Taken from boost/config/select_platform_config.hpp,
// However, we define macros, not strings, for platforms.
#if (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GLIBC__)) && !defined(_CRAYC) && !defined(__FreeBSD_kernel__) && !defined(__GNU__)
    #define UHD_PLATFORM_LINUX
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define UHD_PLATFORM_WIN32
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    #define UHD_PLATFORM_MACOS
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD_kernel__)
    #define UHD_PLATFORM_BSD
#endif
