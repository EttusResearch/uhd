//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/device_addr.hpp>
#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>
#include <boost/current_function.hpp>

#define THROW_ASSERT(expr) \
if (not (expr)){ \
    throw std::runtime_error(str( \
        boost::format("%s %s %s %s") \
        % #expr % BOOST_CURRENT_FUNCTION % __FILE__ % __LINE__ \
    )); \
}

int main(void){
    try{
        std::cout << "Testing mac addr:" << std::endl;
        const std::string mac_addr_str("00:01:23:45:67:89");
        usrp_uhd::mac_addr_t mac_addr(mac_addr_str);
        std::cout << "Input: " << mac_addr_str << std::endl;
        std::cout << "Output: " << mac_addr << std::endl;
        THROW_ASSERT(mac_addr.to_string() == mac_addr_str);

        std::cout << "Testing ip addr:" << std::endl;
        const std::string ip_addr_str("192.168.1.10");
        usrp_uhd::ip_addr_t ip_addr(ip_addr_str);
        std::cout << "Input: " << ip_addr_str << std::endl;
        std::cout << "Output: " << ip_addr << std::endl;
        THROW_ASSERT(ip_addr.to_string() == ip_addr_str);

        std::cout << "done" << std::endl;
    }catch(std::exception const& e){
        std::cerr << "Exception: " << e.what() << std::endl;
        return ~0;
    }
    return 0;
}
