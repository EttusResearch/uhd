//
// Copyright 2014-15 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Example for GPIO testing and bit banging.
//
// This example was originally designed to test the 12 bit wide front panel
// GPIO on the X300 series and has since been adapted to work with any GPIO
// bank on any USRP and provide optional bit banging.  Please excuse the
// clutter.  Also, there is no current way to detect the width of the
// specified GPIO bank, so the user must specify the width with the --bits
// flag if more than 12 bits.
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
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>

static const std::string GPIO_DEFAULT_CPU_FORMAT = "fc32";
static const std::string GPIO_DEFAULT_OTW_FORMAT = "sc16";
static const double GPIO_DEFAULT_RX_RATE         = 500e3;
static const double GPIO_DEFAULT_TX_RATE         = 500e3;
static const double GPIO_DEFAULT_DWELL_TIME      = 2.0;
static const size_t GPIO_DEFAULT_NUM_BITS        = 12;
static const std::string GPIO_DEFAULT_CTRL       = "0x0"; // all as user controlled
static const std::string GPIO_DEFAULT_DDR        = "0x0"; // all as inputs
static const std::string GPIO_DEFAULT_OUT        = "0x0";
constexpr size_t GPIO_MIN_NUM_BITS               = 5;

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
    const size_t num_bits,
    const bool has_src_api)
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

    if (!has_src_api) {
        return;
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


bool check_rb_values(const uint32_t rb,
    uint32_t expected,
    const uint32_t num_bits,
    const uint32_t loopback_num_bits)
{
    if (loopback_num_bits) {
        const uint32_t lb_mask = (1 << loopback_num_bits) - 1;
        expected |= ((expected & lb_mask) << GPIO_MIN_NUM_BITS);
    }
    if ((rb & expected) != expected) {
        std::cout << "fail:" << std::endl;
        for (size_t bit = 0; bit < num_bits; bit++) {
            if ((expected & GPIO_BIT(bit)) && ((rb & GPIO_BIT(bit)) == 0)) {
                std::cout << "Bit " << bit << " should be set, but is not. ";
                if (loopback_num_bits && bit >= GPIO_MIN_NUM_BITS) {
                    std::cout << "Are GPIO pins correctly looped back?";
                }
                std::cout << std::endl;
            }
        }
        return false;
    }
    std::cout << "pass:" << std::endl;
    return true;
}


void run_bitbang_test(uhd::usrp::multi_usrp::sptr usrp,
    const std::string gpio_bank,
    const std::string port,
    const uint32_t ddr,
    const uint32_t out,
    const uint32_t mask,
    const uint32_t num_bits,
    const std::chrono::milliseconds dwell_time,
    const bool repeat,
    const bool has_src_api)
{
    // Set all pins to "GPIO", and DDR/OUT to whatever the user requested
    usrp->set_gpio_attr(gpio_bank, "CTRL", 0, mask);
    usrp->set_gpio_attr(gpio_bank, "DDR", ddr, mask);
    usrp->set_gpio_attr(gpio_bank, "OUT", out, mask);

    // print out initial state of GPIO
    std::cout << "\nConfigured GPIO values:" << std::endl;
    output_reg_values(gpio_bank, port, usrp, num_bits, has_src_api);
    std::cout << std::endl;
    std::signal(SIGINT, &sig_int_handler);

    do {
        // dwell and continuously read back GPIO values
        auto stop_time = std::chrono::steady_clock::now() + dwell_time;
        while (not stop_signal_called and std::chrono::steady_clock::now() < stop_time) {
            std::cout << "\rREADBACK: "
                      << to_bit_string(
                             usrp->get_gpio_attr(gpio_bank, "READBACK"), num_bits);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::cout << std::endl;
    } while (repeat && !stop_signal_called);
}


struct stream_helper_type
{
    stream_helper_type(uhd::usrp::multi_usrp::sptr usrp,
        double rx_rate,
        double tx_rate,
        const std::string& cpu,
        const std::string& otw,
        const std::chrono::milliseconds dwell_time)
        : rx_args(cpu, otw), tx_args(cpu, otw), dwell_time(dwell_time)
    {
        rx_cmd.stream_now = true;

        if (usrp->get_rx_num_channels()) {
            rx_stream = usrp->get_rx_stream(rx_args);
            usrp->set_rx_rate(rx_rate);
        }
        if (usrp->get_tx_num_channels()) {
            tx_stream = usrp->get_tx_stream(tx_args);
            usrp->set_tx_rate(tx_rate);
        }

        const size_t rx_spp = rx_stream ? rx_stream->get_max_num_samps() : 0;
        const size_t tx_spp = tx_stream ? tx_stream->get_max_num_samps() : 0;
        nsamps_per_buff     = std::max(rx_spp, tx_spp);

        if (rx_stream) {
            rx_buff.resize(nsamps_per_buff * uhd::convert::get_bytes_per_item(cpu));
            for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++) {
                rx_buffs.push_back(&rx_buff.front()); // same buffer for each channel
            }
        }
        if (tx_stream) {
            tx_buff.resize(nsamps_per_buff * uhd::convert::get_bytes_per_item(cpu));
            for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
                tx_buffs.push_back(&tx_buff.front()); // same buffer for each channel
        }

        tx_md.has_time_spec  = false;
        tx_md.start_of_burst = true;
    }

    void start_stream(bool tx, bool rx)
    {
        if (tx && rx) {
            rx_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
            rx_stream->issue_stream_cmd(rx_cmd);
            tx_md.start_of_burst = true;
            tx_md.end_of_burst   = false;
            auto stop_time       = std::chrono::steady_clock::now() + dwell_time;
            while (
                not stop_signal_called and std::chrono::steady_clock::now() < stop_time) {
                try {
                    tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
                    tx_md.start_of_burst = false;
                    rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
                } catch (...) {
                }
            }
            return;
        }

        if (tx) {
            auto stop_time       = std::chrono::steady_clock::now() + dwell_time;
            tx_md.start_of_burst = true;
            tx_md.end_of_burst   = false;
            while (
                not stop_signal_called and std::chrono::steady_clock::now() < stop_time) {
                try {
                    tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
                    tx_md.start_of_burst = false;
                } catch (...) {
                }
            }
        }

        if (rx) {
            rx_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
            rx_stream->issue_stream_cmd(rx_cmd);
            auto stop_time = std::chrono::steady_clock::now() + dwell_time;
            while (
                not stop_signal_called and std::chrono::steady_clock::now() < stop_time) {
                try {
                    rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
                } catch (...) {
                }
            }
        }
    }

    void stop_stream(bool tx, bool rx)
    {
        if (tx) {
            tx_md.end_of_burst = true;
            try {
                tx_stream->send(tx_buffs, nsamps_per_buff, tx_md, timeout);
            } catch (...) {
            }
        }
        if (rx) {
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            // clear out any data left in the rx stream
            try {
                rx_stream->recv(rx_buffs, nsamps_per_buff, rx_md, timeout);
            } catch (...) {
            }
        }
    }

    uhd::stream_args_t rx_args;
    uhd::stream_args_t tx_args;
    uhd::rx_streamer::sptr rx_stream;
    uhd::tx_streamer::sptr tx_stream;
    uhd::stream_cmd_t rx_cmd{uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS};

    size_t nsamps_per_buff;
    std::vector<char> rx_buff;
    std::vector<char> tx_buff;
    std::vector<void*> rx_buffs, tx_buffs;

    uhd::rx_metadata_t rx_md;
    uhd::tx_metadata_t tx_md;

    double timeout = 0.01;

    const std::chrono::milliseconds dwell_time;
};


int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args;
    std::string cpu, otw;
    double rx_rate, tx_rate, dwell;
    // This is the argument for set_gpio_attr(), not the connector name:
    std::string gpio_bank;
    std::string port;
    size_t num_bits;
    uint32_t loopback_num_bits = 0;
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
        ("list_banks", "print list of banks before running tests")
        ("cpu", po::value<std::string>(&cpu)->default_value(GPIO_DEFAULT_CPU_FORMAT), "cpu data format")
        ("otw", po::value<std::string>(&otw)->default_value(GPIO_DEFAULT_OTW_FORMAT), "over the wire data format")
        ("rx_rate", po::value<double>(&rx_rate)->default_value(GPIO_DEFAULT_RX_RATE), "rx sample rate")
        ("tx_rate", po::value<double>(&tx_rate)->default_value(GPIO_DEFAULT_TX_RATE), "tx sample rate")
        ("dwell", po::value<double>(&dwell)->default_value(GPIO_DEFAULT_DWELL_TIME), "dwell time in seconds for each test case")
        ("bank", po::value<std::string>(&gpio_bank)->default_value(""), "name of gpio bank (defaults to first bank in list)")
        ("port", po::value<std::string>(&port)->default_value(""), "name of gpio port (source bank). If not specified, defaults to the first bank")
        ("bits", po::value<size_t>(&num_bits)->default_value(GPIO_DEFAULT_NUM_BITS), "number of bits in gpio bank")
        ("bitbang", "single test case where user sets values for CTRL, DDR, and OUT registers")
        ("check_loopback", "check that lower half of pins is looped back onto upper half")
        ("src", po::value<std::string>(&src_str), "GPIO source value (not available on all USRPs)")
        ("ddr", po::value<std::string>(&ddr_str)->default_value(GPIO_DEFAULT_DDR), "GPIO DDR reg value")
        ("out", po::value<std::string>(&out_str)->default_value(GPIO_DEFAULT_OUT), "GPIO OUT reg value")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    /*** Sanity-check arguments **********************************************/
    // print the help message
    if (vm.count("help")) {
        std::cout << "gpio " << desc << std::endl;
        return EXIT_SUCCESS;
    }
    if (vm.count("check_loopback")) {
        // For a proper test, we need at least 5 pins (4xATR + 1xGPIO). That
        // means we also want at *most* 5 pins to be looped back for this test.
        if (num_bits <= GPIO_MIN_NUM_BITS) {
            loopback_num_bits = 0;
        } else {
            loopback_num_bits = std::min(GPIO_MIN_NUM_BITS, num_bits - GPIO_MIN_NUM_BITS);
        }
        std::cout << "Checking external GPIO loopback! Expecting the following external "
                     "connections: "
                  << std::endl;
        for (size_t gpio = 0; gpio + loopback_num_bits < num_bits; ++gpio) {
            std::cout << "GPIO " << gpio << " --> " << gpio + loopback_num_bits
                      << std::endl;
        }
    }

    const auto dwell_time = std::chrono::milliseconds(static_cast<int64_t>(dwell * 1000));

    /*** Set up USRP device and GPIO banks ************************************/
    std::cout << std::endl;
    std::cout << "Creating the usrp device with: " << args << "..." << std::endl;
    auto usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << "Using Device: " << usrp->get_pp_string() << std::endl;
    bool has_src_api = true;
    // Handle if the port is unspecified
    if (port.empty()) {
        try {
            port = usrp->get_gpio_src_banks(0).front();
        } catch (const uhd::not_implemented_error&) {
            has_src_api = false;
        }
    }
    if (gpio_bank.empty()) {
        gpio_bank = usrp->get_gpio_banks(0).front();
    }
    if (has_src_api) {
        std::cout << "Using GPIO connector: " << port << std::endl;
    }

    if (vm.count("list_banks")) {
        std::cout << "Available GPIO banks: " << std::endl;
        auto banks = usrp->get_gpio_banks(0);
        for (auto& bank : banks) {
            std::cout << "* " << bank << std::endl;
        }
    }
    std::cout << "Using GPIO bank: " << gpio_bank << std::endl;
    // subdev spec
    if (!tx_subdev_spec.empty())
        usrp->set_tx_subdev_spec(tx_subdev_spec);
    if (!rx_subdev_spec.empty())
        usrp->set_rx_subdev_spec(rx_subdev_spec);
    std::cout << "  rx_subdev_spec: " << usrp->get_rx_subdev_spec(0).to_string()
              << std::endl;
    std::cout << "  tx_subdev_spec: " << usrp->get_tx_subdev_spec(0).to_string()
              << std::endl;
    // set GPIO driver source
    if (vm.count("src")) {
        std::vector<std::string> gpio_src;
        typedef boost::char_separator<char> separator;
        boost::tokenizer<separator> tokens(src_str, separator(" "));
        std::copy(tokens.begin(), tokens.end(), std::back_inserter(gpio_src));
        // If someone provides --src, this will throw an exception
        // on devices that don't have the source API. That's OK, because we
        // have no way of honoring the caller's request, and the exception
        // thrown is a good way to communicate that back to the call site.
        usrp->set_gpio_src(port, gpio_src);
    }

    // print out initial unconfigured state of GPIO
    std::cout << "Initial GPIO values:" << std::endl;
    output_reg_values(gpio_bank, port, usrp, num_bits, has_src_api);

    // configure GPIO registers
    uint32_t ddr        = strtoul(ddr_str.c_str(), NULL, 0);
    uint32_t out        = strtoul(out_str.c_str(), NULL, 0);
    uint32_t ctrl       = 0;
    uint32_t atr_idle   = 0;
    uint32_t atr_rx     = 0;
    uint32_t atr_tx     = 0;
    uint32_t atr_duplex = 0;
    uint32_t mask       = (1 << num_bits) - 1;

    // The bitbang test is its own thing
    if (vm.count("bitbang")) {
        run_bitbang_test(usrp,
            gpio_bank,
            port,
            ddr,
            out,
            mask,
            num_bits,
            dwell_time,
            bool(vm.count("repeat")),
            has_src_api);
        return EXIT_SUCCESS;
    }

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

    // set data direction register (DDR)
    usrp->set_gpio_attr(gpio_bank, "DDR", ddr, mask);

    // set control register
    usrp->set_gpio_attr(gpio_bank, "CTRL", ctrl, mask);

    // set output values (OUT)
    usrp->set_gpio_attr(gpio_bank, "OUT", out, mask);

    // set ATR registers
    usrp->set_gpio_attr(gpio_bank, "ATR_0X", atr_idle, mask);
    usrp->set_gpio_attr(gpio_bank, "ATR_RX", atr_rx, mask);
    usrp->set_gpio_attr(gpio_bank, "ATR_TX", atr_tx, mask);
    usrp->set_gpio_attr(gpio_bank, "ATR_XX", atr_duplex, mask);

    // print out initial state of FP GPIO
    std::cout << "\nConfigured GPIO values:" << std::endl;
    output_reg_values(gpio_bank, port, usrp, num_bits, has_src_api);
    std::cout << std::endl;

    // set up streams
    stream_helper_type stream_helper(usrp, rx_rate, tx_rate, cpu, otw, dwell_time);

    int loop          = 0;
    int failures      = 0;
    bool tests_failed = false;

    // register signal handler
    std::signal(SIGINT, &sig_int_handler);

    // Test the mask parameter of the multi_usrp::set_gpio_attr API
    // We only need to test once with no dwell time
    std::cout << "\nTesting mask..." << std::flush;
    // send a value of all 1's to the DDR with a mask for only upper most bit
    usrp->set_gpio_attr(gpio_bank, "DDR", ~0, GPIO_BIT(num_bits - 1));
    // upper most bit should now be 1, but all the other bits should be unchanged
    failures += int(!check_rb_values(usrp->get_gpio_attr(gpio_bank, "DDR") & mask,
        ddr | GPIO_BIT(num_bits - 1),
        num_bits, 0));
    output_reg_values(gpio_bank, port, usrp, num_bits, has_src_api);
    // restore DDR value
    usrp->set_gpio_attr(gpio_bank, "DDR", ddr, mask);

    /*************************************************************************/
    /* Setup complete, start running test                                    */
    /*************************************************************************/
    while (not stop_signal_called) {
        if (vm.count("repeat"))
            std::cout << "Press Ctrl + C to quit..." << std::endl;

        /*** Test 1: User-controlled GPIO + ATR idle *************************/
        std::cout << "\nTesting user controlled GPIO and ATR idle output..."
                  << std::flush;
        usrp->set_gpio_attr(gpio_bank, "OUT", GPIO_BIT(4), GPIO_BIT(4));
        auto stop_time = std::chrono::steady_clock::now() + dwell_time;
        while (not stop_signal_called and std::chrono::steady_clock::now() < stop_time) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        failures += int(!check_rb_values(usrp->get_gpio_attr(gpio_bank, "READBACK"),
            GPIO_BIT(4) | GPIO_BIT(0),
            num_bits, loopback_num_bits));
        output_reg_values(gpio_bank, port, usrp, num_bits, has_src_api);
        usrp->set_gpio_attr(gpio_bank, "OUT", 0, GPIO_BIT(4));
        if (stop_signal_called)
            break;

        /*** Test 2: ATR RX **************************************************/
        if (stream_helper.rx_stream) {
            // test ATR RX by receiving for 1 second
            std::cout << "\nTesting ATR RX output..." << std::flush;
            stream_helper.start_stream(false, true);
            failures += int(!check_rb_values(usrp->get_gpio_attr(gpio_bank, "READBACK"),
                GPIO_BIT(1),
                num_bits, loopback_num_bits));
            output_reg_values(gpio_bank, port, usrp, num_bits, has_src_api);
            stream_helper.stop_stream(false, true);
        }
        if (stop_signal_called)
            break;

        /*** Test 3: ATR TX **************************************************/
        if (stream_helper.tx_stream) {
            // test ATR TX by transmitting for 1 second
            std::cout << "\nTesting ATR TX output..." << std::flush;
            stream_helper.start_stream(true, false);
            failures += int(!check_rb_values(usrp->get_gpio_attr(gpio_bank, "READBACK"),
                GPIO_BIT(2),
                num_bits, loopback_num_bits));
            output_reg_values(gpio_bank, port, usrp, num_bits, has_src_api);
            stream_helper.stop_stream(true, false);
        }
        if (stop_signal_called)
            break;

        /*** Test 4: ATR FDX *************************************************/
        if (stream_helper.rx_stream and stream_helper.tx_stream) {
            // test ATR full duplex by transmitting and receiving
            std::cout << "\nTesting ATR full duplex output..." << std::flush;
            stream_helper.start_stream(true, true);
            failures += int(!check_rb_values(usrp->get_gpio_attr(gpio_bank, "READBACK"),
                GPIO_BIT(3),
                num_bits, loopback_num_bits));
            output_reg_values(gpio_bank, port, usrp, num_bits, has_src_api);
            stream_helper.stop_stream(true, true);
        }

        std::cout << std::endl;
        if (failures) {
            tests_failed = true;
            std::cout << failures << " tests failed" << std::endl;
        } else {
            std::cout << "All tests passed!" << std::endl;
        }

        if (!vm.count("repeat")) {
            break;
        }
        failures = 0;

        if (not stop_signal_called) {
            std::cout << "\nLoop " << ++loop << " completed" << std::endl;
        }
    }

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return int(tests_failed);
}
