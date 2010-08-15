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

#include <uhd/utils/warning.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <vector>

using namespace uhd;

void uhd::print_warning(const std::string &msg){
    //print the warning message
    std::cerr << std::endl << "Warning:" << std::endl;
    BOOST_FOREACH(const std::string &line, std::split_string(msg, "\n")){
        std::cerr << "    " << line << std::endl;
    }
}
