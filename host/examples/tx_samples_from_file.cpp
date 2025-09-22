//
// Copyright 2011-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <complex>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

namespace po = boost::program_options;
namespace fs = std::filesystem;

static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

template <typename samp_type>
void send_from_file(
    uhd::tx_streamer::sptr tx_stream, const std::string& file, size_t samps_per_buff)
{
    uhd::tx_metadata_t md;
    md.start_of_burst = false;
    md.end_of_burst   = false;
    std::vector<samp_type> buff(samps_per_buff);
    std::vector<samp_type*> buffs(tx_stream->get_num_channels(), &buff.front());
    std::ifstream infile(file.c_str(), std::ifstream::binary);

    // loop until the entire file has been read

    while (not md.end_of_burst and not stop_signal_called) {
        infile.read((char*)&buff.front(), buff.size() * sizeof(samp_type));
        size_t num_tx_samps = size_t(infile.gcount() / sizeof(samp_type));

        md.end_of_burst = infile.eof();

        const size_t samples_sent = tx_stream->send(buffs, num_tx_samps, md);
        if (samples_sent != num_tx_samps) {
            UHD_LOG_ERROR("TX-STREAM",
                "The tx_stream timed out sending " << num_tx_samps << " samples ("
                                                   << samples_sent << " sent).");
            return;
        }
    }

    infile.close();
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // program documentation string
    const std::string program_doc =
        "usage: tx_samples_from_file [-h] [--args ARGS] --file FILE -r RATE -f FREQ\n"
        "                              [--type {double,float,short}] [--spb SPB]\n"
        "                              [--lo-offset LO_OFFSET] [--gain GAIN]\n"
        "                              [--power POWER] [--ant ANT] [--subdev SUBDEV]\n"
        "                              [--bw BW] [--ref {internal,external,mimo,gpsdo}]\n"
        "                              [--otw {sc16,sc8}] [--delay DELAY]\n"
        "                              [--channel CHANNEL] [--channels CHANNELS]\n"
        "                              [--repeat] [--int-n]"
        "\n\n"
        "This example demonstrates how to transmit samples from a binary file using\n"
        "the UHD multi_usrp API. It allows you to play back pre-recorded or\n"
        "arbitrary baseband data through a multiple TX channels and multiple USRP\n"
        "devices.\n"
        "\n"
        "Key features:\n"
        "  - Supports simultaneous transmission of complex baseband samples read\n"
        "    from file from multiple TX channels and multiple USRP devices.\n"
        "  - Supports multiple sample formats.\n"
        "  - Allows repeated playback with an optional delay between individual\n"
        "    transmissions of the file content.\n"
        "  - Configurable sample rate, frequency, gain, bandwidth, LO offset,\n"
        "    antenna, and more via command-line options.\n"
        "\n"
        "Supported file formats:\n"
        "  - Raw binary files containing complex samples in one of the following\n"
        "    formats: 'double' (64-bit float, fc64), 'float' (32-bit float, fc32),\n"
        "    or 'short' (16-bit integer, sc16, scaled to int16 range -32768 to 32767).\n"
        "  - The format is selected with the --type argument. The file must match\n"
        "    the selected type and contain interleaved IQ samples.\n"
        "\n"
        "How to create compatible files for playback:\n"
        "  - Use example rx_samples_to_file to record samples.\n"
        "  - Use Python, Matlab or C++ to generate binary files.\n"
        "    Using Python this can be done with numpy:\n"
        "      import numpy as np\n"
        "      # Create a complex array of samples\n"
        "      iq = np.random.randn(1000) + 1j * np.random.randn(1000)\n"
        "      # Write to a binary file\n"
        "      iq.astype(np.complex64).tofile('iq_data_fc32.bin')\n"
        "\n"
        "Usage examples:\n"
        " 1. Transmit samples from a 16-bit signed integer file at 2.4 GHz,\n"
        "    and repeat without delay:\n"
        "      tx_samples_from_file --args \"addr=192.168.10.2\" --freq 2.4e9\n"
        "                           --rate 10e6 --file \"iq_data_sc16.bin\"\n"
        "                           --type short --repeat\n"
        " 2. Transmit samples from a 32-bit float file, and repeat with 1s delay:\n"
        "      tx_samples_from_file --args \"addr=192.168.10.2\" --freq 2.4e9\n"
        "                           --rate 10e6 --file \"iq_data_fc32.bin\"\n"
        "                           --type float --repeat --delay 1.0\n"
        " 3. Transmit IQ samples for four channels from a file using two USRPs,\n"
        "    each with two channels, at a sample rate of 10 MHz:\n"
        "      tx_waveforms --args \"addr0=192.168.10.2,addr1=192.168.10.3\"\n"
        "                   --freq 2.4e9 --rate 10e6 --channels \"0,1,2,3\"\n"
        "                   --file \"iq_data_4chan_fc32.bin\" --type float\n";

    // variables to be set by po
    std::string args, file, type, ant, subdev, ref, otw, channels;
    size_t spb, single_channel;
    double rate, freq, gain, power, bw, delay, lo_offset;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show this help message and exit.")
        ("args", po::value<std::string>(&args)->default_value(""), "USRP device arguments, which holds "
            "multiple key-value pairs separated by commas."
            "\nFor a list of available options for a specific USRP model, see the UHD manual."
            "\nFor USRPs supporting multi-device operation, this option is also used to define how multiple USRPs are "
            "grouped to form a single virtual USRP device."
            "\nIf not specified, UHD will connect to the first device it can find."
            "\nExamples:"
            "\n  --args \"type=b200,serial=30A\" (single device)"
            "\n  --args \"addr0=192.168.10.2,addr1=192.168.10.3\" (multiple devices)")
        ("file", po::value<std::string>(&file)->required(), "Name of the raw binary file to read and transmit "
            "data from. The file must use the data format indicated by the --type argument.")
        ("type", po::value<std::string>(&type)->default_value("short"), "Specifies the data format of the "
            "file specified by the --file option. The file must contain interleaved IQ samples in order I0_Ch0, Q0_Ch0, "
            "I0_Ch1, Q0_Ch1, ..., I0_ChN, Q0_ChN, I1_Ch0, Q1_Ch0, ... and in one of the following numeric formats: "
            "'double' (64-bit float, fc64), 'float' (32-bit float, fc32), or 'short' (16-bit integer, sc16, scaled to "
            "int16 range -32768 to 32767).")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "Size of the host data buffer that is "
            "allocated for each Tx channel."
            "\nLarger values may improve throughput. Typical value is between 1,000 and 10,000 samples.")
        ("rate,r", po::value<double>(&rate)->required(), "Sample rate in samples/second. Note that each USRP "
            "device only supports a set of discrete sample rates, which depend on the hardware model and configuration. "
            "If you request a rate that is not supported, the USRP device will automatically select and use the closest "
            "available rate.")
        ("freq,f", po::value<double>(&freq)->required(), "RF center frequency in Hz.")
        ("lo-offset", po::value<double>(&lo_offset)->default_value(0.0),
            "LO offset for the frontend in Hz.")
        ("gain", po::value<double>(&gain), "Gain for the RF chain in dB. Will be ignored, if --power is "
            "specified.")
        ("power", po::value<double>(&power), "Transmit power in dBm."
            "\nThis option is available only, if it is supported by the USRP. An error is returned otherwise.")
        ("ant", po::value<std::string>(&ant), "Antenna port selection string selecting a specific antenna "
            "port for USRP daughterboards having multiple antenna connectors per RF channel."
            "\nExample: --ant \"TX/RX\"")
        ("subdev", po::value<std::string>(&subdev), "TX subdevice configuration defining the mapping of "
            "channels to RF TX paths."
            "\nThe format and available values depend on your USRP model. If not specified, the channels will be numbered "
            "in order of the devices, daughterboard slots, and their RF TX channels."
            "\nFor typical applications, this default subdevice configuration is sufficient."
            "\nNote: this example program expects a single-USRP subdevice configuration which is applied to all USRPs "
            "equally, if multiple USRPs are configured."
            "\nExample:"
            "\nAssume we have an X310 with two CBX daughterboards installed. Then the default channel mapping is:"
            "\n  - Ch 0 -> A:0 (1st CBX in slot A, RF TX 0)"
            "\n  - Ch 1 -> B:0 (2nd CBX in slot B, RF TX 0)"
            "\nSpecifying --subdev=\"B:0 A:0\" would change the channel mapping to:"
            "\n  - Ch 0 -> B:0 (2nd CBX in slot B RF TX 0)"
            "\n  - Ch 1 -> A:0 (1st CBX in slot A RF TX 0)")
        ("bw", po::value<double>(&bw), "Sets the analog frontend filter bandwidth in Hz. Not all USRP devices "
            "support programmable bandwidth; if an unsupported value is requested, the device will use the nearest "
            "supported bandwidth instead.")
        ("ref", po::value<std::string>(&ref), "Sets the source for the frequency reference. Available values "
            "depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("otw", po::value<std::string>(&otw)->default_value("sc16"), "Specifies the over-the-wire (OTW) data "
            "format used for transmission between the host and the USRP device. Common values are \"sc16\" (16-bit signed "
            "complex) and \"sc8\" (8-bit signed complex). Using \"sc8\" can reduce network bandwidth at the cost of "
            "dynamic range."
            "\nNote, that not all conversions between CPU and OTW formats are possible.")
        ("repeat", "Enables repeated transmission of the file data. Optionally having a --delay in between "
            "repetitions.")
        ("delay", po::value<double>(&delay)->default_value(0.0), "Specify a delay between repeated "
            "transmission of the file data (in seconds). Requires repeated transmission enabled by the --repeat flag.")
        ("channel", po::value<size_t>(&single_channel), "Specifies which single channel to use. E.g. \"0\", "
            "\"1\". This option cannot be used together with --channels option.")
        ("channels", po::value<std::string>(&channels), "Specifies which channels to use. E.g. \"0\", \"1\", "
            "\"0,1\", etc.  This option cannot be used together with --channel option.")
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

    bool repeat = vm.count("repeat") > 0;

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // Channels
    std::vector<size_t> channel_nums;
    std::vector<std::string> channels_split;
    if (vm.count("channel")) {
        if (vm.count("channels")) {
            std::cout << "ERROR: Cannot specify 'channel' and 'channels'!" << std::endl;
            return EXIT_FAILURE;
        }
        if (single_channel >= usrp->get_tx_num_channels())
            throw std::runtime_error("Invalid channel specified.");
        channel_nums.push_back(single_channel);
    } else {
        // Provide default
        if (!vm.count("channels"))
            channels = "0";
        // Split string into 1 or more channels
        boost::split(channels_split, channels, boost::is_any_of("\"',"));
        for (std::string channel : channels_split) {
            if (boost::lexical_cast<size_t>(channel) >= usrp->get_tx_num_channels())
                throw std::runtime_error("Invalid channel(s) specified.");
            channel_nums.push_back(boost::lexical_cast<size_t>(channel));
        }
    }

    // Lock mboard clocks
    if (vm.count("ref")) {
        usrp->set_clock_source(ref);
    }

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev"))
        usrp->set_tx_subdev_spec(subdev);

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    // set the sample rate
    if (not vm.count("rate")) {
        std::cerr << "Please specify the sample rate with --rate" << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    for (std::size_t channel : channel_nums) {
        usrp->set_tx_rate(rate, channel);
        std::cout << boost::format("Actual TX Rate: %f Msps...")
                         % (usrp->get_tx_rate(channel) / 1e6)
                  << std::endl
                  << std::endl;
    }

    // set the center frequency
    if (not vm.count("freq")) {
        std::cerr << "Please specify the center frequency with --freq" << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting TX Freq: %f MHz...") % (freq / 1e6) << std::endl;
    std::cout << boost::format("Setting TX LO Offset: %f MHz...") % (lo_offset / 1e6)
              << std::endl;
    uhd::tune_request_t tune_request;
    tune_request = uhd::tune_request_t(freq, lo_offset);
    if (vm.count("int-n"))
        tune_request.args = uhd::device_addr_t("mode_n=integer");
    for (std::size_t channel : channel_nums) {
        usrp->set_tx_freq(tune_request, channel);
        std::cout << boost::format("Actual TX Freq: %f MHz...")
                         % (usrp->get_tx_freq(channel) / 1e6)
                  << std::endl
                  << std::endl;
    }

    // set the rf gain
    if (vm.count("power")) {
        for (std::size_t channel : channel_nums) {
            if (!usrp->has_tx_power_reference(channel)) {
                std::cout << "ERROR: USRP does not have a reference power API on channel "
                          << channel << "!" << std::endl;
                return EXIT_FAILURE;
            }
            std::cout << "Setting TX output power: " << power << " dBm..." << std::endl;
            usrp->set_tx_power_reference(power, channel);
            std::cout << "Actual TX output power: "
                      << usrp->get_tx_power_reference(channel) << " dBm..." << std::endl;
        }

        if (vm.count("gain")) {
            std::cout << "WARNING: If you specify both --power and --gain, "
                         " the latter will be ignored."
                      << std::endl;
        }
    } else if (vm.count("gain")) {
        for (std::size_t channel : channel_nums) {
            std::cout << boost::format("Setting TX Gain: %f dB...") % gain << std::endl;
            usrp->set_tx_gain(gain, channel);
            std::cout << boost::format("Actual TX Gain: %f dB...")
                             % usrp->get_tx_gain(channel)
                      << std::endl
                      << std::endl;
        }
    }

    // set the analog frontend filter bandwidth
    if (vm.count("bw")) {
        std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % (bw / 1e6)
                  << std::endl;
        for (std::size_t channel : channel_nums) {
            usrp->set_tx_bandwidth(bw, channel);
            std::cout << boost::format("Actual TX Bandwidth: %f MHz...")
                             % (usrp->get_tx_bandwidth(channel) / 1e6)
                      << std::endl
                      << std::endl;
        }
    }

    // set the antenna
    if (vm.count("ant")) {
        for (std::size_t channel : channel_nums) {
            usrp->set_tx_antenna(ant, channel);
        }
    }
    // allow for some setup time:
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Check Ref and LO Lock detect
    std::vector<std::string> sensor_names;
    for (std::size_t channel : channel_nums) {
        sensor_names = usrp->get_tx_sensor_names(channel);
        if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked")
            != sensor_names.end()) {
            uhd::sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked", channel);
            std::cout << boost::format("Checking TX: %s ...") % lo_locked.to_pp_string()
                      << std::endl;
            UHD_ASSERT_THROW(lo_locked.to_bool());
        }
    }
    sensor_names = usrp->get_mboard_sensor_names(0);
    if ((ref == "mimo")
        and (std::find(sensor_names.begin(), sensor_names.end(), "mimo_locked")
             != sensor_names.end())) {
        uhd::sensor_value_t mimo_locked = usrp->get_mboard_sensor("mimo_locked", 0);
        std::cout << boost::format("Checking TX: %s ...") % mimo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(mimo_locked.to_bool());
    }
    if ((ref == "external")
        and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked")
             != sensor_names.end())) {
        uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked", 0);
        std::cout << boost::format("Checking TX: %s ...") % ref_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(ref_locked.to_bool());
    }

    // set sigint if user wants to receive
    if (repeat) {
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }

    // create a transmit streamer
    std::string cpu_format;
    if (type == "double")
        cpu_format = "fc64";
    else if (type == "float")
        cpu_format = "fc32";
    else if (type == "short")
        cpu_format = "sc16";

    uhd::stream_args_t stream_args(cpu_format, otw);
    stream_args.channels             = channel_nums;
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    // send from file
    if (!fs::exists(file)) {
        std::cerr << "ERROR: File not found: " << file << std::endl;
        return EXIT_FAILURE;
    }
    do {
        if (type == "double")
            send_from_file<std::complex<double>>(tx_stream, file, spb);
        else if (type == "float")
            send_from_file<std::complex<float>>(tx_stream, file, spb);
        else if (type == "short")
            send_from_file<std::complex<short>>(tx_stream, file, spb);
        else
            throw std::runtime_error("Unknown type " + type);

        if (repeat and delay > 0.0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(delay * 1000)));
        }
    } while (repeat and not stop_signal_called);

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
