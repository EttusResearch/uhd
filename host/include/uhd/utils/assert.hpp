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

#include <uhd/utils/algorithm.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>
#include <stdexcept>

namespace uhd{

    class assert_error : public std::logic_error{
    public:
        explicit assert_error(const std::string& what_arg) : logic_error(what_arg){
            /* NOP */
        }
    };

    #define ASSERT_THROW(_x) if (not (_x)) { \
        throw uhd::assert_error(str(boost::format( \
            "Assertion Failed:\n  %s:%d\n  %s\n  ---> %s <---" \
        ) % __FILE__ % __LINE__ % BOOST_CURRENT_FUNCTION % std::string(#_x))); \
    }

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
        BOOST_FOREACH(T e, iterable){
            if (e != iterable.begin()[0]) possible_values += ", ";
            possible_values += boost::lexical_cast<std::string>(e);
        }
        throw uhd::assert_error(str(boost::format(
                "Error: %s is not a valid %s. "
                "Possible values are: [%s]."
            )
            % boost::lexical_cast<std::string>(elem)
            % what % possible_values
        ));
    }

}//namespace uhd

#endif /* INCLUDED_UHD_UTILS_ASSERT_HPP */
