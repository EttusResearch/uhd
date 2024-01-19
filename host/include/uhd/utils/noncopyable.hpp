//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#ifdef UHD_AVOID_BOOST

namespace uhd {

/*! Non-copyable class
 *
 * This is a re-implementation of boost::noncopyable using C++11 features.
 * Deriving a class from this one will disallow it being assigned or copied:
 *
 * ~~~~{.cpp}
 * #include <uhdlib/utils/noncopyable.hpp>
 *
 * class C : uhd::noncopyable {};
 *
 * C c1;
 * C c2(c1); // Won't work
 * C c3;
 * c3 = c1; // Won't work
 * ~~~~
 */
class noncopyable
{
public:
    noncopyable()  = default;
    ~noncopyable() = default;

    noncopyable(const noncopyable&)            = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

} /* namespace uhd */

#else

#    include <boost/core/noncopyable.hpp>
namespace uhd {
typedef boost::noncopyable noncopyable;
}

#endif
