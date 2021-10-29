//
// Copyright 2014-15 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Example for GPIO testing and bit banging.
//
// This example was originally designed to test the 11 bit wide front panel
// GPIO on the X300 series and has since been adapted to work with any GPIO
// bank on any USRP and provide optional bit banging.  Please excuse the
// clutter.  Also, there is no current way to detect the width of the
// specified GPIO bank, so the user must specify the width with the --bits
// flag if more than 11 bits.
//
// GPIO Testing:
// For testing, GPIO bits are set as follows:
// GPIO[0] = ATR output 1 at idle
// GPIO[1] = ATR output 1 during RX
// GPIO[2] = ATR output 1 during TX
// GPIO[3] = ATR output 1 during full duplex
// GPIO[4] = output
// GPIO[n:5] = input (all other pins)
// The testing cycles through idle, TX, RX, and full duplex, dwelling on each
// test case (default 2 seconds), and then comparing the readback register with
// the expected values of the outputs for verification.  The values of all GPIO
// registers are displayed at the end of each test case.  Outputs can be
// physically looped back to inputs to manually verify the inputs.
//
// GPIO Bit Banging:
// GPIO banks have the standard registers of DDR for data direction and OUT
// for output values.  Users can bit bang the GPIO bits by using this example
// with the --bitbang flag and specifying the --ddr and --out flags to set the
// values of the corresponding registers.  The READBACK register is
// continuously read for the duration of the dwell time (default 2 seconds) so
// users can monitor changes on the inputs.
//
// Automatic Transmit/Receive (ATR):
// In addition to the standard DDR and OUT registers, the GPIO banks also
// have ATR (Automatic Transmit/Receive) control registers that allow the
// GPIO pins to be automatically set to specific values when the USRP is
// idle, transmitting, receiving, or operating in full duplex mode.  The
// description of these registers is below:
// CTRL - Control (0=manual, 1=ATR)
// ATR_0X - Values to be set when idle
// ATR_RX - Output values to be set when receiving
// ATR_TX - Output values to be set when transmitting
// ATR_XX - Output values to be set when operating in full duplex
// This code below contains examples of setting all these registers.  On
// devices with multiple radios, the ATR driver for the front panel GPIO
// defaults to the state of the first radio (0 or A). This can be changed
// on a bit-by-bit basis by writing to the register:
// The ATR source can also be controlled, ie. drive from Radio0 or Radio1.
// SRC - Source (RFA=Radio0, RFB=Radio1, etc.)
//
// The UHD API
// The multi_usrp::set_gpio_attr() method is the UHD API for configuring and
// controlling the GPIO banks.  The parameters to the method are:
// bank - the name of the GPIO bank (typically "FP0" for front panel GPIO,
//                                   "TX<n>" for TX daughter card GPIO, or
//                                   "RX<n>" for RX daughter card GPIO)
// attr - attribute (register) to change ("SRC", "DDR", "OUT", "CTRL",
//                                        "ATR_0X", "ATR_RX", "ATR_TX",
//                                        "ATR_XX")
// value - the value to be set
// mask - a mask indicating which bits in the specified attribute register are
//          to be changed (default is all bits).

#include <uhd/convert.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <stdint.h>
#include <stdlib.h>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

static const std::string GPIO_DEFAULT_CPU_FORMAT = "fc32";
static const std::string GPIO_DEFAULT_OTW_FORMAT = "sc16";
static const double GPIO_DEFAULT_RX_RATE         = 500e3;
static const double GPIO_DEFAULT_TX_RATE         = 500e3;
static const double GPIO_DEFAULT_DWELL_TIME      = 2.0;
static const std::string GPIO_DEFAULT_GPIO       = "FP0";
static const size_t GPIO_DEFAULT_NUM_BITS        = 11;
static const std::string GPIO_DEFAULT_CTRL       = "0x0"; // all as user controlled
static const std::string GPIO_DEFAULT_DDR        = "0x0"; // all as inputs
static const std::string GPIO_DEFAULT_OUT        = "0x0";

static inline uint32_t GPIO_BIT(const size_t x)
{
    return (1 << x);
}

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

std::string to_bit_string(uint32_t val, const size_t num_bits)
{
    std::string out;
    for (int i = num_bits - 1; i >= 0; i--) {
        std::string bit = ((val >> i) & 1) ? "1" : "0";
        out += "  ";
        out += bit;
    }
    return out;
}

void output_reg_values(const std::string& bank,
    const std::string& port,
    const uhd::usrp::multi_usrp::sptr& usrp,
    const size_t num_bits)
{
    const std::vector<std::string> attrs = {
        "CTRL", "DDR", "ATR_0X", "ATR_RX", "ATR_TX", "ATR_XX", "OUT", "READBACK"};
    std::cout << (boost::format("%10s:") % "Bit");
    for (int i = num_bits - 1; i >= 0; i--)
        std::cout << (boost::format(" %2d") % i);
    std::cout << std::endl;
    for (const auto& attr : attrs) {
        const uint32_t gpio_bits = uint32_t(usrp->get_gpio_attr(bank, attr));
        std::cout << (boost::format("%10s:%s") % attr
                         % to_bit_string(gpio_bits, num_bits))
                  << std::endl;
    }

    // GPIO Src - get_gpio_src() not supported for all devices
    try {
        const auto gpio_src = usrp->get_gpio_src(port);
        std::cout << boost::format("%10s:") % "SRC";
        for (auto src : gpio_src) {
            std::cout << " " << src;
        }
        std::cout << std::endl;
    } catch (const uhd::not_implemented_error& e) {
        std::cout << "Ignoring " << e.what() << std::endl;
    } catch (...) {
        throw;
    }
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args;
    std::string cpu, otw;
    double rx_rate, tx_rate, dwell;
    std::string gpio;
    std::string port;
    size_t num_bits;
    std::string src_str;
    std::string ctrl_str;
    std::string ddr_str;
    std::string out_str;
    std::string tx_subdev_spec;
    std::string rx_subdev_spec;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("tx_subdev_spec", po::value<std::string>(&tx_subdev_spec)->default_value(""), "A:0, B:0, or A:0 B:0")
        ("rx_subdev_spec", po::value<std::string>(&rx_subdev_spec)->default_value(""), "A:0, B:0, or A:0 B:0")
        ("repeat", "repeat loop until Ctrl-C is pressed")
        ("list-banks", "print list of banks before running tests")
        ("cpu", po::value<std::string>(&cpu)->default_value(GPIO_DEFAULT_CPU_FORMAT), "cpu data format")
        ("otw", po::value<std::string>(&otw)->default_value(GPIO_DEFAULT_OTW_FORMAT), "over the wire data format")
        ("rx_rate", po::value<double>(&rx_rate)->default_value(GPIO_DEFAULT_RX_RATE), "rx sample rate")
        ("tx_rate", po::value<double>(&tx_rate)->default_value(GPIO_DEFAULT_TX_RATE), "tx sample rate")
        ("dwell", po::value<double>(&dwell)->default_value(GPIO_DEFAULT_DWELL_TIME), "dwell time in seconds for each test case")
        ("bank", po::value<std::string>(&gpio)->default_value(GPIO_DEFAULT_GPIO), "name of gpio bank")
        ("port", po::value<std::string>(&port)->default_value(""), "name of gpio port. If not specified, defaults to the GPIO bank")
        ("bits", po::value<size_t>(&num_bits)->default_value(GPIO_DEFAULT_NUM_BITS), "number of bits in gpio bank")
        ("bitbang", "single test case where user sets values for CTRL, DDR, and OUT registers")
        ("src", po::value<std::string>(&src_str), "GPIO SRC reg value")
        ("ddr", po::value<std::string>(&ddr_str)->default_value(GPIO_DEFAULT_DDR), "GPIO DDR reg value")
        ("out", po::value<std::string>(&out_str)->default_value(GPIO_DEFAULT_OUT), "GPIO OUT reg value")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("gpio %s") % desc << std::endl;
        return ~0;
    }

    // Handle if the port is unspecified
    if (port.empty()) {
        port = gpio;
    }

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    if (vm.count("list-banks")) {
        std::cout << "Available GPIO banks: " << std::endl;
        auto banks = usrp->get_gpio_banks(0);
        for (auto& bank : banks) {
            std::cout << "* " << bank << std::endl;
        }
    }
    std::cout << "Using GPIO bank: " << gpio << std::endl;

    // subdev spec
    if (!tx_subdev_spec.empty())
        usrp->set_tx_subdev_spec(tx_subdev_spec);
    if (!rx_subdev_spec.empty())
        usrp->set_rx_subdev_spec(rx_subdev_spec);
    std::cout << boost::format("  rx_subdev_spec: %s")
                     % usrp->get_rx_subdev_spec(0).to_string()
              << std::endl;
    std::cout << boost::format("  tx_subdev_spec: %s")
                     % usrp->get_tx_subdev_spec(0).to_string()
              << std::endl;

    // print out initial unconfigured state of GPIO
    std::cout << "Initial GPIO values:" << std::endl;
    output_reg_values(gpio, port, usrp, num_bits);

    // configure GPIO registers
    uint32_t ddr        = strtoul(ddr_str.c_str(), NULL, 0);
    uint32_t out        = strtoul(out_str.c_str(), NULL, 0);
    uint32_t ctrl       = 0;
    uint32_t atr_idle   = 0;
    uint32_t atr_rx     = 0;
    uint32_t atr_tx     = 0;
    uint32_t atr_duplex = 0;
    uint32_t mask       = (1 << num_bits) - 1;

    if (!vm.count("bitbang")) {
        // set up GPIO outputs:
        // GPIO[0] = ATR output 1 at idle
        ctrl |= GPIO_BIT(0);
        atr_idle |= GPIO_BIT(0);
        ddr |= GPIO_BIT(0);

        // GPIO[1] = ATR output 1 during RX
        ctrl |= GPIO_BIT(1);
        ddr |= GPIO_BIT(1);
        atr_rx |= GPIO_BIT(1);

        // GPIO[2] = ATR output 1 during TX
        ctrl |= GPIO_BIT(2);
        ddr |= GPIO_BIT(2);
        atr_tx |= GPIO_BIT(2);

        // GPIO[3] = ATR output 1 during full duplex
        ctrl |= GPIO_BIT(3);
        ddr |= GPIO_BIT(3);
        atr_duplex |= GPIO_BIT(3);

        // GPIO[4] = output
        ddr |= GPIO_BIT(4);
    }

    // set GPIO driver source
    if (vm.count("src")) {
        std::vector<std::string> gpio_src;
        typedef boost::char_separator<char> separator;
        boost::tokenizer<separator> tokens(src_str, separator(" "));
        std::copy(tokens.begin(), tokens.end(), std::back_inserter(gpio_src));
        usrp->set_gpio_src(port, gpio_src);
    }

    // set data direction register (DDR)
    usrp->set_gpio_attr(gpio, "DDR", ddr, mask);

    // set control register
    usrp->set_gpio_attr(gpio, "CTRL", ctrl, mask);

    // set output values (OUT)
    usrp->set_gpio_attr(gpio, "OUT", out, mask);

    // set ATR registers
    usrp->set_gpio_attr(gpio, "ATR_0X", atr_idle, mask);
    usrp->set_gpio_attr(gpio, "ATR_RX", atr_rx, mask);
    usrp->set_gpio_attr(gpio, "ATR_TX", atr_tx, mask);
    usrp->set_gpio_attr(gpio, "ATR_XX", atr_duplex, mask);

    // print out initial state of FP GPIO
    std::cout << "\nConfigured GPIO values:" << std::endl;
    output_reg_values(gpio, port, usrp, num_bits);
    std::cout << std::endl;

    // set up streams
    uhd::stream_args_t rx_args(cpu, otw);
    uhd::stream_args_t tx_args(cpu, otw);
    uhd::rx_streamer::sptr rx_stream;
    uhd::tx_streamer::sptr tx_stream;
    uhd::stream_cmd_t rx_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    rx_cmd.stream_now = true;
    if (usrp->get_rx_num_channels()) {
        rx_stream = usrp->get_rx_stream(rx_args);
        usrp->set_rx_rate(rx_rate);
    }
    if (usrp->get_tx_num_channels()) {
        tx_stream = usrp->get_tx_stream(tx_args);
        usrp->set_tx_rate(tx_rate);
    }

    // set up buffers for tx and rx
    const size_t rx_spp          = rx_stream ? rx_stream->get_max_num_samps() : 0;
    const size_t tx_spp          = tx_stream ? tx_stream->get_max_num_samps() : 0;
    const size_t nsamps_per_buff = std::max(rx_spp, tx_spp);
    std::vector<char> rx_buff(nsamps_per_buff * uhd::convert::get_bytes_per_item(cpu));
    std::vector<char> tx_buff(nsamps_per_buff * uhd::convert::get_bytes_per_item(cpu));
    std::vector<void*> rx_buffs, tx_buffs;
    if (rx_stream) {
        for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++) {
            rx_buffs.push_back(&rx_buff.front()); // same buffer for each channel
        }
    }
    if (tx_stream) {
        for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
            tx_buffs.push_back(&tx_buff.front()); // same buffer for each channel
    }

    uhd::rx_metadata_t rx_md;
    uhd::tx_metadata_t tx_md;
    tx_md.has_time_spec  = false;
    tx_md.start_of_burst = true;
    double timeout       = 0.01;
    auto dwell_time      = std::chrono::milliseconds(static_cast<int64_t>(dwell * 1000));

    int loop = 0;
    uint32_t rb, expected;

    // register signal handler
    std::signal(SIGINT, &sig_int_handler);

    if (!vm.count("bitbang")) {
        // Test the mask parameter of the multi_usrp::set_gpio_attr API
        // We only need to test once with no dwell time
        std::cout << "\nTesting mask..." << std::flush;
        // send a value of all 1's to the DDR with a mask for only upper most bit
        usrp->set_gpio_attr(gpio, "DDR", ~0, GPIO_BIT(num_bits - 1));
        // upper most bit should now be 1, but all the other bits should be unchanged
        rb       = usrp->get_gpio_attr(gpio, "DDR") & mask;
        expected = ddr | GPIO_BIT(num_bits - 1);
        if (rb == expected)
            std::cout << "pass:" << std::endl;
        else
            std::cout << "fail:" << std::endl;
        output_reg_values(gpio, port, usrp, num_bits);
        // restore DDR value
        usrp->set_gpio_attr(gpio, "DDR", ddr, mask);
    }

    while (not stop_signal_called) {
        int failures = 0;

        if (vm.count("repeat"))
            std::cout << "Press Ctrl + C to quit..." << std::endl;

        if (vm.count("bitbang")) {
            // dwell and continuously read back GPIO values
            auto stop_time = std::chrono::steady_clock::now() + dwell_time;
            while (
                not stop_signal_called and std::chrono::steady_clock::now() < stop_time) {
                rb = usrp->get_gpio_attr(gpio, "READBACK");
                std::cout << "\rREADBACK: " << to_bit_string(rb, num_bits);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            std::cout << std::endl;
        } else {
            // test user controlled GPIO and ATR idle by setting bit 4 high for 1 second
            std::cout << "\nTesting user controlled GPIO and ATR idle output..."
                      << std::flush;
            usrp->set_gpio_attr(gpio, "OUT", GPIO_BIT(4), GPIO_BIT(4));
            auto stop_time = std::chrono::steady_clock::now() + dwell_time;
            while (
                not stop_signal_called and std::chrono::steady_clock::now() < stop_time) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            rb       = usrp->get_gpio_attr(gpio, "READBACK");
            expected = GPIO_BIT(4) | GPIO_BIT(0);
            if ((rb & expected) != expected) {
                ++failures;
                std::cout << "fail:" << std::endl;
                if ((rb & GPIO_BIT(0)) == 0)
                    std::cout << "Bit 0 should be set, but is not" << std::endl;
                if ((rb & GPIO_BIT(4)) == 0)
                    std::cout << "Bit 4 should be set, but is not" << std::endl;
            } else {
                std::cout << "pass:" << std::endl;
            }
            output_reg_values(gpio, port, usrp, num_bits);
            usrp->set_gpio_attr(gpio, "OUT", 0, GPIO_BIT(4));
            if (stop_signal_called)
                break;

            if (rx_stream) {
                // test ATR RX by receiving for 1 second
                std::cout << "\nTesting ATR RX output..." << std::flush;
                rx_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
                rx_stream->issue_stream_cmd(rx_cmd);
                stop_time = std::chrono::steady_clock::now() + dwell_time;
                while (not stop_signal_called
                       and std::chrono::steady_clock::now() < stop_time) {
                    try {
                        rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
                    } catch (...) {
                    }
                }
                rb       = usrp->get_gpio_attr(gpio, "READBACK");
                expected = GPIO_BIT(1);
                if ((rb & expected) != expected) {
                    ++failures;
                    std::cout << "fail:" << std::endl;
                    std::cout << "Bit 1 should be set, but is not" << std::endl;
                } else {
                    std::cout << "pass:" << std::endl;
                }
                output_reg_values(gpio, port, usrp, num_bits);
                rx_stream->issue_stream_cmd(
                    uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
                // clear out any data left in the rx stream
                try {
                    rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
                } catch (...) {
                }
            }
            if (stop_signal_called)
                break;

            if (tx_stream) {
                // test ATR TX by transmitting for 1 second
                std::cout << "\nTesting ATR TX output..." << std::flush;
                stop_time            = std::chrono::steady_clock::now() + dwell_time;
                tx_md.start_of_burst = true;
                tx_md.end_of_burst   = false;
                while (not stop_signal_called
                       and std::chrono::steady_clock::now() < stop_time) {
                    try {
                        tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
                        tx_md.start_of_burst = false;
                    } catch (...) {
                    }
                }
                rb       = usrp->get_gpio_attr(gpio, "READBACK");
                expected = GPIO_BIT(2);
                if ((rb & expected) != expected) {
                    ++failures;
                    std::cout << "fail:" << std::endl;
                    std::cout << "Bit 2 should be set, but is not" << std::endl;
                } else {
                    std::cout << "pass:" << std::endl;
                }
                output_reg_values(gpio, port, usrp, num_bits);
                tx_md.end_of_burst = true;
                try {
                    tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
                } catch (...) {
                }
            }
            if (stop_signal_called)
                break;

            if (rx_stream and tx_stream) {
                // test ATR full duplex by transmitting and receiving
                std::cout << "\nTesting ATR full duplex output..." << std::flush;
                rx_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
                rx_stream->issue_stream_cmd(rx_cmd);
                tx_md.start_of_burst = true;
                tx_md.end_of_burst   = false;
                stop_time            = std::chrono::steady_clock::now() + dwell_time;
                while (not stop_signal_called
                       and std::chrono::steady_clock::now() < stop_time) {
                    try {
                        tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
                        tx_md.start_of_burst = false;
                        rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
                    } catch (...) {
                    }
                }

                // Read GPIO
                rb = usrp->get_gpio_attr(gpio, "READBACK");

                // Stop streaming
                rx_stream->issue_stream_cmd(
                    uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
                tx_md.end_of_burst = true;
                try {
                    tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
                } catch (...) {
                }

                // clear out any data left in the rx stream
                try {
                    rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
                } catch (...) {
                }

                // Analyze results
                expected = GPIO_BIT(3);
                if ((rb & expected) != expected) {
                    ++failures;
                    std::cout << "fail:" << std::endl;
                    std::cout << "Bit 3 should be set, but is not" << std::endl;
                } else {
                    std::cout << "pass:" << std::endl;
                }
                output_reg_values(gpio, port, usrp, num_bits);
            }

            std::cout << std::endl;
            if (failures)
                std::cout << failures << " tests failed" << std::endl;
            else
                std::cout << "All tests passed!" << std::endl;
        }

        if (!vm.count("repeat"))
            break;

        if (not stop_signal_called)
            std::cout << (boost::format("\nLoop %d completed") % ++loop) << std::endl;
    }

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
