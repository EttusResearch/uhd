//
// Copyright 2010-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "wavetable.hpp"
#include <uhd/exception.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/thread.hpp>
#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

namespace po = boost::program_options;

/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

/***********************************************************************
 * Main function
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // program documentation string
    const std::string program_doc =
        "usage: tx_waveforms [-h] -r RATE -f FREQ [--args ARGS] [--spb SPB]\n"
        "                    [--nsamps NSAMPS] [--lo-offset LO_OFFSET] [--gain GAIN]\n"
        "                    [--power POWER] [--ant ANT] [--subdev SUBDEV] [--bw BW]\n"
        "                    [--wave-type {CONST,SQUARE,RAMP,SINE}]\n"
        "                    [--wave-freq WAVE_FREQ] [--wave-ampl WAVE_AMPL]\n"
        "                    [--ref {internal,external,mimo,gpsdo}]\n"
        "                    [--pps {internal,external,mimo,gpsdo}] [--otw {sc16,sc8}]\n"
        "                    [--channels CHANNELS] [--int-n]"
        "\n\n"
        "This example demonstrates how to stream waveforms from a local data buffer\n"
        "using the UHD multi_usrp API. It supports synchronized transmission from\n"
        "multiple TX channels and multiple USRP devices.\n"
        "\n"
        "Key features:\n"
        "  - Supports simultaneous transmission from multiple USRP devices and from\n"
        "    multiple TX channels.\n"
        "  - Streams data from a local buffer directly to the USRP device(s).\n"
        "  - Allows selection of waveform types CONST, SINE, SQUARE, and RAMP.\n"
        "  - Configurable sample rate, frequency, gain, bandwidth, LO offset,\n"
        "    antenna, and more via command-line options.\n"
        "  - Handles device clock and time reference selection for synchronization.\n"
        "  - Demonstrates timed streaming for synchronized multi-channel/MIMO TX.\n"
        "\n"
        "Usage examples:\n"
        " 1. Single-USRP, single-channel transmission of default waveform (const)\n"
        "    at carrier frequency of 2.4 GHz and at a rate of 10 MSps:\n"
        "      tx_waveforms --args=\"addr=192.168.10.2\" --freq 2.4e9 --rate 10e6\n"
        " 2. Single-USRP, dual-channel transmission of a 1 MHz sine wave\n"
        "    at carrier frequency of 2.4 GHz and at a rate of 10 MSps:\n"
        "      tx_waveforms --args=\"addr=192.168.10.2\" --freq 2.4e9 --rate 10e6\n"
        "                   --channels \"0,1\" --wave-type SINE --wave-freq 1e6\n"
        " 3. Two USRPs transmitting a 1 MHz sine wave synchronously on two channels\n"
        "    each at a carrier frequency of 2.4 GHz and at a rate of 10 MSps:\n"
        "      tx_waveforms --args=\"addr=192.168.10.2,addr=192.168.10.3\" --freq 2.4e9\n"
        "                   --rate 10e6 --channels \"0,1,2,3\" --wave-type SINE\n"
        "                   --wave-freq 1e6 --pps mimo\n";
    // variables to be set by po
    std::string args, wave_type, ant, subdev, ref, pps, otw, channel_list;
    uint64_t total_num_samps;
    size_t spb;
    double rate, freq, gain, power, wave_freq, bw, lo_offset;
    float ampl;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show this help message and exit.")
        ("args", po::value<std::string>(&args)->default_value(""), "USRP device arguments, which holds "
            "multiple key value pairs separated by commas."
            "\nFor a list of available options for a specific USRP model, see the UHD manual."
            "\nFor USRPs supporting multi device operation, this option is also used to define how multiple USRPs are "
            "grouped to form a single virtual USRP device."
            "\nIf not specified, UHD will connect to the first device it can find."
            "\nExamples:"
            "\n  --args \"type=b200,serial=30A\" (single device)"
            "\n  --args \"addr0=192.168.10.2,addr1=192.168.10.3\" (multiple devices)")
        ("spb", po::value<size_t>(&spb)->default_value(0), "Size of the host data buffer that is allocated "
            "for each Tx channel."
            "\nLarger values may improve throughput. Typical value is between 1,000 and 10,000 samples. If not specified, "
            "the size is determined automatically based on the maximum number of samples per buffer supported by the UHD "
            "transmit streamer.")
        ("nsamps", po::value<uint64_t>(&total_num_samps)->default_value(0), "Total number of samples to "
            "transmit. Transmission stops when this number is reached. If set to 0, data will be continuously "
            "transmitted.")
        ("rate,r", po::value<double>(&rate)->required(), "Sample rate in samples/second.")
        ("freq,f", po::value<double>(&freq)->required(), "RF center frequency in Hz.")
        ("lo-offset", po::value<double>(&lo_offset)->default_value(0.0),
            "LO offset for the frontend in Hz.")
        ("gain", po::value<double>(&gain), "Gain for the RF chain in dB. Will be ignored, if --power is "
            "specified.")
        ("power", po::value<double>(&power), "Transmit power in dBm."
            "\nThis option is available only, if it is supported by the USRP. An error is returned otherwise.")
        ("ant", po::value<std::string>(&ant), "Antenna port selection string selecting a specific antenna "
            "port for USRP daughterboards having multiple antenna connectors per RF channel."
            "\nNote: this example program expects a single antenna port selection string which is applied to all channels "
            "equally."
            "\nExample: --ant \"TX/RX\"")
        ("subdev", po::value<std::string>(&subdev), "TX subdevice configuration defining the mapping of "
            "channels to RF TX paths."
            "\nThe format and available values depend on your USRP model. If not specified, the channels will get numbered "
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
        ("bw", po::value<double>(&bw), "Analog frontend filter bandwidth in Hz."
            "\nNote that not all USRPs support programmable analog frontend bandwidth and this option might get ignored or "
            "throw an error.")
        ("wave-type", po::value<std::string>(&wave_type)->default_value("CONST"), "Baseband waveform type to "
            "generate."
            "\nAvailable types are CONST (real), SQUARE (real), RAMP (real), and SINE (complex)")
        ("wave-freq", po::value<double>(&wave_freq)->default_value(0), "Baseband waveform frequency in Hz."
            "\nThis option is required for waveform types SQUARE, RAMP, and SINE.")
        ("wave-ampl", po::value<float>(&ampl)->default_value(0.3), "Baseband waveform amplitude in the range "
            "[0 to 0.7].")
        ("ref", po::value<std::string>(&ref), "Sets the source for the frequency reference. Available values "
            "depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("pps", po::value<std::string>(&pps), "Specifies the PPS source for time synchronization. Available "
            "values depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("otw", po::value<std::string>(&otw)->default_value("sc16"), "Specifies the over-the-wire (OTW) data "
            "format used for transmission between the host and the USRP device. Common values are \"sc16\" (16-bit signed "
            "complex) and \"sc8\" (8-bit signed complex). Using \"sc8\" can reduce network bandwidth at the cost of "
            "dynamic range."
            "\nNote, that not all conversions between CPU and OTW formats are possible.")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "Specifies which channels to "
            "use. E.g. \"0\", \"1\", \"0,1\", etc.")
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

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev"))
        usrp->set_tx_subdev_spec(subdev, uhd::usrp::multi_usrp::ALL_MBOARDS);

    // detect which channels to use
    std::vector<std::string> channel_strings;
    std::vector<size_t> channel_nums;
    boost::split(channel_strings, channel_list, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < channel_strings.size(); ch++) {
        size_t chan = std::stoi(channel_strings[ch]);
        if (chan >= usrp->get_tx_num_channels())
            throw std::runtime_error("Invalid channel(s) specified.");
        else
            channel_nums.push_back(std::stoi(channel_strings[ch]));
    }

    // Lock mboard clocks
    if (vm.count("ref")) {
        usrp->set_clock_source(ref);
    }

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

    // for the const wave, set the wave freq for small samples per period
    if (wave_freq == 0) {
        if (wave_type == "CONST") {
            wave_freq = usrp->get_tx_rate(channel_nums.front()) / 2;
        } else {
            throw std::runtime_error(
                "wave freq cannot be 0 with wave type other than CONST");
        }
    }

    // pre-compute the waveform values
    const wave_table_class wave_table(wave_type, ampl);
    const size_t step =
        std::lround(wave_freq / usrp->get_tx_rate(channel_nums.front()) * wave_table_len);
    size_t index = 0;

    // Defer setting the frequency and LO offset until synchronization setup is complete,
    // configuring here only gains, bandwidth, and antenna
    for (std::size_t channel : channel_nums) {
        // set the rf gain
        if (vm.count("power")) {
            if (!usrp->has_tx_power_reference(channel)) {
                std::cout << "ERROR: USRP does not have a reference power API on channel "
                          << channel << "!" << std::endl;
                return EXIT_FAILURE;
            }
            std::cout << "Setting TX output power: " << power << " dBm..." << std::endl;
            usrp->set_tx_power_reference(power - wave_table.get_power(), channel);
            std::cout << "Actual TX output power: "
                      << usrp->get_tx_power_reference(channel) + wave_table.get_power()
                      << " dBm..." << std::endl;
            if (vm.count("gain")) {
                std::cout << "WARNING: If you specify both --power and --gain, "
                             " the latter will be ignored."
                          << std::endl;
            }
        } else if (vm.count("gain")) {
            std::cout << boost::format("Setting TX Gain: %f dB...") % gain << std::endl;
            usrp->set_tx_gain(gain, channel);
            std::cout << boost::format("Actual TX Gain: %f dB...")
                             % usrp->get_tx_gain(channel)
                      << std::endl
                      << std::endl;
        }

        // set the analog frontend filter bandwidth
        if (vm.count("bw")) {
            std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % bw
                      << std::endl;
            usrp->set_tx_bandwidth(bw, channel);
            std::cout << boost::format("Actual TX Bandwidth: %f MHz...")
                             % usrp->get_tx_bandwidth(channel)
                      << std::endl
                      << std::endl;
        }

        // set the antenna
        if (vm.count("ant"))
            usrp->set_tx_antenna(ant, channel);
    }

    // error when the waveform is not possible to generate
    if (std::abs(wave_freq) > usrp->get_tx_rate() / 2) {
        throw std::runtime_error("wave freq out of Nyquist zone");
    }
    if (usrp->get_tx_rate() / std::abs(wave_freq) > wave_table_len / 2) {
        throw std::runtime_error("wave freq too small for table");
    }

    // create a transmit streamer
    // linearly map channels (index0 = channel0, index1 = channel1, ...)
    uhd::stream_args_t stream_args("fc32", otw);
    stream_args.channels             = channel_nums;
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    // allocate a buffer which we re-use for each channel
    if (spb == 0) {
        spb = tx_stream->get_max_num_samps() * 10;
    }
    std::vector<std::complex<float>> buff(spb);
    std::vector<std::complex<float>*> buffs(channel_nums.size(), &buff.front());

    // pre-fill the buffer with the waveform
    for (size_t n = 0; n < buff.size(); n++) {
        buff[n] = wave_table(index += step);
    }

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    if (channel_nums.size() > 1) {
        // Sync times
        if (pps == "mimo") {
            UHD_ASSERT_THROW(usrp->get_num_mboards() == 2);

            // make mboard 1 a slave over the MIMO Cable
            usrp->set_time_source("mimo", 1);

            // set time on the master (mboard 0)
            usrp->set_time_now(uhd::time_spec_t(0.0), 0);

            // sleep a bit while the slave locks its time to the master
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            if (pps == "internal" or pps == "external" or pps == "gpsdo")
                usrp->set_time_source(pps);
            usrp->set_time_unknown_pps(uhd::time_spec_t(0.0));
            std::this_thread::sleep_for(
                std::chrono::seconds(1)); // wait for pps sync pulse
        }
    } else {
        usrp->set_time_now(0.0);
    }

    // Now, that clock sync setup is complete do timed tuning of LOs and NCOs
    std::cout << boost::format("Setting TX Freq: %f MHz...") % (freq / 1e6) << std::endl;
    std::cout << boost::format("Setting TX LO Offset: %f MHz...") % (lo_offset / 1e6)
              << std::endl;

    // use timed tuning for more than one channel
    const bool timed_tuning     = channel_nums.size() > 1;
    const float cmd_time_offset = 0.1;

    if (timed_tuning) {
        const uhd::time_spec_t now      = usrp->get_time_now();
        const uhd::time_spec_t cmd_time = now + uhd::time_spec_t(cmd_time_offset);
        usrp->set_command_time(cmd_time);
    }

    for (std::size_t channel : channel_nums) {
        uhd::tune_request_t tune_request(freq, lo_offset);
        if (vm.count("int-n"))
            tune_request.args = uhd::device_addr_t("mode_n=integer");
        usrp->set_tx_freq(tune_request, channel);
    }

    // clear command time and wait to finish for timed tuning
    if (timed_tuning) {
        usrp->clear_command_time();
        std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(
            1e3 * cmd_time_offset))); // wait until the command time has passed for sure
    }

    for (std::size_t channel : channel_nums) {
        std::cout << boost::format("Actual TX Freq: %f MHz...")
                         % (usrp->get_tx_freq(channel) / 1e6)
                  << std::endl
                  << std::endl;
    }

    // Allow for some setup time: In particular, LOs and other tuning-related
    // components might need some time to lock. 1 second is way more than enough
    // time. If there were no sleep time at all, we might see LO lock errors
    // as the following check does not include a polling loop.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Check Ref and LO Lock detect
    std::vector<std::string> sensor_names;
    const size_t tx_sensor_chan = channel_nums.empty() ? 0 : channel_nums[0];
    sensor_names                = usrp->get_tx_sensor_names(tx_sensor_chan);
    if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked")
        != sensor_names.end()) {
        uhd::sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked", tx_sensor_chan);
        std::cout << boost::format("Checking TX: %s ...") % lo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }
    const size_t mboard_sensor_idx = 0;
    sensor_names                   = usrp->get_mboard_sensor_names(mboard_sensor_idx);
    if ((ref == "mimo")
        and (std::find(sensor_names.begin(), sensor_names.end(), "mimo_locked")
             != sensor_names.end())) {
        uhd::sensor_value_t mimo_locked =
            usrp->get_mboard_sensor("mimo_locked", mboard_sensor_idx);
        std::cout << boost::format("Checking TX: %s ...") % mimo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(mimo_locked.to_bool());
    }
    if ((ref == "external")
        and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked")
             != sensor_names.end())) {
        uhd::sensor_value_t ref_locked =
            usrp->get_mboard_sensor("ref_locked", mboard_sensor_idx);
        std::cout << boost::format("Checking TX: %s ...") % ref_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(ref_locked.to_bool());
    }

    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop streaming..." << std::endl;

    // Set up metadata. We start streaming a bit in the future
    // to allow MIMO operation:
    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = true;
    md.time_spec      = usrp->get_time_now() + uhd::time_spec_t(0.1);

    // send data until the signal handler gets called
    // or if we accumulate the number of samples specified (unless it's 0)
    uint64_t num_acc_samps = 0;
    while (true) {
        // Break on the end of duration or CTRL-C
        if (stop_signal_called) {
            break;
        }
        // Break when we've received nsamps
        if (total_num_samps > 0 and num_acc_samps >= total_num_samps) {
            break;
        }

        // send the entire contents of the buffer
        num_acc_samps += tx_stream->send(buffs, buff.size(), md);

        // fill the buffer with the waveform
        for (size_t n = 0; n < buff.size(); n++) {
            buff[n] = wave_table(index += step);
        }

        md.start_of_burst = false;
        md.has_time_spec  = false;
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send("", 0, md);

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
