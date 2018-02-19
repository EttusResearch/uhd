//
// Copyright 2013-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/format.hpp>

namespace po = boost::program_options;

void print_image_loader_warning(const std::string &fpga_path, const po::variables_map &vm)
{
    // Newline + indent
    #ifdef UHD_PLATFORM_WIN32
    const std::string nl = " ^\n    ";
    #else
    const std::string nl = " \\\n    ";
    #endif

    // Generate equivalent uhd_image_loader command
    std::string uhd_image_loader = "uhd_image_loader --args=\"type=x300";

    if(vm.count("addr") > 0){
        uhd_image_loader += str(boost::format(",addr=%s")
                                % vm["addr"].as<std::string>());
        if(vm.count("configure") > 0){
            uhd_image_loader += ",configure";
        }
        if(vm.count("verify") > 0){
            uhd_image_loader += ",verify";
        }
    }
    else{
        uhd_image_loader += str(boost::format(",resource=%s")
                                % vm["resource"].as<std::string>());

        /*
         * Since we have a default value, vm.count("rpc-port") will
         * always be > 0, so only add the option if a different port
         * is given.
         */
        if(vm["rpc-port"].as<std::string>() != "5444"){
            uhd_image_loader += str(boost::format(",rpc-port=%s")
                                    % vm["rpc-port"].as<std::string>());
        }
    }

    if(vm.count("type") > 0){
        uhd_image_loader += str(boost::format(",fpga=%s")
                                % vm["type"].as<std::string>());
    }

    uhd_image_loader += "\"";

    /*
     * The --type option overrides any given path, so only add an FPGA path
     * if there was no --type argument.
     */
    if(vm.count("type") == 0){
        uhd_image_loader += str(boost::format("%s--fpga-path=\"%s\"")
                                % nl % fpga_path);
    }

    std::cout << "************************************************************************************************" << std::endl
              << "ERROR: This utility has been removed in this version of UHD. Use this command:" << std::endl
              << std::endl
              << uhd_image_loader << std::endl
              << std::endl
              << "************************************************************************************************" << std::endl
              << std::endl;
}

int main(int argc, char *argv[]){
    std::string ip_addr, resource, fpga_path, image_type, rpc_port;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Display this help message.")
        ("addr", po::value<std::string>(&ip_addr)->default_value("1.2.3.4"), "Specify an IP address.")
        ("resource", po::value<std::string>(&resource), "Specify an NI-RIO resource.")
        ("rpc-port", po::value<std::string>(&rpc_port)->default_value("5444"), "Specify a port to communicate with the RPC server.")
        ("type", po::value<std::string>(&image_type)->default_value("HG"), "Specify an image type (1G, HG, XG), leave blank for current type.")
        ("fpga-path", po::value<std::string>(&fpga_path)->default_value("/path/to/fpga-image.bit"), "Specify an FPGA path (overrides --type option).")
        ("configure", "Initialize FPGA with image currently burned to flash (Ethernet only).")
        ("verify", "Verify data downloaded to flash (Ethernet only, download will take much longer)")
        ("list", "List all available X3x0 devices.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    print_image_loader_warning(fpga_path, vm);

    return EXIT_FAILURE;
}

