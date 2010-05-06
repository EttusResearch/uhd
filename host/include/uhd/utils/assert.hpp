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

#ifndef INCLUDED_UHD_UTILS_ASSERT_HPP
#define INCLUDED_UHD_UTILS_ASSERT_HPP

#include <uhd/config.hpp>
#include <uhd/utils/exception.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <string>

namespace uhd{

    //! The exception to throw when assertions fail
    struct UHD_API assert_error : std::runtime_error{
        assert_error(const std::string &what);
    };

    //! Throw an assert error with throw-site information
    #define UHD_ASSERT_THROW(_x) if (not (_x)) \
        throw uhd::assert_error(UHD_THROW_SITE_INFO("assertion failed: " + std::string(#_x)))

    /*!
     * Check that an element is found in a container.
     * If not, throw a meaningful assertion error.
     * The "what" in the error will show what is
     * being set and a list of known good values.
     *
     * \param range a list of possible settings
     * \param value an element that may be in the list
     * \param what a description of what is being set
     * \throw assertion_error when elem not in list
     */
    template<typename T, typename Range> void assert_has(
        const Range &range,
        const T &value,
        const std::string &what = "unknown"
    ){
        if (std::has(range, value)) return;
        std::string possible_values = "";
        size_t i = 0;
        BOOST_FOREACH(const T &v, range){
            if (i++ > 0) possible_values += ", ";
            possible_values += boost::lexical_cast<std::string>(v);
        }
        throw uhd::assert_error(str(boost::format(
                "assertion failed:\n"
                "  %s is not a valid %s.\n"
                "  possible values are: [%s].\n"
            )
            % boost::lexical_cast<std::string>(value)
            % what % possible_values
        ));
    }

}//namespace uhd

#endif /* INCLUDED_UHD_UTILS_ASSERT_HPP */
