//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018,2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <complex>
#include <csignal>
#include <iostream>

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string program_doc =
        "usage: tx_bursts [-h] [--args ARGS] [-f FREQ] [-r RATE] [--secs SECS]\n"
        "                      [--repeat] [--rep-delay REP_DELAY] [--nsamps NSAMPS]\n"
        "                      [--ampl AMPL] [--gain GAIN] [--dilv]\n"
        "                      [--channels CHANNELS] [--int-n] [--subdev SUBDEV]\n"
        "                      [--ref {internal,external,mimo,gpsdo}]\n"
        "                      [--lo-offset LO_OFFSET] [--bw BW]"
        "\n\n"
        "This example program illustrates how to use metadata to schedule and\n"
        "control timed burst transmission with a USRP device.\n"
        "It demonstrates how to transmit a sequence of timed bursts using the UHD\n"
        "multi_usrp API by configuring the USRP for transmission and scheduling\n"
        "multiple bursts of samples at precise hardware timestamps. The program\n"
        "shows the use of burst control flags, specifically the start_of_burst\n"
        "and end_of_burst fields in the transmit metadata, to mark the beginning\n"
        "and end of each burst, enabling precise burst-based signaling. This\n"
        "approach is essential for applications such as protocol emulation,\n"
        "radar, or time-slotted communication systems that require accurate\n"
        "timing and non-continuous transmissions.\n"
        "For demonstration, each burst transmits samples with constant I and Q\n"
        "value.\n"
        "\n"
        "Usage examples:\n"
        "  1. Transmits a single burst of 10,000 samples at 2.4 GHz, starting 1\n"
        "     second in the future, with a sample rate of 1 MSps and constant\n"
        "     baseband data with I and Q component having a value of 0.5:\n"
        "       tx_bursts --args=\"addr=192.168.10.2\" --freq=2.4e9 --rate=1e6\n"
        "                 --secs=1.0 --nsamps=10000 --ampl=0.5\n"
        "  2. Repeated transmission of bursts of 10,000 samples at 2.4 GHz,\n"
        "     starting 1 second in the future, with a sample rate of 1 MSps. Each\n"
        "     burst is repeated every 0.5 seconds until interrupted:\n"
        "       tx_bursts --args=\"addr=192.168.10.2\" --freq=2.4e9 --rate=1e6\n"
        "                 --secs=1.0 --nsamps=10000 --repeat --rep-delay=0.5\n";
    // variables to be set by po
    std::string args, channel_list, subdev, ref;
    double seconds_in_future, rate, freq, rep_rate, gain, lo_offset, bw;
    size_t total_num_samps;
    float ampl;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show this help message and exit.")
        ("args", po::value<std::string>(&args)->default_value(""), "Single USRP device selection and "
            "configuration arguments."
            "\nSpecify key-value pairs (e.g., addr, serial, type, master_clock_rate) separated by commas."
            "\nSee the UHD manual for model-specific options."
            "\nExamples:"
            "\n  --args \"addr=192.168.10.2\""
            "\n  --args \"addr=192.168.10.2,master_clock_rate=200e6\""
            "\nIf not specified, UHD connects to the first available device.")
        ("secs", po::value<double>(&seconds_in_future)->default_value(1.5), "Delay in seconds before "
            "transmitting the first burst.")
        ("repeat", "Continuously repeat the burst transmission until interrupted.")
        ("rep-delay", po::value<double>(&rep_rate)->default_value(0.5), "Time in seconds between scheduled "
            "start of each burst (start-to-start interval). Applies only if --repeat is configured.")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(10000), "Total number of samples to "
            "transmit per burst.")
        ("rate,r", po::value<double>(&rate)->default_value(100e6/16), "TX sample rate in samples/second. Note "
            "that each USRP device only supports a set of discrete sample rates, which depend on the hardware model and "
            "configuration. If you request a rate that is not supported, the USRP device will automatically select and "
            "use the closest available rate.")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "Value assigned to both I and Q "
            "component of each complex baseband sample (i.e., each sample is ampl + j*ampl).")
        ("freq,f", po::value<double>(&freq)->default_value(0), "RF center frequency in Hz.")
        ("gain", po::value<double>(&gain)->default_value(0), "TX gain for the RF chain in dB.")
        ("dilv", "Disables inner-loop verbose status prints.")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "Specifies which channels to "
            "use. E.g. \"0\", \"1\", \"0,1\", etc.")
        ("int-n", "Use integer-N tuning for USRP RF synthesizers. With this mode, the LO can only be tuned in "
            "discrete steps, which are integer multiples of the reference frequency. This mode can improve phase noise "
            "and spurious performance at the cost of coarser frequency resolution.")
        ("subdev", po::value<std::string>(&subdev), "TX subdevice configuration string, selecting which RF "
            "path to use for the receive channel."
            "\nThis example always uses channel 0; use --subdev to select which RF path is mapped to channel 0."
            "\nThe format and available values depend on your USRP model. If not specified, the default subdevice will be "
            "used."
            "\nExample:"
            "\nAssume we have an X310 with two UBX daughterboards installed."
            "\nBy default, channel 0 maps to A:0 (1st UBX in slot A, RF TX 0), which is equivalent to --subdev \"A:0\"."
            "\nTo use the 2nd UBX (slot B, RF TX 0), specify --subdev \"B:0\".")
        ("ref", po::value<std::string>(&ref), "Sets the source for the frequency reference. Available values "
            "depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("lo-offset", po::value<double>(&lo_offset)->default_value(0.0),
            "LO offset for the frontend in Hz.")
        ("bw", po::value<double>(&bw), "Sets the analog frontend filter bandwidth for the TX path in Hz. Not "
            "all USRP devices support programmable bandwidth; if an unsupported value is requested, the device will use "
            "the nearest supported bandwidth instead.")
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
    bool repeat  = vm.count("repeat") != 0;

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // Lock mboard clocks
    if (vm.count("ref")) {
        usrp->set_clock_source(ref);
    }

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev"))
        usrp->set_tx_subdev_spec(subdev);

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    // detect which channels to use
    std::vector<std::string> channel_strings;
    std::vector<size_t> channel_nums;
    boost::split(channel_strings, channel_list, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < channel_strings.size(); ch++) {
        size_t chan = std::stoi(channel_strings[ch]);
        if (chan >= usrp->get_tx_num_channels()) {
            throw std::runtime_error("Invalid channel(s) specified.");
        } else
            channel_nums.push_back(std::stoi(channel_strings[ch]));
    }

    // set the tx sample rate
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate() / 1e6)
              << std::endl
              << std::endl;

    // set the center frequency
    if (not vm.count("freq")) {
        std::cerr << "Please specify the center frequency with --freq" << std::endl;
        return ~0;
    }

    std::cout << "Requesting TX Freq: " << freq / 1e6 << " MHz..." << std::endl;
    std::cout << "Requesting TX LO Offset: " << lo_offset / 1e6 << " MHz..." << std::endl;

    for (size_t i = 0; i < channel_nums.size(); i++) {
        uhd::tune_request_t tune_request;
        tune_request = uhd::tune_request_t(freq, lo_offset);

        if (vm.count("int-n"))
            tune_request.args = uhd::device_addr_t("mode_n=integer");
        usrp->set_tx_freq(tune_request, channel_nums[i]);
    }
    std::cout << "Actual TX Freq: " << (usrp->get_tx_freq(channel_nums.front()) / 1e6)
              << " MHz..." << std::endl
              << std::endl;

    std::cout << "Requesting TX Gain: " << gain << " dB ..." << std::endl;
    for (size_t i = 0; i < channel_nums.size(); i++)
        usrp->set_tx_gain(gain, channel_nums[i]);
    std::cout << "Actual TX Gain: " << (usrp->get_tx_gain(channel_nums.front())) << "..."
              << std::endl
              << std::endl;

    // set the analog frontend filter bandwidth
    if (vm.count("bw")) {
        std::cout << "Requesting TX Bandwidth: " << (bw / 1e6) << " MHz..." << std::endl;
        usrp->set_tx_bandwidth(bw);
        std::cout << "Actual TX Bandwidth: "
                  << usrp->get_tx_bandwidth(channel_nums.front()) / 1e6 << " MHz..."
                  << std::endl
                  << std::endl;
    }

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    usrp->set_time_now(uhd::time_spec_t(0.0));

    // create a transmit streamer
    uhd::stream_args_t stream_args("fc32"); // complex floats
    stream_args.channels             = channel_nums;
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    // allocate buffer with data to send
    const size_t spb = tx_stream->get_max_num_samps();

    std::vector<std::complex<float>> buff(spb, std::complex<float>(ampl, ampl));
    std::vector<std::complex<float>*> buffs(channel_nums.size(), &buff.front());

    std::signal(SIGINT, &sig_int_handler);
    if (repeat)
        std::cout << "Press Ctrl + C to quit..." << std::endl;

    double time_to_send = seconds_in_future;

    do {
        // setup metadata for the first packet
        uhd::tx_metadata_t md;
        md.start_of_burst = true;
        md.end_of_burst   = false;
        md.has_time_spec  = true;
        md.time_spec      = uhd::time_spec_t(time_to_send);

        // the first call to send() will block this many seconds before sending:
        double timeout = std::max(rep_rate, seconds_in_future)
                         + 0.1; // timeout (delay before transmit + padding)

        size_t num_acc_samps = 0; // number of accumulated samples
        while (num_acc_samps < total_num_samps) {
            size_t samps_to_send = total_num_samps - num_acc_samps;
            if (samps_to_send > spb) {
                samps_to_send = spb;
            } else {
                md.end_of_burst = true;
            }

            // send a single packet
            size_t num_tx_samps = tx_stream->send(buffs, samps_to_send, md, timeout);
            // do not use time spec for subsequent packets
            md.has_time_spec  = false;
            md.start_of_burst = false;

            if (num_tx_samps < samps_to_send) {
                std::cerr << "Send timeout..." << std::endl;
                if (stop_signal_called) {
                    exit(EXIT_FAILURE);
                }
            }

            if (verbose) {
                std::cout << boost::format("Sent packet: %u samples") % num_tx_samps
                          << std::endl;
            }

            num_acc_samps += num_tx_samps;
        }

        time_to_send += rep_rate;

        std::cout << std::endl << "Waiting for async burst ACK... " << std::flush;
        uhd::async_metadata_t async_md;
        size_t acks = 0;
        // loop through all messages for the ACK packets (may have underflow messages in
        // queue)
        while (acks < channel_nums.size()
               and tx_stream->recv_async_msg(async_md, seconds_in_future)) {
            if (async_md.event_code == uhd::async_metadata_t::EVENT_CODE_BURST_ACK) {
                acks++;
            }
        }
        std::cout << (acks == channel_nums.size() ? "success" : "fail") << std::endl;
    } while (not stop_signal_called and repeat);

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
