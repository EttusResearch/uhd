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

#ifndef INCLUDED_UHD_UTILS_ASSERT_HAS_HPP
#define INCLUDED_UHD_UTILS_ASSERT_HAS_HPP

#include <uhd/config.hpp>
#include <string>

namespace uhd{

    /*!
     * Check that an element is found in a container.
     * If not, throw a meaningful assertion error.
     * The "what" in the error will show what is
     * being set and a list of known good values.
     *
     * \param range a list of possible settings
     * \param value an element that may be in the list
     * \param what a description of what the value is
     * \throw assertion_error when elem not in list
     */
    template<typename T, typename Range> void assert_has(
        const Range &range,
        const T &value,
        const std::string &what = "unknown"
    );

}//namespace uhd

#include <uhd/utils/assert_has.ipp>

#endif /* INCLUDED_UHD_UTILS_ASSERT_HAS_HPP */
