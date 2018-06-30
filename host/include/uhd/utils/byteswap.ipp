//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_BYTESWAP_IPP
#define INCLUDED_UHD_UTILS_BYTESWAP_IPP

/***********************************************************************
 * Platform-specific implementation details for byteswap below:
 **********************************************************************/
#if defined(BOOST_MSVC) //http://msdn.microsoft.com/en-us/library/a3140177%28VS.80%29.aspx
    #include <cstdlib>

    UHD_INLINE uint16_t uhd::byteswap(uint16_t x){
        return _byteswap_ushort(x);
    }

    UHD_INLINE uint32_t uhd::byteswap(uint32_t x){
        return _byteswap_ulong(x);
    }

    UHD_INLINE uint64_t uhd::byteswap(uint64_t x){
        return _byteswap_uint64(x);
    }

#elif defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 3

    UHD_INLINE uint16_t uhd::byteswap(uint16_t x){
        return (x>>8) | (x<<8); //DNE return __builtin_bswap16(x);
    }

    UHD_INLINE uint32_t uhd::byteswap(uint32_t x){
        return __builtin_bswap32(x);
    }

    UHD_INLINE uint64_t uhd::byteswap(uint64_t x){
        return __builtin_bswap64(x);
    }

#elif defined(UHD_PLATFORM_MACOS)
    #include <libkern/OSByteOrder.h>

    UHD_INLINE uint16_t uhd::byteswap(uint16_t x){
        return OSSwapInt16(x);
    }

    UHD_INLINE uint32_t uhd::byteswap(uint32_t x){
        return OSSwapInt32(x);
    }

    UHD_INLINE uint64_t uhd::byteswap(uint64_t x){
        return OSSwapInt64(x);
    }

#elif defined(UHD_PLATFORM_LINUX)
    #include <byteswap.h>

    UHD_INLINE uint16_t uhd::byteswap(uint16_t x){
        return bswap_16(x);
    }

    UHD_INLINE uint32_t uhd::byteswap(uint32_t x){
        return bswap_32(x);
    }

    UHD_INLINE uint64_t uhd::byteswap(uint64_t x){
        return bswap_64(x);
    }

#else //http://www.koders.com/c/fidB93B34CD44F0ECF724F1A4EAE3854BA2FE692F59.aspx

    UHD_INLINE uint16_t uhd::byteswap(uint16_t x){
        return (x>>8) | (x<<8);
    }

    UHD_INLINE uint32_t uhd::byteswap(uint32_t x){
        return (uint32_t(uhd::byteswap(uint16_t(x&0xfffful)))<<16) | (uhd::byteswap(uint16_t(x>>16)));
    }

    UHD_INLINE uint64_t uhd::byteswap(uint64_t x){
        return (uint64_t(uhd::byteswap(uint32_t(x&0xffffffffull)))<<32) | (uhd::byteswap(uint32_t(x>>32)));
    }

#endif

/***********************************************************************
 * Define the templated network to/from host conversions
 **********************************************************************/
#include <boost/detail/endian.hpp>

namespace uhd {

template<typename T> UHD_INLINE T ntohx(T num){
    #ifdef BOOST_BIG_ENDIAN
        return num;
    #else
        return uhd::byteswap(num);
    #endif
}

template<typename T> UHD_INLINE T htonx(T num){
    #ifdef BOOST_BIG_ENDIAN
        return num;
    #else
        return uhd::byteswap(num);
    #endif
}

template<typename T> UHD_INLINE T wtohx(T num){
    #ifdef BOOST_BIG_ENDIAN
        return uhd::byteswap(num);
    #else
        return num;
    #endif
}

template<typename T> UHD_INLINE T htowx(T num){
    #ifdef BOOST_BIG_ENDIAN
        return uhd::byteswap(num);
    #else
        return num;
    #endif
}

} /* namespace uhd */

#endif /* INCLUDED_UHD_UTILS_BYTESWAP_IPP */
