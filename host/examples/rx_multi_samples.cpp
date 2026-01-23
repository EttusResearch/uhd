//
// Copyright 2011 Ettus Research LLC
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
#include <chrono>
#include <complex>
#include <iostream>
#include <thread>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string program_doc =
        "usage: rx_multi_samples [-h] [--args ARGS] [--secs SECS] [--nsamps NSAMPS]\n"
        "                        [-r RATE] [--sync {now,pps,mimo}] [--subdev SUBDEV]\n"
        "                        [--dilv] [--channels CHANNELS]"
        "\n\n"
        "This is a demonstration of how to receive time and frequency\n"
        "aligned data from multiple USRP devices and/or multiple channels.\n"
        "\n"
        "Note: This program does not set the RX frequency; for your information,\n"
        "      the actual RX frequency is displayed in the console.\n"
        "\n"
        "Key features:\n"
        "  - Supports simultaneous reception from multiple USRP devices and from\n"
        "    multiple RX channels.\n"
        "  - Handles device synchronization using PPS signal or MIMO cable. See\n"
        "    --sync option for details.\n"
        "\n"
        "Usage example:\n"
        "  - Capture baseband data synchronously from two USRP devices, each with\n"
        "    two channels, using a sampling rate of 10 MSps:\n"
        "      rx_multi_samples --args=\"addr0=192.168.10.2,addr1=192.168.10.3\"\n"
        "                       --rate 10e06 --channels \"0,1,2,3\" --sync pps\n";
    // variables to be set by po
    std::string args, sync, subdev, channel_list;
    double seconds_in_future;
    size_t total_num_samps;
    double rate;

    // setup the program options
    po::options_description desc("Allowed options");
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
        ("secs", po::value<double>(&seconds_in_future)->default_value(1.5), "USRP time in seconds, at which "
            "reception will start. Note that USRP time is set to zero during synchronization, so this option sets a "
            "relative delay from the sync point.")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(10000), "Total number of samples to "
            "receive. The program stops when this number is reached.")
        ("rate,r", po::value<double>(&rate)->default_value(100e6/16), "RX sample rate in samples/second. Note "
            "that each USRP device only supports a set of discrete sample rates, which depend on the hardware model and "
            "configuration. If you request a rate that is not supported, the USRP device will automatically select and "
            "use the closest available rate.")
        ("sync", po::value<std::string>(&sync)->default_value("now"), "Synchronization method:"
            "\n  now  - Start immediately, no synchronization"
            "\n  pps  - Synchronize to external Pulse Per Second (PPS) signal"
            "\n  mimo - Synchronize using a MIMO cable between USRPs")
        ("subdev", po::value<std::string>(&subdev), "RX subdevice configuration defining the mapping of "
            "channels to RF RX paths."
            "\nThe format and available values depend on your USRP model. If not specified, the channels will be numbered "
            "in order of the devices, daughterboard slots, and their RF RX channels."
            "\nFor typical applications, this default subdevice configuration is sufficient."
            "\nNote: this example program expects a single-USRP subdevice configuration which is applied to all USRPs "
            "equally, if multiple USRPs are configured."
            "\nExample:"
            "\nAssume we have an X310 with two UBX daughterboards installed. Then the default channel mapping is:"
            "\n  - Ch 0 -> A:0 (1st UBX in slot A, RF RX 0)"
            "\n  - Ch 1 -> B:0 (2nd UBX in slot B, RF RX 0)"
            "\nSpecifying --subdev=\"B:0 A:0\" would change the channel mapping to:"
            "\n  - Ch 0 -> B:0 (2nd UBX in slot B RF RX 0)"
            "\n  - Ch 1 -> A:0 (1st UBX in slot A RF RX 0)")
        ("dilv", "Disables inner-loop verbose status prints on received packets.")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "Specifies which channels to "
            "use. E.g. \"0\", \"1\", \"0,1\", etc.")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    // print the help message
    if (vm.count("help")) {
        std::cout << program_doc << std::endl;
        std::cout << desc << std::endl;
        return ~0;
    }
    po::notify(vm); // only called if --help was not requested

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD RX Multi Samples %s") % desc << std::endl;
        std::cout
            << "    This is a demonstration of how to receive aligned data from multiple "
               "channels.\n"
               "    This example can receive from multiple DSPs, multiple motherboards, "
               "or both.\n"
               "    The MIMO cable or PPS can be used to synchronize the configuration. "
               "See --sync\n"
               "\n"
               "    Specify --subdev to select multiple channels per motherboard.\n"
               "      Ex: --subdev=\"0:A 0:B\" to get 2 channels on a Basic RX.\n"
               "\n"
               "    Specify --args to select multiple motherboards in a configuration.\n"
               "      Ex: --args=\"addr0=192.168.10.2, addr1=192.168.10.3\"\n"
            << std::endl;
        return ~0;
    }

    bool verbose = vm.count("dilv") == 0;

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev"))
        usrp->set_rx_subdev_spec(subdev); // sets across all mboards

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    // set the rx sample rate (sets across all channels)
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate() / 1e6)
              << std::endl
              << std::endl;

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    if (sync == "now") {
        // This is not a true time lock, the devices will be off by a few RTT.
        // Rather, this is just to allow for demonstration of the code below.
        usrp->set_time_now(uhd::time_spec_t(0.0));
    } else if (sync == "pps") {
        usrp->set_time_source("external");
        usrp->set_time_unknown_pps(uhd::time_spec_t(0.0));
        std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for pps sync pulse
    } else if (sync == "mimo") {
        UHD_ASSERT_THROW(usrp->get_num_mboards() == 2);

        // make mboard 1 a slave over the MIMO Cable
        usrp->set_clock_source("mimo", 1);
        usrp->set_time_source("mimo", 1);

        // set time on the master (mboard 0)
        usrp->set_time_now(uhd::time_spec_t(0.0), 0);

        // sleep a bit while the slave locks its time to the master
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // detect which channels to use
    std::vector<std::string> channel_strings;
    std::vector<size_t> channel_nums;
    boost::split(channel_strings, channel_list, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < channel_strings.size(); ch++) {
        size_t chan = std::stoi(channel_strings[ch]);
        if (chan >= usrp->get_rx_num_channels()) {
            throw std::runtime_error("Invalid channel(s) specified.");
        } else
            channel_nums.push_back(std::stoi(channel_strings[ch]));
    }

    // create a receive streamer
    // linearly map channels (index0 = channel0, index1 = channel1, ...)
    uhd::stream_args_t stream_args("fc32"); // complex floats
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
    rx_stream->issue_stream_cmd(stream_cmd); // tells all channels to stream

    // meta-data will be filled in by recv()
    uhd::rx_metadata_t md;

    // allocate buffers to receive with samples (one buffer per channel)
    const size_t samps_per_buff = rx_stream->get_max_num_samps();
    std::vector<std::vector<std::complex<float>>> buffs(
        usrp->get_rx_num_channels(), std::vector<std::complex<float>>(samps_per_buff));

    // create a vector of pointers to point to each of the channel buffers
    std::vector<std::complex<float>*> buff_ptrs;
    for (size_t i = 0; i < buffs.size(); i++)
        buff_ptrs.push_back(&buffs[i].front());

    // the first call to recv() will block this many seconds before receiving
    double timeout = seconds_in_future + 0.1; // timeout (delay before receive + padding)

    size_t num_acc_samps = 0; // number of accumulated samples
    while (num_acc_samps < total_num_samps) {
        // receive a single packet
        size_t num_rx_samps = rx_stream->recv(buff_ptrs, samps_per_buff, md, timeout);

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
