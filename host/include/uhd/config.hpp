//
// Copyright 2010-2011 Ettus Research LLC
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

#ifndef INCLUDED_UHD_CONFIG_HPP
#define INCLUDED_UHD_CONFIG_HPP

#include <boost/config.hpp>

#ifdef BOOST_MSVC
// suppress warnings
//# pragma warning(push)
//# pragma warning(disable: 4511) // copy constructor can't not be generated
//# pragma warning(disable: 4512) // assignment operator can't not be generated
//# pragma warning(disable: 4100) // unreferenced formal parameter
//# pragma warning(disable: 4996) // <symbol> was declared deprecated
# pragma warning(disable: 4355) // 'this' : used in base member initializer list
//# pragma warning(disable: 4706) // assignment within conditional expression
# pragma warning(disable: 4251) // class 'A<T>' needs to have dll-interface to be used by clients of class 'B'
//# pragma warning(disable: 4127) // conditional expression is constant
//# pragma warning(disable: 4290) // C++ exception specification ignored except to ...
//# pragma warning(disable: 4180) // qualifier applied to function type has no meaning; ignored
# pragma warning(disable: 4275) // non dll-interface class ... used as base for dll-interface class ...
//# pragma warning(disable: 4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data
//# pragma warning(disable: 4511) // 'class' : copy constructor could not be generated
//# pragma warning(disable: 4250) // 'class' : inherits 'method' via dominance
# pragma warning(disable: 4200) // nonstandard extension used : zero-sized array in struct/union

// define logical operators
#include <ciso646>

// define ssize_t
#include <cstddef>
typedef ptrdiff_t ssize_t;

#endif //BOOST_MSVC

//define cross platform attribute macros
#if defined(BOOST_MSVC)
    #define UHD_EXPORT         __declspec(dllexport)
    #define UHD_IMPORT         __declspec(dllimport)
    #define UHD_INLINE         __forceinline
    #define UHD_DEPRECATED     __declspec(deprecated)
    #define UHD_ALIGNED(x)     __declspec(align(x))
#elif defined(__GNUG__) && __GNUG__ >= 4
    #define UHD_EXPORT         __attribute__((visibility("default")))
    #define UHD_IMPORT         __attribute__((visibility("default")))
    #define UHD_INLINE         inline __attribute__((always_inline))
    #define UHD_DEPRECATED     __attribute__((deprecated))
    #define UHD_ALIGNED(x)     __attribute__((aligned(x)))
#else
    #define UHD_EXPORT
    #define UHD_IMPORT
    #define UHD_INLINE         inline
    #define UHD_DEPRECATED
    #define UHD_ALIGNED(x)
#endif

// Define API declaration macro
#ifdef UHD_DLL_EXPORTS
    #define UHD_API UHD_EXPORT
#else
    #define UHD_API UHD_IMPORT
#endif // UHD_DLL_EXPORTS

// Platform defines for conditional parts of headers:
// Taken from boost/config/select_platform_config.hpp,
// however, we define macros, not strings for platforms.
#if defined(linux) || defined(__linux) || defined(__linux__)
    #define UHD_PLATFORM_LINUX
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define UHD_PLATFORM_WIN32
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    #define UHD_PLATFORM_MACOS
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #define UHD_PLATFORM_BSD
#endif

#endif /* INCLUDED_UHD_CONFIG_HPP */
