//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_UHD_UTILS_EXCEPTION_HPP
#define INCLUDED_UHD_UTILS_EXCEPTION_HPP

#include <uhd/config.hpp>
#include <boost/current_function.hpp>
#include <stdexcept>
#include <string>

/*!
 * Create a formated string with throw-site information.
 * Fills in the function name, file name, and line number.
 * \param what the std::exeption message
 * \return the formatted exception message
 */
#define UHD_THROW_SITE_INFO(what) std::string( \
    std::string(what) + "\n" + \
    "  in " + std::string(BOOST_CURRENT_FUNCTION) + "\n" + \
    "  at " + std::string(__FILE__) + ":" + BOOST_STRINGIZE(__LINE__) + "\n" \
)

/*!
 * Throws an invalid code path exception with throw-site information.
 * Use this macro in places that code execution is not supposed to go.
 */
#define UHD_THROW_INVALID_CODE_PATH() \
    throw std::runtime_error(UHD_THROW_SITE_INFO("invalid code path"))

#endif /* INCLUDED_UHD_UTILS_EXCEPTION_HPP */
