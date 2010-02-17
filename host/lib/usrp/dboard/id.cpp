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

#include <uhd/usrp/dboard/id.hpp>
#include <boost/format.hpp>
#include <uhd/dict.hpp>

using namespace uhd::usrp::dboard;

std::ostream& operator<<(std::ostream &os, const dboard_id_t &id){
    //map the dboard ids to string representations
    uhd::dict<dboard_id_t, std::string> id_to_str;
    id_to_str[ID_NONE]     = "none";
    id_to_str[ID_BASIC_TX] = "basic tx";
    id_to_str[ID_BASIC_RX] = "basic rx";

    //get the string representation
    if (id_to_str.has_key(id)){
        os << id_to_str[id];
    }
    else{
        os << boost::format("dboard id %u") % unsigned(id);
    }
    return os;
}
