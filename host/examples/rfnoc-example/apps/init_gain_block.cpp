//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Example application to show how to write applications that depend on both UHD
// and out-of-tree RFNoC modules.
//
// It will see if a USRP is runnging the gain block, if so, it will test to see
// if it can change the gain.

#include <uhd/exception.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/utils/safe_main.hpp>
#include <rfnoc/example/gain_block_control.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    std::string args;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "USRP device address args")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << "Init RFNoC gain block " << desc << std::endl;
        std::cout << std::endl
                  << "This application attempts to find a gain block in a USRP "
                     "and tries to peek/poke registers..\n"
                  << std::endl;
        return EXIT_SUCCESS;
    }

    // Create RFNoC graph object:
    auto graph = uhd::rfnoc::rfnoc_graph::make(args);

    // Verify we have a gain block:
    auto gain_blocks = graph->find_blocks<rfnoc::example::gain_block_control>("");
    if (gain_blocks.empty()) {
        std::cout << "No gain block found." << std::endl;
        return EXIT_FAILURE;
    }

    auto gain_block =
        graph->get_block<rfnoc::example::gain_block_control>(gain_blocks.front());
    if (!gain_block) {
        std::cout << "ERROR: Failed to extract block controller!" << std::endl;
        return EXIT_FAILURE;
    }
    constexpr uint32_t new_gain_value = 42;
    gain_block->set_gain_value(new_gain_value);
    const uint32_t gain_value_read = gain_block->get_gain_value();

    if (gain_value_read != new_gain_value) {
        std::cout << "ERROR: Readback of gain value not working! "
                  << "Expected: " << new_gain_value << " Read: " << gain_value_read
                  << std::endl;
        return EXIT_FAILURE;
    } else {
        std::cout << "Gain value read/write loopback successful!" << std::endl;
    }

    return EXIT_SUCCESS;
}
