//
// Copyright 2012-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>

namespace po = boost::program_options;

/***********************************************************************
 * Find USRP N2XX with specified IP address and return type
 **********************************************************************/
void print_image_loader_warning(const std::string &fw_path,
                                const std::string &fpga_path,
                                const po::variables_map &vm){

    // Newline + indent
    #ifdef UHD_PLATFORM_WIN32
    const std::string nl = " ^\n    ";
    #else
    const std::string nl = " \\\n    ";
    #endif

    std::string uhd_image_loader = str(boost::format("uhd_image_loader --args=\"type=usrp2,addr=%s")
                                       % vm["addr"].as<std::string>());
    if(vm.count("auto-reboot") > 0)
        uhd_image_loader += ",reset";
    if(vm.count("overwrite-safe") > 0)
        uhd_image_loader += ",overwrite-safe";
    if(vm.count("dont-check-rev") > 0)
        uhd_image_loader += ",dont-check-rev";

    uhd_image_loader += "\"";

    if(vm.count("no-fw") == 0){
        uhd_image_loader += str(boost::format("%s--fw-path=\"%s\"")
                                % nl % fw_path);
    }
    else{
        uhd_image_loader += str(boost::format("%s--no-fw")
                                % nl);
    }

    if(vm.count("no-fpga") == 0){
        uhd_image_loader += str(boost::format("%s--fpga-path=\"%s\"")
                                % nl % fpga_path);
    }
    else{
        uhd_image_loader += str(boost::format("%s--no-fpga")
                                % nl);
    }

    std::cout << "************************************************************************************************" << std::endl
              << "ERROR: This utility has been removed in this version of UHD. Use this command:" << std::endl
              << std::endl
              << uhd_image_loader << std::endl
              << std::endl
              << "************************************************************************************************" << std::endl
              << std::endl;
}

int main(int argc, char *argv[])
{
    //Establish user options
    std::string fw_path;
    std::string ip_addr;
    std::string fpga_path;

    po::options_description desc("Allowed options:");
    desc.add_options()
        ("help", "Display this help message.")
        ("addr", po::value<std::string>(&ip_addr)->default_value("192.168.10.2"), "Specify an IP address.")
        ("fw", po::value<std::string>(&fw_path), "Specify a filepath for a custom firmware image.")
        ("fpga", po::value<std::string>(&fpga_path), "Specify a filepath for a custom FPGA image.")
        ("no-fw", "Do not burn a firmware image.")
        ("no-fpga", "Do not burn an FPGA image.")
        ("overwrite-safe", "Overwrite safe images (not recommended).")
        ("dont-check-rev", "Don't verify images are for correct model before burning.")
        ("auto-reboot", "Automatically reboot N2XX without prompting.")
        ("list", "List available N2XX USRP devices.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    print_image_loader_warning(fw_path, fpga_path, vm);

    return EXIT_FAILURE;
}
