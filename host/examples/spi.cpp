//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Example for SPI testing.
//
// This example shows how to work with SPI which is based on the GPIO
// interface of the X410.

#include <uhd/features/spi_getter_iface.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <stdlib.h>
#include <boost/program_options.hpp>
#include <iostream>

static const std::string SPI_DEFAULT_GPIO      = "GPIOA";
static const size_t SPI_DEFAULT_CLK_PIN        = 0;
static const size_t SPI_DEFAULT_MISO_PIN       = 1;
static const size_t SPI_DEFAULT_MOSI_PIN       = 2;
static const size_t SPI_DEFAULT_CS_PIN         = 3;
static const size_t SPI_DEFAULT_PAYLOAD_LENGTH = 32;
static const std::string SPI_DEFAULT_PAYLOAD   = "0xfefe";
static const size_t SPI_DEFAULT_CLK_DIVIDER    = 4;

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args;
    size_t clk;
    size_t miso;
    size_t mosi;
    size_t cs;
    size_t payload_length;
    size_t clk_divider;
    std::string payload_str;
    uint32_t payload;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("list-banks", "print list of banks before running tests")
        ("clk", po::value<size_t>(&clk)->default_value(SPI_DEFAULT_CLK_PIN), "number of pin for SPI clock")
        ("mosi", po::value<size_t>(&mosi)->default_value(SPI_DEFAULT_MOSI_PIN), "number of pin for MOSI")
        ("miso", po::value<size_t>(&miso)->default_value(SPI_DEFAULT_MISO_PIN), "number of pin for MISO")
        ("cs", po::value<size_t>(&cs)->default_value(SPI_DEFAULT_CS_PIN), "number of pin for chip select")
        ("payload", po::value<std::string>(&payload_str)->default_value(SPI_DEFAULT_PAYLOAD), "payload as integer value")
        ("length", po::value<size_t>(&payload_length)->default_value(SPI_DEFAULT_PAYLOAD_LENGTH), "payload length in bits")
        ("clk-div", po::value<size_t>(&clk_divider)->default_value(SPI_DEFAULT_CLK_DIVIDER), "clock divider for SPI")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << argv[0] << " " << desc << std::endl;
        return ~0;
    }

    // create a usrp device
    std::cout << std::endl;
    std::cout << "Creating the usrp device with: " << args << "..." << std::endl;
    auto usrp = uhd::usrp::multi_usrp::make(args);

    if (vm.count("list-banks")) {
        std::cout << "Available GPIO banks: " << std::endl;
        auto banks = usrp->get_gpio_banks(0);
        for (auto& bank : banks) {
            std::cout << "* " << bank << std::endl;
        }
    }

    // Get the SPI getter interface from where we'll get the SPI interface itself
    if (!usrp->get_radio_control().has_feature<uhd::features::spi_getter_iface>()) {
        std::cout << "Error: Could not find SPI_Getter_Iface. Please check if your FPGA "
                     "image is up to date.\n";
        return EXIT_FAILURE;
    }
    auto& spi_getter_iface =
        usrp->get_radio_control().get_feature<uhd::features::spi_getter_iface>();

    // Set all available pins to SPI for GPIO0 and GPIO1
    std::vector<std::string> sources(12, "DB0_SPI");
    usrp->set_gpio_src("GPIO0", sources);
    usrp->set_gpio_src("GPIO1", sources);

    // Create slave configuration per slave
    uhd::features::spi_slave_config_t slave_cfg;
    slave_cfg.slave_clk  = clk;
    slave_cfg.slave_miso = miso;
    slave_cfg.slave_mosi = mosi;
    slave_cfg.slave_ss   = cs;

    // The vector holds the slave configs with index=slave number
    std::vector<uhd::features::spi_slave_config_t> slave_cfgs;
    slave_cfgs.push_back(slave_cfg);

    // Set the data direction register
    uint32_t outputs = 0x0;
    outputs |= 1 << slave_cfg.slave_clk;
    outputs |= 1 << slave_cfg.slave_mosi;
    outputs |= 1 << slave_cfg.slave_ss;
    usrp->set_gpio_attr("GPIOA", "DDR", outputs & 0xFFFFFF);
    auto spi_ref = spi_getter_iface.get_spi_ref(slave_cfgs);

    std::cout << "Using pins: " << std::endl
              << "  Clock = " << (int)(slave_cfg.slave_clk) << std::endl
              << "  MOSI  = " << (int)(slave_cfg.slave_mosi) << std::endl
              << "  MISO  = " << (int)(slave_cfg.slave_miso) << std::endl
              << "  CS    = " << (int)(slave_cfg.slave_ss) << std::endl
              << std::endl;

    payload = strtoul(payload_str.c_str(), NULL, 0);
    std::cout << "Writing payload: 0x" << std::hex << payload << " with length "
              << std::dec << payload_length << " bits" << std::endl;
    // The spi_config_t holds items like the clock divider and the MISO
    // and MOSI edges
    uhd::spi_config_t config;
    config.divider            = clk_divider;
    config.use_custom_divider = true;
    config.mosi_edge          = config.EDGE_RISE;
    config.miso_edge          = config.EDGE_FALL;

    // Do the SPI transaction. There are write() and read() methods available, too.
    std::cout << "Performing SPI transaction..." << std::endl;
    uint32_t read_data = spi_ref->transact_spi(0, config, payload, payload_length, true);
    std::cout << "Data read: 0x" << std::hex << read_data << std::endl;

    return EXIT_SUCCESS;
}
