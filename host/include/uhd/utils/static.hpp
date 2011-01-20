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
