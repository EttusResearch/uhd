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

#ifndef INCLUDED_UHD_UTILS_ASSERT_HAS_IPP
#define INCLUDED_UHD_UTILS_ASSERT_HAS_IPP

#include <uhd/utils/algorithm.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

namespace uhd{

    template<typename T, typename Range> UHD_INLINE void assert_has(
        const Range &range,
        const T &value,
        const std::string &what
    ){
        if (uhd::has(range, value)) return;
        std::string possible_values = "";
        size_t i = 0;
        BOOST_FOREACH(const T &v, range){
            if (i++ > 0) possible_values += ", ";
            possible_values += boost::lexical_cast<std::string>(v);
        }
        throw uhd::assertion_error(str(boost::format(
                "assertion failed:\n"
                "  %s is not a valid %s.\n"
                "  possible values are: [%s].\n"
            )
            % boost::lexical_cast<std::string>(value)
            % what % possible_values
        ));
    }

}//namespace uhd

#endif /* INCLUDED_UHD_UTILS_ASSERT_HAS_IPP */
