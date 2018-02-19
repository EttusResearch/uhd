//
// Copyright 2015-2017 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <csignal>
#include <cstdlib>
#include <iostream>

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <uhd/config.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/program_options.hpp>

namespace fs = boost::filesystem;
namespace po = boost::program_options;

static std::string device_type = "";
static int num_ctrl_c = 0;

/*
 * If the user presses Ctrl+C, warn them that they may corrupt their device.
 * If they press it again, provide instructions on restoring the device
 * (if applicable) and exit.
 */
void sigint_handler(int){
    num_ctrl_c++;
    if(num_ctrl_c == 1){
        std::cout << std::endl
                  << "Are you sure you want to abort? If you do, your device will likely" << std::endl
                  << "be in an unstable or unusable state." << std::endl
                  << "Press Ctrl+C again to abort." << std::endl << std::endl;
    }
    else{
        std::cout << std::endl << uhd::image_loader::get_recovery_instructions(device_type) << std::endl;
        exit(EXIT_FAILURE);
    }
}

int UHD_SAFE_MAIN(int argc, char *argv[]){

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>()->default_value(""), "Device args, optional loader args")
        ("fw-path", po::value<std::string>()->default_value(""), "Firmware path (uses default if none specified)")
        ("fpga-path", po::value<std::string>()->default_value(""), "FPGA path (uses default if none specified)")
        ("out-path", po::value<std::string>()->default_value(""), "Output path/filename of the downloaded FPGA .bit file")
        ("no-fw", "Don't burn firmware")
        ("no-fpga", "Don't Burn FPGA")
        ("download", "Download an image to a bit/bin file")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Help message
    if (vm.count("help")){
        std::cout << "UHD Image Loader" << std::endl
                  << std::endl
                  << "Load firmware and/or FPGA images onto an Ettus Research device." << std::endl
                  << std::endl
                  << desc << std::endl;
        return EXIT_FAILURE;
    }

    // Convert user options
    uhd::image_loader::image_loader_args_t image_loader_args;
    image_loader_args.args          = vm["args"].as<std::string>();
    image_loader_args.load_firmware = (vm.count("no-fw")    == 0);
    image_loader_args.load_fpga     = (vm.count("no-fpga")  == 0);
    image_loader_args.download      = (vm.count("download") != 0);
    image_loader_args.firmware_path = vm["fw-path"].as<std::string>();
    image_loader_args.fpga_path     = vm["fpga-path"].as<std::string>();
    image_loader_args.out_path      = vm["out-path"].as<std::string>();

    // Force user to specify a device
    if(not image_loader_args.args.has_key("type")){
        throw uhd::runtime_error("You must specify a device type.");
    }

    // Clean up paths, if given
    if(image_loader_args.firmware_path != ""){
        #ifndef UHD_PLATFORM_WIN32
        if(image_loader_args.firmware_path.find("~") == 0){
            image_loader_args.firmware_path.replace(0,1,getenv("HOME"));
        }
        #endif /* UHD_PLATFORM_WIN32 */
        image_loader_args.firmware_path = fs::absolute(image_loader_args.firmware_path).string();
    }
    if(image_loader_args.fpga_path != ""){
        #ifndef UHD_PLATFORM_WIN32
        if(image_loader_args.fpga_path.find("~") == 0){
            image_loader_args.fpga_path.replace(0,1,getenv("HOME"));
        }
        #endif /* UHD_PLATFORM_WIN32 */
        image_loader_args.fpga_path = fs::absolute(image_loader_args.fpga_path).string();
    }
    if(image_loader_args.out_path != ""){
        #ifndef UHD_PLATFORM_WIN32
        if(image_loader_args.out_path.find("~") == 0){
            image_loader_args.out_path.replace(0,1,getenv("HOME"));
        }
        #endif /* UHD_PLATFORM_WIN32 */
        image_loader_args.out_path = fs::absolute(image_loader_args.out_path).string();
    }

    // Detect which type of device we're working with
    device_type = image_loader_args.args.get("type","");

    std::signal(SIGINT, &sigint_handler);
    if(not uhd::image_loader::load(image_loader_args)){
        std::cerr << "No applicable UHD devices found" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
