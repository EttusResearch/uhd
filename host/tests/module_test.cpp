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

#include <uhd/utils/static.hpp>
#include <iostream>

UHD_STATIC_BLOCK(module_test){
    std::cout << "---------------------------------------" << std::endl;
    std::cout << "-- Good news, everyone!" << std::endl;
    std::cout << "-- The test module has been loaded." << std::endl;
    std::cout << "---------------------------------------" << std::endl;
}
