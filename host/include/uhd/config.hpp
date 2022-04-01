//
// Copyright 2010-2011,2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/config.hpp>
#include <boost/version.hpp>

#ifdef BOOST_MSVC
// suppress warnings
//# pragma warning(push)
//# pragma warning(disable: 4511) // copy constructor can't not be generated
//# pragma warning(disable: 4512) // assignment operator can't not be generated
//# pragma warning(disable: 4100) // unreferenced formal parameter
//# pragma warning(disable: 4996) // <symbol> was declared deprecated
#    pragma warning(disable : 4355) // 'this' : used in base member initializer list
//# pragma warning(disable: 4706) // assignment within conditional expression
#    pragma warning(disable : 4251) // class 'A<T>' needs to have dll-interface to be used
                                    // by clients of class 'B'
//# pragma warning(disable: 4127) // conditional expression is constant
//# pragma warning(disable: 4290) // C++ exception specification ignored except to ...
//# pragma warning(disable: 4180) // qualifier applied to function type has no meaning;
// ignored
#    pragma warning(disable : 4275) // non dll-interface class ... used as base for
                                    // dll-interface class ...
//# pragma warning(disable: 4267) // 'var' : conversion from 'size_t' to 'type', possible
// loss of data # pragma warning(disable: 4511) // 'class' : copy constructor could not be
// generated
#    pragma warning(disable : 4250) // 'class' : inherits 'method' via dominance
#    pragma warning( \
        disable : 4200) // nonstandard extension used : zero-sized array in struct/union

// define logical operators
#    include <ciso646>

// define ssize_t
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#    include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif /* _SSIZE_T_DEFINED */

#endif // BOOST_MSVC

// define cross platform attribute macros
#if defined(BOOST_MSVC)
#    define UHD_EXPORT __declspec(dllexport)
#    define UHD_IMPORT __declspec(dllimport)
#    define UHD_EXPORT_HEADER
#    define UHD_IMPORT_HEADER
#    define UHD_INLINE __forceinline
#    define UHD_FORCE_INLINE __forceinline
#    define UHD_DEPRECATED __declspec(deprecated)
#    define UHD_ALIGNED(x) __declspec(align(x))
#    define UHD_UNUSED(x) x
#    define UHD_FALLTHROUGH
#    define UHD_FUNCTION __FUNCTION__
#    define UHD_PRETTY_FUNCTION __FUNCSIG__
#elif defined(__MINGW32__)
#    define UHD_EXPORT __declspec(dllexport)
#    define UHD_IMPORT __declspec(dllimport)
#    define UHD_EXPORT_HEADER
#    define UHD_IMPORT_HEADER
#    define UHD_INLINE inline
#    define UHD_FORCE_INLINE inline
#    define UHD_DEPRECATED __declspec(deprecated)
#    define UHD_ALIGNED(x) __declspec(align(x))
#    define UHD_UNUSED(x) x __attribute__((unused))
#    define UHD_FALLTHROUGH
#    define UHD_FUNCTION __func__
#    define UHD_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(__clang__)
#    define UHD_EXPORT __attribute__((visibility("default")))
#    define UHD_IMPORT __attribute__((visibility("default")))
#    define UHD_EXPORT_HEADER __attribute__((visibility("default")))
#    define UHD_IMPORT_HEADER __attribute__((visibility("default")))
#    define UHD_INLINE inline __attribute__((always_inline))
#    define UHD_FORCE_INLINE inline __attribute__((always_inline))
#    define UHD_DEPRECATED __attribute__((deprecated))
#    define UHD_ALIGNED(x) __attribute__((aligned(x)))
#    define UHD_UNUSED(x) x __attribute__((unused))
#    if __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 8)
#        define UHD_FALLTHROUGH [[clang::fallthrough]];
#    else
#        define UHD_FALLTHROUGH
#    endif
#    define UHD_FUNCTION __func__
#    define UHD_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(__GNUG__) && __GNUG__ >= 4
#    define UHD_EXPORT __attribute__((visibility("default")))
#    define UHD_IMPORT __attribute__((visibility("default")))
#    define UHD_EXPORT_HEADER __attribute__((visibility("default")))
#    define UHD_IMPORT_HEADER __attribute__((visibility("default")))
#    define UHD_INLINE inline __attribute__((always_inline))
#    define UHD_FORCE_INLINE inline __attribute__((always_inline))
#    define UHD_DEPRECATED __attribute__((deprecated))
#    define UHD_ALIGNED(x) __attribute__((aligned(x)))
#    define UHD_UNUSED(x) x __attribute__((unused))
#    if __GNUG__ >= 7
#        define UHD_FALLTHROUGH __attribute__((fallthrough));
#    else
#        define UHD_FALLTHROUGH
#    endif
#    define UHD_FUNCTION __func__
#    define UHD_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#    define UHD_EXPORT
#    define UHD_IMPORT
#    define UHD_EXPORT_HEADER
#    define UHD_IMPORT_HEADER
#    define UHD_INLINE inline
#    define UHD_FORCE_INLINE inline
#    define UHD_DEPRECATED
#    define UHD_ALIGNED(x)
#    define UHD_UNUSED(x) x
#    define UHD_FALLTHROUGH
#    define UHD_FUNCTION __func__
#    define UHD_PRETTY_FUNCTION __func__
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
#    define UHD_API
#    define UHD_API_HEADER
#else
#    ifdef UHD_DLL_EXPORTS
#        define UHD_API UHD_EXPORT
#        define UHD_API_HEADER UHD_EXPORT_HEADER
#    else
#        define UHD_API UHD_IMPORT
#        define UHD_API_HEADER UHD_IMPORT_HEADER
#    endif // UHD_DLL_EXPORTS
#endif // UHD_STATIC_LIB

// Platform defines for conditional parts of headers:
// Taken from boost/config/select_platform_config.hpp,
// however, we define macros, not strings for platforms.
#if (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GLIBC__)) \
    && !defined(_CRAYC) && !defined(__FreeBSD_kernel__) && !defined(__GNU__)
#    define UHD_PLATFORM_LINUX
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#    define UHD_PLATFORM_WIN32
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#    define UHD_PLATFORM_MACOS
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(__FreeBSD_kernel__)
#    define UHD_PLATFORM_BSD
#endif

// Define 'stringize' preprocessor macros. The stringize macro, XSTR, takes
// variable arguments so that it can deal with strings that contain commas.
// There are two different versions because MSVC handles this syntax a bit
// differently than other compilers.
#if defined(BOOST_MSVC)
#    define XSTR(x, ...) #    x
#else
#    define XSTR(x...) #    x
#endif

#define STR(x) XSTR(x)
