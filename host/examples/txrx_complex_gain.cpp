//
// Copyright 2025 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

/***********************************************************************
 * UHD TX/RX Complex Gain Example
 *
 * This example demonstrates applying complex gain to TX and RX channels.
 *
 * The following usage example applies a TX gain of 0.5+0.5j immediately
 * and a RX gain of 0.5-0.5j after 10 us:
 * ./txrx_complex_gain --args addr0=192.168.10.2
 *      --tx-rate 1e6 --rx-rate 1e6 --tx-freq 1e9 --rx-freq 1e9 \
 *      --tx-gain 0.5+0.5j --rx-gain 0.5-0.5j --rx-gain-delay 10e-6 \
 *      --tx-channels 0 --rx-channels 0,1 --ampl 0.5 --wave-freq 1e5 \
 *      --wave-type CONST --nsamps 1000
 ***********************************************************************/

#include "wavetable.hpp"

#include <uhd/features/complex_gain_iface.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <cmath>
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
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
 * transmit_worker function
 * A function to be used in a thread for transmitting
 **********************************************************************/
void transmit_worker(std::vector<std::complex<float>>& buff,
    wave_table_class wave_table,
    uhd::tx_streamer::sptr tx_streamer,
    uhd::tx_metadata_t metadata,
    size_t step,
    size_t index,
    int num_channels)
{
    std::vector<std::complex<float>*> buffs(num_channels, &buff.front());

    // send data until the signal handler gets called
    while (not stop_signal_called) {
        // fill the buffer with the waveform
        for (size_t n = 0; n < buff.size(); n++) {
            buff[n] = wave_table(index += step);
        }

        // send the entire contents of the buffer
        tx_streamer->send(buffs, buff.size(), metadata);

        metadata.start_of_burst = false;
        metadata.has_time_spec  = false;
    }

    // send a mini EOB packet
    metadata.end_of_burst = true;
    tx_streamer->send("", 0, metadata);
}

/***********************************************************************
 * Utilities
 **********************************************************************/
//! Change to filename, e.g. from usrp_samples.dat to usrp_samples.00.dat,
//  but only if multiple names are to be generated.
std::string generate_out_filename(
    const std::string& base_fn, size_t n_names, size_t this_name)
{
    if (n_names == 1) {
        return base_fn;
    }

    boost::filesystem::path base_fn_fp(base_fn);
    base_fn_fp.replace_extension(boost::filesystem::path(
        str(boost::format("%02d%s") % this_name % base_fn_fp.extension().string())));
    return base_fn_fp.string();
}

/***********************************************************************
 * recv_to_file function
 **********************************************************************/
template <typename samp_type>
void recv_to_file(uhd::usrp::multi_usrp::sptr usrp,
    const std::string& cpu_format,
    const std::string& wire_format,
    const std::string& file,
    size_t samps_per_buff,
    int num_requested_samples,
    double receive_time,
    std::vector<size_t> rx_channel_nums)
{
    int num_total_samps = 0;
    // create a receive streamer
    uhd::stream_args_t stream_args(cpu_format, wire_format);
    stream_args.channels             = rx_channel_nums;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // Prepare buffers for received samples and metadata
    uhd::rx_metadata_t md;
    std::vector<std::vector<samp_type>> buffs(
        rx_channel_nums.size(), std::vector<samp_type>(samps_per_buff));
    // create a vector of pointers to point to each of the channel buffers
    std::vector<samp_type*> buff_ptrs;
    for (size_t i = 0; i < buffs.size(); i++) {
        buff_ptrs.push_back(&buffs[i].front());
    }

    // Create one ofstream object per channel
    // (use shared_ptr because ofstream is non-copyable)
    std::vector<std::shared_ptr<std::ofstream>> outfiles;
    for (size_t i = 0; i < buffs.size(); i++) {
        const std::string this_filename = generate_out_filename(file, buffs.size(), i);
        outfiles.push_back(std::shared_ptr<std::ofstream>(
            new std::ofstream(this_filename.c_str(), std::ofstream::binary)));
    }
    UHD_ASSERT_THROW(outfiles.size() == buffs.size());
    UHD_ASSERT_THROW(buffs.size() == rx_channel_nums.size());
    bool overflow_message = true;
    // We increase the first timeout to cover for the delay between now + the
    // command time, plus 500ms of buffer. In the loop, we will then reduce the
    // timeout for subsequent receives.
    double timeout = receive_time - usrp->get_time_now().get_real_secs() + 0.5f;

    // setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = num_requested_samples;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = uhd::time_spec_t(receive_time);
    rx_stream->issue_stream_cmd(stream_cmd);

    while (not stop_signal_called
           and (num_requested_samples > num_total_samps or num_requested_samples == 0)) {
        size_t num_rx_samps = rx_stream->recv(buff_ptrs, samps_per_buff, md, timeout);
        timeout             = 0.1f; // small timeout for subsequent recv

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << "Timeout while streaming" << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
            if (overflow_message) {
                overflow_message = false;
                std::cerr
                    << boost::format(
                           "Got an overflow indication. Please consider the following:\n"
                           "  Your write medium must sustain a rate of %fMB/s.\n"
                           "  Dropped samples will not be written to the file.\n"
                           "  Please modify this example for your purposes.\n"
                           "  This message will not appear again.\n")
                           % (usrp->get_rx_rate() * sizeof(samp_type) / 1e6);
            }
            continue;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            throw std::runtime_error("Receiver error " + md.strerror());
        }

        num_total_samps += num_rx_samps;

        for (size_t i = 0; i < outfiles.size(); i++) {
            outfiles[i]->write(
                (const char*)buff_ptrs[i], num_rx_samps * sizeof(samp_type));
        }
    }

    // Shut down receiver
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);

    // Close files
    for (size_t i = 0; i < outfiles.size(); i++) {
        outfiles[i]->close();
    }
}

/***********************************************************************
 * Template function to get a feature interface for a given channel
 **********************************************************************/
template <typename FeatureInterface>
FeatureInterface& get_feature_interface(uhd::usrp::multi_usrp::sptr usrp, size_t chan)
{
    if (!usrp->get_radio_control(chan).has_feature<FeatureInterface>()) {
        throw std::runtime_error(
            "The requested feature is not available on this device.");
    }
    return usrp->get_radio_control(chan).get_feature<FeatureInterface>();
}

/***********************************************************************
 * Helper function to parse complex number strings like "1.0+0.0j"
 **********************************************************************/
std::complex<double> parse_complex(const std::string& str)
{
    std::istringstream iss(str);
    double real = 0.0, imag = 0.0;

    if (str.find('+') != std::string::npos || str.find('-', 1) != std::string::npos) {
        // Format: "real+imagj" or "real-imagj"
        iss >> real >> imag;
    } else {
        iss >> real;
    }

    return std::complex<double>(real, imag);
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string program_doc =
        "usage: txrx_complex_gain [--help] [--args ARGS] [--tx-subdev TX_SUBDEV]\n"
        "                         [--rx-subdev RX_SUBDEV] [--file FILE]\n"
        "                         [--type {double,float,short}]\n"
        "                         [--nsamps NSAMPS] [--settling SETTLING]\n"
        "                         [--spb SPB] [--tx-rate TX_RATE]\n"
        "                         [--rx-rate RX_RATE] [--tx-freq TX_FREQ]\n"
        "                         [--rx-freq RX_FREQ] [--rx-delay RX_DELAY]\n"
        "                         [--pps {internal,external,gpsdo}]\n"
        "                         [--wave-ampl WAVE_AMPL]\n"
        "                         [--wave-type {CONST,SQUARE,RAMP,SINE}]\n"
        "                         [--wave-freq WAVE_FREQ] [--tx-gain TX_GAIN]\n"
        "                         [--rx-gain RX_GAIN]\n"
        "                         [--tx-gain-delay TX_GAIN_DELAY]\n"
        "                         [--rx-gain-delay RX_GAIN_DELAY]\n"
        "                         [--tx-channels TX_CHANNELS]\n"
        "                         [--rx-channels RX_CHANNELS]"
        "\n\n"
        "UHD TX/RX Complex Gain Example.\n"
        "This example demonstrates how to use the UHD multi_usrp C++ API to\n"
        "leverage the complex gain feature of RFNoC-capable USRP devices.\n"
        "This feature enables users to apply complex-valued scaling coefficients\n"
        "to the baseband sample streams of TX and RX radios for precise and timed\n"
        "adjustment of magnitude and phase.\n"
        "The program transmits baseband samples of predefined complex waveforms\n"
        "on one or more TX channels and simultaneously receives baseband samples\n"
        "on one or more RX channels. Complex gain adjustments can be scheduled to\n"
        "take effect at specific times, with multiple commands queued for future\n"
        "execution (up to the command queue depth, typically 32).\n"
        "Appropriate loopback connections between TX and RX antennas are assumed,\n"
        "so that the effects of complex gain adjustments can be observed in the\n"
        "received samples. For each configured RX channel, the received baseband\n"
        "data is saved to a binary IQ file, which can be used to visualize and\n"
        "validate magnitude and phase changes, as well as their timing\n"
        "relationships. If multiple USRPs are used, the program supports time\n"
        "alignment to a common reference using an external PPS source (see the\n"
        "--pps option).\n"
        "The complex gain feature is available in the standard bitfiles for most\n"
        "RFNoC-capable USRP models.\n"
        "\n"
        "Timing Overview:\n"
        "  - The USRP device time is initialized to zero. For multiple devices,\n"
        "    use --pps option to synchronize their time bases.\n"
        "  - After a configurable settling time, the TX streamer is started. This\n"
        "    TX start time serves as the reference for all subsequent timed\n"
        "    actions.\n"
        "  - The RX streamer is started after an optional rx-delay. By default,\n"
        "    rx-delay is zero, such that TX and RX streaming start\n"
        "    simultaneously. This enables precise observation of timing\n"
        "    relationships in the saved IQ files.\n"
        "  - Complex gain commands for TX and RX channels are scheduled with\n"
        "    optional delays, relative to the TX start time.\n"
        "\n"
        "    |               |               |         |         |\n"
        "   =+===============+===============+=========+==== usrp time ==>\n"
        "    |               |               |         |         |\n"
        "    0            Tx Start        Rx Start   Apply     Apply\n"
        "  (pps)      (reference time)       |      TX Gain   RX Gain\n"
        "    |               |               |         |         |\n"
        "    |               |               |         |         |\n"
        "    |-- settling -->|-- rx-delay -->|         |         |\n"
        "                    |-- tx-gain-delay ------->|         |\n"
        "                    |-- rx-gain-delay ----------------->|\n"
        "                                    |\n"
        "                                    |========= file time ========>\n"
        "\n"
        "Configuration of gain values:\n"
        "  - Scheduled gain changes are controlled by the options\n"
        "    --tx-channels/--rx-channels, --tx-gain/--rx-gain, and\n"
        "    --tx-gain-delay/--rx-gain-delay.\n"
        "  - The number of scheduled gain changes is determined by the maximum\n"
        "    length of the corresponding channels, gain, and delay arrays.\n"
        "  - For each scheduled gain change, elements are selected from the\n"
        "    arrays using modulo indexing: element = array[index % array_length]\n"
        "  - This approach allows flexible scheduling, such as:\n"
        "    + Applying multiple gain values to the same channel at different\n"
        "      times (if the gain array is longer than the channel array)\n"
        "    + Repeating gain values and delays across channels as needed\n"
        "\n"
        "Usage examples:\n"
        "  1. Single USRP, single channel, TX/RX loopback, schedule one TX and\n"
        "     one RX gain change at different times.\n"
        "     This example program call assumes a single USRP having a TX/RX\n"
        "     loopback connection from TX channel 0 to RX channel 1.\n"
        "     It transmits and receives a sine wave of 1 MHz at a sampling rate\n"
        "     of 10 MSps and applies the following complex gain factors:\n"
        "       a) TX gain factor of of (-10 dB, -45 deg) at delay of 10 ms\n"
        "       a) RX gain factor of of (-10 dB, +45 deg) at delay of 20 ms\n"
        "\n"
        "     txrx_complex_gain --args \"addr=192.168.10.2\"\n"
        "                       --tx-freq 2.4e09 --rx-freq 2.4e09 \n"
        "                       --tx-rate 10e6 --rx-rate 10e6 --nsamps 400000\n"
        "                       --tx-channels 0 --rx-channels 1\n"
        "                       --tx-gain \"0.22-0.22i\" --tx-gain-delay \"0.01\"\n"
        "                       --rx-gain \"0.22+0.22i\" --rx-gain-delay \"0.02\"\n"
        "                       --wave-type SINE --wave-freq 1e6\n"
        "\n"
        "     When analyzing the baseband data from the saved file\n"
        "     usrp_samples.dat, the magnitude and gain changes can be observed at\n"
        "     their configured delays.\n"
        "\n"
        "     A simple python visualization program could look like:\n"
        "\n"
        "       import numpy as np\n"
        "       import matplotlib.pyplot as plt\n"
        "\n"
        "       filename = \"usrp_samples.dat\"\n"
        "       sampling_rate = 10e6\n"
        "\n"
        "       # Read binary file (sc16 format)\n"
        "       data = np.fromfile(filename, dtype=np.int16) / float(2**15-1)\n"
        "       data = data[::2] + 1j * data[1::2]\n"
        "\n"
        "       # X-axis: sample index divided by sampling rate (in seconds)\n"
        "       x = np.arange(len(data)) / sampling_rate\n"
        "\n"
        "       # Magnitude in dB\n"
        "       magnitude_db = 20 * np.log10(np.abs(data) + 1e-14)  # add epsilon\n"
        "       to avoid log(0)\n"
        "\n"
        "       # Phase in degrees\n"
        "       phase_deg = np.angle(data, deg=True)\n"
        "\n"
        "       # Plot\n"
        "       fig, axs = plt.subplots(2, 1, sharex=True, figsize=(10, 6))\n"
        "\n"
        "       axs[0].plot(x, magnitude_db)\n"
        "       axs[0].set_ylabel(\"Magnitude (dB)\")\n"
        "       axs[0].set_title(\"Magnitude in dB\")\n"
        "\n"
        "       axs[1].plot(x, phase_deg)\n"
        "       axs[1].set_ylabel(\"Phase (degrees)\")\n"
        "       axs[1].set_xlabel(\"Time (s)\")\n"
        "       axs[1].set_title(\"Phase in Degrees\")\n"
        "\n"
        "       plt.tight_layout()\n"
        "       plt.show()\n"
        "\n"
        "  2. Two USRPs with 2 TX/RX channels each, schedule TX phase changes for\n"
        "     all TX channels at the same time.\n"
        "     Synchronize the device time of two USRP devices with 2 TX/RX channels\n"
        "     each using an external pps signal and adjust the TX phases for the TX\n"
        "     channels by TX channel specific values of [45, 90, 135, 270] degrees\n"
        "     and at the same delay of 20ms. We also configure all 4 RX channels,\n"
        "     such that, if each TX is connected to one RX channel, the applied\n"
        "     phase changes can be validated by means of analyzing the recorded IQ\n"
        "     baseband files.\n"
        "\n"
        "    txrx_complex_gain --args \"addr0=192.168.10.2,addr1=192.168.10.3\"\n"
        "                      --tx-freq 2.4e09 --rx-freq 2.4e09\n"
        "                      --tx-rate 1e6 --rx-rate 1e6 --nsamps 40000\n"
        "                      --tx-channels \"0,1,2,3\" --rx-channels \"0,1,2,3\"\n"
        "                      --tx-gain \"0.707+0.707j, 0+1j, -0.707+0.707j, 0-1j\"\n"
        "                      --tx-gain-delay \"0.02\" --pps external\n"
        "                      --wave-type SINE --wave-freq 100e3\n"
        "\n"
        "  3. Two USRPs with 2 TX/RX channels each, schedule simultaneous TX\n"
        "     phase changes for all TX channels at two different times.\n"
        "     Similar to example above, but now the TX phases of the four TX\n"
        "     channels are set to\n"
        "       a) [  45,  90, 135, 270] degrees at a delay of 20 ms and\n"
        "       b) [-135, -90, -45,  90] degrees at a delay of 30 ms.\n"
        "\n"
        "     txrx_complex_gain --args \"addr0=192.168.10.2,addr1=192.168.10.3\"\n"
        "                       --tx-freq 2.4e09 --rx-freq 2.4e09\n"
        "                       --tx-rate 1e6 --rx-rate 1e6 --nsamps 40000\n"
        "                       --tx-channels \"0,1,2,3\" --rx-channels \"0,1,2,3\"\n"
        "                       --tx-gain \"0.707+0.707j, 0+1j, -0.707+0.707j, 0-1j, "
        "-0.707-0.707j, 0-1j, 0.707-0.707j, 0+1j\"\n"
        "                       --tx-gain-delay \"0.02, 0.02, 0.02, 0.02, 0.03, 0.03, "
        "0.03, 0.03\"\n"
        "                       --pps external --wave-type SINE --wave-freq 100e3\n";
    // TX variables to be set by po
    std::string args, tx_gain_list, tx_gain_delay, tx_channels, tx_subdev, wave_type;
    double tx_rate, tx_freq, wave_freq;
    float wave_ampl;

    // RX variables to be set by po
    std::string rx_gain_list, rx_gain_delay, rx_channels, rx_subdev, type, file, pps;
    size_t total_num_samps, spb;
    double rx_rate, rx_freq, rx_delay, settling;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "Show this help message and exit.")
        ("args", po::value<std::string>(&args)->default_value(""), "USRP "
        "device selection and configuration arguments."
        "\nSpecify key-value pairs (e.g., addr, serial, type, master_clock_rate) "
        "separated by commas."
        "\nFor multi-device setups, specify multiple IP addresses (e.g., addr0, "
        "addr1) to group multiple USRPs into a single virtual device."
        "\nSee the UHD manual for model-specific options."
        "\nExamples:"
        "\n  --args \"addr=192.168.10.2\""
        "\n  --args \"addr=192.168.10.2,master_clock_rate=200e6\""
        "\n  --args \"addr0=192.168.10.2,addr1=192.168.10.3\""
        "\nIf not specified, UHD connects to the first available device.")
        ("tx-subdev", po::value<std::string>(&tx_subdev), "TX subdevice "
        "configuration defining the mapping of channels to RF TX paths."
        "\nThe format and available values depend on your USRP model. If not "
        "specified, the channels will be numbered in order of the devices, "
        "daughterboard slots, and their RF TX channels."
        "\nFor typical applications, this default subdevice configuration is "
        "sufficient."
        "\nNote: this example program expects a single-USRP subdevice configuration "
        "which is applied to all USRPs equally, if multiple USRPs are configured."
        "\nExample:"
        "\nAssume we have an X310 with two UBX daughterboards installed. Then the "
        "default channel mapping is:"
        "\n  - Ch 0 -> A:0 (1st UBX in slot A, RF TX 0)"
        "\n  - Ch 1 -> B:0 (2nd UBX in slot B, RF TX 0)"
        "\nSpecifying --subdev=\"B:0 A:0\" would change the channel mapping to:"
        "\n  - Ch 0 -> B:0 (2nd UBX in slot B RF TX 0)"
        "\n  - Ch 1 -> A:0 (1st UBX in slot A RF TX 0)")
        ("rx-subdev", po::value<std::string>(&rx_subdev), "RX subdevice "
        "configuration defining the mapping of channels to RF RX paths."
        "\nThe format and available values depend on your USRP model. If not "
        "specified, the channels will be numbered in order of the devices, "
        "daughterboard slots, and their RF RX channels."
        "\nFor typical applications, this default subdevice configuration is "
        "sufficient."
        "\nNote: this example program expects a single-USRP subdevice configuration "
        "which is applied to all USRPs equally, if multiple USRPs are configured."
        "\nExample:"
        "\nAssume we have an X310 with two UBX daughterboards installed. Then the "
        "default channel mapping is:"
        "\n  - Ch 0 -> A:0 (1st UBX in slot A, RF RX 0)"
        "\n  - Ch 1 -> B:0 (2nd UBX in slot B, RF RX 0)"
        "\nSpecifying --subdev=\"B:0 A:0\" would change the channel mapping to:"
        "\n  - Ch 0 -> B:0 (2nd UBX in slot B RF RX 0)"
        "\n  - Ch 1 -> A:0 (1st UBX in slot A RF RX 0)")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"),
          "Base name for the raw IQ binary data files to which samples "
        "received on each channel will be written (one file per channel).")
        ("type", po::value<std::string>(&type)->default_value("short"),
          "Specifies the data format of the file. The data will be "
        "written as interleaved IQ samples in one of the following numeric "
        "formats: 'double' (64-bit float, fc64), 'float' (32-bit float, fc32), or "
        "'short' (16-bit integer, sc16, scaled to int16 range -32768 to 32767)."
        "\nChoosing 'short' as the file format matches the default sc16 "
        "over-the-wire format and is usually sufficient. Using 'float' or "
        "'double' does not improve precision, but may be more convenient for "
        "post-processing or for compatibility with certain analysis tools.")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0),
          "Total number of samples to receive. The program stops when "
        "this number is reached.")
        ("settling", po::value<double>(&settling)->default_value(double(1.0)),
          "settling time (seconds) before transmitting/receiving")
        ("spb", po::value<size_t>(&spb)->default_value(0), "Size of the "
        "host data buffer allocated for the TX streamer, specified in number of "
        "samples (spb)."
        "\nLarger values can improve throughput but may increase latency. Typical "
        "values range from 1,000 to 10,000 samples, but optimal values depend on "
        "device, transport, and application requirements."
        "\nIf not specified, UHD automatically selects the value based on the "
        "maximum value supported by the USRP TX streamer.")
        ("tx-rate", po::value<double>(&tx_rate), "TX sample rate in "
        "samples/second. Note that each USRP device only supports a set of "
        "discrete sample rates, which depend on the hardware model and "
        "configuration. If you request a rate that is not supported, the USRP "
        "device will automatically select and use the closest available rate.")
        ("rx-rate", po::value<double>(&rx_rate), "RX sample rate in "
        "samples/second. Note that each USRP device only supports a set of "
        "discrete sample rates, which depend on the hardware model and "
        "configuration. If you request a rate that is not supported, the USRP "
        "device will automatically select and use the closest available rate.")
        ("tx-freq", po::value<double>(&tx_freq), "TX RF center frequency "
        "in Hz.")
        ("rx-freq", po::value<double>(&rx_freq), "RX RF center frequency "
        "in Hz.")
        ("rx-delay", po::value<double>(&rx_delay)->default_value(0), "Optional "
        "delay (in seconds) before the RX streamer starts. By default, this is "
        "set to 0, so the RX streamer begins simultaneously with the TX streamer. "
        "This ensures that the baseband samples written to the output file "
        "accurately capture the timing of complex gain changes applied to both TX "
        "and RX.")
        ("pps", po::value<std::string>(&pps), "Specifies the PPS source "
        "for time synchronization. Available values depend on the USRP model. "
        "Typical values are 'internal', 'external', and 'gpsdo'.")
        ("wave-ampl", po::value<float>(&wave_ampl)->default_value(float(0.3)),
          "Baseband waveform amplitude in the range [0 to 0.7].")
        ("wave-type", po::value<std::string>(&wave_type)->default_value("CONST"),
          "Baseband waveform type to generate."
        "\nAvailable types are CONST (real), SQUARE (real), RAMP (real), and SINE "
        "(complex)")
        ("wave-freq", po::value<double>(&wave_freq)->default_value(0),
          "Baseband waveform frequency in Hz."
        "\nThis option is required for waveform types SQUARE, RAMP, and SINE.")
        ("tx-gain", po::value<std::string>(&tx_gain_list),
          "Comma-separated list of complex gain coefficients to apply to "
        "TX channels (format: 'real+imagj', e.g. '1.0+0j,0+1.0j'). If fewer gain "
        "values are provided than TX channels (--tx-channels) or delay values "
        "(--tx-gain-delay), the values are assigned by cycling through the list "
        "as needed."
        "\nIf not specified, the complex gain is not changed, i.e. stays at unity "
        "complex gain.")
        ("rx-gain", po::value<std::string>(&rx_gain_list),
          "Comma-separated list of complex gain coefficients to apply to "
        "RX channels (format: 'real+imagj', e.g. '1.0+0j,0+1.0j'). If fewer gain "
        "values are provided than RX channels (--rx-channels) or delay values "
        "(--rx-gain-delay), the values are assigned by cycling through the list "
        "as needed."
        "\nIf not specified, the complex gain is not changed, i.e. stays at unity "
        "complex gain.")
        ("tx-gain-delay", po::value<std::string>(&tx_gain_delay)->default_value("0.0"),
          "Comma-separated list of delay times (in seconds) for applying "
        "TX gain values, relative to TX streaming start. If fewer delay values "
        "are provided than TX channels (--tx-channels) or than gain values "
        "(--tx-gain), the values are assigned by cycling through the list as "
        "needed."
        "\nIf not specified, TX gain values are applied immediately.")
        ("rx-gain-delay", po::value<std::string>(&rx_gain_delay)->default_value("0.0"),
          "Comma-separated list of delay times (in seconds) for applying "
        "RX gain values, relative to TX streaming start. If fewer delay values "
        "are provided than RX channels (--rx-channels) or than gain values "
        "(--rx-gain), the values are assigned by cycling through the list as "
        "needed."
        "\nIf not specified, RX gain values are applied immediately.")
        ("tx-channels", po::value<std::string>(&tx_channels)->default_value("0"),
          "Specifies which TX channels to use. E.g. \"0\", \"1\", "
        "\"0,1\", etc.")
        ("rx-channels", po::value<std::string>(&rx_channels)->default_value("0"),
          "Specifies which RX channels to use. E.g. \"0\", \"1\", "
        "\"0,1\", etc.")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        std::cout << program_doc << std::endl;
        std::cout << desc << std::endl;
        return ~0;
    }
    po::notify(vm); // only called if --help was not requested

    // parse complex gain values from strings
    std::vector<std::string> tx_gain_strs;
    std::vector<std::string> rx_gain_strs;
    std::vector<std::complex<double>> tx_gain_val;
    std::vector<std::complex<double>> rx_gain_val;
    boost::split(tx_gain_strs, tx_gain_list, boost::is_any_of("\"',"));
    boost::split(rx_gain_strs, rx_gain_list, boost::is_any_of("\"',"));
    try {
        for (const auto& gain_str : tx_gain_strs) {
            if (!gain_str.empty()) {
                tx_gain_val.push_back(parse_complex(gain_str));
            }
        }
        for (const auto& gain_str : rx_gain_strs) {
            if (!gain_str.empty()) {
                rx_gain_val.push_back(parse_complex(gain_str));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing gain values: " << e.what() << std::endl;
        std::cerr << "Use format like '1.0+0.0j' for complex numbers" << std::endl;
        return EXIT_FAILURE;
    }

    // parse gain delay
    std::vector<double> rx_gain_delays, tx_gain_delays;
    std::vector<std::string> rx_time_str, tx_time_str;

    if (vm.count("rx-gain-delay")) {
        boost::split(rx_time_str, rx_gain_delay, boost::is_any_of("\"',"));
        for (const auto& time_str : rx_time_str) {
            rx_gain_delays.push_back(std::stod(time_str));
        }
    }

    if (vm.count("tx-gain-delay")) {
        boost::split(tx_time_str, tx_gain_delay, boost::is_any_of("\"',"));
        for (const auto& time_str : tx_time_str) {
            tx_gain_delays.push_back(std::stod(time_str));
        }
    }

    // create a usrp device
    std::cout << std::endl;
    uhd::usrp::multi_usrp::sptr multi_usrp;
    std::cout << "Creating the multi_usrp device with: " << args << "..." << std::endl;
    multi_usrp = uhd::usrp::multi_usrp::make(args);

    // parse channel list
    std::vector<size_t> tx_channel_nums, rx_channel_nums;
    std::vector<std::string> tx_channel_str, rx_channel_str;
    boost::split(tx_channel_str, tx_channels, boost::is_any_of("\"',"));
    boost::split(rx_channel_str, rx_channels, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < tx_channel_str.size(); ch++) {
        size_t chan = std::stoi(tx_channel_str[ch]);
        if (chan >= multi_usrp->get_tx_num_channels()) {
            throw std::runtime_error("Invalid TX channel(s) specified.");
        } else {
            tx_channel_nums.push_back(chan);
        }
    }
    for (size_t ch = 0; ch < rx_channel_str.size(); ch++) {
        size_t chan = std::stoi(rx_channel_str[ch]);
        if (chan >= multi_usrp->get_rx_num_channels()) {
            throw std::runtime_error("Invalid RX channel(s) specified.");
        } else {
            rx_channel_nums.push_back(chan);
        }
    }

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("tx-subdev")) {
        multi_usrp->set_tx_subdev_spec(tx_subdev);
    }
    if (vm.count("rx-subdev")) {
        multi_usrp->set_rx_subdev_spec(rx_subdev);
    }
    std::cout << "Using device: " << multi_usrp->get_pp_string() << std::endl;

    // set the transmit sample rate
    if (not vm.count("tx-rate")) {
        std::cerr << "Please specify the transmit sample rate with --tx-rate"
                  << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (tx_rate / 1e6)
              << std::endl;
    multi_usrp->set_tx_rate(tx_rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...")
                     % (multi_usrp->get_tx_rate() / 1e6)
              << std::endl
              << std::endl;


    // set the receive sample rate
    if (not vm.count("rx-rate")) {
        std::cerr << "Please specify the sample rate with --rx-rate" << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rx_rate / 1e6)
              << std::endl;
    multi_usrp->set_rx_rate(rx_rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...")
                     % (multi_usrp->get_rx_rate() / 1e6)
              << std::endl
              << std::endl;

    // set the transmit center frequency
    if (not vm.count("tx-freq")) {
        std::cerr << "Please specify the transmit center frequency with --tx-freq"
                  << std::endl;
        return ~0;
    }
    for (size_t chan_idx = 0; chan_idx < tx_channel_nums.size(); chan_idx++) {
        size_t chan = tx_channel_nums[chan_idx];

        std::cout << "Configuring TX Channel " << chan << std::endl;
        std::cout << boost::format("Setting TX Freq: %f MHz...") % (tx_freq / 1e6)
                  << std::endl;
        uhd::tune_request_t tx_tune_request(tx_freq);
        multi_usrp->set_tx_freq(tx_tune_request, chan);
        std::cout << boost::format("Actual TX Freq: %f MHz...")
                         % (multi_usrp->get_tx_freq(chan) / 1e6)
                  << std::endl
                  << std::endl;
    }


    // set the receive center frequency
    if (not vm.count("rx-freq")) {
        std::cerr << "Please specify the center frequency with --rx-freq" << std::endl;
        return ~0;
    }
    for (size_t chan_idx = 0; chan_idx < rx_channel_nums.size(); chan_idx++) {
        size_t chan = rx_channel_nums[chan_idx];
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (rx_freq / 1e6)
                  << std::endl;
        uhd::tune_request_t rx_tune_request(rx_freq);
        multi_usrp->set_rx_freq(rx_tune_request, chan);
        std::cout << boost::format("Actual RX Freq: %f MHz...")
                         % (multi_usrp->get_rx_freq(chan) / 1e6)
                  << std::endl
                  << std::endl;
    }

    // check ref and LO lock detect
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for LO's to lock
    std::vector<std::string> tx_sensor_names, rx_sensor_names;
    for (size_t ch = 0; ch < tx_channel_nums.size(); ch++) {
        size_t channel  = tx_channel_nums[ch];
        tx_sensor_names = multi_usrp->get_tx_sensor_names(channel);
        if (std::find(tx_sensor_names.begin(), tx_sensor_names.end(), "lo_locked")
            != tx_sensor_names.end()) {
            uhd::sensor_value_t lo_locked =
                multi_usrp->get_tx_sensor("lo_locked", channel);
            std::cout << boost::format("Checking TX Channel %d: %s ...") % channel
                             % lo_locked.to_pp_string()
                      << std::endl;
            if (!lo_locked.to_bool()) {
                throw uhd::runtime_error(
                    "ERROR: LO is not locked for TX channel " + std::to_string(channel)
                    + ". Ensure frequency is supported, check cabling for external "
                      "reference clock if applicable, try increasing settling time, "
                      "verify that TX/RX frequencies match for shared LO "
                      "daughterboards.");
            }
        }
    }
    for (size_t ch = 0; ch < rx_channel_nums.size(); ch++) {
        size_t channel  = rx_channel_nums[ch];
        rx_sensor_names = multi_usrp->get_rx_sensor_names(channel);
        if (std::find(rx_sensor_names.begin(), rx_sensor_names.end(), "lo_locked")
            != rx_sensor_names.end()) {
            uhd::sensor_value_t lo_locked =
                multi_usrp->get_rx_sensor("lo_locked", channel);
            std::cout << boost::format("Checking RX Channel %d: %s ...") % channel
                             % lo_locked.to_pp_string()
                      << std::endl;
            if (!lo_locked.to_bool()) {
                throw uhd::runtime_error(
                    "ERROR: LO is not locked for RX channel " + std::to_string(channel)
                    + ". Ensure frequency is supported, check cabling for external "
                      "reference clock if applicable, try increasing settling time, "
                      "verify that TX/RX frequencies match for shared LO "
                      "daughterboards.");
            }
        }
    }

    // for the const wave, set the wave freq for small samples per period
    if (wave_freq == 0 and wave_type == "CONST") {
        wave_freq = multi_usrp->get_tx_rate() / 2;
    }
    // error when the waveform is not possible to generate
    if (std::abs(wave_freq) > multi_usrp->get_tx_rate() / 2) {
        throw std::runtime_error("wave freq out of Nyquist zone");
    }
    if (multi_usrp->get_tx_rate() / std::abs(wave_freq) > wave_table_len / 2) {
        throw std::runtime_error("wave freq too small for table");
    }
    // pre-compute the waveform values
    const wave_table_class wave_table(wave_type, wave_ampl);
    const size_t step =
        std::lround(wave_freq / multi_usrp->get_tx_rate() * wave_table_len);
    size_t index = 0;

    // define a reference time in the future considering settling (streamers etc.)
    const double usrp_init_time = 0;
    const double reference_time = usrp_init_time + settling;
    // ... TX streaming shall start at the chosen reference time
    const double tx_delay = 0;

    // synchronously initialize USRP time across multiple motherboards to usrp_init_time
    if (multi_usrp->get_num_mboards() == 1) {
        // single USRP: no need to align to PPS
        multi_usrp->set_time_now(uhd::time_spec_t(usrp_init_time));

    } else if (pps == "external" or pps == "gpsdo") {
        multi_usrp->set_time_source(pps);
        multi_usrp->set_time_unknown_pps(uhd::time_spec_t(usrp_init_time));

    } else {
        // multiple USRPs without external PPS
        std::cout << "No external PPS available, using internal time." << std::endl;
        multi_usrp->set_time_now(uhd::time_spec_t(usrp_init_time));
    }

    // create a transmit streamer
    // linearly map channels (index0 = channel0, index1 = channel1, ...)
    uhd::stream_args_t stream_args("fc32", "sc16");
    stream_args.channels             = tx_channel_nums;
    uhd::tx_streamer::sptr tx_stream = multi_usrp->get_tx_stream(stream_args);

    // allocate a buffer which we re-use for each channel
    if (spb == 0)
        spb = tx_stream->get_max_num_samps() * 10;
    std::vector<std::complex<float>> buff(spb);

    // setup the metadata flags
    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = true;
    md.time_spec      = uhd::time_spec_t(reference_time + tx_delay);

    if (vm.count("tx-gain")) {
        // print current TX complex gain values
        for (size_t chan_idx = 0; chan_idx < tx_channel_nums.size(); chan_idx++) {
            size_t chan = tx_channel_nums[chan_idx];
            try {
                // get TX complex gain interface for channel
                auto& tx_cgain =
                    get_feature_interface<uhd::features::tx_complex_gain_iface>(
                        multi_usrp, chan);
                // get current TX gain
                auto current_tx_gain =
                    tx_cgain.get_gain_coeff(multi_usrp->get_tx_radio_channel(chan));
                std::cout << "Current TX complex gain for channel " << chan << " is "
                          << current_tx_gain << std::endl;
            } catch (const std::exception& e) {
                std::cout << "TX Complex Gain not available on channel " << chan << ": "
                          << e.what() << std::endl;
            }
        }
        // schedule TX complex gain for each specified TX channel at specified delay
        // values
        size_t num_tx_gain_changes = std::max(
            tx_channel_nums.size(), std::max(tx_gain_val.size(), tx_gain_delays.size()));
        for (size_t i = 0; i < num_tx_gain_changes; i++) {
            const size_t chan               = tx_channel_nums[i % tx_channel_nums.size()];
            const std::complex<double> gain = tx_gain_val[i % tx_gain_val.size()];
            const double gain_delay         = vm.count("tx-gain-delay")
                                                  ? tx_gain_delays[i % tx_gain_delays.size()]
                                                  : 0.0;
            try {
                // get TX complex gain interface for channel
                auto& tx_cgain =
                    get_feature_interface<uhd::features::tx_complex_gain_iface>(
                        multi_usrp, chan);
                // set TX gain for this channel (use channel index for array access)
                // channel to apply gain needs to be translated to block channel
                tx_cgain.set_gain_coeff(gain,
                    multi_usrp->get_tx_radio_channel(chan),
                    reference_time + gain_delay);
                std::cout << "Schedule TX complex gain: "
                          << " Channel: " << std::setw(2) << chan
                          << " Delay: " << std::setw(6) << std::fixed
                          << std::setprecision(4) << gain_delay << "s"
                          << " Gain: " << std::setw(7) << std::right << std::fixed
                          << std::setprecision(3) << gain.real()
                          << (gain.imag() >= 0 ? "+" : "-") << std::right << std::fixed
                          << std::setprecision(3) << std::abs(gain.imag()) << "j"
                          << std::endl;
            } catch (const std::exception& e) {
                std::cout << "TX Complex Gain not applied on channel " << chan << ": "
                          << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
    }
    if (vm.count("rx-gain")) {
        // print current RX complex gain values
        for (size_t chan_idx = 0; chan_idx < rx_channel_nums.size(); chan_idx++) {
            size_t chan = rx_channel_nums[chan_idx];
            try {
                // get RX complex gain interface for channel
                auto& rx_cgain =
                    get_feature_interface<uhd::features::rx_complex_gain_iface>(
                        multi_usrp, chan);
                // get current RX gain
                auto current_rx_gain =
                    rx_cgain.get_gain_coeff(multi_usrp->get_rx_radio_channel(chan));
                std::cout << "Current RX complex gain for channel " << chan << " is "
                          << current_rx_gain << std::endl;
            } catch (const std::exception& e) {
                std::cout << "RX Complex Gain not available on channel " << chan << ": "
                          << e.what() << std::endl;
            }
        }
        // schedule RX complex gain for each specified RX channel at specified delay
        // values.
        size_t num_rx_gain_changes = std::max(
            rx_channel_nums.size(), std::max(rx_gain_val.size(), rx_gain_delays.size()));
        for (size_t i = 0; i < num_rx_gain_changes; i++) {
            const size_t chan               = rx_channel_nums[i % rx_channel_nums.size()];
            const std::complex<double> gain = rx_gain_val[i % rx_gain_val.size()];
            const double gain_delay         = vm.count("rx-gain-delay")
                                                  ? rx_gain_delays[i % rx_gain_delays.size()]
                                                  : 0.0;
            try {
                // get RX complex gain interface for channel
                auto& rx_cgain =
                    get_feature_interface<uhd::features::rx_complex_gain_iface>(
                        multi_usrp, chan);
                // set RX gain for this channel (use channel index for array access)
                // channel to apply gain needs to be translated to block channel
                rx_cgain.set_gain_coeff(gain,
                    multi_usrp->get_rx_radio_channel(chan),
                    reference_time + gain_delay);
                std::cout << "Schedule RX complex gain: "
                          << " Channel: " << std::setw(2) << chan
                          << " Delay: " << std::setw(6) << std::fixed
                          << std::setprecision(4) << gain_delay << "s"
                          << " Gain: " << std::setw(7) << std::right << std::fixed
                          << std::setprecision(3) << gain.real()
                          << (gain.imag() >= 0 ? "+" : "-") << std::right << std::fixed
                          << std::setprecision(3) << std::abs(gain.imag()) << "j"
                          << std::endl;
            } catch (const std::exception& e) {
                std::cout << "RX Complex Gain not applied on channel " << chan << ": "
                          << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    // set up signal handlers
    if (total_num_samps == 0) {
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }
    // start transmit worker thread
    std::thread transmit_thread([&]() {
        transmit_worker(
            buff, wave_table, tx_stream, md, step, index, tx_channel_nums.size());
    });
    // receive samples to file
    if (type == "double")
        recv_to_file<std::complex<double>>(multi_usrp,
            "fc64",
            "sc16",
            file,
            spb,
            total_num_samps,
            settling,
            rx_channel_nums);
    else if (type == "float")
        recv_to_file<std::complex<float>>(multi_usrp,
            "fc32",
            "sc16",
            file,
            spb,
            total_num_samps,
            settling,
            rx_channel_nums);
    else if (type == "short")
        recv_to_file<std::complex<short>>(multi_usrp,
            "sc16",
            "sc16",
            file,
            spb,
            total_num_samps,
            settling,
            rx_channel_nums);
    // clean up transmit worker
    stop_signal_called = true;
    transmit_thread.join();
    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
