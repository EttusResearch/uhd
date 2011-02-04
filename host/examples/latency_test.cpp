//
// Copyright 2010-2011 Ettus Research LLC
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
#include <uhd/usrp/single_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    double wait_time;
    size_t total_num_samps;
    size_t samps_per_packet;
    double rate, freq;
    float ampl;
    double delta_t;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("wait", po::value<double>(&wait_time)->default_value(0.1), "wait time between test cycles")
        ("rtt", po::value<double>(&delta_t)->default_value(0.001), "Round-Trip time to test")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(100), "total number of samples to transmit")
        ("spp", po::value<size_t>(&samps_per_packet)->default_value(1000), "number of samples per packet")
        ("rate", po::value<double>(&rate)->default_value(100e6/4), "rate of outgoing samples")
        ("freq", po::value<double>(&freq)->default_value(0), "rf center frequency in Hz")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of each sample")
        ("dilv", "specify to disable inner-loop verbose")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("Latency Test %s") % desc << std::endl;
        return ~0;
    }

    bool verbose = vm.count("dilv") == 0;

    //create a usrp device
    std::cout << std::endl;
    //std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::single_usrp::sptr sdev = uhd::usrp::single_usrp::make(args);
    uhd::device::sptr dev = sdev->get_device();
    //std::cout << boost::format("Using Device: %s") % sdev->get_pp_string() << std::endl;

    //set the tx sample rate
    sdev->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (sdev->get_tx_rate()/1e6) << std::endl;

    //set the rx sample rate
    sdev->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (sdev->get_rx_rate()/1e6) << std::endl;

    //allocate data to send
    std::vector<std::complex<float> > txbuff(samps_per_packet, std::complex<float>(ampl, ampl));

    //allocate receive buffer
    std::vector<std::complex<float> > rxbuff(samps_per_packet, std::complex<float>(ampl, ampl));


    int time_error = 0;
    int ack = 0;
    int underflow = 0;
    int other = 0;
    // **************************************************
    if(verbose)
      std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    sdev->set_time_now(uhd::time_spec_t(0.0));

    //setup streaming
    std::cout << std::endl;
    if(verbose)
      std::cout << boost::format(
        "Begin receiving %u samples, %f seconds in the future..."
    ) % total_num_samps % wait_time << std::endl;
    
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = total_num_samps;
    stream_cmd.stream_now = false;

    uhd::tx_metadata_t tx_md;

    size_t num_packets = (total_num_samps+samps_per_packet-1)/samps_per_packet;


    double cur_time = 0.0;
    sdev->set_time_now(uhd::time_spec_t(cur_time));

    for(int j=0;j<100;j++) {
      int err = 0;
      cur_time += wait_time;
      stream_cmd.time_spec = uhd::time_spec_t(cur_time);
      sdev->issue_stream_cmd(stream_cmd);
      size_t num_acc_samps = 0;
      uhd::rx_metadata_t rx_md;
      while(num_acc_samps < total_num_samps){
        std::vector<std::complex<float> > rxbuff(dev->get_max_recv_samps_per_packet());
        size_t num_rx_samps = dev->recv(
					&rxbuff.front(), rxbuff.size(), rx_md,
					uhd::io_type_t::COMPLEX_FLOAT32,
					uhd::device::RECV_MODE_ONE_PACKET
					);
	
        switch(rx_md.error_code){
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
	  break;
        case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
	  if (num_acc_samps == 0) continue;
	  std::cout << boost::format
	    ("Got timeout before all samples received, possible packet loss, exiting loop...") << std::endl;
	  err = 1;
	  goto done_loop;
        default:
	  std::cout << boost::format("Got error code 0x%x, exiting loop...") % rx_md.error_code << std::endl;
	  err = 1;
	  goto done_loop;
        }
	
        if(verbose) std::cout << boost::format("Got packet: %u samples, %u full secs, %f frac secs") 
	  % num_rx_samps % rx_md.time_spec.get_full_secs() % rx_md.time_spec.get_frac_secs() << std::endl;
	
        num_acc_samps += num_rx_samps;
      } done_loop:
      
      if(verbose) std::cout << "Done receiving samps" << std::endl;
      if(err)
	continue;
      for (size_t i = 0; i < num_packets; i++){
        //setup the metadata flags per fragment
        tx_md.start_of_burst = (i == 0);              //only first packet has SOB
        tx_md.end_of_burst   = (i == num_packets-1);  //only last packet has EOB
        tx_md.has_time_spec  = (i == 0);              //only first packet has time
	
        size_t samps_to_send = std::min(total_num_samps - samps_per_packet*i, samps_per_packet);
	
        //send the entire packet (driver fragments internally)
	tx_md.time_spec = uhd::time_spec_t(cur_time+delta_t);
        size_t num_tx_samps = dev->send
	  (&txbuff.front(), samps_to_send, tx_md,
	   uhd::io_type_t::COMPLEX_FLOAT32,
	   uhd::device::SEND_MODE_FULL_BUFF,
	   //send will backup into the host this many seconds before sending:
	   0.1 //timeout (delay before transmit + padding)
	   );
        if (num_tx_samps < samps_to_send) std::cout << "Send timeout..." << std::endl;
        if(verbose)
	  std::cout << std::endl << boost::format("Sent %d samples") % num_tx_samps << std::endl;
      }
      
      uhd::async_metadata_t async_md;
      if (not dev->recv_async_msg(async_md)){
	std::cout << boost::format
	  ("failed:\n    Async message recv timed out.\n") << std::endl;
      } else {
	switch(async_md.event_code){
	case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR :
	  time_error++;
	  break;
	case uhd::async_metadata_t::EVENT_CODE_BURST_ACK :
	  ack++;
	  break;
	case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW :
	  underflow++;
	  break;
	default:
	  std::cout << boost::format
	    ("failed:\n    Got unexpected event code 0x%x.\n") 
	    % async_md.event_code << std::endl;
	  other++;
	  break;
	}     
      }
    }
    std::cout << boost::format("ACK %d, UNDERFLOW %d, TIME_ERR %d, other %d") % ack 
      % underflow % time_error % other << std::endl;
    return 0;
}
