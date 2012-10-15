//
// Copyright 2012 Ettus Research LLC
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
#include <boost/math/special_functions/round.hpp>
#include <iostream>
#include <complex>

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
 * RX Hammer
 **********************************************************************/
void rx_hammer(uhd::usrp::multi_usrp::sptr usrp, const std::string &rx_cpu, const std::string &rx_otw){
    uhd::set_thread_priority_safe();

    //create a receive streamer
    uhd::stream_args_t stream_args(rx_cpu, rx_otw);
    for (size_t ch = 0; ch < usrp->get_num_mboards(); ch++) //linear channel mapping
        stream_args.channels.push_back(ch);
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    //print pre-test summary
    std::cout << boost::format(
        "Testing receive rate %f Msps"
    ) % (usrp->get_rx_rate()/1e6) << std::endl;

    //setup variables and allocate buffer
    uhd::rx_metadata_t md;
    const size_t max_samps_per_packet = rx_stream->get_max_num_samps();
    std::vector<char> buff(max_samps_per_packet*uhd::convert::get_bytes_per_item(rx_cpu));
    std::vector<void *> buffs;
    for (size_t ch = 0; ch < stream_args.channels.size(); ch++)
        buffs.push_back(&buff.front()); //same buffer for each channel
    bool had_an_overflow = false;
    uhd::time_spec_t last_time;
    const double rate = usrp->get_rx_rate();
    double timeout = 1;

    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(0.05);
    cmd.stream_now = (buffs.size() == 1);
    srand( time(NULL) );

    while (not boost::this_thread::interruption_requested()){
        cmd.num_samps = rand() % 100000;
        usrp->issue_stream_cmd(cmd);
        num_rx_samps += rx_stream->recv(buffs, max_samps_per_packet, md, timeout, true);

        //handle the error codes
        switch(md.error_code){
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
            if (had_an_overflow){
                had_an_overflow = false;
                num_dropped_samps += boost::math::iround((md.time_spec - last_time).get_real_secs()*rate);
            }
            break;

        case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
            had_an_overflow = true;
            last_time = md.time_spec;
            num_overflows++;
            break;

        default:
            std::cerr << "Error code: " << md.error_code << std::endl;
            std::cerr << "Unexpected error on recv, continuing..." << std::endl;
            break;
        }
    }
}

/***********************************************************************
 * TX Hammer
 **********************************************************************/
void tx_hammer(uhd::usrp::multi_usrp::sptr usrp, const std::string &tx_cpu, const std::string &tx_otw){
    uhd::set_thread_priority_safe();

    //create a transmit streamer
    uhd::stream_args_t stream_args(tx_cpu, tx_otw);
    for (size_t ch = 0; ch < usrp->get_num_mboards(); ch++) //linear channel mapping
        stream_args.channels.push_back(ch);
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
    uhd::tx_metadata_t md;
    std::vector<std::complex<float> > buff(10000);

    //print pre-test summary
    std::cout << boost::format(
        "Testing transmit rate %f Msps"
    ) % (usrp->get_tx_rate()/1e6) << std::endl;

    //setup variables and allocate buffer
    std::srand( time(NULL) );
    while(not boost::this_thread::interruption_requested()){
        size_t total_num_samps = rand() % 100000;
        size_t num_acc_samps = 0;
        float timeout = 1;

        usrp->set_time_now(uhd::time_spec_t(0.0));
        while(num_acc_samps < total_num_samps){

            //send a single packet
            num_tx_samps += tx_stream->send(&buff, tx_stream->get_max_num_samps(), md, timeout);

            num_acc_samps += std::min(total_num_samps-num_acc_samps, tx_stream->get_max_num_samps());
        }
        //send a mini EOB packet
        md.end_of_burst = true;
        tx_stream->send("", 0, md);
    }
}

void tx_hammer_async_helper(uhd::usrp::multi_usrp::sptr usrp){
    //setup variables and allocate buffer
    uhd::async_metadata_t async_md;

    while (not boost::this_thread::interruption_requested()){

        if (not usrp->get_device()->recv_async_msg(async_md)) continue;

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

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("duration", po::value<double>(&duration)->default_value(10.0), "if random, specify duration for the test in seconds")
        ("rx_rate", po::value<double>(&rx_rate), "specify to perform a RX rate test (sps)")
        ("tx_rate", po::value<double>(&tx_rate), "specify to perform a TX rate test (sps)")
        ("rx_otw", po::value<std::string>(&rx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for RX")
        ("tx_otw", po::value<std::string>(&tx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for TX")
        ("rx_cpu", po::value<std::string>(&rx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for RX")
        ("tx_cpu", po::value<std::string>(&tx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for TX")
        ("mode", po::value<std::string>(&mode)->default_value("none"), "multi-channel sync mode option: none, mimo")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help") or (vm.count("rx_rate") + vm.count("tx_rate")) == 0){
        //std::cout << boost::format("UHD Transport Hammer - %s") % desc << std::endl;
        std::cout <<
        "UHD Transport Hammer: a transport layer stress test that continuously\n"
        "calls for random amounts of TX and RX samples\n\n";
        std::cout << desc << std::endl <<
        "    Specify --rx_rate for a receive-only test.\n"
        "    Specify --tx_rate for a transmit-only test.\n"
        "    Specify both options for a full-duplex test.\n"
        << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(args);
    if (not device_addrs.empty() and device_addrs.at(0).get("type", "") == "usrp1"){
        std::cerr << "*** Warning! ***" << std::endl;
        std::cerr << "Results will be inaccurate on USRP1 due to insufficient features.\n" << std::endl;
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

    //spawn the receive test thread
    if (vm.count("rx_rate")){
        usrp->set_rx_rate(rx_rate);
        thread_group.create_thread(boost::bind(&rx_hammer, usrp, rx_cpu, rx_otw));
    }

    //spawn the transmit test thread
    if (vm.count("tx_rate")){
        usrp->set_tx_rate(tx_rate);
        thread_group.create_thread(boost::bind(&tx_hammer, usrp, tx_cpu, tx_otw));
        thread_group.create_thread(boost::bind(&tx_hammer_async_helper, usrp));
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
        "Transport Hammer summary:\n"
        "  Num received samples:    %u\n"
        "  Num dropped samples:     %u\n"
        "  Num overflows detected:  %u\n"
        "  Num transmitted samples: %u\n"
        "  Num sequence errors:     %u\n"
        "  Num underflows detected: %u\n"
    ) % num_rx_samps % num_dropped_samps % num_overflows % num_tx_samps % num_seq_errors % num_underflows << std::endl;

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}
