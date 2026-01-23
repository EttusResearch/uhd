//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <complex>
#include <iostream>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // program documentation string
    const std::string program_doc =
        "usage: tx_timed_samples [--help] [--args ARGS] [--secs SECS] [--nsamps NSAMPS]\n"
        "                        [--rate RATE] [--ampl AMPL] [--otw {sc16,sc8}]\n"
        "                        [--dilv]"
        "\n\n"
        "This example demonstrates how to stream complex baseband data from a local\n"
        "data buffer and schedule it for transmission at a user-specified USRP time\n"
        "using the UHD multi_usrp API.\n"
        "For demonstration purposes, the program transmits a constant complex value\n"
        "on a single channel of a single USRP device.\n"
        "\n"
        "Note: This program does not set the TX frequency; for your information,\n"
        "      the actual TX frequency is displayed in the console.\n"
        "\n"
        "Key features:\n"
        "  - Set the USRP time to 0.\n"
        "  - Use metadata to schedule a timed transmissions starting at a specific\n"
        "    USRP device time.\n"
        "\n"
        "Usage example:\n"
        " - Transmit 10,000 samples at 6.25 MSps, scheduled for USRP time 1.5 seconds\n"
        "   (i.e., 1.5 seconds in the future, since USRP time is initialized to zero):\n"
        "     tx_timed_samples --args \"addr=192.168.10.2\" --rate 6.25e6 --secs 1.5\n"
        "                      --nsamps 10000\n";

    // variables to be set by po
    std::string args;
    std::string otw;
    double seconds_in_future;
    size_t total_num_samps;
    double rate;
    float ampl;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "Show this help message and exit.")
        ("args", po::value<std::string>(&args)->default_value(""), "USRP device arguments, which holds "
            "multiple key-value pairs separated by commas."
            "\nFor a list of available options for a specific USRP model, see the UHD manual."
            "\nIf not specified, UHD will connect to the first device it can find."
            "\nExample:"
            "\n  --args \"type=b200,serial=30A\"")
        ("secs", po::value<double>(&seconds_in_future)->default_value(1.5), "USRP device time in seconds at "
            "which transmission will start. This is relative to the device time, which is initialized to zero by this "
            "program. For example, --secs 1.5 schedules transmission 1.5 seconds in the future.")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(10000), "Total number of samples to "
            "transmit. Transmission stops when this number is reached.")
        ("rate", po::value<double>(&rate)->default_value(100e6/16), "Sample rate in samples/second. Note that "
            "each USRP device only supports a set of discrete sample rates, which depend on the hardware model and "
            "configuration. If you request a rate that is not supported, the USRP device will automatically select and "
            "use the closest available rate.")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "The amplitude of the complex baseband "
            "signal that is scheduled for transmission.")
        ("otw", po::value<std::string>(&otw)->default_value(""), "Specifies the over-the-wire (OTW) data "
            "format used for transmission between the host and the USRP device. Common values are \"sc16\" (16-bit signed "
            "complex) and \"sc8\" (8-bit signed complex). Using \"sc8\" can reduce network bandwidth at the cost of "
            "dynamic range."
            "\nNote, that not all conversions between CPU and OTW formats are possible.")
        ("dilv", "Disables inner-loop verbose status prints on transmitted packets.")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << program_doc << std::endl;
        std::cout << desc << std::endl;
        return ~0;
    }

    bool verbose = vm.count("dilv") == 0;

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    // print the actual TX frequency
    std::cout << boost::format("Actual TX Freq: %f MHz") % (usrp->get_tx_freq() / 1e06)
              << std::endl;

    // set the tx sample rate
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate() / 1e6)
              << std::endl
              << std::endl;

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    usrp->set_time_now(uhd::time_spec_t(0.0));

    // create a transmit streamer
    uhd::stream_args_t stream_args("fc32", otw); // complex floats
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    // allocate buffer with data to send
    std::vector<std::complex<float>> buff(
        tx_stream->get_max_num_samps(), std::complex<float>(ampl, ampl));

    // setup metadata for the first packet
    uhd::tx_metadata_t md;
    md.start_of_burst = false;
    md.end_of_burst   = false;
    md.has_time_spec  = true;
    md.time_spec      = uhd::time_spec_t(seconds_in_future);

    // the first call to send() will block this many seconds before sending:
    const double timeout =
        seconds_in_future + 0.1; // timeout (delay before transmit + padding)

    size_t num_acc_samps = 0; // number of accumulated samples
    while (num_acc_samps < total_num_samps) {
        size_t samps_to_send = std::min(total_num_samps - num_acc_samps, buff.size());

        // send a single packet
        size_t num_tx_samps = tx_stream->send(&buff.front(), samps_to_send, md, timeout);

        // do not use time spec for subsequent packets
        md.has_time_spec = false;

        if (num_tx_samps < samps_to_send)
            std::cerr << "Send timeout..." << std::endl;
        if (verbose)
            std::cout << boost::format("Sent packet: %u samples") % num_tx_samps
                      << std::endl;

        num_acc_samps += num_tx_samps;
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send("", 0, md);

    std::cout << std::endl << "Waiting for async burst ACK... " << std::flush;
    uhd::async_metadata_t async_md;
    bool got_async_burst_ack = false;
    // loop through all messages for the ACK packet (may have underflow messages in queue)
    while (not got_async_burst_ack and tx_stream->recv_async_msg(async_md, timeout)) {
        got_async_burst_ack =
            (async_md.event_code == uhd::async_metadata_t::EVENT_CODE_BURST_ACK);
    }
    std::cout << (got_async_burst_ack ? "success" : "fail") << std::endl;

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
