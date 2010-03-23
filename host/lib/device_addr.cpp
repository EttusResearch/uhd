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

#include <uhd/device_addr.hpp>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

//----------------------- u2 mac addr wrapper ------------------------//
uhd::mac_addr_t::mac_addr_t(const std::string &mac_addr_str_){
    std::string mac_addr_str = (mac_addr_str_ == "")? "ff:ff:ff:ff:ff:ff" : mac_addr_str_;

    //ether_aton_r(str.c_str(), &mac_addr);
    boost::uint8_t p[6] = {0x00, 0x50, 0xC2, 0x85, 0x30, 0x00}; // Matt's IAB

    try{
        //only allow patterns of xx:xx or xx:xx:xx:xx:xx:xx
        //the IAB above will fill in for the shorter pattern
        if (mac_addr_str.size() != 5 and mac_addr_str.size() != 17)
            throw std::runtime_error("expected exactly 5 or 17 characters");

        //split the mac addr hex string at the colons
        std::vector<std::string> hex_strs;
        boost::split(hex_strs, mac_addr_str, boost::is_any_of(":"));
        for (size_t i = 0; i < hex_strs.size(); i++){
            int hex_num;
            std::istringstream iss(hex_strs[i]);
            iss >> std::hex >> hex_num;
            p[i] = boost::uint8_t(hex_num);
        }

    }
    catch(std::exception const& e){
        throw std::runtime_error(str(
            boost::format("Invalid mac address: %s\n\t%s") % mac_addr_str % e.what()
        ));
    }

    memcpy(&mac_addr, p, sizeof(mac_addr));
}

std::string uhd::mac_addr_t::to_string(void) const{
    //ether_ntoa_r(&mac_addr, addr_buf);
    const boost::uint8_t *p = reinterpret_cast<const boost::uint8_t *>(&mac_addr);
    return str(
        boost::format("%02x:%02x:%02x:%02x:%02x:%02x")
        % int(p[0]) % int(p[1]) % int(p[2])
        % int(p[3]) % int(p[4]) % int(p[5])
    );
}

std::ostream& operator<<(std::ostream &os, const uhd::mac_addr_t &x){
    os << x.to_string();
    return os;
}

//----------------------- usrp device_addr_t wrapper -------------------------//
std::string uhd::device_addr::to_string(const uhd::device_addr_t &device_addr){
    std::stringstream ss;
    BOOST_FOREACH(std::string key, device_addr.get_keys()){
        ss << boost::format("%s: %s") % key % device_addr[key] << std::endl;
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream &os, const uhd::device_addr_t &device_addr){
    os << uhd::device_addr::to_string(device_addr);
    return os;
}
