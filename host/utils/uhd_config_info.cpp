//
// Copyright 2015-2016 National Instruments Corp.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/build_info.hpp>
#include <uhd/version.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_main.hpp>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char* argv[]) {
    // Program Options
    po::options_description desc("Allowed Options");
    desc.add_options()
        ("build-date",         "Print build date")
        ("c-compiler",         "Print C compiler")
        ("cxx-compiler",       "Print C++ compiler")
        ("c-flags",            "Print C compiler flags")
        ("cxx-flags",          "Print C++ compiler flags")
        ("enabled-components", "Print built-time enabled components")
        ("install-prefix",     "Print install prefix")
        ("boost-version",      "Print Boost version")
        ("libusb-version",     "Print libusb version")
        ("pkg-path",           "Print pkg path")
        ("images-dir",         "Print images dir")
        ("abi-version",        "Print ABI version string")
        ("print-all",          "Print everything")
        ("version",            "Print this UHD build's version")
        ("help",               "Print help message")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Print the help message
    if(vm.count("help") > 0) {
        std::cout << boost::format("UHD Config Info - %s") % desc << std::endl;
        return EXIT_FAILURE;
    }

    bool print_all = (vm.count("print-all") > 0);

    if(vm.count("version") > 0 or print_all) {
        std::cout << "UHD " << uhd::get_version_string() << std::endl;
    }
    if(vm.count("build-date") > 0 or print_all) {
        std::cout << "Build date: " << uhd::build_info::build_date() << std::endl;
    }
    if(vm.count("c-compiler") > 0 or print_all) {
        std::cout << "C compiler: " << uhd::build_info::c_compiler() << std::endl;
    }
    if(vm.count("cxx-compiler") > 0 or print_all) {
        std::cout << "C++ compiler: " << uhd::build_info::cxx_compiler() << std::endl;
    }
    if(vm.count("c-flags") > 0 or print_all) {
        std::cout << "C flags: " << uhd::build_info::c_flags() << std::endl;
    }
    if(vm.count("cxx-flags") > 0 or print_all) {
        std::cout << "C++ flags: " << uhd::build_info::cxx_flags() << std::endl;
    }
    if(vm.count("enabled-components") > 0 or print_all) {
        std::cout << "Enabled components: " << uhd::build_info::enabled_components() << std::endl;
    }
    if(vm.count("install-prefix") > 0 or print_all) {
        std::cout << "Install prefix: " << uhd::build_info::install_prefix() << std::endl;
    }
    if(vm.count("boost-version") > 0 or print_all) {
        std::cout << "Boost version: " << uhd::build_info::boost_version() << std::endl;
    }
    if(vm.count("libusb-version") > 0 or print_all) {
        std::string _libusb_version = uhd::build_info::libusb_version();
        std::cout << "Libusb version: " << (_libusb_version.empty() ? "N/A" : _libusb_version) << std::endl;
    }
    if(vm.count("pkg-path") > 0 or print_all) {
        std::cout << "Package path: " << uhd::get_pkg_path() << std::endl;
    }
    if(vm.count("images-dir") > 0 or print_all) {
        std::cout << "Images directory: " << uhd::get_images_dir("") << std::endl;
    }
    if(vm.count("abi-version") > 0 or print_all) {
        std::cout << "ABI version string: " << uhd::get_abi_string() << std::endl;
    }

    return EXIT_SUCCESS;
}
