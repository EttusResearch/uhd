//
// Copyright 2010-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
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
        "usage: rx_samples_to_udp [-h] [--args ARGS] [-f FREQ] [-r RATE]\n"
        "                         [--nsamps NSAMPS] [--gain GAIN] [--ant ANT]\n"
        "                         [--subdev SUBDEV] [--bw BW] [--port PORT]\n"
        "                         [--addr ADDR]\n"
        "                         [--ref {internal,external,mimo,gpsdo}]\n"
        "                         [--int-n]"
        "\n\n"
        "This example program configures a single USRP device to receive\n"
        "RF signals on one channel, stores the resulting complex baseband data in\n"
        "a local buffer on the host computer, and streams the data from that\n"
        "buffer over UDP to a specified network destination.\n"
        "It is suitable for applications where received IQ data needs to be\n"
        "transmitted over a network for remote processing, analysis, or\n"
        "recording.\n"
        "The captured baseband samples are streamed in fc32 data format (32-bit\n"
        "float) with I and Q being interleaved.\n"
        "\n"
        "Note: The USRP may round the requested sample rate to the nearest\n"
        "      supported value; use the actual rate shown in the console for\n"
        "      subsequent processing of the data.\n"
        "\n"
        "Usage example:\n"
        "  Below, we demonstrate how this example program can be used to forward\n"
        "  baseband samples received from the USRP via UDP to a local UDP server,\n"
        "  which stores the data in a file for subsequent analysis.\n"
        "  Step 1: Use netcat to set up a UDP server that listens on port\n"
        "          7124 and redirect the output to a file:\n"
        "          netcat -ul -p 7124 > usrp_samples_fc32.dat\n"
        "  Step 2: Run this example with UDP destination localhost and port 7124\n"
        "          rx_samples_to_udp --args \"addr=192.168.10.2\" --freq 2.4e09\n"
        "                            --rate 7.68e06 --addr localhost --port 7124\n"
        "                            --nsamps 10000\n"
        "  Step 3: Stop the UDP server and check the written file and optionally\n"
        "          do further processing of the data.\n";
    // variables to be set by po
    std::string args, file, ant, subdev, ref;
    size_t total_num_samps;
    double rate, freq, gain, bw;
    std::string addr, port;

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
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(1000), "Total number of samples to "
            "receive. The program stops when this number is reached.")
        ("rate,r", po::value<double>(&rate)->default_value(100e6/16), "Sample rate in samples/second. Note that each USRP "
            "device only supports a set of discrete sample rates, which depend on the hardware model and configuration. "
            "If you request a rate that is not supported, the USRP device will automatically select and use the closest "
            "available rate. For accurate processing of received baseband data, always verify and use the actual sample "
            "rate.")
        ("freq,f", po::value<double>(&freq)->default_value(0), "RF center frequency in Hz.")
        ("gain", po::value<double>(&gain)->default_value(0), "RX gain for the RF chain in dB.")
        ("ant", po::value<std::string>(&ant), "Antenna port selection string selecting a specific antenna "
            "port for USRP daughterboards having multiple antenna connectors per RF channel."
            "\nExample: --ant \"TX/RX\"")
        ("subdev", po::value<std::string>(&subdev), "RX subdevice configuration string, selecting which RF "
            "path to use for the receive channel."
            "\nThis example program always uses channel 0; use --subdev to select which RF path is mapped to channel 0."
            "\nThe format and available values depend on your USRP model. If not specified, the default subdevice will be "
            "used."
            "\nExample:"
            "\nAssume we have an X310 with two UBX daughterboards installed."
            "\nBy default, channel 0 maps to A:0 (1st UBX in slot A, RF RX 0), which is equivalent to --subdev \"A:0\"."
            "\nTo use the 2nd UBX (slot B, RF RX 0), specify --subdev \"B:0\".")
        ("bw", po::value<double>(&bw), "Sets the analog frontend filter bandwidth for the RX path in Hz. Not "
            "all USRP devices support programmable bandwidth; if an unsupported value is requested, the device will use "
            "the nearest supported bandwidth instead.")
        ("port", po::value<std::string>(&port)->default_value("7124"), "Server UDP port.")
        ("addr", po::value<std::string>(&addr)->default_value("192.168.1.10"), "Server address (hostname or "
            "IP).")
        ("ref", po::value<std::string>(&ref), "Sets the source for the frequency reference. Available values "
            "depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("int-n", "Use integer-N tuning for USRP RF synthesizers. With this mode, the LO can only be tuned in "
            "discrete steps, which are integer multiples of the reference frequency. This mode can improve phase noise "
            "and spurious performance at the cost of coarser frequency resolution.")
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

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    // Lock mboard clocks
    if (vm.count("ref")) {
        usrp->set_clock_source(ref);
    }

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev")) {
        usrp->set_rx_subdev_spec(subdev);
    }

    // set the rx sample rate
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate() / 1e6)
              << std::endl
              << std::endl;

    // set the rx center frequency
    std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq / 1e6) << std::endl;
    uhd::tune_request_t tune_request(freq);
    if (vm.count("int-n"))
        tune_request.args = uhd::device_addr_t("mode_n=integer");
    usrp->set_rx_freq(tune_request);
    std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq() / 1e6)
              << std::endl
              << std::endl;

    // set the rx rf gain
    std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
    usrp->set_rx_gain(gain);
    std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_rx_gain()
              << std::endl
              << std::endl;

    // set the analog frontend filter bandwidth
    if (vm.count("bw")) {
        std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw / 1e6)
                  << std::endl;
        usrp->set_rx_bandwidth(bw);
        std::cout << boost::format("Actual RX Bandwidth: %f MHz...")
                         % (usrp->get_rx_bandwidth() / 1e6)
                  << std::endl
                  << std::endl;
    }

    // set the antenna
    if (vm.count("ant"))
        usrp->set_rx_antenna(ant);

    std::this_thread::sleep_for(std::chrono::seconds(1)); // allow for some setup time

    // Check Ref and LO Lock detect
    std::vector<std::string> sensor_names;
    sensor_names = usrp->get_rx_sensor_names(0);
    if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked")
        != sensor_names.end()) {
        uhd::sensor_value_t lo_locked = usrp->get_rx_sensor("lo_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % lo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }
    sensor_names = usrp->get_mboard_sensor_names(0);
    if ((ref == "mimo")
        and (std::find(sensor_names.begin(), sensor_names.end(), "mimo_locked")
             != sensor_names.end())) {
        uhd::sensor_value_t mimo_locked = usrp->get_mboard_sensor("mimo_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % mimo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(mimo_locked.to_bool());
    }
    if ((ref == "external")
        and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked")
             != sensor_names.end())) {
        uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % ref_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(ref_locked.to_bool());
    }

    // create a receive streamer
    uhd::stream_args_t stream_args("fc32"); // complex floats
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // setup streaming
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = total_num_samps;
    stream_cmd.stream_now = true;
    rx_stream->issue_stream_cmd(stream_cmd);

    // loop until total number of samples reached
    size_t num_acc_samps = 0; // number of accumulated samples
    uhd::rx_metadata_t md;
    std::vector<std::complex<float>> buff(rx_stream->get_max_num_samps());
    uhd::transport::udp_simple::sptr udp_xport =
        uhd::transport::udp_simple::make_connected(addr, port);

    while (num_acc_samps < total_num_samps) {
        size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md);

        // handle the error codes
        switch (md.error_code) {
            case uhd::rx_metadata_t::ERROR_CODE_NONE:
                break;

            case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
                if (num_acc_samps == 0)
                    continue;
                std::cout << boost::format("Got timeout before all samples received, "
                                           "possible packet loss, exiting loop...")
                          << std::endl;
                goto done_loop;

            default:
                std::cout << boost::format("Got error code 0x%x, exiting loop...")
                                 % md.error_code
                          << std::endl;
                goto done_loop;
        }

        // send complex single precision floating point samples over udp
        udp_xport->send(boost::asio::buffer(buff, num_rx_samps * sizeof(buff.front())));

        num_acc_samps += num_rx_samps;
    }
done_loop:

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
