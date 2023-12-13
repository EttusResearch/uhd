//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/features/adc_self_calibration_iface.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/version.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <sstream>

namespace po = boost::program_options;
using namespace uhd;

/****************************************************************************
 * main
 ***************************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("version", "print the version string and exit")
        ("args", po::value<std::string>()->default_value(""), "device address args")
    ;
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << "UHD ADC self calibration " << desc << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("version")) {
        std::cout << uhd::get_version_string() << std::endl;
        return EXIT_SUCCESS;
    }
    const uhd::device_addr_t args  = vm["args"].as<std::string>();
    rfnoc::rfnoc_graph::sptr graph = rfnoc::rfnoc_graph::make(args);

    size_t num_calibrations = 0;
    for (auto radio_id : graph->find_blocks("Radio")) {
        auto radio_blk = graph->get_block<uhd::rfnoc::radio_control>(radio_id);
        if (radio_blk->has_feature<uhd::features::adc_self_calibration_iface>()) {
            auto& feature =
                radio_blk->get_feature<uhd::features::adc_self_calibration_iface>();

            const std::string ch_list = args.get("cal_ch_list", "");

            // Run it on all (enabled) channels
            const size_t num_channels = radio_blk->get_num_output_ports();
            for (size_t i = 0; i < num_channels; i++) {
                auto abs_ch =
                    radio_blk->get_block_id().get_block_count() * num_channels + i;
                if (ch_list.size() == 0
                    or ch_list.find(std::to_string(abs_ch)) != std::string::npos) {
                    std::cout << "Calibrating on channel " << i << " of " << radio_id
                              << "..." << std::endl;
                    feature.run(i, vm["args"].as<std::string>());
                    std::cout << "Finished!" << std::endl;
                    num_calibrations++;
                }
            }
        }
    }
    if (num_calibrations > 0) {
        std::cout << "Calibrated " << num_calibrations << " channels" << std::endl;
    } else {
        std::cerr << "WARNING: Did not find any channels to calibrate!" << std::endl;
    }

    return EXIT_SUCCESS;
}
