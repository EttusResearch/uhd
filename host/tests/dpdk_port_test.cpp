//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/transport/dpdk/common.hpp>
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

int main(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    int status = 0;
    std::string args;
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "UHD-DPDK args")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    auto dpdk_args = uhd::device_addr_t(args);
    auto ctx = uhd::transport::dpdk::dpdk_ctx::get();
    ctx->init(args);

    uhd::transport::dpdk::dpdk_port* port = ctx->get_port(0);
    std::cout << "Port 0 MTU: " << port->get_mtu() << std::endl;
    status = ctx->get_port_link_status(0);
    std::cout << "Port 0 Link up: " << status << std::endl;
    ctx.reset();
    return 0;
}
