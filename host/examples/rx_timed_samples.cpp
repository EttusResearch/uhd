//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <complex>
#include <iostream>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string program_doc =
        "usage: rx_timed_samples [-h] [--args ARGS] [-r RATE] [--otw {sc16,sc8}]\n"
        "                        [--secs SECS] [--nsamps NSAMPS] [--dilv]\n"
        "                        [--channels CHANNELS]"
        "\n\n"
        "This example program demonstrates how to receive samples at a\n"
        "user-specified relative, future time using the UHD multi_usrp API.\n"
        "\n"
        "Note: This example program does not set the RX frequency; for your\n"
        "information, the actual RX frequency is displayed in the console.\n"
        "\n"
        "Key features:\n"
        "  - Supports multiple USRPs and multiple channels.\n"
        "  - Schedules timed reception to begin at a specified relative, future time.\n"
        "    - Initialize the time base of all USRP devices to zero.\n"
        "    - Use streaming metadata to schedule a timed receiption starting at\n"
        "      a specific relative, future time.\n"
        "  - Streams the received complex baseband data to a buffer for further\n"
        "    processing.\n"
        "  - Provides verbose packet status reporting.\n"
        "\n"
        "Note: Although this example program supports simultaneous reception from\n"
        "multiple USRP devices, it does not synchronize them to a common time\n"
        "reference such as an external PPS pulse.\n"
        "\n"
        "Usage example:\n"
        " - Receive 10,000 samples at 10 MSps, starting at USRP time 1.5 seconds\n"
        "   (i.e., 1.5 seconds in the future, since USRP time is initialized to\n"
        "    zero):\n"
        "     rx_timed_samples --args \"addr=192.168.10.2\" --rate 10e6\n"
        "                      --secs 1.5 --nsamps 10000\n";
    // variables to be set by po
    std::string args;
    std::string otw;
    double seconds_in_future;
    size_t total_num_samps;
    double rate;
    std::string channel_list;

    // setup the program options
    po::options_description desc("Allowed options");
    po::options_description alias_options;
    po::options_description all_options;
    // clang-format off
    desc.add_options()
        ("help,h", "Show this help message and exit.")
        ("args", po::value<std::string>(&args)->default_value(""), "USRP device selection and configuration "
            "arguments."
            "\nSpecify key-value pairs (e.g., addr, serial, type, master_clock_rate) separated by commas."
            "\nFor multi-device setups, specify multiple IP addresses (e.g., addr0, addr1) to group multiple USRPs into a "
            "single virtual device."
            "\nSee the UHD manual for model-specific options."
            "\nExamples:"
            "\n  --args \"addr=192.168.10.2\""
            "\n  --args \"addr=192.168.10.2,master_clock_rate=200e6\""
            "\n  --args \"addr0=192.168.10.2,addr1=192.168.10.3\""
            "\nIf not specified, UHD connects to the first available device.")
        ("otw", po::value<std::string>(&otw)->default_value(""), "Specifies the over-the-wire (OTW) data "
            "format used for transmission between the host and the USRP device. Common values are \"sc16\" (16-bit signed "
            "complex) and \"sc8\" (8-bit signed complex). Using \"sc8\" can reduce network bandwidth at the cost of "
            "dynamic range."
            "\nNote, that not all conversions between CPU and OTW formats are possible.")
        ("secs", po::value<double>(&seconds_in_future)->default_value(1.5), "Relative, future time in seconds, at which "
            "reception will start. Note that this example program initializes the USRP time base to zero, so this option sets a "
            "relative delay into the future.")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(10000), "Total number of samples to "
            "receive. The program stops when this number is reached.")
        ("rate,r", po::value<double>(&rate)->default_value(100e6/16), "RX sample rate in samples/second. Note "
            "that each USRP device only supports a set of discrete sample rates, which depend on the hardware model and "
            "configuration. If you request a rate that is not supported, the USRP device will automatically select and "
            "use the closest available rate.")
        ("dilv", "Disables inner-loop verbose status prints on received packets.")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "Specifies which channels to "
            "use. E.g. \"0\", \"1\", \"0,1\", etc.")
    ;
    alias_options.add_options()
        ("wire", po::value<std::string>(&otw), "") // alias for --otw for backward compatibility
    ;
    all_options.add(desc).add(alias_options);
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, all_options), vm);
    // print the help message
    if (vm.count("help")) {
        std::cout << program_doc << std::endl;
        std::cout << desc << std::endl;
        return ~0;
    }
    po::notify(vm); // only called if --help was not requested

    bool verbose = vm.count("dilv") == 0;

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    // detect which channels to use
    std::vector<std::string> channel_strings;
    std::vector<size_t> channel_nums;
    boost::split(channel_strings, channel_list, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < channel_strings.size(); ch++) {
        size_t chan = std::stoi(channel_strings[ch]);
        if (chan >= usrp->get_tx_num_channels() or chan >= usrp->get_rx_num_channels()) {
            throw std::runtime_error("Invalid channel(s) specified.");
        } else
            channel_nums.push_back(std::stoi(channel_strings[ch]));
    }

    // set the rx sample rate
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate() / 1e6)
              << std::endl
              << std::endl;

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    usrp->set_time_now(uhd::time_spec_t(0.0));

    // create a receive streamer
    uhd::stream_args_t stream_args("fc32", otw); // complex floats
    stream_args.channels             = channel_nums;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // setup streaming
    std::cout << std::endl;
    std::cout << boost::format("Begin streaming %u samples, %f seconds in the future...")
                     % total_num_samps % seconds_in_future
              << std::endl;
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = total_num_samps;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = uhd::time_spec_t(seconds_in_future);
    rx_stream->issue_stream_cmd(stream_cmd);

    // meta-data will be filled in by recv()
    uhd::rx_metadata_t md;

    // allocate buffer to receive with samples
    std::vector<std::complex<float>> buff(rx_stream->get_max_num_samps());
    std::vector<void*> buffs;
    for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); // same buffer for each channel

    // the first call to recv() will block this many seconds before receiving
    double timeout = seconds_in_future + 0.1; // timeout (delay before receive + padding)

    size_t num_acc_samps = 0; // number of accumulated samples
    while (num_acc_samps < total_num_samps) {
        // receive a single packet
        size_t num_rx_samps = rx_stream->recv(buffs, buff.size(), md, timeout, true);

        // use a small timeout for subsequent packets
        timeout = 0.1;

        // handle the error code
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT)
            break;
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            throw std::runtime_error(
                str(boost::format("Receiver error %s") % md.strerror()));
        }

        if (verbose)
            std::cout << boost::format(
                             "Received packet: %u samples, %u full secs, %f frac secs")
                             % num_rx_samps % md.time_spec.get_full_secs()
                             % md.time_spec.get_frac_secs()
                      << std::endl;

        num_acc_samps += num_rx_samps;
    }

    if (num_acc_samps < total_num_samps)
        std::cerr << "Receive timeout before all samples received..." << std::endl;

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
