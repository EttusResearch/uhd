//
// Copyright 2014-15 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Example for GPIO testing and bit banging on X3xx series USRPs
// with front-panel GPIO bit muxing between radios.
//
// This example was modified from the test of an 11-bit wide front-panel
// GPIO bank on any USRP, with optional bit banging.  This version specifically
// exercises the ability on the X300 series to have the front-panel GPIO bits
// individually muxed between control by Radio A or Radio B.  Please excuse the
// clutter.  Also, there is no current way to detect the width of the
// specified GPIO bank, so the user must specify the width with the --bits
// flag if more than 12 bits.
//
// GPIO Testing:
// For testing, GPIO bits are set as follows:
// GPIO[0]  = radio A ATR output 1 at idle
// GPIO[1]  = radio A ATR output 1 during RX
// GPIO[2]  = radio A ATR output 1 during TX
// GPIO[3]  = radio A ATR output 1 during full duplex
// GPIO[4]  = radio A manual output
// GPIO[5]  = input
// GPIO[6]  = radio B ATR output 1 at idle
// GPIO[7]  = radio B ATR output 1 during RX
// GPIO[8]  = radio B ATR output 1 during TX
// GPIO[9]  = radio B ATR output 1 during full duplex
// GPIO[10] = radio B manual output
// GPIO[11] = input
//
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
// SRC - Source (0=Radio0, 1=Radio1, etc.)
//
// The UHD API
// The multi_usrp::set_gpio_attr() method is the UHD API for configuring and
// controlling the GPIO banks.  The parameters to the method are:
// bank - the name of the GPIO bank ("FP0" for FP GPIO bank of radio 0,
//                                   "FP1" for FP GPIO bank of radio 1,
//                                   "TX<n>" for TX daughter card GPIO, or
//                                   "RX<n>" for RX daughter card GPIO)
// attr - attribute (register) to change ("SRC", "DDR", "OUT", "CTRL",
//                                        "ATR_0X", "ATR_RX", "ATR_TX",
//                                        "ATR_XX")
// value - the value to be set
// mask - a mask indicating which bits in the specified attribute register are
//          to be changed (default is all bits).

#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/convert.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <csignal>
#include <iostream>
#include <thread>
#include <chrono>
#include <stdlib.h>
#include <stdint.h>

static const std::string        GPIO_DEFAULT_CPU_FORMAT = "fc32";
static const std::string        GPIO_DEFAULT_OTW_FORMAT = "sc16";
static const double             GPIO_DEFAULT_RX_RATE    = 500e3;
static const double             GPIO_DEFAULT_TX_RATE    = 500e3;
static const double             GPIO_DEFAULT_DWELL_TIME = 2.0;
static const size_t             GPIO_DEFAULT_NUM_BITS   = 12;
static const std::string        GPIO_DEFAULT_GPIO       = "FP0";
static const std::string        GPIO_DEFAULT_SRC        = "";       // empty string sets source for each bit based on ATR settings
static const std::string        GPIO_DEFAULT_CTRL       = "0x0";    // all as user controlled
static const std::string        GPIO_DEFAULT_DDR        = "0x0";    // all as inputs
static const std::string        GPIO_DEFAULT_OUT        = "0x0";

static inline uint32_t GPIO_BIT(const size_t x)
{
    return (1 << x);
}

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

std::string to_bit_string(uint32_t val, const size_t num_bits)
{
    std::string out;
    for (int i = num_bits - 1; i >= 0; i--)
    {
        std::string bit = ((val >> i) & 1) ? "1" : "0";
        out += "  ";
        out += bit;
    }
    return out;
}

void output_reg_values(
    const std::string bank,
    const uhd::usrp::multi_usrp::sptr &usrp,
    const size_t num_bits
) {
    const std::vector<std::string> attrs = {
        "SRC",
        "CTRL",
        "DDR",
        "ATR_0X",
        "ATR_RX",
        "ATR_TX",
        "ATR_XX",
        "OUT",
        "READBACK"
    };
    std::cout << (boost::format("%10s ") % "Bit");
    for (int i = num_bits - 1; i >= 0; i--)
        std::cout << (boost::format(" %2d") % i);
    std::cout << std::endl;
    for (const auto &attr : attrs) {
        const uint32_t gpio_bits = uint32_t(usrp->get_gpio_attr(bank, attr));
        std::cout
            << (boost::format("%10s:%s")
                % attr % to_bit_string(gpio_bits, num_bits))
            << std::endl;
    }
}

int UHD_SAFE_MAIN(int argc, char *argv[])
{
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    std::string cpu, otw;
    double rx_rate, tx_rate, dwell;
    size_t num_bits;
    std::string src_str;
    std::string ctrl_str;
    std::string ddr_str;
    std::string out_str;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("cpu", po::value<std::string>(&cpu)->default_value(GPIO_DEFAULT_CPU_FORMAT), "cpu data format")
        ("otw", po::value<std::string>(&otw)->default_value(GPIO_DEFAULT_OTW_FORMAT), "over the wire data format")
        ("rx_rate", po::value<double>(&rx_rate)->default_value(GPIO_DEFAULT_RX_RATE), "rx sample rate")
        ("tx_rate", po::value<double>(&tx_rate)->default_value(GPIO_DEFAULT_TX_RATE), "tx sample rate")
        ("dwell", po::value<double>(&dwell)->default_value(GPIO_DEFAULT_DWELL_TIME), "dwell time in seconds for each test case")
        ("bits", po::value<size_t>(&num_bits)->default_value(GPIO_DEFAULT_NUM_BITS), "number of bits in gpio bank")
        ("bitbang", "single test case where user sets values for CTRL, DDR, and OUT registers")
        ("src", po::value<std::string>(&src_str)->default_value(GPIO_DEFAULT_SRC), "GPIO SRC reg value")
        ("ddr", po::value<std::string>(&ddr_str)->default_value(GPIO_DEFAULT_DDR), "GPIO DDR reg value")
        ("out", po::value<std::string>(&out_str)->default_value(GPIO_DEFAULT_OUT), "GPIO OUT reg value")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("gpio %s") % desc << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    //this test is specifically for the x300 series with front-panel GPIO source mux control
    if (usrp->get_mboard_name(0).substr(0, 2) != "X3") {
        std::cout << "This test is only for X3xx devices with front-panel GPIO source mux control." << std::endl;
        return ~0;
    }

    //subdev spec set-up
    const std::vector<std::string> subdev_specs = {"A:0", "B:0"};

    //print out initial unconfigured state of FP GPIO
    const std::vector<std::string> fp_gpio_attrs = {"FP0", "FP1"};
    for (int i = 0; i < 2; i++) {
        std::cout << "Initial " << subdev_specs[i] << " GPIO values:" << std::endl;
        output_reg_values(fp_gpio_attrs[i], usrp, num_bits);
    }

    //configure GPIO registers
    std::vector<uint32_t> ddr(2, strtoul(ddr_str.c_str(), NULL, 0));
    std::vector<uint32_t> out(2, strtoul(out_str.c_str(), NULL, 0));
    std::vector<uint32_t> ctrl(2, 0);
    std::vector<uint32_t> atr_idle(2, 0);
    std::vector<uint32_t> atr_rx(2, 0);
    std::vector<uint32_t> atr_tx(2, 0);
    std::vector<uint32_t> atr_duplex(2, 0);
    uint32_t mask = (1 << num_bits) - 1;

    if (!vm.count("bitbang"))
    {
        //set up GPIO outputs:
        //idle state
        ctrl[0] |= GPIO_BIT(0);
        atr_idle[0] |= GPIO_BIT(0);
        ddr[0] |= GPIO_BIT(0);

        ctrl[1] |= GPIO_BIT(6);
        atr_idle[1] |= GPIO_BIT(6);
        ddr[1] |= GPIO_BIT(6);

        //RX state
        ctrl[0] |= GPIO_BIT(1);
        ddr[0] |= GPIO_BIT(1);
        atr_rx[0] |= GPIO_BIT(1);

        ctrl[1] |= GPIO_BIT(7);
        ddr[1] |= GPIO_BIT(7);
        atr_rx[1] |= GPIO_BIT(7);

        //TX state
        ctrl[0] |= GPIO_BIT(2);
        ddr[0] |= GPIO_BIT(2);
        atr_tx[0] |= GPIO_BIT(2);

        ctrl[1] |= GPIO_BIT(8);
        ddr[1] |= GPIO_BIT(8);
        atr_tx[1] |= GPIO_BIT(8);

        //full duplex state
        ctrl[0] |= GPIO_BIT(3);
        ddr[0] |= GPIO_BIT(3);
        atr_duplex[0] |= GPIO_BIT(3);

        ctrl[1] |= GPIO_BIT(9);
        ddr[1] |= GPIO_BIT(9);
        atr_duplex[1] |= GPIO_BIT(9);

        //manual output
        out[0] |= GPIO_BIT(4);
        ddr[0] |= GPIO_BIT(4);

        out[1] |= GPIO_BIT(10);
        ddr[1] |= GPIO_BIT(10);
    }

    // set GPIO driver source; if user did not specify, default each FP GPIO's
    // bit source to whichever channel has an ATR bit set
    uint32_t src_reg;
    if (src_str == "")
        src_reg = ddr[1];
    else
        src_reg = strtoul(src_str.c_str(), NULL, 0);
    
    // Radio 0 (A:0) controls the source mux register for FP GPIO
    std::cout << "Source mux register is " << src_reg << std::endl;
    usrp->set_gpio_attr(fp_gpio_attrs[0], "SRC", src_reg, mask);

    for (int i = 0; i < 2; i++) {
        //set data direction register (DDR)
        usrp->set_gpio_attr(fp_gpio_attrs[i], "DDR", ddr[i], mask);

        //set control register
        usrp->set_gpio_attr(fp_gpio_attrs[i], "CTRL", ctrl[i], mask);

        //set output values (OUT) to 0; manual output to be toggled below
        usrp->set_gpio_attr(fp_gpio_attrs[i], "OUT", 0, mask);

        //set ATR registers
        usrp->set_gpio_attr(fp_gpio_attrs[i], "ATR_0X", atr_idle[i], mask);
        usrp->set_gpio_attr(fp_gpio_attrs[i], "ATR_RX", atr_rx[i], mask);
        usrp->set_gpio_attr(fp_gpio_attrs[i], "ATR_TX", atr_tx[i], mask);
        usrp->set_gpio_attr(fp_gpio_attrs[i], "ATR_XX", atr_duplex[i], mask);

        //print out initial state of FP GPIO
        std::cout << "\nConfigured " << subdev_specs[i] << " GPIO values:" << std::endl;
        output_reg_values(fp_gpio_attrs[i], usrp, num_bits);
        std::cout << std::endl;
    }

    // common parameters
    uhd::rx_metadata_t rx_md;
    uhd::tx_metadata_t tx_md;
    uhd::time_spec_t stop_time;
    double timeout = 0.01;
    uhd::time_spec_t dwell_time(dwell);
    uint32_t rb, expected;

    //register signal handler
    std::signal(SIGINT, &sig_int_handler);

    for (int i = 0; i < 2; i++)
    {
        // setting subdev radio
        usrp->set_tx_subdev_spec(subdev_specs[i]);
        usrp->set_rx_subdev_spec(subdev_specs[i]);
        std::cout << boost::format("Current Subdev Spec:") << std::endl;
        std::cout << boost::format("  rx_subdev_spec: %s") % usrp->get_rx_subdev_spec(0).to_string() << std::endl;
        std::cout << boost::format("  tx_subdev_spec: %s") % usrp->get_tx_subdev_spec(0).to_string() << std::endl;

        //set up streams
        uhd::stream_args_t rx_args(cpu, otw);
        uhd::stream_args_t tx_args(cpu, otw);
        uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(rx_args);
        uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(tx_args);
        uhd::stream_cmd_t rx_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        rx_cmd.stream_now = true;
        tx_md.has_time_spec = false;
        tx_md.start_of_burst = true;
        usrp->set_rx_rate(rx_rate);
        usrp->set_tx_rate(tx_rate);

        //set up buffers for tx and rx
        const size_t max_samps_per_packet = rx_stream->get_max_num_samps();
        const size_t nsamps_per_buff = max_samps_per_packet;
        std::vector<char> rx_buff(max_samps_per_packet*uhd::convert::get_bytes_per_item(cpu));
        std::vector<char> tx_buff(max_samps_per_packet*uhd::convert::get_bytes_per_item(cpu));
        std::vector<void *> rx_buffs, tx_buffs;
        for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
            rx_buffs.push_back(&rx_buff.front()); //same buffer for each channel
        for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
            tx_buffs.push_back(&tx_buff.front()); //same buffer for each channel

        if (!vm.count("bitbang"))
        {
            // Test the mask parameter of the multi_usrp::set_gpio_attr API
            std::cout << "\nTesting mask on " << subdev_specs[i] << " ..." << std::flush;
            //send a value of all 1's to the DDR with a mask for only upper most bit
            usrp->set_gpio_attr(fp_gpio_attrs[i], "DDR", ~0, GPIO_BIT(num_bits - 1));
            //upper most bit should now be 1, but all the other bits should be unchanged
            rb = usrp->get_gpio_attr(fp_gpio_attrs[i], "DDR") & mask;
            expected = ddr[i] | GPIO_BIT(num_bits - 1);
            if (rb == expected)
                std::cout << "pass:" << std::endl;
            else
                std::cout << "fail:" << std::endl;
            output_reg_values(fp_gpio_attrs[i], usrp, num_bits);
            //restore DDR value
            usrp->set_gpio_attr(fp_gpio_attrs[i], "DDR", ddr[i], mask);
        }

        int failures = 0;

        if (vm.count("bitbang"))
        {
            // dwell and continuously read back GPIO values
            stop_time = usrp->get_time_now() + dwell_time;
            while (not stop_signal_called and usrp->get_time_now() < stop_time)
            {
                rb = usrp->get_gpio_attr(fp_gpio_attrs[i], "READBACK");
                std::cout << "\rREADBACK of " << subdev_specs[i] << ": " << to_bit_string(rb, num_bits);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            std::cout << std::endl;
        }
        else
        {
            // test user controlled GPIO and ATR idle
            std::cout << "\nTesting user controlled GPIO and ATR idle output on " << subdev_specs[i] << " ..." << std::flush;
            usrp->set_gpio_attr(fp_gpio_attrs[i], "OUT", out[i], out[i]);
            stop_time = usrp->get_time_now() + dwell_time;
            while (not stop_signal_called and usrp->get_time_now() < stop_time)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            rb = usrp->get_gpio_attr(fp_gpio_attrs[i], "READBACK");
            expected = atr_idle[i] | out[i];
            if ((rb & expected) != expected)
            {
                ++failures;
                std::cout << "fail:" << std::endl;
            } else {
                std::cout << "pass:" << std::endl;
            }
            output_reg_values(fp_gpio_attrs[i], usrp, num_bits);
            usrp->set_gpio_attr(fp_gpio_attrs[i], "OUT", 0, out[i]);
            if (stop_signal_called)
                break;

            // test ATR RX
            std::cout << "\nTesting ATR RX output on " << subdev_specs[i] << " ..." << std::flush;
            rx_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
            rx_stream->issue_stream_cmd(rx_cmd);
            stop_time = usrp->get_time_now() + dwell_time;
            while (not stop_signal_called and usrp->get_time_now() < stop_time)
            {
                try {
                    rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
                } catch(...){}
            }
            rb = usrp->get_gpio_attr(fp_gpio_attrs[i], "READBACK");
            expected = atr_rx[i];
            if ((rb & expected) != expected)
            {
                ++failures;
                std::cout << "fail:" << std::endl;
            } else {
                std::cout << "pass:" << std::endl;
            }
            output_reg_values(fp_gpio_attrs[i], usrp, num_bits);
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            //clear out any data left in the rx stream
            try {
                rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
            } catch(...){}
            if (stop_signal_called)
                break;

            // test ATR TX
            std::cout << "\nTesting ATR TX output on " << subdev_specs[i] << " ..." << std::flush;
            stop_time = usrp->get_time_now() + dwell_time;
            tx_md.start_of_burst = true;
            tx_md.end_of_burst = false;
            while (not stop_signal_called and usrp->get_time_now() < stop_time)
            {
                try {
                    tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
                    tx_md.start_of_burst = false;
                } catch(...){}
            }
            rb = usrp->get_gpio_attr(fp_gpio_attrs[i], "READBACK");
            expected = atr_tx[i];
            if ((rb & expected) != expected)
            {
                ++failures;
                std::cout << "fail:" << std::endl;
            } else {
                std::cout << "pass:" << std::endl;
            }
            output_reg_values(fp_gpio_attrs[i], usrp, num_bits);
            tx_md.end_of_burst = true;
            try {
                tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
            } catch(...){}
            if (stop_signal_called)
                break;

            // test ATR XX
            std::cout << "\nTesting ATR full duplex output on " << subdev_specs[i] << " ..." << std::flush;
            rx_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
            rx_stream->issue_stream_cmd(rx_cmd);
            tx_md.start_of_burst = true;
            tx_md.end_of_burst = false;
            stop_time = usrp->get_time_now() + dwell_time;
            while (not stop_signal_called and usrp->get_time_now() < stop_time)
            {
                try {
                    tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
                    tx_md.start_of_burst = false;
                    rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
                } catch(...){}
            }
            rb = usrp->get_gpio_attr(fp_gpio_attrs[i], "READBACK");
            expected = atr_duplex[i];
            if ((rb & expected) != expected)
            {
                ++failures;
                std::cout << "fail:" << std::endl;
            } else {
                std::cout << "pass:" << std::endl;
            }
            output_reg_values(fp_gpio_attrs[i], usrp, num_bits);
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            tx_md.end_of_burst = true;
            try {
                tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
            } catch(...){}
            //clear out any data left in the rx stream
            try {
                rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
            } catch(...){}

            std::cout << std::endl;
            if (failures)
                std::cout << failures << " tests failed" << std::endl;
            else
                std::cout << "All tests passed!" << std::endl;

            // allowing time to finish up
            stop_time = usrp->get_time_now() + dwell_time;
            while (not stop_signal_called and usrp->get_time_now() < stop_time)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

    }

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
