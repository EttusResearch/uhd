//
// Copyright 2011-2013 Ettus Research LLC
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
#include <iostream>
#include <complex>
#include <cstdlib>

namespace po = boost::program_options;

/***********************************************************************
 * Test result variables
 **********************************************************************/
unsigned long long num_overflows = 0;
unsigned long long num_underflows = 0;
unsigned long long num_rx_samps = 0;
unsigned long long num_tx_samps = 0;
unsigned long long num_dropped_samps = 0;
unsigned long long num_seq_errors = 0;

/***********************************************************************
 * Benchmark RX Rate
 **********************************************************************/
void benchmark_rx_rate(uhd::usrp::multi_usrp::sptr usrp, const std::string &rx_cpu, uhd::rx_streamer::sptr rx_stream){
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
    cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(0.05);
    cmd.stream_now = (buffs.size() == 1);
    rx_stream->issue_stream_cmd(cmd);

    while (not boost::this_thread::interruption_requested()){
        try {
          num_rx_samps += rx_stream->recv(buffs, max_samps_per_packet, md)*rx_stream->get_num_channels();
        }
        catch (...) {
          /* apparently, the boost thread interruption can sometimes result in
             throwing exceptions not of type boost::exception, this catch allows
             this thread to still attempt to issue the STREAM_MODE_STOP_CONTINUOUS
          */
          break;
        }

        //handle the error codes
        switch(md.error_code){
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
            if (had_an_overflow){
                had_an_overflow = false;
                num_dropped_samps += (md.time_spec - last_time).to_ticks(rate);
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

        default:
            std::cerr << "Receiver error: " << md.strerror() << std::endl;
            std::cerr << "Unexpected error on recv, continuing..." << std::endl;
            break;
        }
    }
    rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
}

/***********************************************************************
 * Benchmark TX Rate
 **********************************************************************/
void benchmark_tx_rate(uhd::usrp::multi_usrp::sptr usrp, const std::string &tx_cpu, uhd::tx_streamer::sptr tx_stream){
    uhd::set_thread_priority_safe();

    //print pre-test summary
    std::cout << boost::format(
        "Testing transmit rate %f Msps on %u channels"
    ) % (usrp->get_tx_rate()/1e6) % tx_stream->get_num_channels() << std::endl;

    //setup variables and allocate buffer
    uhd::tx_metadata_t md;
    md.time_spec = usrp->get_time_now() + uhd::time_spec_t(0.05);
    const size_t max_samps_per_packet = tx_stream->get_max_num_samps();
    std::vector<char> buff(max_samps_per_packet*uhd::convert::get_bytes_per_item(tx_cpu));
    std::vector<const void *> buffs;
    for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); //same buffer for each channel
    md.has_time_spec = (buffs.size() != 1);

    while (not boost::this_thread::interruption_requested()){
        num_tx_samps += tx_stream->send(buffs, max_samps_per_packet, md)*tx_stream->get_num_channels();;
        md.has_time_spec = false;
    }

    //send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send(buffs, 0, md);
}

void benchmark_tx_rate_async_helper(uhd::tx_streamer::sptr tx_stream){
    //setup variables and allocate buffer
    uhd::async_metadata_t async_md;

    while (not boost::this_thread::interruption_requested()){

        if (not tx_stream->recv_async_msg(async_md)) continue;

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
    double duration;
    double rx_rate, tx_rate;
    std::string rx_otw, tx_otw;
    std::string rx_cpu, tx_cpu;
    std::string mode;
    std::string channel_list;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("duration", po::value<double>(&duration)->default_value(10.0), "duration for the test in seconds")
        ("rx_rate", po::value<double>(&rx_rate), "specify to perform a RX rate test (sps)")
        ("tx_rate", po::value<double>(&tx_rate), "specify to perform a TX rate test (sps)")
        ("rx_otw", po::value<std::string>(&rx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for RX")
        ("tx_otw", po::value<std::string>(&tx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for TX")
        ("rx_cpu", po::value<std::string>(&rx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for RX")
        ("tx_cpu", po::value<std::string>(&tx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for TX")
        ("mode", po::value<std::string>(&mode)->default_value("none"), "multi-channel sync mode option: none, mimo")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
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

    //create a usrp device
    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(args, uhd::device::USRP);
    if (not device_addrs.empty() and device_addrs.at(0).get("type", "") == "usrp1"){
        std::cerr << "*** Warning! ***" << std::endl;
        std::cerr << "Benchmark results will be inaccurate on USRP1 due to insufficient features.\n" << std::endl;
    }
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    if (mode == "mimo"){
        usrp->set_clock_source("mimo", 0);
        usrp->set_time_source("mimo", 0);
        boost::this_thread::sleep(boost::posix_time::seconds(1));
    }

    boost::thread_group thread_group;

    //detect which channels to use
    std::vector<std::string> channel_strings;
    std::vector<size_t> channel_nums;
    boost::split(channel_strings, channel_list, boost::is_any_of("\"',"));
    for(size_t ch = 0; ch < channel_strings.size(); ch++){
        size_t chan = boost::lexical_cast<int>(channel_strings[ch]);
        if(chan >= usrp->get_tx_num_channels() or chan >= usrp->get_rx_num_channels()){
            throw std::runtime_error("Invalid channel(s) specified.");
        }
        else channel_nums.push_back(boost::lexical_cast<int>(channel_strings[ch]));
    }

    //spawn the receive test thread
    if (vm.count("rx_rate")){
        usrp->set_rx_rate(rx_rate);
        //create a receive streamer
        uhd::stream_args_t stream_args(rx_cpu, rx_otw);
        stream_args.channels = channel_nums;
        uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
        thread_group.create_thread(boost::bind(&benchmark_rx_rate, usrp, rx_cpu, rx_stream));
    }

    //spawn the transmit test thread
    if (vm.count("tx_rate")){
        usrp->set_tx_rate(tx_rate);
        //create a transmit streamer
        uhd::stream_args_t stream_args(tx_cpu, tx_otw);
        stream_args.channels = channel_nums;
        uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
        thread_group.create_thread(boost::bind(&benchmark_tx_rate, usrp, tx_cpu, tx_stream));
        thread_group.create_thread(boost::bind(&benchmark_tx_rate_async_helper, tx_stream));
    }

    //sleep for the required duration
    const long secs = long(duration);
    const long usecs = long((duration - secs)*1e6);
    boost::this_thread::sleep(boost::posix_time::seconds(secs) + boost::posix_time::microseconds(usecs));

    //interrupt and join the threads
    thread_group.interrupt_all();
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
    ) % num_rx_samps % num_dropped_samps % num_overflows % num_tx_samps % num_seq_errors % num_underflows << std::endl;

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;
    return EXIT_SUCCESS;
}
