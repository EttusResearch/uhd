//
// Copyright 2011 Ettus Research LLC
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
