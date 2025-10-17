//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "wavetable.hpp"
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <cmath>
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
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
 * transmit_worker function
 * A function to be used in a thread for transmitting
 **********************************************************************/
void transmit_worker(std::vector<std::complex<float>> buff,
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
 * recv_to_file function
 **********************************************************************/
template <typename samp_type>
void recv_to_file(uhd::usrp::multi_usrp::sptr usrp,
    const std::string& cpu_format,
    const std::string& wire_format,
    const std::string& file,
    size_t samps_per_buff,
    int num_requested_samples,
    double settling_time,
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
    double timeout = settling_time + 0.5f;

    // setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = num_requested_samples;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = usrp->get_time_now() + uhd::time_spec_t(settling_time);
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
 * Main function
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string program_doc =
        "usage: txrx_loopback_to_file [-h] --tx-rate TX_RATE --rx-rate RX_RATE\n"
        "                                  --tx-freq TX_FREQ --rx-freq RX_FREQ\n"
        "                             [--tx-args TX_ARGS] [--rx-args RX_ARGS]\n"
        "                             [--file FILE]\n"
        "                             [--type {double,float,short}]\n"
        "                             [--nsamps NSAMPS] [--settling SETTLING]\n"
        "                             [--spb SPB] [--tx-gain TX_GAIN]\n"
        "                             [--rx-gain RX_GAIN] [--tx-ant TX_ANT]\n"
        "                             [--rx-ant RX_ANT] [--tx-subdev TX_SUBDEV]\n"
        "                             [--rx-subdev RX_SUBDEV] [--tx-bw TX_BW]\n"
        "                             [--rx-bw RX_BW]\n"
        "                             [--wave-type {CONST,SQUARE,RAMP,SINE}]\n"
        "                             [--wave-freq WAVE_FREQ]\n"
        "                             [--wave-ampl WAVE_AMPL]\n"
        "                             [--ref {internal,external,mimo,gpsdo}]\n"
        "                             [--otw {sc16,sc8}]\n"
        "                             [--tx-channels TX_CHANNELS]\n"
        "                             [--rx-channels RX_CHANNELS] [--tx-int-n]\n"
        "                             [--rx-int-n]"
        "\n\n"
        "This example demonstrates how to use the UHD multi_usrp C++ API\n"
        "to transmit a generated waveform and simultaneously receive signals\n"
        "using USRP devices. Transmission and reception can be performed on the\n"
        "same USRP or on separate USRP devices, and both TX and RX can use\n"
        "multiple channels across one or more devices.\n"
        "\n"
        "Key features:\n"
        "  - Generates predefined baseband waveforms (CONST, SINE, SQUARE, or\n"
        "    RAMP).\n"
        "  - Transmits the same waveform on all selected TX channels.\n"
        "  - Starts RX streaming at a precise hardware timestamp for synchronized\n"
        "    capture.\n"
        "  - Supports independent configuration of transmit and receive devices,\n"
        "    including sample rate, frequency, gain, bandwidth, and channel\n"
        "    mapping.\n"
        "  - Handles multi-channel and multi-device setups for advanced use\n"
        "    cases.\n"
        "  - Saves received data to raw binary files, with one file per RX\n"
        "    channel.\n"
        "  - Stops automatically when the requested number of samples has been\n"
        "    received, or continues until interrupted by the user.\n"
        "\n"
        "Usage examples:\n"
        "  1. Loopback operation of a single USRP transmitting and receiving on\n"
        "     one channel:\n"
        "     txrx_loopback_to_file --tx-args \"addr=192.168.10.2\"\n"
        "                           --rx-args \"addr=192.168.10.2\"\n"
        "                           --tx-freq 2.4e09 --rx-freq 2.4e09\n"
        "                           --tx-rate 10e06 --rx-rate 10e06 --nsamps 10000\n"
        "  2. Loopback operation of a single USRP transmitting on one channel and\n"
        "     receiving on two channels:\n"
        "     txrx_loopback_to_file --tx-args \"addr=192.168.10.2\"\n"
        "                           --rx-args \"addr=192.168.10.2\"\n"
        "                           --tx-freq 2.4e09 --rx-freq 2.4e09\n"
        "                           --tx-rate 10e06 --rx-rate 10e06\n"
        "                           --tx-channels \"0\" --rx-channels \"0,1\"\n"
        "                           --wave-type SINE --wave-freq 1e6\n"
        "                           --nsamps 10000\n"
        "  3. One transmit USRP and two receive USRP devices which are synchronized by\n"
        "     an external pps pulse with transmission on one channel and reception\n"
        "     on four channels:\n"
        "     txrx_loopback_to_file --tx-args \"addr=192.168.10.2\"\n"
        "                           --rx-args \"addr0=192.168.10.2,addr1=192.168.10.3\"\n"
        "                           --tx-freq 2.4e09 --rx-freq 2.4e09\n"
        "                           --tx-rate 10e06 --rx-rate 10e06 --tx-channels \"0\"\n"
        "                           --rx-channels \"0,1,2,3\" --pps \"external\"\n"
        "                           --wave-type SINE --wave-freq 1e6\n"
        "                           --nsamps 10000\n";
    // transmit variables to be set by po
    std::string tx_args, wave_type, tx_ant, tx_subdev, ref, otw, tx_channels;
    double tx_rate, tx_freq, tx_gain, wave_freq, tx_bw;
    float ampl;

    // receive variables to be set by po
    std::string rx_args, file, type, rx_ant, rx_subdev, rx_channels;
    size_t total_num_samps, spb;
    double rx_rate, rx_freq, rx_gain, rx_bw;
    double settling;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show this help message and exit.")
        ("tx-args", po::value<std::string>(&tx_args)->default_value(""), "USRP device selection and "
            "configuration arguments for the transmit USRP device(s)."
            "\nSpecify key-value pairs (e.g., addr, serial, type, master_clock_rate) separated by commas."
            "\nFor multi-device setups, specify multiple IP addresses (e.g., addr0, addr1) to group multiple USRPs into a "
            "single virtual device."
            "\nSee the UHD manual for model-specific options."
            "\nExamples:"
            "\n  --args \"addr=192.168.10.2\""
            "\n  --args \"addr=192.168.10.2,master_clock_rate=200e6\""
            "\n  --args \"addr0=192.168.10.2,addr1=192.168.10.3\""
            "\nIf not specified, UHD connects to the first available device.")
        ("rx-args", po::value<std::string>(&rx_args)->default_value(""), "USRP device selection and "
            "configuration arguments for the receive USRP device(s).")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "Base name of the raw "
            "binary file to which data received on each channel will be written (one file per channel).")
        ("type", po::value<std::string>(&type)->default_value("short"), "Specifies the data format of the "
            "file. The data will be written as interleaved IQ samples in one of the following numeric formats: 'double' "
            "(64-bit float, fc64), 'float' (32-bit float, fc32), or 'short' (16-bit integer, sc16, scaled to int16 range "
            "-32768 to 32767)."
            "\nChoosing 'short' as the file format matches the default sc16 over-the-wire format and is usually sufficient. Using "
            "'float' or 'double' does not improve precision, but may be more convenient for post processing or for "
            "compatibility with certain analysis tools.")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "Total number of samples to "
            "receive. The program stops when this number is reached. If set to 0, data will be continuously received and "
            "written to file.")
        ("settling", po::value<double>(&settling)->default_value(double(0.2)), "Settling time in seconds "
            "before receiving.")
        ("spb", po::value<size_t>(&spb)->default_value(0), "Specifies the size (in samples) of the host-side "
            "data buffer. For TX, a single buffer of this size is used for all transmit channels. For RX, a separate "
            "buffer of this size is allocated for each receive channel. Larger values may improve throughput. If set to "
            "0, the size is determined automatically based on the buffer size of the UHD transmit streamer.")
        ("tx-rate", po::value<double>(&tx_rate)->required(), "TX sample rate in samples/second. Note that "
            "each USRP device only supports a set of discrete sample rates, which depend on the hardware model and "
            "configuration. If you request a rate that is not supported, the USRP device will automatically select and "
            "use the closest available rate.")
        ("rx-rate", po::value<double>(&rx_rate)->required(), "RX sample rate in samples/second.")
        ("tx-freq", po::value<double>(&tx_freq)->required(), "TX RF center frequency in Hz.")
        ("rx-freq", po::value<double>(&rx_freq)->required(), "RX RF center frequency in Hz.")
        ("tx-gain", po::value<double>(&tx_gain), "TX gain for the RF chain in dB.")
        ("rx-gain", po::value<double>(&rx_gain), "RX gain for the RF chain in dB.")
        ("tx-ant", po::value<std::string>(&tx_ant), "TX antenna port selection string selecting a specific "
            "antenna port for USRP daughterboards having multiple antenna connectors per RF channel."
            "\nExample: --ant \"TX/RX\"")
        ("rx-ant", po::value<std::string>(&rx_ant), "RX antenna port selection string.")
        ("tx-subdev", po::value<std::string>(&tx_subdev), "TX subdevice configuration defining the mapping of "
            "channels to RF TX paths."
            "\nThe format and available values depend on your USRP model. If not specified, the channels will be numbered "
            "in order of the devices, daughterboard slots, and their RF TX channels."
            "\nFor typical applications, this default subdevice configuration is sufficient."
            "\nNote: this example program expects a single-USRP subdevice configuration which is applied to all USRPs "
            "equally, if multiple USRPs are configured."
            "\nExample:"
            "\nAssume we have an X310 with two UBX daughterboards installed. Then the default channel mapping is:"
            "\n  - Ch 0 -> A:0 (1st UBX in slot A, RF TX 0)"
            "\n  - Ch 1 -> B:0 (2nd UBX in slot B, RF TX 0)"
            "\nSpecifying --subdev=\"B:0 A:0\" would change the channel mapping to:"
            "\n  - Ch 0 -> B:0 (2nd UBX in slot B RF TX 0)"
            "\n  - Ch 1 -> A:0 (1st UBX in slot A RF TX 0)")
        ("rx-subdev", po::value<std::string>(&rx_subdev), "RX subdevice configuration defining the mapping of "
            "channels to RF RX paths.")
        ("tx-bw", po::value<double>(&tx_bw), "Sets the analog frontend filter bandwidth for the TX path in "
            "Hz. Not all USRP devices support programmable bandwidth; if an unsupported value is requested, the device "
            "will use the nearest supported bandwidth instead.")
        ("rx-bw", po::value<double>(&rx_bw), "Sets the analog frontend filter bandwidth for the RX path in "
            "Hz.")
        ("wave-type", po::value<std::string>(&wave_type)->default_value("CONST"), "Baseband waveform type to "
            "generate."
            "\nAvailable types are CONST (real), SQUARE (real), RAMP (real), and SINE (complex)")
        ("wave-freq", po::value<double>(&wave_freq)->default_value(0), "Baseband waveform frequency in Hz."
            "\nThis option is required for waveform types SQUARE, RAMP, and SINE.")
        ("wave-ampl", po::value<float>(&ampl)->default_value(float(0.3)), "Baseband waveform amplitude in the "
            "range [0 to 0.7].")
        ("ref", po::value<std::string>(&ref), "Sets the source for the frequency reference. Available values "
            "depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("otw", po::value<std::string>(&otw)->default_value("sc16"), "Specifies the over-the-wire (OTW) data "
            "format used for transmission between the host and the USRP device. Common values are \"sc16\" (16-bit signed "
            "complex) and \"sc8\" (8-bit signed complex). Using \"sc8\" can reduce network bandwidth at the cost of "
            "dynamic range."
            "\nNote, that not all conversions between CPU and OTW formats are possible.")
        ("tx-channels", po::value<std::string>(&tx_channels)->default_value("0"), "Specifies which TX "
            "channels to use. E.g. \"0\", \"1\", \"0,1\", etc.")
        ("rx-channels", po::value<std::string>(&rx_channels)->default_value("0"), "Specifies which RX "
            "channels to use. E.g. \"0\", \"1\", \"0,1\", etc.")
        ("tx-int-n", "Use integer-N tuning for USRP TX. With this mode, the LO can only be tuned in discrete "
            "steps, which are integer multiples of the reference frequency. This mode can improve phase noise and "
            "spurious performance at the cost of coarser frequency resolution.")
        ("rx-int-n", "Use integer-N tuning for USRP RX.")
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

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the transmit usrp device with: %s...") % tx_args
              << std::endl;
    uhd::usrp::multi_usrp::sptr tx_usrp = uhd::usrp::multi_usrp::make(tx_args);
    std::cout << std::endl;
    std::cout << boost::format("Creating the receive usrp device with: %s...") % rx_args
              << std::endl;
    uhd::usrp::multi_usrp::sptr rx_usrp = uhd::usrp::multi_usrp::make(rx_args);

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("tx-subdev"))
        tx_usrp->set_tx_subdev_spec(tx_subdev);
    if (vm.count("rx-subdev"))
        rx_usrp->set_rx_subdev_spec(rx_subdev);

    // detect which channels to use
    std::vector<std::string> tx_channel_strings;
    std::vector<size_t> tx_channel_nums;
    boost::split(tx_channel_strings, tx_channels, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < tx_channel_strings.size(); ch++) {
        size_t chan = std::stoi(tx_channel_strings[ch]);
        if (chan >= tx_usrp->get_tx_num_channels()) {
            throw std::runtime_error("Invalid TX channel(s) specified.");
        } else
            tx_channel_nums.push_back(std::stoi(tx_channel_strings[ch]));
    }
    std::vector<std::string> rx_channel_strings;
    std::vector<size_t> rx_channel_nums;
    boost::split(rx_channel_strings, rx_channels, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < rx_channel_strings.size(); ch++) {
        size_t chan = std::stoi(rx_channel_strings[ch]);
        if (chan >= rx_usrp->get_rx_num_channels()) {
            throw std::runtime_error("Invalid RX channel(s) specified.");
        } else
            rx_channel_nums.push_back(std::stoi(rx_channel_strings[ch]));
    }

    // Lock mboard clocks
    if (vm.count("ref")) {
        tx_usrp->set_clock_source(ref);
        rx_usrp->set_clock_source(ref);
    }

    std::cout << "Using TX Device: " << tx_usrp->get_pp_string() << std::endl;
    std::cout << "Using RX Device: " << rx_usrp->get_pp_string() << std::endl;

    // set the transmit sample rate
    if (not vm.count("tx-rate")) {
        std::cerr << "Please specify the transmit sample rate with --tx-rate"
                  << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (tx_rate / 1e6)
              << std::endl;
    tx_usrp->set_tx_rate(tx_rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...")
                     % (tx_usrp->get_tx_rate() / 1e6)
              << std::endl
              << std::endl;

    // set the receive sample rate
    if (not vm.count("rx-rate")) {
        std::cerr << "Please specify the sample rate with --rx-rate" << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rx_rate / 1e6)
              << std::endl;
    rx_usrp->set_rx_rate(rx_rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...")
                     % (rx_usrp->get_rx_rate() / 1e6)
              << std::endl
              << std::endl;

    // set the transmit center frequency
    if (not vm.count("tx-freq")) {
        std::cerr << "Please specify the transmit center frequency with --tx-freq"
                  << std::endl;
        return ~0;
    }

    for (size_t ch = 0; ch < tx_channel_nums.size(); ch++) {
        size_t channel = tx_channel_nums[ch];
        if (tx_channel_nums.size() > 1) {
            std::cout << "Configuring TX Channel " << channel << std::endl;
        }
        std::cout << boost::format("Setting TX Freq: %f MHz...") % (tx_freq / 1e6)
                  << std::endl;
        uhd::tune_request_t tx_tune_request(tx_freq);
        if (vm.count("tx-int-n"))
            tx_tune_request.args = uhd::device_addr_t("mode_n=integer");
        tx_usrp->set_tx_freq(tx_tune_request, channel);
        std::cout << boost::format("Actual TX Freq: %f MHz...")
                         % (tx_usrp->get_tx_freq(channel) / 1e6)
                  << std::endl
                  << std::endl;

        // set the rf gain
        if (vm.count("tx-gain")) {
            std::cout << boost::format("Setting TX Gain: %f dB...") % tx_gain
                      << std::endl;
            tx_usrp->set_tx_gain(tx_gain, channel);
            std::cout << boost::format("Actual TX Gain: %f dB...")
                             % tx_usrp->get_tx_gain(channel)
                      << std::endl
                      << std::endl;
        }

        // set the analog frontend filter bandwidth
        if (vm.count("tx-bw")) {
            std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % tx_bw
                      << std::endl;
            tx_usrp->set_tx_bandwidth(tx_bw, channel);
            std::cout << boost::format("Actual TX Bandwidth: %f MHz...")
                             % tx_usrp->get_tx_bandwidth(channel)
                      << std::endl
                      << std::endl;
        }

        // set the antenna
        if (vm.count("tx-ant"))
            tx_usrp->set_tx_antenna(tx_ant, channel);
    }

    for (size_t ch = 0; ch < rx_channel_nums.size(); ch++) {
        size_t channel = rx_channel_nums[ch];
        if (rx_channel_nums.size() > 1) {
            std::cout << "Configuring RX Channel " << channel << std::endl;
        }

        // set the receive center frequency
        if (not vm.count("rx-freq")) {
            std::cerr << "Please specify the center frequency with --rx-freq"
                      << std::endl;
            return ~0;
        }
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (rx_freq / 1e6)
                  << std::endl;
        uhd::tune_request_t rx_tune_request(rx_freq);
        if (vm.count("rx-int-n"))
            rx_tune_request.args = uhd::device_addr_t("mode_n=integer");
        rx_usrp->set_rx_freq(rx_tune_request, channel);
        std::cout << boost::format("Actual RX Freq: %f MHz...")
                         % (rx_usrp->get_rx_freq(channel) / 1e6)
                  << std::endl
                  << std::endl;

        // set the receive rf gain
        if (vm.count("rx-gain")) {
            std::cout << boost::format("Setting RX Gain: %f dB...") % rx_gain
                      << std::endl;
            rx_usrp->set_rx_gain(rx_gain, channel);
            std::cout << boost::format("Actual RX Gain: %f dB...")
                             % rx_usrp->get_rx_gain(channel)
                      << std::endl
                      << std::endl;
        }

        // set the receive analog frontend filter bandwidth
        if (vm.count("rx-bw")) {
            std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (rx_bw / 1e6)
                      << std::endl;
            rx_usrp->set_rx_bandwidth(rx_bw, channel);
            std::cout << boost::format("Actual RX Bandwidth: %f MHz...")
                             % (rx_usrp->get_rx_bandwidth(channel) / 1e6)
                      << std::endl
                      << std::endl;
        }

        // set the receive antenna
        if (vm.count("rx-ant"))
            rx_usrp->set_rx_antenna(rx_ant, channel);
    }

    // Align times in the RX USRP (the TX USRP does not require time-syncing)
    if (rx_usrp->get_num_mboards() > 1) {
        rx_usrp->set_time_unknown_pps(uhd::time_spec_t(0.0));
    }

    // for the const wave, set the wave freq for small samples per period
    if (wave_freq == 0 and wave_type == "CONST") {
        wave_freq = tx_usrp->get_tx_rate() / 2;
    }

    // error when the waveform is not possible to generate
    if (std::abs(wave_freq) > tx_usrp->get_tx_rate() / 2) {
        throw std::runtime_error("wave freq out of Nyquist zone");
    }
    if (tx_usrp->get_tx_rate() / std::abs(wave_freq) > wave_table_len / 2) {
        throw std::runtime_error("wave freq too small for table");
    }

    // pre-compute the waveform values
    const wave_table_class wave_table(wave_type, ampl);
    const size_t step = std::lround(wave_freq / tx_usrp->get_tx_rate() * wave_table_len);
    size_t index      = 0;

    // create a transmit streamer
    // linearly map channels (index0 = channel0, index1 = channel1, ...)
    uhd::stream_args_t stream_args("fc32", otw);
    stream_args.channels             = tx_channel_nums;
    uhd::tx_streamer::sptr tx_stream = tx_usrp->get_tx_stream(stream_args);

    // allocate a buffer which we re-use for each channel
    if (spb == 0)
        spb = tx_stream->get_max_num_samps() * 10;
    std::vector<std::complex<float>> buff(spb);
    int num_channels = tx_channel_nums.size();

    // setup the metadata flags
    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = true;
    md.time_spec = uhd::time_spec_t(0.5); // give us 0.5 seconds to fill the tx buffers

    // Check Ref and LO Lock detect
    std::vector<std::string> tx_sensor_names, rx_sensor_names;
    tx_sensor_names = tx_usrp->get_tx_sensor_names(0);
    if (std::find(tx_sensor_names.begin(), tx_sensor_names.end(), "lo_locked")
        != tx_sensor_names.end()) {
        uhd::sensor_value_t lo_locked = tx_usrp->get_tx_sensor("lo_locked", 0);
        std::cout << boost::format("Checking TX: %s ...") % lo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }
    rx_sensor_names = rx_usrp->get_rx_sensor_names(0);
    if (std::find(rx_sensor_names.begin(), rx_sensor_names.end(), "lo_locked")
        != rx_sensor_names.end()) {
        uhd::sensor_value_t lo_locked = rx_usrp->get_rx_sensor("lo_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % lo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }

    tx_sensor_names = tx_usrp->get_mboard_sensor_names(0);
    if ((ref == "mimo")
        and (std::find(tx_sensor_names.begin(), tx_sensor_names.end(), "mimo_locked")
             != tx_sensor_names.end())) {
        uhd::sensor_value_t mimo_locked = tx_usrp->get_mboard_sensor("mimo_locked", 0);
        std::cout << boost::format("Checking TX: %s ...") % mimo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(mimo_locked.to_bool());
    }
    if ((ref == "external")
        and (std::find(tx_sensor_names.begin(), tx_sensor_names.end(), "ref_locked")
             != tx_sensor_names.end())) {
        uhd::sensor_value_t ref_locked = tx_usrp->get_mboard_sensor("ref_locked", 0);
        std::cout << boost::format("Checking TX: %s ...") % ref_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(ref_locked.to_bool());
    }

    rx_sensor_names = rx_usrp->get_mboard_sensor_names(0);
    if ((ref == "mimo")
        and (std::find(rx_sensor_names.begin(), rx_sensor_names.end(), "mimo_locked")
             != rx_sensor_names.end())) {
        uhd::sensor_value_t mimo_locked = rx_usrp->get_mboard_sensor("mimo_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % mimo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(mimo_locked.to_bool());
    }
    if ((ref == "external")
        and (std::find(rx_sensor_names.begin(), rx_sensor_names.end(), "ref_locked")
             != rx_sensor_names.end())) {
        uhd::sensor_value_t ref_locked = rx_usrp->get_mboard_sensor("ref_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % ref_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(ref_locked.to_bool());
    }

    if (total_num_samps == 0) {
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }

    // reset usrp time to prepare for transmit/receive
    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    tx_usrp->set_time_now(uhd::time_spec_t(0.0));

    // start transmit worker thread
    std::thread transmit_thread([&]() {
        transmit_worker(buff, wave_table, tx_stream, md, step, index, num_channels);
    });

    // recv to file
    if (type == "double")
        recv_to_file<std::complex<double>>(
            rx_usrp, "fc64", otw, file, spb, total_num_samps, settling, rx_channel_nums);
    else if (type == "float")
        recv_to_file<std::complex<float>>(
            rx_usrp, "fc32", otw, file, spb, total_num_samps, settling, rx_channel_nums);
    else if (type == "short")
        recv_to_file<std::complex<short>>(
            rx_usrp, "sc16", otw, file, spb, total_num_samps, settling, rx_channel_nums);
    else {
        // clean up transmit worker
        stop_signal_called = true;
        transmit_thread.join();
        throw std::runtime_error("Unknown type " + type);
    }

    // clean up transmit worker
    stop_signal_called = true;
    transmit_thread.join();

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
