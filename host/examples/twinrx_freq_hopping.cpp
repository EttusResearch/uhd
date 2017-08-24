//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// FFT conversion
#include "ascii_art_dft.hpp"

#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>

#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread_time.hpp>

#include <fstream>

/*
 * This example shows how to implement fast frequency hopping using an X-Series
 * motherboard and a TwinRX daughterboard.
 *
 * The TwinRX daughterboard is different than previous daughterboards in that it has two
 * RX channels, each with a set of Local Oscillators (LOs). Either channel can be configured
 * to use either LO set, allowing for the two channels to share an LO source.
 *
 * The TwinRX can be used like any other daughterboard, as the multi_usrp::set_rx_freq()
 * function will automatically calculate and set the two LO frequencies as needed.
 * However, this adds to the overall tuning time. If the LO frequencies are manually set
 * with the multi_usrp::set_rx_lo_freq() function, the TwinRX will will not perform the
 * calculation itself, resulting in a faster tune time. This example shows how to take
 * advantage of this as follows:
 *
 * 1. Tune across the given frequency range, storing the calculated LO frequencies along
 *    the way.
 * 2. Use timed commands to tell the TwinRX to receive bursts of samples at given intervals.
 * 3. For each frequency, tune the LOs for the inactive channel for the next frequency and
 *    receive at the current frequency.
 * 4. If applicable, send the next timed command for streaming.
 */

namespace pt = boost::posix_time;
namespace po = boost::program_options;

typedef std::vector<std::complex<float> > recv_buff_t;
typedef std::vector<recv_buff_t> recv_buffs_t;

// Global objects
static uhd::usrp::multi_usrp::sptr usrp;
static uhd::rx_streamer::sptr rx_stream;
static recv_buffs_t buffs;
static size_t recv_spb, spb;

static std::vector<double> rf_freqs;

static uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);

double receive_interval;

// Define the active channel (connected to antenna) and the unused channel
size_t ACTIVE_CHAN = 0;
size_t UNUSED_CHAN = 1;

const int X300_COMMAND_FIFO_DEPTH = 16;


// This is a helper function for receiving samples from the USRP
static void twinrx_recv(recv_buff_t &buffer) {

    size_t num_acc_samps = 0;
    uhd::rx_metadata_t md;

    // Repeatedly retrieve samples until the entire acquisition is received
    while (num_acc_samps < spb) {
        size_t num_to_recv = std::min<size_t>(recv_spb, (spb - num_acc_samps));

        // recv call will block until samples are ready or the call times out
        size_t num_recvd = rx_stream->recv(&buffer[num_acc_samps], num_to_recv, md, receive_interval);

        if(md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            std::cout << md.strerror() << std::endl;
            break;
        }
        num_acc_samps += num_recvd;
    }
}

// Function to write the acquisition FFT to a binary file
static void write_fft_to_file(const std::string &fft_path) {
    std::cout << "Calculating FFTs (this may take a while)... " << std::flush;
    std::ofstream ofile(fft_path.c_str(), std::ios::binary);
    BOOST_FOREACH(const recv_buff_t &buff, buffs) {
                    std::vector<float> fft = ascii_art_dft::log_pwr_dft(&buff.front(), buff.size());
                    ofile.write((char*)&fft[0], (sizeof(float)*fft.size()));
                }
    ofile.close();
    std::cout << "done." << std::endl;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    // Program options
    std::string args, fft_path, subdev, ant;
    double rate, gain;
    double start_freq, end_freq;

    // Set up the program options
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "Print this help message")
            ("args", po::value<std::string>(&args)->default_value(""), "UHD device args")
            ("subdev", po::value<std::string>(&subdev)->default_value("A:0 A:1"), "Subdevice specification")
            ("ant", po::value<std::string>(&ant)->default_value("RX1"), "RX Antenna")
            ("start-freq", po::value<double>(&start_freq), "Start frequency (defaults to lowest valid frequency)")
            ("end-freq", po::value<double>(&end_freq), "End frequency (defaults to highest valid frequency)")
            ("receive-interval", po::value<double>(&receive_interval)->default_value(5e-3), "Interval between scheduled receives")
            ("rate", po::value<double>(&rate)->default_value(1e6), "Incoming sample rate")
            ("gain", po::value<double>(&gain)->default_value(60), "RX gain")
            ("spb", po::value<size_t>(&spb)->default_value(1024), "Samples per buffer")
            ("fft-path", po::value<std::string>(&fft_path), "Output an FFT to this file (optional)")
            ("repeat", "repeat sweep until Ctrl-C is pressed")
            ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("help")) {
        std::cout << "TwinRX Frequency Hopping Example - " << desc << std::endl;
        return EXIT_SUCCESS;
    }

    // Create a USRP device
    std::cout << boost::format("\nCreating the USRP device with args: \"%s\"...\n") % args;
    usrp = uhd::usrp::multi_usrp::make(args);

    // Make sure the USRP is an X3xx with a TwinRX
    uhd::dict<std::string, std::string> info = usrp->get_usrp_rx_info();
    if(info.get("mboard_id").find("X3") == std::string::npos) {
        throw uhd::runtime_error("This example can only be used with an X-Series motherboard.");
    }
    if(info.get("rx_id").find("TwinRX") == std::string::npos) {
        throw uhd::runtime_error("This example can only be used with a TwinRX daughterboard.");
    }

    // Validate frequency range
    uhd::freq_range_t rx_freq_range = usrp->get_rx_freq_range();
    if (!vm.count("start-freq")) {
        start_freq = rx_freq_range.start();
    }
    if (!vm.count("end-freq")) {
        end_freq = rx_freq_range.stop();
    }
    if (start_freq < rx_freq_range.start() or end_freq > rx_freq_range.stop()) {
        throw uhd::runtime_error((boost::format("Start and stop frequencies must be between %d and %d MHz")
                                            % (rx_freq_range.start() / 1e6) % (rx_freq_range.stop() / 1e6)).str());
    }
    if (start_freq > end_freq) {
        throw uhd::runtime_error("Start frequency must be less than end frequency.");
    }
    if ((end_freq - start_freq) > 0 and (end_freq - start_freq) < rate) {
        throw uhd::runtime_error("The sample rate must be less than the range between the start and end frequencies.");
    }

    // Set TwinRX settings
    usrp->set_rx_subdev_spec(subdev);

    // Set the unused channel to not use any LOs. This allows the active channel to control them.
    usrp->set_rx_lo_source("disabled", uhd::usrp::multi_usrp::ALL_LOS, UNUSED_CHAN);

    // Set user settings
    std::cout << boost::format("Setting antenna to:     %s\n") % ant;
    usrp->set_rx_antenna(ant, ACTIVE_CHAN);
    std::cout << boost::format("Actual antenna:         %s\n") % usrp->get_rx_antenna(ACTIVE_CHAN);

    std::cout << boost::format("Setting sample rate to: %d\n") % rate;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual sample rate:     %d\n") % usrp->get_rx_rate();

    std::cout << boost::format("Setting gain to: %d\n") % gain;
    usrp->set_rx_gain(gain);
    std::cout << boost::format("Actual gain:     %d\n") % usrp->get_rx_gain();

    // Get an rx_streamer from the device
    uhd::stream_args_t stream_args("fc32", "sc16");
    stream_args.channels.push_back(0);
    rx_stream = usrp->get_rx_stream(stream_args);
    recv_spb = rx_stream->get_max_num_samps();

    // Calculate the frequency hops
    for (double rx_freq = start_freq; rx_freq <= end_freq; rx_freq += rate) {
        rf_freqs.push_back(rx_freq);
    }
    std::cout << boost::format("Total Hops: %d\n") % rf_freqs.size();

    // Set up buffers
    buffs = recv_buffs_t(rf_freqs.size(), recv_buff_t(spb));

    // Tune the active channel to the first frequency and reset the USRP's time
    usrp->set_rx_freq(rf_freqs[0], ACTIVE_CHAN);
    usrp->set_time_now(uhd::time_spec_t(0.0));

    // Configure the stream command which will be issued to acquire samples at each frequency
    stream_cmd.num_samps = spb;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec = uhd::time_spec_t(0.0);

    // Stream commands will be scheduled at regular intervals
    uhd::time_spec_t receive_interval_ts = uhd::time_spec_t(receive_interval);

    // Issue stream commands to fill the command queue on the FPGA
    size_t num_initial_cmds = std::min<size_t>(X300_COMMAND_FIFO_DEPTH, rf_freqs.size());
    size_t num_issued_commands;

    for (num_issued_commands = 0; num_issued_commands < num_initial_cmds; num_issued_commands++) {
        stream_cmd.time_spec += receive_interval_ts;
        rx_stream->issue_stream_cmd(stream_cmd);
    }

    // Hop frequencies and acquire bursts of samples at each until done sweeping
    while(1) {

        std::cout << "Scanning..." << std::endl;
        auto start_time = boost::get_system_time();

        for (size_t i = 0; i < rf_freqs.size(); i++) {
            // Swap the mapping of synthesizers by setting the LO source
            // The unused channel will always
            std::string lo_src = (i % 2) ? "companion" : "internal";
            usrp->set_rx_lo_source(lo_src, uhd::usrp::multi_usrp::ALL_LOS, ACTIVE_CHAN);

            // Preconfigure the next frequency
            usrp->set_rx_freq(rf_freqs[(i+1) % rf_freqs.size()], UNUSED_CHAN);

            // Program the current frequency
            // This frequency was already pre-programmed in the previous iteration so the local oscillators
            // are already tuned. This call will only configure front-end filter, amplifiers, etc
            usrp->set_rx_freq(rf_freqs[i], ACTIVE_CHAN);

            // Receive one burst of samples
            twinrx_recv(buffs[i]);

            // Schedule another acquisition if necessary
            if (vm.count("repeat") or num_issued_commands < rf_freqs.size()) {
                stream_cmd.time_spec += receive_interval_ts;
                rx_stream->issue_stream_cmd(stream_cmd);
                num_issued_commands++;
            }
        }

        auto end_time = boost::get_system_time();
        std::cout
            << boost::format("Sweep done in %d milliseconds.\n")
                % ((end_time - start_time).total_milliseconds() * 1000);

        // Optionally convert received samples to FFT and write to file
        if(vm.count("fft-path")) {
            write_fft_to_file(fft_path);
        }

        if (!vm.count("repeat")){
            break;
        }
    }

    std::cout << "Done!" << std::endl;

    usrp.reset();
    return EXIT_SUCCESS;
}

