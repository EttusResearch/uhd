//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp> //for split
#include <iostream>
#include <sstream>

namespace po = boost::program_options;

namespace {
    std::string make_border(const std::string &text)
    {
        std::stringstream ss;
        ss << "  _____________________________________________________\n";
        ss << " /" << std::endl;
        std::vector<std::string> lines;
        boost::split(lines, text, boost::is_any_of("\n"));
        while (lines.back().empty()) {
            lines.pop_back(); //strip trailing newlines
        }
        if (not lines.empty()) {
            lines[0] = "    " + lines[0]; //indent the title line
        }
        for (const std::string& line : lines)
        {
            ss << "|   " << line << std::endl;
        }
        return ss.str();
    }
}

using namespace uhd::usrp;

std::string db_sensors_string(
        const std::string& tx_rx,
        multi_usrp::sptr usrp,
        const size_t mb_idx
) {
    std::stringstream ss;

    ss << tx_rx << " Sensors: \n" << std::endl;
    const size_t num_chans = (tx_rx == "RX")
        ?  usrp->get_rx_subdev_spec(mb_idx).size()
        :  usrp->get_tx_subdev_spec(mb_idx).size()
    ;

    for (size_t chan_idx = 0; chan_idx < num_chans; chan_idx++) {
        ss << "Chan " << chan_idx << ": " << std::endl;
        const auto sensors = (tx_rx == "RX")
            ? usrp->get_rx_sensor_names(chan_idx)
            : usrp->get_tx_sensor_names(chan_idx);
        for (const auto& sensor : sensors) {
            const auto sensor_value = (tx_rx == "RX")
                ? usrp->get_rx_sensor(sensor, chan_idx)
                : usrp->get_tx_sensor(sensor, chan_idx)
            ;
            ss << "* " << sensor_value.to_pp_string() << std::endl;
        }
        ss << std::endl;
    }

    return ss.str();
}

std::string mboard_sensors_string(multi_usrp::sptr usrp, const size_t mb_idx)
{
    std::stringstream ss;
    ss << "Sensors for motherboard " << mb_idx << ": \n" << std::endl;
    const auto mboard_sensors = usrp->get_mboard_sensor_names(mb_idx);
    for (const auto& mboard_sensor : mboard_sensors) {
        const auto sensor_value =
            usrp->get_mboard_sensor(mboard_sensor, mb_idx);
        ss << "* " << sensor_value.to_pp_string() << std::endl;
    }
    ss << make_border(db_sensors_string("RX", usrp, mb_idx)) << std::endl;
    ss << make_border(db_sensors_string("TX", usrp, mb_idx)) << std::endl;

    return ss.str();
}

int UHD_SAFE_MAIN(int argc, char *argv[]){

    // Variables to be set by command line options
    std::string usrp_args;

    // Set up program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Display this help message")
        ("args", po::value<std::string>(&usrp_args), "USRP device arguments")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Print the help message
    if (vm.count("help")){
        std::cout << std::endl << "Print sensor values"
                  << std::endl << std::endl;
        std::cout << "This example shows how to query sensors from"
                  << "a USRP device.\n" << std::endl;
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    // Create a Multi-USRP device
    std::cout << "\nCreating the USRP device with: " << usrp_args << std::endl;
    multi_usrp::sptr usrp = multi_usrp::make(usrp_args);

    const size_t num_mboards = usrp->get_num_mboards();
    std::cout << "Device contains " << num_mboards
              << " motherboard(s)." << std::endl << std::endl;

    for (size_t mboard_idx = 0; mboard_idx < num_mboards; mboard_idx++) {
        std::cout << make_border(mboard_sensors_string(usrp, mboard_idx))
                  << std::endl;
    }

    return EXIT_SUCCESS;
}
