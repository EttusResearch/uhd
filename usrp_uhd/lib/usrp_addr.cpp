//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp_addr.hpp>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <stdexcept>

//----------------------- u2 mac addr wrapper ------------------------//
usrp::mac_addr_t::mac_addr_t(const std::string &str){
    //ether_aton_r(str.c_str(), &d_mac_addr);
    bool good = false;
    char p[6] = {0x00, 0x50, 0xC2, 0x85, 0x30, 0x00}; // Matt's IAB

    switch (str.size()){
    case 5:
      good = sscanf(str.c_str(), "%hhx:%hhx", &p[4], &p[5]) == 2;
      break;
    case 17:
      good = sscanf(str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &p[0], &p[1], &p[2], &p[3], &p[4], &p[5]) == 6;
      break;
    }

    if (not good) throw std::runtime_error("Invalid mac address: " + str);
    memcpy(&d_mac_addr, p, sizeof(d_mac_addr));
}

std::string usrp::mac_addr_t::to_string(void) const{
    char addr_buf[128];
    //ether_ntoa_r(&d_mac_addr, addr_buf);
    const uint8_t *p = reinterpret_cast<const uint8_t *>(&d_mac_addr);
    sprintf(addr_buf, "%02x:%02x:%02x:%02x:%02x:%02x",
        p[0], p[1], p[2], p[3], p[4], p[5]);
    return std::string(addr_buf);
}

std::ostream& operator<<(std::ostream &os, const usrp::mac_addr_t &x){
    os << x.to_string();
    return os;
}

//----------------------- u2 ipv4 wrapper ----------------------------//
usrp::ip_addr_t::ip_addr_t(const std::string &str){
    int ret = inet_pton(AF_INET, str.c_str(), &d_ip_addr);
    if (ret == 0) throw std::runtime_error("Invalid ip address: " + str);
}

std::string usrp::ip_addr_t::to_string(void) const{
    char addr_buf[128];
    inet_ntop(AF_INET, &d_ip_addr, addr_buf, INET_ADDRSTRLEN);
    return std::string(addr_buf);
}

std::ostream& operator<<(std::ostream &os, const usrp::ip_addr_t &x){
    os << x.to_string();
    return os;
}

//----------------------- usrp usrp_addr_t wrapper -------------------------//
usrp::usrp_addr_t::usrp_addr_t(usrp_addr_type_t usrp_addr_type){
    type = usrp_addr_type;
    virtual_args.num_rx_dsps = 0;
    virtual_args.num_tx_dsps = 0;
    virtual_args.num_dboards = 0;
    usb_args.vendor_id = 0xffff;
    usb_args.product_id = 0xffff;
    eth_args.ifc = "eth0";
    eth_args.mac_addr = mac_addr_t("ff:ff:ff:ff:ff:ff");
    udp_args.ip_addr = ip_addr_t("255.255.255.255");
}

std::string usrp::usrp_addr_t::to_string(void) const{
    std::stringstream out;
    out << "USRP Type: ";
    switch(type){
    case USRP_ADDR_TYPE_AUTO:
        out << "Automatic" << std::endl;
        break;
    case USRP_ADDR_TYPE_VIRTUAL:
        out << "Virtual" << std::endl;
        out << "Num RX DSPs: " << virtual_args.num_rx_dsps << std::endl;
        out << "Num TX DSPs: " << virtual_args.num_rx_dsps << std::endl;
        out << "Num dboards: " << virtual_args.num_dboards << std::endl;
        break;
    case USRP_ADDR_TYPE_USB:
        out << "USB Port" << std::endl;
        out << "Vendor ID: 0x" << std::hex << usb_args.vendor_id << std::endl;
        out << "Product ID: 0x" << std::hex << usb_args.product_id << std::endl;
        break;
    case USRP_ADDR_TYPE_ETH:
        out << "Raw Ethernet" << std::endl;
        out << "Interface: " << eth_args.ifc << std::endl;
        out << "MAC Addr: " << eth_args.mac_addr << std::endl;
        break;
    case USRP_ADDR_TYPE_UDP:
        out << "UDP Socket" << std::endl;
        out << "IP Addr: " << udp_args.ip_addr << std::endl;
        break;
    case USRP_ADDR_TYPE_GPMC:
        out << "GPMC" << std::endl;
        break;
    default:
        out << "Unknown" << std::endl;
    }
    out << std::endl;
    return out.str();
}

std::ostream& operator<<(std::ostream &os, const usrp::usrp_addr_t &x)
{
  os << x.to_string();
  return os;
}
