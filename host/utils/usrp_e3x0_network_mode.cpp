//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../lib/usrp/e300/e300_network.hpp"
#include <uhd/device.hpp>
#include <uhd/exception.hpp>

#include <uhd/transport/if_addrs.hpp>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>

#include <iostream>

namespace po = boost::program_options;

static void check_network_ok(void)
{
    using namespace uhd::transport;
    using namespace boost::asio::ip;
    std::vector<if_addrs_t> addrs = get_if_addrs();

    if(addrs.size() == 1 and addrs.at(0).inet == address_v4::loopback().to_string())
        throw uhd::runtime_error(
            "No network address except for loopback found.\n"
            "Make sure your DHCP server is working or configure a static IP");
}

int main(int argc, char *argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("fpga", po::value<std::string>(), "fpga image to load")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD E3x0 Network Mode %s") % desc << std::endl;
        return EXIT_FAILURE;
    }
    uhd::device_addr_t args;
    if(vm.count("fpga")) {
        args["fpga"] = vm["fpga"].as<std::string>();
    }

    try {
        check_network_ok();
        uhd::usrp::e300::network_server::sptr server = uhd::usrp::e300::network_server::make(args);
        server->run();
    } catch (uhd::assertion_error &e) {
        std::cout << "This executable is supposed to run on the device, not on the host." << std::endl
                       << "Please refer to the manual section on operating your e3x0 device in network mode." << std::endl;
        return EXIT_FAILURE;
    } catch (uhd::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
