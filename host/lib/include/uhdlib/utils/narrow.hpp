//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// This narrowing code is inspired by the guidelines support library by
// Microsoft: https://github.com/Microsoft/GSL/
//
// The C++ guidelines are published here:
// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md
//
// There are some minor details between this implementation and the Microsoft
// implementation, but they are similar (the main difference is the exception
// type thrown).
// The original code was published under the following license:
//
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// [End of GSL license]
///////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_UHDLIB_UTILS_NARROW_HPP
#define INCLUDED_UHDLIB_UTILS_NARROW_HPP

#include <uhd/exception.hpp>
#include <utility>

#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4127) // conditional expression is constant
#endif                          // _MSC_VER

namespace uhd {

/*! Static typecast which expresses intent of narrowing
 *
 * Use this for any conversion that narrows, e.g.:
 *
 * \code{.cpp}
 * uint16_t x = peek16();
 * uint8_t y = narrow_cast<uint8_t>(x);
 * \endcode
 */
template <class T, class U>
inline constexpr T narrow_cast(U&& u) noexcept
{
    return static_cast<T>(std::forward<U>(u));
}

/*! Like narrow_cast, but will throw an exception on failure.
 *
 * Call this if you're not sure if the cast will work as expected (e.g. when
 * narrowing user input).
 *
 * \throws uhd::narrowing_error on failure
 */
template <class T, class U>
inline T narrow(U u)
{
    T t = narrow_cast<T>(u);
    if (static_cast<U>(t) != u) {
        throw narrowing_error("");
    }
    if (!std::integral_constant<bool, std::is_signed<T>::value == std::is_signed<U>::value>::value
            && ((t < T{}) != (u < U{}))) {
        throw narrowing_error("");
    }
    return t;
}

} /* namespace uhd */

#if defined(_MSC_VER)
#    pragma warning(pop)
#endif // _MSC_VER

#endif /* INCLUDED_UHDLIB_UTILS_NARROW_HPP */
