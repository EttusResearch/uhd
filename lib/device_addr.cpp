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

//----------------------- u2 mac addr wrapper ------------------------//
uhd::mac_addr_t::mac_addr_t(const std::string &mac_addr_str_){
    std::string mac_addr_str = (mac_addr_str_ == "")? "ff:ff:ff:ff:ff:ff" : mac_addr_str_;

    //ether_aton_r(str.c_str(), &mac_addr);
    uint8_t p[6] = {0x00, 0x50, 0xC2, 0x85, 0x30, 0x00}; // Matt's IAB

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
            p[i] = uint8_t(hex_num);
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
    const uint8_t *p = reinterpret_cast<const uint8_t *>(&mac_addr);
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
uhd::device_addr_t::device_addr_t(device_addr_type_t device_addr_type){
    type = device_addr_type;
    virtual_args.num_rx_dsps = 0;
    virtual_args.num_tx_dsps = 0;
    virtual_args.num_dboards = 0;
    usb_args.vendor_id = 0xffff;
    usb_args.product_id = 0xffff;
    eth_args.ifc = "eth0";
    eth_args.mac_addr = "";
    udp_args.addr = "";
    discovery_args.mboard_id = ~0;
}

std::string uhd::device_addr_t::to_string(void) const{
    std::ostringstream out;
    out << "USRP Type: ";
    switch(type){
    case DEVICE_ADDR_TYPE_AUTO:
        out << "Automatic" << std::endl;
        break;
    case DEVICE_ADDR_TYPE_VIRTUAL:
        out << "Virtual" << std::endl;
        out << "Num RX DSPs: " << virtual_args.num_rx_dsps << std::endl;
        out << "Num TX DSPs: " << virtual_args.num_rx_dsps << std::endl;
        out << "Num dboards: " << virtual_args.num_dboards << std::endl;
        break;
    case DEVICE_ADDR_TYPE_USB:
        out << "USB Port" << std::endl;
        out << "Vendor ID: 0x" << std::hex << usb_args.vendor_id << std::endl;
        out << "Product ID: 0x" << std::hex << usb_args.product_id << std::endl;
        break;
    case DEVICE_ADDR_TYPE_ETH:
        out << "Raw Ethernet" << std::endl;
        out << "Interface: " << eth_args.ifc << std::endl;
        out << "MAC Addr: " << eth_args.mac_addr << std::endl;
        break;
    case DEVICE_ADDR_TYPE_UDP:
        out << "UDP Socket" << std::endl;
        out << "Address: " << udp_args.addr << std::endl;
        break;
    case DEVICE_ADDR_TYPE_GPMC:
        out << "GPMC" << std::endl;
        break;
    default:
        out << "Unknown" << std::endl;
    }
    out << std::endl;
    return out.str();
}

std::ostream& operator<<(std::ostream &os, const uhd::device_addr_t &x)
{
  os << x.to_string();
  return os;
}
