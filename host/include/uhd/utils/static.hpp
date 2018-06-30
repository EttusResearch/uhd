//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_STATIC_HPP
#define INCLUDED_UHD_UTILS_STATIC_HPP

#include <uhd/config.hpp>

/*!
 * Defines a function that implements the "construct on first use" idiom
 * \param _t the type definition for the instance
 * \param _x the name of the defined function
 * \return a reference to the lazy instance
 */
#define UHD_SINGLETON_FCN(_t, _x) static _t &_x(){static _t _x; return _x;}

/*!
 * Defines a static code block that will be called before main()
 * The static block will catch and print exceptions to std error.
 * \param _x the unique name of the fixture (unique per source)
 */
#define UHD_STATIC_BLOCK(_x) \
    void _x(void); \
    static _uhd_static_fixture _x##_fixture(&_x, #_x); \
    void _x(void)

//! Helper for static block, constructor calls function
struct UHD_API _uhd_static_fixture{
    _uhd_static_fixture(void (*)(void), const char *);
};

#endif /* INCLUDED_UHD_UTILS_STATIC_HPP */
