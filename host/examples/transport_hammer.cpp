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
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>
#include <time.h>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    double delay;
    size_t total_num_samps;
    double rate;
	int step;
	int rand_begin;
	int rand_end;
	size_t samp_begin;
	size_t samp_end;
	float ampl;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
		("delay", po::value<double>(&delay)->default_value(0.0), "delay between sample jumps")
		("step", po::value<int>(&step)->default_value(1), "delta between number of samples collected/sent")
		("samp_begin", po::value<size_t>(&samp_begin)->default_value(1), "Beginning number of samples")
		("samp_end", po::value<size_t>(&samp_end)->default_value(32768), "End number of samples")
        ("rate", po::value<double>(&rate)->default_value(3.125e6), "rate of incoming samples")
		("tx", "specify to use TX samples instead of RX samples")
		("rx", "specify to use RX samples (already default")
		("rand_vals", "specify to continuously use random numbers of samples")
		("rand_begin", po::value<int>(&rand_begin)->default_value(0), "specify minimum value outputted by random number generator")
		("rand_end", po::value<int>(&rand_end)->default_value(2000), "specify maximum value outputting by random number generator")
		("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of each sample")
        ("verbose", "Enables verbosity")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Transport Hammer - %s") % desc << std::endl;
        return ~0;
    }
	
	bool use_tx = vm.count("tx") > 0;
	bool rand_vals = vm.count("rand_vals") > 0;
	bool verbose = vm.count("verbose") > 0;
	
	if (use_tx)
	{
		//create a usrp device
		std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
		uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
		std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
		
		//set the tx sample rate
		std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate/1e6) << std::endl;
		usrp->set_tx_rate(rate);
		std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;
		usrp->set_time_now(uhd::time_spec_t(0.0));
		
		//create a transmit streamer
		uhd::stream_args_t stream_args("fc32"); //complex floats
		uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
		
		//allocate buffer with data to send
		std::vector<std::complex<float> > buff(tx_stream->get_max_num_samps(), std::complex<float>(ampl, ampl));
		
		//setup metadata for the first packet
		uhd::tx_metadata_t md;
		md.start_of_burst = false;
		md.end_of_burst = false;
		md.has_time_spec = false;
		//md.time_spec = uhd::time_spec_t(seconds_in_future);
		
		
		if(rand_vals){
			
			srand(time(NULL));
			
			while(true){
				size_t total_num_samps = (rand() % (rand_end - rand_begin)) + rand_begin;
				size_t num_acc_samps = 0;
				float timeout = 0;
				
				std::cout << "----------------------------------------" << std::endl;
				std::cout << boost::format("About to send %u samples.") % total_num_samps << std::endl << std::endl;
				
				usrp->set_time_now(uhd::time_spec_t(0.0));
				
				while(num_acc_samps < total_num_samps){
					size_t samps_to_send = std::min(total_num_samps - num_acc_samps, buff.size());
					if(verbose) std::cout << boost::format("Sent %u samples.") % samps_to_send << std::endl;
					
					
					//send a single packet
					size_t num_tx_samps = tx_stream->send(&buff.front(), samps_to_send, md, timeout);
					
					if(num_tx_samps < samps_to_send) std::cerr << "Send timeout..." << std::endl;
					//if(verbose) std::cout << boost::format("Sent packet: %u samples") % num_tx_samps << std::endl;
					
					num_acc_samps += num_tx_samps;
				}
				
				md.end_of_burst   = true;
				tx_stream->send("", 0, md);
				
				if(verbose) std::cout << std::endl;
				std::cout << "Waiting for async burst ACK... " << std::endl << std::flush;
				uhd::async_metadata_t async_md;
				bool got_async_burst_ack = false;
				//loop through all messages for the ACK packet (may have underflow messages in queue)
				while (not got_async_burst_ack and usrp->get_device()->recv_async_msg(async_md, timeout)){
					got_async_burst_ack = (async_md.event_code == uhd::async_metadata_t::EVENT_CODE_BURST_ACK);
				}
				std::cout << (got_async_burst_ack? "Success!" : "Failure...") << std::endl << std::endl;
				
				std::cout << boost::format("Successfully sent %u samples.") % total_num_samps << std::endl << "----------------------------------------" << std::endl << std::endl;
				
				sleep(delay);
				
			}
		}
		else{
			
			float timeout = 0;
			
			if(verbose){
				std::cout << "About to start sending samples." << std::endl;
				std::cout << boost::format("Samples will start at %u and end with %u with steps of %u.") % samp_begin % samp_end % step << std::endl;
				
				sleep(2);
			}
			
			for(int i = int(samp_begin); i <= int(samp_end); i += step){
				
				std::cout << "----------------------------------------" << std::endl;
				
				usrp->set_time_now(uhd::time_spec_t(0.0));
				
				std::cout << boost::format("About to send %u samples.") % i << std::endl;
				if(verbose) std::cout << std::endl;
				
				size_t num_acc_samps = 0; //number of accumulated samples
				total_num_samps = i;
				
				while(num_acc_samps < total_num_samps){
					size_t samps_to_send = std::min(total_num_samps - num_acc_samps, buff.size());
					
					if(verbose) std::cout << boost::format("Sent %u samples.") % samps_to_send << std::endl;
					
					//send a single packet
					size_t num_tx_samps = tx_stream->send(
						&buff.front(), samps_to_send, md, timeout
					);
					
					if (num_tx_samps < samps_to_send) std::cerr << "Send timeout..." << std::endl;
					
					num_acc_samps += num_tx_samps;
				}
				
				//send a mini EOB packet
				md.end_of_burst   = true;
				tx_stream->send("", 0, md);
				
				std::cout << std::endl << "Waiting for async burst ACK... " << std::endl << std::flush;
				uhd::async_metadata_t async_md;
				bool got_async_burst_ack = false;
				//loop through all messages for the ACK packet (may have underflow messages in queue)
				while (not got_async_burst_ack and usrp->get_device()->recv_async_msg(async_md, timeout)){
					got_async_burst_ack = (async_md.event_code == uhd::async_metadata_t::EVENT_CODE_BURST_ACK);
				}
				std::cout << (got_async_burst_ack? "Success!" : "Failure...") << std::endl << std::endl;
				
				std::cout << boost::format("Successfully sent %u samples.") % i << std::endl << "----------------------------------------" << std::endl << std::endl;
				
				sleep(delay);
			}
			
			//finished
			std::cout << "Done!" << std::endl << std::endl;
		}
		
		return 0;
	}
	else
	{
		//create a usrp device
		std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
		uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
		std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
		
		//set the rx sample rate
		std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate/1e6) << std::endl;
		usrp->set_rx_rate(rate);
		std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate()/1e6) << std::endl << std::endl;
		

		
		if(rand_vals){
			//random here
			
			srand(time(NULL));
			
			while(true){
				total_num_samps = (rand() % (rand_end - rand_begin)) + rand_begin;
				
				usrp->set_time_now(uhd::time_spec_t(0.0));
				
				//create a receive streamer
				uhd::stream_args_t stream_args("fc32"); //complex floats
				uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
				
				std::cout << std::endl << "----------------------------------------" << std::endl;
				std::cout << boost::format("About to receive %u samples.") % total_num_samps << std::endl << std::endl;
				
				uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
				stream_cmd.num_samps = total_num_samps;
				stream_cmd.stream_now = true;
				usrp->issue_stream_cmd(stream_cmd);
				
				//meta-data will be filled in by recv()
				uhd::rx_metadata_t md;
				
				//allocate buffer to receive with samples
				std::vector<std::complex<float> > buff(rx_stream->get_max_num_samps());
				double timeout = 0;
				
				size_t num_acc_samps = 0; //number of accumulated samples
				while(num_acc_samps < total_num_samps){
					//receive a single packet
					size_t num_rx_samps = rx_stream->recv(
						&buff.front(), buff.size(), md, timeout, true
					);
					
					//handle the error code
					if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) break;
					if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
						throw std::runtime_error(str(boost::format(
							"Unexpected error code 0x%x"
						) % md.error_code));
					}
					if(verbose) std::cout << boost::format("Received %u samples.") % num_rx_samps << std::endl;
					num_acc_samps += num_rx_samps;
				}
				sleep(delay);
				
				if (num_acc_samps < total_num_samps) std::cerr << "Receive timeout before all samples received..." << std::endl;
				
				if(verbose) std::cout << std::endl;
				std::cout << boost::format("Successfully received %u samples.") % total_num_samps << std::endl << "----------------------------------------" << std::endl;
			}
		}
		else{
			
			if(verbose) {
				std::cout << std::endl << "About to start receiving samples." << std::endl;
				std::cout << boost::format("Samples will start at %u and end with %u with steps of %u.") % samp_begin % samp_end % step << std::endl << std::endl;
				
				sleep(2);
			}
			
			for(int i = int(samp_begin); i <= int(samp_end); i += step){
				
				usrp->set_time_now(uhd::time_spec_t(0.0));
				
				//create a receive streamer
				uhd::stream_args_t stream_args("fc32"); //complex floats
				uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
				
				//setup streaming
				std::cout << std::endl;
				std::cout << "----------------------------------------" << std::endl;
				std::cout << boost::format("About to receive %u samples.") % i << std::endl << std::endl;
				uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
				stream_cmd.num_samps = i;
				stream_cmd.stream_now = true;
				usrp->issue_stream_cmd(stream_cmd);
				
				//meta-data will be filled in by recv()
				uhd::rx_metadata_t md;
				
				//allocate buffer to receive with samples
				std::vector<std::complex<float> > buff(rx_stream->get_max_num_samps());
				
				double timeout = 0;
				
				size_t num_acc_samps = 0; //number of accumulated samples
				while(int(num_acc_samps) < i){
					//receive a single packet
					size_t num_rx_samps = rx_stream->recv(
						&buff.front(), buff.size(), md, timeout, true
					);
					
					//handle the error code
					if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) break;
					if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
						throw std::runtime_error(str(boost::format(
							"Unexpected error code 0x%x"
						) % md.error_code));
					}
					
					if(verbose) std::cout << boost::format("Received %u samples.") % num_rx_samps << std::endl;
					
					num_acc_samps += num_rx_samps;
					
				}
				if(verbose) std::cout << std::endl;
				std::cout << boost::format("Successfully received %u samples.") % i << std::endl << "----------------------------------------" << std::endl;
				
				sleep(delay);
				
				//std::cout << boost::format("num_acc_samps=%u, i=%u") % num_acc_samps % i << std::endl;
				
				if (int(num_acc_samps) < i) std::cerr << "Receive timeout before all samples received..." << std::endl;
			}
		}
	return 0;
	}
}