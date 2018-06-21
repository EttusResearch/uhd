//
// Copyright 2011-2015 Ettus Research LLC
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
#include <uhd/convert.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
//#include <boost/atomic.hpp>
#include <iostream>
#include <complex>
#include <cstdlib>

namespace po = boost::program_options;

const double CLOCK_TIMEOUT = 1000;  // 1000mS timeout for external clock locking
const double INIT_DELAY    = 0.05;  // 50mS initial delay before transmit
//typedef boost::atomic<bool>   atomic_bool;
// We'll fake atomic bools for now, for more backward compat.
// This is just an example, after all.
typedef bool atomic_bool;

/***********************************************************************
 * Test result variables
 **********************************************************************/
unsigned long long num_overflows = 0;
unsigned long long num_underflows = 0;
unsigned long long num_rx_samps = 0;
unsigned long long num_tx_samps = 0;
unsigned long long num_dropped_samps = 0;
unsigned long long num_seq_errors = 0;
unsigned long long num_late_commands = 0;
unsigned long long num_timeouts = 0;

/***********************************************************************
 * Benchmark RX Rate
 **********************************************************************/
void benchmark_rx_rate(
        uhd::usrp::multi_usrp::sptr usrp,
        const std::string &rx_cpu,
        uhd::rx_streamer::sptr rx_stream,
        bool random_nsamps,
        atomic_bool& burst_timer_elapsed
) {
    uhd::set_thread_priority_safe();

    //print pre-test summary
    std::cout << boost::format(
        "Testing receive rate %f Msps on %u channels"
    ) % (usrp->get_rx_rate()/1e6) % rx_stream->get_num_channels() << std::endl;

    //setup variables and allocate buffer
    uhd::rx_metadata_t md;
    const size_t max_samps_per_packet = rx_stream->get_max_num_samps();
    std::vector<char> buff(max_samps_per_packet*uhd::convert::get_bytes_per_item(rx_cpu));
    std::vector<void *> buffs;
    for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); //same buffer for each channel
    bool had_an_overflow = false;
    uhd::time_spec_t last_time;
    const double rate = usrp->get_rx_rate();

    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(INIT_DELAY);
    cmd.stream_now = (buffs.size() == 1);
    rx_stream->issue_stream_cmd(cmd);

    const float burst_pkt_time = std::max(0.100, (2 * max_samps_per_packet/rate));
    float recv_timeout = burst_pkt_time + INIT_DELAY;

    bool stop_called = false;
    while (true) {
        //if (burst_timer_elapsed.load(boost::memory_order_relaxed) and not stop_called) {
        if (burst_timer_elapsed and not stop_called) {
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            stop_called = true;
        }
        if (random_nsamps) {
            cmd.num_samps = rand() % max_samps_per_packet;
            rx_stream->issue_stream_cmd(cmd);
        }
        try {
            num_rx_samps += rx_stream->recv(buffs, max_samps_per_packet, md, recv_timeout)*rx_stream->get_num_channels();
            recv_timeout = burst_pkt_time;
        }
        catch (uhd::io_error &e) {
            std::cerr << "Caught an IO exception. " << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        //handle the error codes
        switch(md.error_code){
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
            if (had_an_overflow){
                had_an_overflow = false;
                num_dropped_samps += (md.time_spec - last_time).to_ticks(rate);
            }
            if ((burst_timer_elapsed or stop_called) and md.end_of_burst)
            {
                return;
            }
            break;

        // ERROR_CODE_OVERFLOW can indicate overflow or sequence error
        case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
            last_time = md.time_spec;
            had_an_overflow = true;
            // check out_of_sequence flag to see if it was a sequence error or overflow
            if (!md.out_of_sequence)
                num_overflows++;
            break;

        case uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND:
            std::cerr << "Receiver error: " << md.strerror() << ", restart streaming..."<< std::endl;
            num_late_commands++;
            // Radio core will be in the idle state. Issue stream command to restart streaming.
            cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(0.05);
            cmd.stream_now = (buffs.size() == 1);
            rx_stream->issue_stream_cmd(cmd);
            break;

        case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
            if (burst_timer_elapsed) {
                return;
            }
            std::cerr << "Receiver error: " << md.strerror() << ", continuing..." << std::endl;
            num_timeouts++;
            break;

            // Otherwise, it's an error
        default:
            std::cerr << "Receiver error: " << md.strerror() << std::endl;
            std::cerr << "Unexpected error on recv, continuing..." << std::endl;
            break;
        }
    }
}

/***********************************************************************
 * Benchmark TX Rate
 **********************************************************************/
void benchmark_tx_rate(
        uhd::usrp::multi_usrp::sptr usrp,
        const std::string &tx_cpu,
        uhd::tx_streamer::sptr tx_stream,
        atomic_bool& burst_timer_elapsed,
        bool random_nsamps=false
) {
    uhd::set_thread_priority_safe();

    //print pre-test summary
    std::cout << boost::format(
        "Testing transmit rate %f Msps on %u channels"
    ) % (usrp->get_tx_rate()/1e6) % tx_stream->get_num_channels() << std::endl;

    //setup variables and allocate buffer
    uhd::tx_metadata_t md;
    md.time_spec = usrp->get_time_now() + uhd::time_spec_t(INIT_DELAY);
    md.has_time_spec = (tx_stream->get_num_channels() > 1);
    const size_t max_samps_per_packet = tx_stream->get_max_num_samps();
    std::vector<char> buff(max_samps_per_packet*uhd::convert::get_bytes_per_item(tx_cpu));
    std::vector<const void *> buffs;
    for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); //same buffer for each channel
    md.has_time_spec = (buffs.size() != 1);

    if (random_nsamps) {
        std::srand((unsigned int)time(NULL));
        //while (not burst_timer_elapsed.load(boost::memory_order_relaxed)) {
        while (not burst_timer_elapsed) {
            size_t total_num_samps = rand() % max_samps_per_packet;
            size_t num_acc_samps = 0;
            const float timeout = 1;

            usrp->set_time_now(uhd::time_spec_t(0.0));
            while(num_acc_samps < total_num_samps){
                //send a single packet
                num_tx_samps += tx_stream->send(buffs, max_samps_per_packet, md, timeout)*tx_stream->get_num_channels();
                num_acc_samps += std::min(total_num_samps-num_acc_samps, tx_stream->get_max_num_samps());
            }
        }
    } else {
        //while (not burst_timer_elapsed.load(boost::memory_order_relaxed)) {
        while (not burst_timer_elapsed) {
            num_tx_samps += tx_stream->send(buffs, max_samps_per_packet, md)*tx_stream->get_num_channels();
            md.has_time_spec = false;
        }
    }

    //send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send(buffs, 0, md);
}

void benchmark_tx_rate_async_helper(
        uhd::tx_streamer::sptr tx_stream,
        atomic_bool& burst_timer_elapsed
) {
    //setup variables and allocate buffer
    uhd::async_metadata_t async_md;
    bool exit_flag = false;

    while (true) {
        //if (burst_timer_elapsed.load(boost::memory_order_relaxed)) {
        if (burst_timer_elapsed) {
            exit_flag = true;
        }

        if (not tx_stream->recv_async_msg(async_md)) {
            if (exit_flag == true)
                return;
            continue;
        }

        //handle the error codes
        switch(async_md.event_code){
        case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
            return;

        case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
        case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET:
            num_underflows++;
            break;

        case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR:
        case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST:
            num_seq_errors++;
            break;

        default:
            std::cerr << "Event code: " << async_md.event_code << std::endl;
            std::cerr << "Unexpected event on async recv, continuing..." << std::endl;
            break;
        }
    }
}

/***********************************************************************
 * Main code + dispatcher
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    std::string rx_subdev, tx_subdev;
    double duration;
    double rx_rate, tx_rate;
    std::string rx_otw, tx_otw;
    std::string rx_cpu, tx_cpu;
    std::string mode, ref, pps;
    std::string channel_list, rx_channel_list, tx_channel_list;
    bool random_nsamps = false;
    atomic_bool burst_timer_elapsed(false);

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("duration", po::value<double>(&duration)->default_value(10.0), "duration for the test in seconds")
        ("rx_subdev", po::value<std::string>(&rx_subdev), "specify the device subdev for RX")
        ("tx_subdev", po::value<std::string>(&tx_subdev), "specify the device subdev for TX")
        ("rx_rate", po::value<double>(&rx_rate), "specify to perform a RX rate test (sps)")
        ("tx_rate", po::value<double>(&tx_rate), "specify to perform a TX rate test (sps)")
        ("rx_otw", po::value<std::string>(&rx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for RX")
        ("tx_otw", po::value<std::string>(&tx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for TX")
        ("rx_cpu", po::value<std::string>(&rx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for RX")
        ("tx_cpu", po::value<std::string>(&tx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for TX")
        ("ref", po::value<std::string>(&ref), "clock reference (internal, external, mimo, gpsdo)")
        ("pps", po::value<std::string>(&pps), "PPS source (internal, external, mimo, gpsdo)")
        ("mode", po::value<std::string>(&mode), "DEPRECATED - use \"ref\" and \"pps\" instead (none, mimo)")
        ("random", "Run with random values of samples in send() and recv() to stress-test the I/O.")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("rx_channels", po::value<std::string>(&rx_channel_list), "which RX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("tx_channels", po::value<std::string>(&tx_channel_list), "which TX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help") or (vm.count("rx_rate") + vm.count("tx_rate")) == 0){
        std::cout << boost::format("UHD Benchmark Rate %s") % desc << std::endl;
        std::cout <<
        "    Specify --rx_rate for a receive-only test.\n"
        "    Specify --tx_rate for a transmit-only test.\n"
        "    Specify both options for a full-duplex test.\n"
        << std::endl;
        return ~0;
    }

    // Random number of samples?
    if (vm.count("random")) {
        std::cout << "Using random number of samples in send() and recv() calls." << std::endl;
        random_nsamps = true;
    }

    if (vm.count("mode")) {
        if (vm.count("pps") or vm.count("ref")) {
            std::cout << "ERROR: The \"mode\" parameter cannot be used with the \"ref\" and \"pps\" parameters.\n" << std::endl;
            return -1;
        } else if (mode == "mimo") {
            ref = pps = "mimo";
            std::cout << "The use of the \"mode\" parameter is deprecated.  Please use \"ref\" and \"pps\" parameters instead\n" << std::endl;
        }
    }

    //create a usrp device
    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(args, uhd::device::USRP);
    if (not device_addrs.empty() and device_addrs.at(0).get("type", "") == "usrp1"){
        std::cerr << "*** Warning! ***" << std::endl;
        std::cerr << "Benchmark results will be inaccurate on USRP1 due to insufficient features.\n" << std::endl;
    }
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    //always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("rx_subdev")) {
        usrp->set_rx_subdev_spec(rx_subdev);
    }
    if (vm.count("tx_subdev")) {
        usrp->set_tx_subdev_spec(tx_subdev);
    }

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
    int num_mboards = usrp->get_num_mboards();

    boost::thread_group thread_group;

    if(vm.count("ref"))
    {
        if (ref == "mimo")
        {
            if (num_mboards != 2) {
                std::cerr << "ERROR: ref = \"mimo\" implies 2 motherboards; your system has " << num_mboards << " boards" << std::endl;
                return -1;
            }
            usrp->set_clock_source("mimo",1);
        } else {
            usrp->set_clock_source(ref);
        }

        if(ref != "internal") {
            std::cout << "Now confirming lock on clock signals..." << std::endl;
            bool is_locked = false;
            boost::system_time end_time = boost::get_system_time() + boost::posix_time::milliseconds(int64_t(CLOCK_TIMEOUT));
            for (int i = 0; i < num_mboards; i++) {
                if (ref == "mimo" and i == 0) continue;
                while((is_locked = usrp->get_mboard_sensor("ref_locked",i).to_bool()) == false and
                            boost::get_system_time() < end_time )
                {
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1));
                }
                if (is_locked == false) {
                    std::cerr << "ERROR: Unable to confirm clock signal locked on board:" << i <<  std::endl;
                    return -1;
                }
                is_locked = false;
            }
        }
    }

    if(vm.count("pps"))
    {
       if(pps == "mimo")
       {
           if (num_mboards != 2) {
               std::cerr << "ERROR: ref = \"mimo\" implies 2 motherboards; your system has " << num_mboards << " boards" << std::endl;
               return -1;
           }
           //make mboard 1 a slave over the MIMO Cable
           usrp->set_time_source("mimo", 1);
       } else {
           usrp->set_time_source(pps);
       }
    }

    //check that the device has sufficient RX and TX channels available
    std::vector<std::string> channel_strings;
    std::vector<size_t> rx_channel_nums;
    if (vm.count("rx_rate")) {
        if (!vm.count("rx_channels")) {
            rx_channel_list = channel_list;
        }

        boost::split(channel_strings, rx_channel_list, boost::is_any_of("\"',"));
        for (size_t ch = 0; ch < channel_strings.size(); ch++) {
            size_t chan = boost::lexical_cast<int>(channel_strings[ch]);
            if (chan >= usrp->get_rx_num_channels()) {
                throw std::runtime_error("Invalid channel(s) specified.");
            } else {
                rx_channel_nums.push_back(boost::lexical_cast<int>(channel_strings[ch]));
            }
        }
    }

    std::vector<size_t> tx_channel_nums;
    if (vm.count("tx_rate")) {
        if (!vm.count("tx_channels")) {
            tx_channel_list = channel_list;
        }

        boost::split(channel_strings, tx_channel_list, boost::is_any_of("\"',"));
        for (size_t ch = 0; ch < channel_strings.size(); ch++) {
            size_t chan = boost::lexical_cast<int>(channel_strings[ch]);
            if (chan >= usrp->get_tx_num_channels()) {
                throw std::runtime_error("Invalid channel(s) specified.");
            } else {
                tx_channel_nums.push_back(boost::lexical_cast<int>(channel_strings[ch]));
            }
        }
    }

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    const bool sync_channels =
            pps == "mimo" or
            ref == "mimo" or
            rx_channel_nums.size() > 1 or
            tx_channel_nums.size() > 1
    ;
    if (!sync_channels) {
       usrp->set_time_now(0.0);
    } else {
       usrp->set_time_unknown_pps(uhd::time_spec_t(0.0));
    }

    //spawn the receive test thread
    if (vm.count("rx_rate")){
        usrp->set_rx_rate(rx_rate);
        //create a receive streamer
        uhd::stream_args_t stream_args(rx_cpu, rx_otw);
        stream_args.channels = rx_channel_nums;
        uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
        thread_group.create_thread(boost::bind(&benchmark_rx_rate, usrp, rx_cpu, rx_stream, random_nsamps, boost::ref(burst_timer_elapsed)));
    }

    //spawn the transmit test thread
    if (vm.count("tx_rate")){
        usrp->set_tx_rate(tx_rate);
        //create a transmit streamer
        uhd::stream_args_t stream_args(tx_cpu, tx_otw);
        stream_args.channels = tx_channel_nums;
        uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
        thread_group.create_thread(boost::bind(&benchmark_tx_rate, usrp, tx_cpu, tx_stream, boost::ref(burst_timer_elapsed), random_nsamps));
        thread_group.create_thread(boost::bind(&benchmark_tx_rate_async_helper, tx_stream, boost::ref(burst_timer_elapsed)));
    }

    //sleep for the required duration
    const long secs = long(duration);
    const long usecs = long((duration - secs)*1e6);
    boost::this_thread::sleep(boost::posix_time::seconds(secs)
            + boost::posix_time::microseconds(usecs)
            + boost::posix_time::milliseconds(int64_t((rx_channel_nums.size() <= 1 and tx_channel_nums.size() <= 1) ? 0 : (INIT_DELAY * 1000)))
    );

    //interrupt and join the threads
    //burst_timer_elapsed.store(true, boost::memory_order_relaxed);
    burst_timer_elapsed = true;
    thread_group.join_all();

    //print summary
    std::cout << std::endl << boost::format(
        "Benchmark rate summary:\n"
        "  Num received samples:    %u\n"
        "  Num dropped samples:     %u\n"
        "  Num overflows detected:  %u\n"
        "  Num transmitted samples: %u\n"
        "  Num sequence errors:     %u\n"
        "  Num underflows detected: %u\n"
        "  Num late commands:       %u\n"
        "  Num timeouts:            %u\n"
    ) % num_rx_samps % num_dropped_samps
      % num_overflows % num_tx_samps
      % num_seq_errors % num_underflows
      % num_late_commands % num_timeouts
      << std::endl;

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
