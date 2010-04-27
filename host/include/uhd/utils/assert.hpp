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
#include <uhd/utils/algorithm.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <boost/exception/info.hpp>
#include <stdexcept>
#include <string>

namespace uhd{

    //! The exception to throw when assertions fail
    struct UHD_API assert_error : virtual std::exception, virtual boost::exception{};

    //! The assertion info, the code that failed
    typedef boost::error_info<struct tag_assert_info, std::string> assert_info;

    //! Throw an assert error with throw-site information
    #define UHD_ASSERT_THROW(_x) if (not (_x)) \
        BOOST_THROW_EXCEPTION(uhd::assert_error() << uhd::assert_info(#_x))

    /*!
     * Check that an element is found in a container.
     * If not, throw a meaningful assertion error.
     * The "what" in the error will show what is
     * being set and a list of known good values.
     *
     * \param iterable a list of possible settings
     * \param elem an element that may be in the list
     * \param what a description of what is being set
     * \throw assertion_error when elem not in list
     */
    template<class T, class Iterable> void assert_has(
        const Iterable &iterable,
        const T &elem,
        const std::string &what = "unknown"
    ){
        if (std::has(iterable, elem)) return;
        std::string possible_values = "";
        size_t i = 0;
        BOOST_FOREACH(const T &e, iterable){
            if (i++ > 0) possible_values += ", ";
            possible_values += boost::lexical_cast<std::string>(e);
        }
        boost::throw_exception(uhd::assert_error() << assert_info(str(boost::format(
                "Error: %s is not a valid %s. "
                "Possible values are: [%s]."
            )
            % boost::lexical_cast<std::string>(elem)
            % what % possible_values
        )));
    }

}//namespace uhd

#endif /* INCLUDED_UHD_UTILS_ASSERT_HPP */
