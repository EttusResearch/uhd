//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_CSV_HPP
#define INCLUDED_UHD_UTILS_CSV_HPP

#include <uhd/config.hpp>
#include <vector>
#include <string>
#include <istream>

namespace uhd{ namespace csv{
    typedef std::vector<std::string> row_type;
    typedef std::vector<row_type> rows_type;

    //! Convert an input stream to csv rows.
    UHD_API rows_type to_rows(std::istream &input);

}} //namespace uhd::csv

#endif /* INCLUDED_UHD_UTILS_CSV_HPP */
