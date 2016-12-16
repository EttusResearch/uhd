//
// Copyright 2016 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

#include <fstream>
#include <iostream>
#include <complex>
#include <utility>

// FFT conversion
#include "ascii_art_dft.hpp"

/*
 * This example shows how to implement fast frequency hopping using an X-Series
 * motherboard and a TwinRX daughterboard.
 *
 * The TwinRX daughterboard is different than previous daughterboards in that it
 * has two RX channels and two LOs. Either channel can be set to use either LO,
 * allowing for the two channels to share an LO source.
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
 * 2. Use timed commands to tell the TwinRX to send samples to the host at given intervals.
 * 3. For each frequency, tune the LOs for the inactive channel for the next frequency and
 *    receive at the current frequency.
 * 4. If applicable, send the next timed command for streaming.
 */

namespace pt = boost::posix_time;
namespace po = boost::program_options;

typedef std::pair<double, double> lo_freqs_t;
typedef std::vector<std::complex<float> > recv_buff_t;
typedef std::vector<recv_buff_t> recv_buffs_t;

double pipeline_time;

// Global objects
static uhd::usrp::multi_usrp::sptr usrp;
static uhd::rx_streamer::sptr rx_stream;
static recv_buffs_t buffs;
static size_t recv_spb, spb;

static std::vector<double> rf_freqs;
static std::vector<lo_freqs_t> lo_freqs;

static uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
static uhd::time_spec_t pipeline_timespec;
static size_t last_cmd_index;

// Determine the active channel (hooked to antenna) and the slave channel
size_t ACTIVE_CHAN = 0;
size_t UNUSED_CHAN = 1;

const std::string ALL_STAGES = "all";

const int X300_COMMAND_FIFO_DEPTH = 16;

static void twinrx_recv(size_t index) {
    size_t num_acc_samps = 0;
    uhd::rx_metadata_t md;

    while(num_acc_samps < spb) {
        size_t num_to_recv = std::min<size_t>(recv_spb, (spb - num_acc_samps));

        size_t num_recvd = rx_stream->recv(
            &buffs[index][num_acc_samps],
            num_to_recv, md, pipeline_time
        );

        if(md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            std::cout << index << " " << md.strerror() << std::endl;
            break;
        }

        num_acc_samps += num_recvd;
    }

    // Send the next stream_cmd
    if(last_cmd_index < buffs.size()) {
        stream_cmd.time_spec += pipeline_timespec;
        rx_stream->issue_stream_cmd(stream_cmd);
        ++last_cmd_index;
    }
}

static void write_fft_to_file(const std::string &fft_path) {
    std::cout << "Creating FFT (this may take a while)..." << std::flush;
    std::ofstream ofile(fft_path.c_str(), std::ios::binary);
    BOOST_FOREACH(const recv_buff_t &buff, buffs) {
        std::vector<float> fft = acsii_art_dft::log_pwr_dft(&buff.front(), buff.size());
        ofile.write((char*)&fft[0], (sizeof(float)*fft.size()));
    }
    ofile.close();
    std::cout << "done." << std::endl;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    // Program options
    std::string args, fft_path, subdev;
    double rate, gain;
    double start_freq, end_freq;

    // Set up the program options
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help", "Print this help message")
    ("args", po::value<std::string>(&args)->default_value(""), "UHD device args")
    ("subdev", po::value<std::string>(&subdev)->default_value("A:0 A:1"), "Subdevice specification")
    ("start-freq", po::value<double>(&start_freq), "Start frequency (defaults to lowest valid frequency)")
    ("end-freq", po::value<double>(&end_freq), "End frequency (defaults to highest valid frequency)")
    ("pipeline-time", po::value<double>(&pipeline_time)->default_value(5e-3), "Time spent tuning and receiving")
    ("rate", po::value<double>(&rate)->default_value(1e6), "Incoming sample rate")
    ("gain", po::value<double>(&gain)->default_value(60), "RX gain")
    ("spb", po::value<size_t>(&spb)->default_value(1024), "Samples per buffer")
    ("fft-path", po::value<std::string>(&fft_path), "Output an FFT to this file (optional)")
    ("repeat", "repeat loop until Ctrl-C is pressed")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("help")) {
        std::cout << "TwinRX Example - " << desc << std::endl;
        return EXIT_SUCCESS;
    }

    // Create a USRP device
    std::cout << std::endl;
    std::cout << boost::format("Creating the USRP device with args: \"%s\"...") % args << std::endl;
    usrp = uhd::usrp::multi_usrp::make(args);

    // Make sure this is an X3xx with a TwinRX
    uhd::dict<std::string, std::string> info = usrp->get_usrp_rx_info();
    if(info.get("mboard_id").find("X3") == std::string::npos) {
        throw uhd::runtime_error("This example can only be used with an X-Series device.");
    }
    if(info.get("rx_id").find("TwinRX") == std::string::npos) {
        throw uhd::runtime_error("This example can only be used with a TwinRX daughterboard.");
    }

    // Validate frequency range
    uhd::freq_range_t rx_freq_range = usrp->get_rx_freq_range();
    if(!vm.count("start-freq")) {
        start_freq = rx_freq_range.start();
    }
    if(!vm.count("end-freq")) {
        end_freq = rx_freq_range.stop();
    }
    if(start_freq > end_freq) {
        throw uhd::runtime_error("Start frequency must be less than end frequency.");
    }
    if((end_freq - start_freq) > 0 and (end_freq - start_freq) < rate) {
        throw uhd::runtime_error("The sample rate must be less than the range between the start and end frequencies.");
    }

    // Set TwinRX settings
    usrp->set_rx_subdev_spec(uhd::usrp::subdev_spec_t(subdev));
    usrp->set_rx_antenna("RX1", 0);
    usrp->set_rx_antenna("RX2", 1);
    
    // Disable the LO for the unused channel
    usrp->set_rx_lo_source("disabled", ALL_STAGES, UNUSED_CHAN);

    // Set user settings
    std::cout << boost::format("\nSetting sample rate to: %d") % rate << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual sample rate:     %d") % usrp->get_rx_rate() << std::endl;

    std::cout << boost::format("\nSetting gain to: %d") % gain << std::endl;
    usrp->set_rx_gain(gain);
    std::cout << boost::format("Actual gain:     %d") % usrp->get_rx_gain() << std::endl;

    // Get a stream from the device
    uhd::stream_args_t stream_args("fc32", "sc16");
    stream_args.channels.push_back(0);
    rx_stream = usrp->get_rx_stream(stream_args);
    recv_spb = rx_stream->get_max_num_samps();

    // Calculate the frequency hops
    for(double rx_freq = start_freq; rx_freq <= end_freq; rx_freq += rate) {
        rf_freqs.push_back(rx_freq);
    }
    std::cout << boost::format("\nTotal Hops: %d") % rf_freqs.size() << std::endl;

    // Set up buffers
    buffs = recv_buffs_t(
        rf_freqs.size(), recv_buff_t(spb)
        );
    
    while(1){
        /*
         * Each receive+tune time gets a set amount of time before moving on to the next. However,
         * the software needs some lead time before the USRP starts to stream the next set of samples.
         */
        pipeline_timespec = uhd::time_spec_t(pipeline_time);
        pt::time_duration polltime_ptime = pt::milliseconds(pipeline_time*1000) - pt::microseconds(20);
        uhd::time_spec_t polltime_duration(double(polltime_ptime.total_microseconds()) / 1e9);

        /*
         * Send some initial timed commands to get started and send the rest as necessary
         * after receiving.
         */
        stream_cmd.num_samps = spb;
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = uhd::time_spec_t(0.0);
        usrp->set_time_now(uhd::time_spec_t(0.0));
        size_t num_initial_cmds = std::min<size_t>(X300_COMMAND_FIFO_DEPTH, rf_freqs.size());
        for(last_cmd_index = 0; last_cmd_index < num_initial_cmds; ++last_cmd_index) {
            stream_cmd.time_spec += pipeline_timespec;
            rx_stream->issue_stream_cmd(stream_cmd);
        }

        std::cout << "\nScanning..." << std::flush;
        uhd::time_spec_t start_time = uhd::time_spec_t::get_system_time();

        // The first pipeline segment is just tuning for the first receive
        uhd::time_spec_t polltime = usrp->get_time_now() + polltime_duration;

        // Initialize the first LO frequency
        usrp->set_rx_freq(rf_freqs[0], ACTIVE_CHAN);

        while(usrp->get_time_now() < polltime);

        for (size_t i = 0; i < rf_freqs.size() - 1; i++) {
           polltime = usrp->get_time_now() + polltime_duration;
           
           // Swap synthesizers by setting the LO source
           std::string lo_src = (i % 2) ? "companion" : "internal";
           usrp->set_rx_lo_source(lo_src, ALL_STAGES, ACTIVE_CHAN);

           // Preconfigure the next frequency
           usrp->set_rx_freq(rf_freqs[i+1], UNUSED_CHAN);

           // Program the current frequency
           // This frequency was already pre-programmed in the previous iteration
           // so this call will only configure front-end filter, etc
           usrp->set_rx_freq(rf_freqs[i], ACTIVE_CHAN);

           twinrx_recv(i);

           while(usrp->get_time_now() < polltime);
        }

        uhd::time_spec_t end_time = uhd::time_spec_t::get_system_time();
        std::cout << boost::format("done in %d seconds.\n") % (end_time - start_time).get_real_secs();

        // Optionally convert received samples to FFT and write to file
        if(vm.count("fft-path")) {
            write_fft_to_file(fft_path);
        }

        std::cout << std::endl << "Done!" << std::endl << std::endl;

        if (!vm.count("repeat")){
            break;
        }

    }
    return EXIT_SUCCESS;
}

