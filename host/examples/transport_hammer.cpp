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
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/math/special_functions/round.hpp>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

/************************************************************************
 * RX Samples
 ************************************************************************/

void rx_hammer(uhd::usrp::multi_usrp::sptr usrp, double rx_rate, bool rx_rand, int rx_low, int rx_high, int rx_step, bool verbose){
    uhd::set_thread_priority_safe();

    //Set RX sample rate
    std::cout << boost::format("Setting RX rate: %f Msps") % (rx_rate/1e6) << std::endl;
    usrp->set_rx_rate(rx_rate);
    std::cout << boost::format("Actual RX rate: %f Msps") % (usrp->get_rx_rate()/1e6) << std::endl << std::endl;

    if(rx_rand){
        std::srand((unsigned int) time(NULL));

        while(true){
            size_t total_num_samps = (rand() % (rx_high - rx_low)) + rx_low;

            usrp->set_time_now(uhd::time_spec_t(0.0));

            //Create a receive streamer
            uhd::stream_args_t stream_args("fc32");
            uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
            
            std::cout << boost::format("About to receive %u samples.") % total_num_samps << std::endl;

            uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
            stream_cmd.num_samps = total_num_samps;
            stream_cmd.stream_now = true;
            usrp->issue_stream_cmd(stream_cmd);

            //Metadata will be filled in by recv()
            uhd::rx_metadata_t md;

            //Allocate buffer to receive with samples
            std::vector<std::complex<float> > buff(rx_stream->get_max_num_samps());
            double timeout = 1;

            size_t num_acc_samps = 0; //Number of accumulated samples
            while(num_acc_samps < total_num_samps){
                //Receive a single packet
                size_t num_rx_samps = rx_stream->recv(
                    &buff.front(), buff.size(), md, timeout, true
                );

                //Handle the error code
                if(md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT){std::cout << "timeout" << std::endl; break;}
                if(md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE && md.error_code != uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
                    std::cout << "Error" << std::endl;
                    throw std::runtime_error(str(boost::format(
                        "Unexpected error code 0x%x"
                    ) % md.error_code));
                }
                num_acc_samps += num_rx_samps;
            }

            if(num_acc_samps < total_num_samps) std::cerr << "Received timeout before all samples were received..." << std::endl;
            else std::cout << boost::format("Successfully received %u samples.") % total_num_samps << std::endl;
        }
    }
    else{
        for(int i = int(rx_low); i <= int(rx_high); i += rx_step){
            usrp->set_time_now(uhd::time_spec_t(0.0));
            
            //Create a receive streamer
            uhd::stream_args_t stream_args("fc32");
            uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
            
            //Set up streaming
            std::cout << boost::format ("About to receive %u samples.") % i << std::endl;
            uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
            stream_cmd.num_samps = i;
            stream_cmd.stream_now = true;
            usrp->issue_stream_cmd(stream_cmd);

            //Metadata will be filled in by recv()
            uhd::rx_metadata_t md;

            //Allocate buffer to receive with samples
            std::vector<std::complex<float> > buff(rx_stream->get_max_num_samps());

            double timeout = 1;
            
            size_t num_acc_samps = 0; //Number of accumulated samples
            while(int(num_acc_samps) < i){
                //Receive a single packet
                size_t num_rx_samps = rx_stream->recv(
                    &buff.front(), buff.size(), md, timeout, true
                );
                
                //Handle the error code
                if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) break;
                if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE && md.error_code != uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
                    throw std::runtime_error(str(boost::format(
                        "Unexpected error code 0x%x"
                    ) % md.error_code));
                }   

                if(verbose) std::cout << boost::format("Received %u samples.") % num_rx_samps << std::endl;

                num_acc_samps += num_rx_samps;

            }
            std::cout << boost::format("Successfully received %u samples.") % i << std::endl;

            if (int(num_acc_samps) < i) std::cerr << "Timeout received before all samples were received..." << std::endl;

        }
    }
}

/************************************************************************
 * TX Samples
 ************************************************************************/

void tx_hammer(uhd::usrp::multi_usrp::sptr usrp, double tx_rate, bool tx_rand, int tx_low, int tx_high, int tx_step, double tx_ampl, bool verbose){
    uhd::set_thread_priority_safe();

    //Set the TX sample rate
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (tx_rate / 1e6) << std::endl;
    usrp->set_tx_rate(tx_rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;
    usrp->set_time_now(uhd::time_spec_t(0.0));

    //Create a transmit streamer
    uhd::stream_args_t stream_args("fc32"); //complex floats
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    //Allocate buffer with data to send
    std::vector<std::complex<float> > buff(tx_stream->get_max_num_samps(), std::complex<float>(tx_ampl, tx_ampl));

    //Setup metadata for the first packet
    uhd::tx_metadata_t md;
    md.start_of_burst = false;
    md.end_of_burst = false;
    md.has_time_spec = false;

    if(tx_rand){
       std::srand((unsigned int) time(NULL));

       while(true){
            size_t total_num_samps = (rand() % (tx_high - tx_low)) + tx_low;
            size_t num_acc_samps = 0;
            float timeout = 1;

            std::cout << boost::format("About to send %u samples.") % total_num_samps << std::endl;

            usrp->set_time_now(uhd::time_spec_t(0.0));

            while(num_acc_samps < total_num_samps){
                size_t samps_to_send = std::min(total_num_samps - num_acc_samps, buff.size());

                //Send a single packet
                size_t num_tx_samps = tx_stream->send(&buff.front(), samps_to_send, md, timeout);

                if(num_tx_samps < samps_to_send) std::cerr << "Send timeout..." << std::endl;

                num_acc_samps += num_tx_samps;
            }

            md.end_of_burst   = true;
            tx_stream->send("", 0, md);

            if(verbose) std::cout << std::endl;
            std::cout << "Waiting for async burst ACK... " << std::flush;
            uhd::async_metadata_t async_md;
            bool got_async_burst_ack = false;

            //Loop through all messages for the ACK packet (may have underflow messages in queue)
            while (not got_async_burst_ack and usrp->get_device()->recv_async_msg(async_md, timeout)){
                got_async_burst_ack = (async_md.event_code == uhd::async_metadata_t::EVENT_CODE_BURST_ACK);
            }
            std::cout << (got_async_burst_ack? "Success!" : "Failure...") << std::endl;

            std::cout << boost::format("Successfully sent %u samples.") % total_num_samps << std::endl;

        }
    }
    else{
        float timeout = 1;

        for(int i = int(tx_low); i <= int(tx_high); i += tx_step){

            usrp->set_time_now(uhd::time_spec_t(0.0));

            std::cout << boost::format("About to send %u samples.") % i << std::endl;
            if(verbose) std::cout << std::endl;

            size_t num_acc_samps = 0; //Number of accumulated samples
            size_t total_num_samps = i;

            while(num_acc_samps < total_num_samps){
                size_t samps_to_send = std::min(total_num_samps - num_acc_samps, buff.size());

                //Send a single packet
                    size_t num_tx_samps = tx_stream->send(
                    &buff.front(), samps_to_send, md, timeout
                );

                if (num_tx_samps < samps_to_send) std::cerr << "Send timeout..." << std::endl;

                num_acc_samps += num_tx_samps;
            }

            //Send a mini EOB packet
            md.end_of_burst   = true;
            tx_stream->send("", 0, md);

            std::cout << std::endl << "Waiting for async burst ACK... " << std::flush;
            uhd::async_metadata_t async_md;
            bool got_async_burst_ack = false;
            //Loop through all messages for the ACK packet (may have underflow messages in queue)
            while (not got_async_burst_ack and usrp->get_device()->recv_async_msg(async_md, timeout)){
                got_async_burst_ack = (async_md.event_code == uhd::async_metadata_t::EVENT_CODE_BURST_ACK);
            }
            std::cout << (got_async_burst_ack? "Success!" : "Failure...") << std::endl;

        }
        //Finished
        std::cout << "Done!" << std::endl;
    }
}

/************************************************************************
 * Main code + dispatcher
 ************************************************************************/

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //Variables to be set by program options
    std::string args;
    double rx_rate;
    int rx_low;
    int rx_high;
    int rx_step;
    double tx_rate;
    int tx_low;
    int tx_high;
    int tx_step;
    double tx_ampl;

    //Set up the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print this help message.")
        ("args", po::value<std::string>(&args)->default_value(""), "Single UHD device address args.")
        ("rx_rate", po::value<double>(&rx_rate), "RX sample rate.")
        ("rx_rand", "Specify to use random amounts of RX samples (between rx_low and rx_high values).")
        ("rx_low", po::value<int>(&rx_low)->default_value(1), "Lowest value of RX samples.")
        ("rx_high", po::value<int>(&rx_high)->default_value(10000), "Highest value of RX samples.")
        ("rx_step", po::value<int>(&rx_step)->default_value(10), "Delta between number of collected RX samples.")
        ("tx_rate", po::value<double>(&tx_rate), "TX sample rate.")
        ("tx_rand", "Specify to use random amounts of TX samples (between tx_low and tx_high values).")
        ("tx_low", po::value<int>(&tx_low)->default_value(1), "Lowest value of TX samples.")
        ("tx_high", po::value<int>(&tx_high)->default_value(10000), "Highest value of TX samples.")
        ("tx_step", po::value<int>(&tx_step)->default_value(10), "Delta between number of sent TX samples.")
        ("tx_ampl", po::value<double>(&tx_ampl)->default_value(0.5), "TX amplitude.")
        ("verbose", "Enables verbosity")
    ;   
    po::variables_map vm; 
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //Set verbose or RX/TX random if requested by user
    bool rx_rand = vm.count("rx_rand") > 0;
    bool tx_rand = vm.count("tx_rand") > 0;
    bool verbose = vm.count("verbose") > 0;

    //Print the help message

    if (vm.count("help") or (vm.count("rx_rate") + vm.count("tx_rate")) == 0){ 
        std::cout << boost::format("UHD Transport Hammer %s") % desc << std::endl;
        std::cout <<
        "    Specify --rx_rate for a receive-only test.\n"
        "    Specify --tx_rate for a transmit-only test.\n"
        "    Specify both options for a full-duplex test.\n"
        << std::endl;
        return ~0; 
    }   

    //Create a USRP device
    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(args);
    std::cout << boost::format("Creating the USRP device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    boost::thread_group thread_group;

    //Spawn the receive test thread
    if (vm.count("rx_rate")){
        usrp->set_rx_rate(rx_rate);
        thread_group.create_thread(boost::bind(&rx_hammer, usrp, rx_rate, rx_rand, rx_low, rx_high, rx_step, verbose));
    }   

    //Spawn the transmit test thread
    if (vm.count("tx_rate")){
        usrp->set_tx_rate(tx_rate);
        thread_group.create_thread(boost::bind(&tx_hammer, usrp, tx_rate, tx_rand, tx_low, tx_high, tx_step, tx_ampl, verbose));
    }

    //Interrupt and join the threads
    boost::this_thread::sleep(boost::posix_time::microseconds(long(1e6)));
    thread_group.interrupt_all();
    thread_group.join_all();
    //Finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}
