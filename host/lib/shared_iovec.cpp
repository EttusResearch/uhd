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

#include <uhd/shared_iovec.hpp>

uhd::shared_iovec::shared_iovec(size_t len_){
    _shared_array = boost::shared_array<uint8_t>(new uint8_t[len_]);
    base = _shared_array.get();
    len = len_;
}

uhd::shared_iovec::~shared_iovec(void){
    /* NOP */
}
