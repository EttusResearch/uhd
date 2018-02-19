//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_ASSERT_HAS_IPP
#define INCLUDED_UHD_UTILS_ASSERT_HAS_IPP

#include <uhd/utils/algorithm.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

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
