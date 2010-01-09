//
// Copyright 2010 Ettus Research LLC
//

#ifndef INCLUDED_USRP_ADDR_HPP
#define INCLUDED_USRP_ADDR_HPP

#include <string>
#include <iostream>
#include <netinet/ether.h>
#include <arpa/inet.h>

namespace usrp{

    /*!
    * Wrapper for an ethernet mac address.
    * Provides conversion between string and binary formats.
    */
    struct mac_addr_t{
        struct ether_addr d_mac_addr;
        mac_addr_t(const std::string &str = "00:00:00:00:00:00");
        std::string to_string(void) const;
    };

    /*!
    * Wrapper for an ipv4 address.
    * Provides conversion between string and binary formats.
    */
    struct ip_addr_t{
        struct in_addr d_ip_addr;
        ip_addr_t(const std::string &str = "0.0.0.0");
        std::string to_string(void) const;
    };

    /*!
    * Possible usrp mboard interface types.
    */
    enum usrp_addr_type_t{
        USRP_ADDR_TYPE_AUTO,
        USRP_ADDR_TYPE_VIRTUAL,
        USRP_ADDR_TYPE_USB,
        USRP_ADDR_TYPE_ETH,
        USRP_ADDR_TYPE_UDP,
        USRP_ADDR_TYPE_GPMC
    };

    /*!
    * Structure to hold properties that identify a usrp mboard.
    */
    struct usrp_addr_t{
        usrp_addr_type_t type;
        struct{
            size_t num_rx_dsps;
            size_t num_tx_dsps;
            size_t num_dboards;
        } virtual_args;
        struct{
            uint16_t vendor_id;
            uint16_t product_id;
        } usb_args;
        struct{
            std::string ifc;
            mac_addr_t mac_addr;
        } eth_args;
        struct{
            ip_addr_t ip_addr;
        } udp_args;
        struct{
            //TODO unknown for now
        } gpmc_args;

        /*!
         * \brief Convert a usrp usrp_addr_t into a string representation
         */
        std::string to_string(void) const;

        /*!
         * \brief Default constructor to initialize the usrp_addr_t struct
         */
        usrp_addr_t(usrp_addr_type_t usrp_addr_type = USRP_ADDR_TYPE_AUTO);
    };

} //namespace usrp

//ability to use types with stream operators
std::ostream& operator<<(std::ostream &os, const usrp::usrp_addr_t &x);
std::ostream& operator<<(std::ostream &os, const usrp::mac_addr_t &x);
std::ostream& operator<<(std::ostream &os, const usrp::ip_addr_t &x);

#endif /* INCLUDED_USRP_ADDR_HPP */
