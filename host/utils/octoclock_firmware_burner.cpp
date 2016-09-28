//
// Copyright 2014-2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>

namespace po = boost::program_options;

void print_image_loader_warning(const std::string &fw_path, const po::variables_map &vm){
    // Newline + indent
    #ifdef UHD_PLATFORM_WIN32
    const std::string nl = " ^\n    ";
    #else
    const std::string nl = " \\\n    ";
    #endif

    std::string uhd_image_loader = str(boost::format("uhd_image_loader --args=\"type=octoclock,addr=%s\""
                                                     "%s --fw-path=%s")
                                       % vm["addr"].as<std::string>() % nl % fw_path);
    std::cout << "************************************************************************************************" << std::endl
              << "ERROR: This utility has been removed in this version of UHD. Use this command:" << std::endl
              << std::endl
              << uhd_image_loader << std::endl
              << std::endl
              << "************************************************************************************************" << std::endl
              << std::endl;
}

int main(int argc, const char *argv[])
{
    std::string ip_addr, firmware_path;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Display this help message.")
        ("addr", po::value<std::string>(&ip_addr)->default_value("addr=1.2.3.4"), "Specify an IP address.")
        ("fw-path", po::value<std::string>(&firmware_path)->default_value("path/to/firmware"), "Specify a custom firmware path.")
        ("list", "List all available OctoClock devices.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    print_image_loader_warning(firmware_path, vm);

    return EXIT_FAILURE;
}
