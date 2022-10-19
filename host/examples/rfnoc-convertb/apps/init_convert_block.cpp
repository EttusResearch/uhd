//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

//
// It will see if a USRP is runnging the bitdown block, if so, it will test to see
// if it can change the bitdown.

#include <uhd/exception.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/utils/safe_main.hpp>
#include <rfnoc/example/bitdown_block_ctrl.hpp>
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
        std::cout << "Init RFNoC bitdown block " << desc << std::endl;
        std::cout << std::endl
                  << "This application attempts to find a bitdown block in a USRP "
                     "and tries to peek/poke registers..\n"
                  << std::endl;
        return EXIT_SUCCESS;
    }

    // Create RFNoC graph object:
    auto graph = uhd::rfnoc::rfnoc_graph::make(args);

    // Verify we have a bitdown block:
    auto bitdown_blocks = graph->find_blocks<uhd::rfnoc::bitdown_block_ctrl>("");
    if (bitdown_blocks.empty()) {
        std::cout << "No bitdown block found." << std::endl;
        return EXIT_FAILURE;
    }

    auto bitdown_block =
        graph->get_block<uhd::rfnoc::bitdown_block_ctrl>(bitdown_blocks.front());
    if (!bitdown_block) {
        std::cout << "ERROR: Failed to extract block controller!" << std::endl;
        return EXIT_FAILURE;
    }
    constexpr uint32_t new_bitdown_value = 42;
    bitdown_block->set_bitdown_value(new_bitdown_value);
    const uint32_t bitdown_value_read = bitdown_block->get_bitdown_value();

    if (bitdown_value_read != new_bitdown_value) {
        std::cout << "ERROR: Readback of bitdown value not working! "
                  << "Expected: " << new_bitdown_value << " Read: " << bitdown_value_read
                  << std::endl;
        return EXIT_FAILURE;
    } else {
        std::cout << "bitdown value read/write loopback successful!" << std::endl;
    }

    return EXIT_SUCCESS;
}
