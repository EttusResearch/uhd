//
// Copyright 2010,2014,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/safe_main.hpp>
#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <fstream>
#include "usrp1_eeprom.h"
#include "b100_eeprom.h"
#ifdef UHD_PLATFORM_LINUX
#include <unistd.h> // syscall constants
#include <fcntl.h> // O_NONBLOCK
#include <sys/syscall.h>
#include <cerrno>
#include <cstring> // for std::strerror
#endif //UHD_PLATFORM_LINUX

const std::string FX2_VENDOR_ID("0x04b4");
const std::string FX2_PRODUCT_ID("0x8613");

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    std::string type;
    std::string image;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("image", po::value<std::string>(), "BIN image file; if not specified, use built-in image")
        ("vid", po::value<std::string>(), "VID of device to program")
        ("pid", po::value<std::string>(), "PID of device to program")
        ("type", po::value<std::string>(&type), "device type (usrp1 or b100, required if using built-in image)")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("USRP EEPROM initialization %s") % desc << std::endl;
        return EXIT_FAILURE;
    }

#ifdef UHD_PLATFORM_LINUX
    //can't find an uninitialized usrp with this mystery usbtest in the way...
    std::string module("usbtest");
    std::ifstream modules("/proc/modules");
    bool module_found = false;
    std::string module_line;
    while(std::getline(modules, module_line) && (!module_found)) {
        module_found = boost::starts_with(module_line, module);
    }
    if(module_found) {
        std::cout << boost::format("Found the '%s' module. Unloading it.\n" ) % module;
        int fail = syscall(__NR_delete_module, module.c_str(), O_NONBLOCK);
        if(fail)
            std::cerr << ( boost::format("Removing the '%s' module failed with error '%s'.\n") % module % std::strerror(errno) );
    }
#endif //UHD_PLATFORM_LINUX

    //load the options into the address
    uhd::device_addr_t device_addr;
    device_addr["type"] = type;
    if(vm.count("vid") or vm.count("pid")) {
        if(not (vm.count("vid") and vm.count("pid") and vm.count("type"))) {
            std::cerr << "ERROR: Must specify vid, pid, and type if specifying any of the two former args" << std::endl;
        } else {
            device_addr["vid"] = vm["vid"].as<std::string>();
            device_addr["pid"] = vm["pid"].as<std::string>();
            device_addr["type"] = vm["type"].as<std::string>();
        }
    } else {
        device_addr["vid"] = FX2_VENDOR_ID;
        device_addr["pid"] = FX2_PRODUCT_ID;
    }
    if(vm.count("image")) {
      //if specified, use external image file
      image = vm["image"].as<std::string>();
    } else {
      //if not specified, use built-ins; requires user to define type
      size_t image_len;
      unsigned const char* image_data;

      if(!vm.count("type")) {
        std::cerr << boost::format("ERROR: Image file not specified and type of device not given. Cannot use built-in images.\n");
        return EXIT_FAILURE;
      }

      std::cout << boost::format("Using built-in image for \"%s\".\n") % type;

      if(vm["type"].as<std::string>() == "usrp1") {
        image_len = usrp1_eeprom_bin_len;
        image_data = usrp1_eeprom_bin;
      } else if(vm["type"].as<std::string>() == "b100") {
        image_len = b100_eeprom_bin_len;
        image_data = b100_eeprom_bin;
      } else {
        std::cerr << boost::format("ERROR: Unsupported device type \"%s\" specified and no EEPROM image file given.\n") % type;
        return EXIT_FAILURE;
      }

      //get temporary file name, and write image to that.
      image = boost::filesystem::unique_path().string();
      std::ofstream tmp_image(image, std::ofstream::binary);
      tmp_image.write((const char*)image_data, image_len);
      tmp_image.close();
    }

    //find and create a control transport to do the writing.

    uhd::device_addrs_t found_addrs = uhd::device::find(device_addr, uhd::device::USRP);

    if (found_addrs.size() == 0){
        std::cerr << "No USRP devices found" << std::endl;
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < found_addrs.size(); i++){
        std::cout << "Writing EEPROM data..." << std::endl;
        //uhd::device_addrs_t devs = uhd::device::find(found_addrs[i]);
        uhd::device::sptr dev = uhd::device::make(found_addrs[i], uhd::device::USRP);
        uhd::property_tree::sptr tree = dev->get_tree();
        tree->access<std::string>("/mboards/0/load_eeprom").set(image);
    }

    //delete temporary image file if we created one
    if(!vm.count("image")) {
      boost::filesystem::remove(image);
    }

    std::cout << "Power-cycle the usrp for the changes to take effect." << std::endl;
    return EXIT_SUCCESS;
}
