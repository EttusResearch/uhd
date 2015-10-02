//
// Copyright 2014-2015 Per Vices
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

#include "crimson_impl.hpp"
#include "crimson_fw_common.h"
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/udp_stream.hpp>
#include <uhd/types/wb_iface.hpp>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/make_shared.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;
namespace pt = boost::posix_time;

class crimson_rx_streamer : public uhd::rx_streamer {
public:
	crimson_rx_streamer(device_addr_t addr, property_tree::sptr tree, std::vector<size_t> channels) {
		init_rx_streamer(addr, tree, channels);
	}

	crimson_rx_streamer(device_addr_t addr, property_tree::sptr tree) {
		init_rx_streamer(addr, tree, std::vector<size_t>(1, 0));
	}

	~crimson_rx_streamer() {
	}

	// number of channels for streamer
	size_t get_num_channels(void) const {
		return _channels.size();
	}

	// max samples per buffer per packet for sfpa, (4-bytes: 16 I, 16Q)
	size_t get_max_num_samps(void) const {
		return _pay_len/4;
	}

	size_t recv(
        	const buffs_type &buffs,
        	const size_t nsamps_per_buff,
        	rx_metadata_t &metadata,
        	const double timeout = 0.1,
        	const bool one_packet = true )
	{
		const size_t vita_hdr = 4;
		const size_t vita_tlr = 1;
		const size_t vita_pck = nsamps_per_buff + vita_hdr + vita_tlr;
		size_t nbytes = 0;

		// temp buffer: vita hdr + data
		uint32_t vita_buf[vita_pck];

		// read from each connected stream and dump it into the buffs[i]
		for (unsigned int i = 0; i < _channels.size(); i++) {

			// clear temp buffer and otuput buffer
			memset(vita_buf, 0, vita_pck * 4);
			memset(buffs[i], 0, nsamps_per_buff * 4);

			// read in vita_pck*4 bytes to temp buffer
			nbytes = _udp_stream[i] -> stream_in(vita_buf, vita_pck * 4, timeout);
			if (nbytes == 0) return 0;

			// copy non-vita packets to buffs[0]
			memcpy(buffs[i], vita_buf + vita_hdr , nsamps_per_buff * 4);
		}

		// process vita timestamps based on the last stream input's time stamp
		uint64_t time_ticks = ((uint64_t)vita_buf[2] << 32) | ((uint64_t)vita_buf[3]);

		// vita counter increments according to sample rate (all channels have to be same sample rate)
		double time = time_ticks / _rate;

		// determine the beginning of time
		if (_start_time == 0) {
			_start_time = time;
		}

		// save the time to metadata
		time = time - _start_time;
		metadata.time_spec = time_spec_t((time_t)time, time - (time_t)time);

		// process vita sequencing
		uint32_t header = vita_buf[0];
		if (_prev_frame > (header & 0xf0000) >> 16) {
			metadata.out_of_sequence = true;
		} else {
			metadata.out_of_sequence = false;
		}
		_prev_frame = (header & 0xf0000) >> 16;

		// populate metadata
		metadata.error_code = rx_metadata_t::ERROR_CODE_NONE;
		metadata.start_of_burst = true;		// valid for a one packet
		metadata.end_of_burst = true;		// valid for a one packet
		metadata.fragment_offset = 0;		// valid for a one packet
		metadata.more_fragments = false;	// valid for a one packet
		metadata.has_time_spec = true;		// valis for Crimson
		return (nbytes / 4) - 5;		// removed the 5 VITA 32-bit words
	}

	void issue_stream_cmd(const stream_cmd_t &stream_cmd) {
	}

private:
	// init function, common to both constructors
	void init_rx_streamer(device_addr_t addr, property_tree::sptr tree, std::vector<size_t> channels) {
		// save the tree
		_tree = tree;
		_channels = channels;
		_prev_frame = 0;
		_start_time = 0;

		// get the property root path
		const fs_path mb_path   = "/mboards/0";
		const fs_path link_path = mb_path / "rx_link";

		// if no channels specified, default to channel 1 (0)
		_channels = _channels.empty() ? std::vector<size_t>(1, 0) : _channels;

		for (unsigned int i = 0; i < _channels.size(); i++) {
			// get the channel parameters
			std::string ch       = boost::lexical_cast<std::string>((char)(_channels[i] + 65));
			std::string udp_port = tree->access<std::string>(link_path / "Channel_"+ch / "port").get();
			std::string ip_addr  = tree->access<std::string>(link_path / "Channel_"+ch / "ip_dest").get();
			std::string iface    = tree->access<std::string>(link_path / "Channel_"+ch / "iface").get();
			_rate = tree->access<double>(mb_path / "rx_dsps" / "Channel_"+ch / "rate" / "value").get();
			_pay_len = tree->access<int>(mb_path / "link" / iface / "pay_len").get();

			// power on the channel
			tree->access<std::string>(mb_path / "rx" / "Channel_"+ch / "pwr").set("1");
			sleep(5);

			// vita enable
			tree->access<std::string>(link_path / "Channel_"+ch / "vita_en").set("1");

			// connect to UDP port
			_udp_stream.push_back(uhd::transport::udp_stream::make_rx_stream( ip_addr, udp_port));
		}
	}

	// helper function to convert 8-bit allignment ==> 32-bit allignment
	void _32_align(uint32_t* data) {
		*data = (*data & 0x000000ff) << 24 |
			(*data & 0x0000ff00) << 8  |
			(*data & 0x00ff0000) >> 8  |
			(*data & 0xff000000) >> 24;
	}

	std::vector<uhd::transport::udp_stream::sptr> _udp_stream;
	std::vector<size_t> _channels;
	property_tree::sptr _tree;
	size_t _prev_frame;
	size_t _pay_len;
	double _rate;
	double _start_time;
};

class crimson_tx_streamer : public uhd::tx_streamer {
public:
	crimson_tx_streamer(device_addr_t addr, property_tree::sptr tree, std::vector<size_t> channels) {
		init_tx_streamer(addr, tree, channels);
	}

	crimson_tx_streamer(device_addr_t addr, property_tree::sptr tree) {
		init_tx_streamer(addr, tree, std::vector<size_t>(1, 0));
	}

	~crimson_tx_streamer() {
	}

	// number of channels for streamer
	size_t get_num_channels(void) const {
		return _channels.size();
	}

	// max samples per buffer per packet for sfpa, (4-bytes: 16 I, 16Q)
	size_t get_max_num_samps(void) const {
		return _pay_len/4;
	}

	size_t send(
        	const buffs_type &buffs,
        	const size_t nsamps_per_buff,
        	const tx_metadata_t &metadata,
        	const double timeout = 0.1)
	{
		//const size_t vita_hdr = 4;
		//const size_t vita_tlr = 1;
		const size_t vita_pck = nsamps_per_buff;// + vita_hdr + vita_tlr;	// vita is disabled
		uint32_t vita_buf[vita_pck];						// buffer to read in data plus room for VITA
		size_t ret;

		// send to each connected stream data in buffs[i]
		for (unsigned int i = 0; i < _channels.size(); i++) {

			// update sample rate if we don't know the sample rate
			if (_samp_rate[i] == 0) {
				std::string ch = boost::lexical_cast<std::string>((char)(_channels[i] + 65));
				_samp_rate[i] = _tree->access<double>("/mboards/0/tx_dsps/Channel_"+ch+"/rate/value").get();
				_samp_rate_usr[i] = _samp_rate[i];
				//Adjust sample rate to fill up beffer in first half second
			//	std::cout  << std::setprecision(20)<< "Sample Rate: " << _samp_rate[i]<< std::endl;
				_samp_rate[i] = _samp_rate[i]+(CRIMSON_BUFF_SIZE);
			//	std::cout  << std::setprecision(20)<< "Sample Rate: " << _samp_rate[i]<< std::endl;
				_last_time[i] = time_spec_t::get_system_time();


			}

			//Flow control init
			//check if flow control is running, if not run it
			if (_flow_running == false)	boost::thread flowcontrolThread(init_flowcontrol,this);

			//clear temp buffer and copy data into it
			memset((void*)vita_buf, 0, vita_pck*4);
			memcpy((void*)vita_buf, buffs[i], nsamps_per_buff * 4);

			// sending samples, restricted to a jumbo frame of CRIMSON_MAX_MTU bytes at a time
			//ret: nbytes in buffer, each sample has 4 bytes.
			ret = 0;
			bool while_first =true;
			while ((ret / 4) < nsamps_per_buff) {
				size_t remaining_bytes = (nsamps_per_buff*4) - ret;

				if (remaining_bytes >= CRIMSON_MAX_MTU) {
					time_spec_t wait = time_spec_t(0, (double)(CRIMSON_MAX_MTU / 4.0) / (double)_samp_rate[i]);
					while ( time_spec_t::get_system_time() - _last_time[i] < wait) {
						//Check for new buffer if exists and handle accordingly
						update_samplerate();
					}
					//update last_time with when it was supposed to have been sent:
					_last_time[i] = _last_time[i]+wait;//time_spec_t::get_system_time();
					ret += _udp_stream[i] -> stream_out((void*)vita_buf + ret, CRIMSON_MAX_MTU);
				} else {
					// wait for flow control
					time_spec_t wait = time_spec_t(0, (double)(remaining_bytes / 4.0) / (double)_samp_rate[i]);

					//maybe use boost::this_thread::sleep(boost::posix_time::microseconds
					while ( time_spec_t::get_system_time() - _last_time[i] < wait) {
						//Check for new buffer if exists and handle accordingly
						update_samplerate();
					}
					//update last_time with when it was supposed to have been sent:
					_last_time[i] = _last_time[i]+wait;//time_spec_t::get_system_time();
					ret += _udp_stream[i] -> stream_out((void*)vita_buf + ret, remaining_bytes);
				}
			}
		}
		return (ret / 4);// -  vita_hdr - vita_tlr;	// vita is disabled
	}

	// async messages are currently disabled
	bool recv_async_msg( async_metadata_t &async_metadata, double timeout = 0.1) {
		return false;
	}

private:
	// init function, common to both constructors
	void init_tx_streamer( device_addr_t addr, property_tree::sptr tree, std::vector<size_t> channels) {
		// save the tree
		_tree = tree;
		_channels = channels;

		// setup the flow control UDP channel
    		_flow_iface = crimson_iface::make( udp_simple::make_connected(
		        addr["addr"], BOOST_STRINGIZE(CRIMSON_FLOW_CNTRL_UDP_PORT)) );

		// get the property root path
		const fs_path mb_path   = "/mboards/0";
		const fs_path prop_path = mb_path / "tx_link";

		// if no channels specified, default to channel 1 (0)
		_channels = _channels.empty() ? std::vector<size_t>(1, 0) : _channels;

		for (unsigned int i = 0; i < _channels.size(); i++) {
			std::string ch       = boost::lexical_cast<std::string>((char)(_channels[i] + 65));
			std::string udp_port = tree->access<std::string>(prop_path / "Channel_"+ch / "port").get();
			std::string iface    = tree->access<std::string>(prop_path / "Channel_"+ch / "iface").get();
			std::string ip_addr  = tree->access<std::string>( mb_path / "link" / iface / "ip_addr").get();
			_pay_len = tree->access<int>(mb_path / "link" / iface / "pay_len").get();

			// power on the channel
			tree->access<std::string>(mb_path / "tx" / "Channel_"+ch / "pwr").set("1");
			sleep(5);

			// vita disable
			tree->access<std::string>(prop_path / "Channel_"+ch / "vita_en").set("0");

			// connect to UDP port
			_udp_stream.push_back(uhd::transport::udp_stream::make_tx_stream(ip_addr, udp_port));

			//Launch thread for flow control

			//Set up initial flow control variables
			_flow_running=false;
			//boost::thread_group tgroup;
			//tgroup.create_thread(boost::bind(init_flowcontrol));
			_buffer_count[0] = 0;
			_buffer_count[1] = 0;
			_buffer_count[2] = 0;
			_buffer_count[3] = 0;
			_buffer_count[4] = 0;
			//boost::thread flowcontrolThread(init_flowcontrol,this);


			// initialize sample rate
			_samp_rate.push_back(0);
			_samp_rate_usr.push_back(0);

			// initialize the _last_time
			_last_time.push_back(time_spec_t(0.0));
		}
	}

	 // Flow Control (should be called once on seperate thread)
	static void init_flowcontrol(crimson_tx_streamer* txstream) {

		//Get flow control updates x times a second
		uint32_t wait = 1000/CRIMSON_UPDATE_PER_SEC;
		txstream->_flow_running = true;
		bool fillToHalf[4] = {true,true,true,true};

		while(true){
			//get data
			txstream->_flow_iface -> poke_str("Read fifo");
			std::string buff_read = txstream->_flow_iface -> peek_str();

			//increment buffer count to say we have data
			txstream->_buffer_count[0]++;
			//DEBUG: Print out adjusted sample rate
			//std::cout  << buff_read<< std::endl;

			// remove the "flow," at the beginning of the string
			buff_read.erase(0, 5);

			//Prevent multiple access
			txstream->_flowcontrol_mutex.lock();

			// read in each fifo level, ignore() will skip the commas
			std::stringstream ss(buff_read);
			for (int j = 0; j < 4; j++) {
				ss >> txstream->_fifo_lvl[j];
				ss.ignore();
				if (txstream->_fifo_lvl[j] > CRIMSON_BUFF_SIZE/2)
					fillToHalf[j] = false;
				if (fillToHalf[j] == true)
					txstream->_buffer_count[j+1]=txstream->_buffer_count[0];
			}
		   std::cout  <<  "bufflevel: " <<txstream->_fifo_lvl[0]<<" 1: "<<txstream->_buffer_count[0]<<" 2: "<<txstream-> _buffer_count[1]<< "Sample Rate: " <<txstream-> _samp_rate[0]<<std::endl;


			//unlock
			txstream->_flowcontrol_mutex.unlock();

			//sleep for the designated update period
			boost::this_thread::sleep(boost::posix_time::milliseconds(wait));
		}



	}
	void update_samplerate(){
		for (unsigned int i = 0; i < _channels.size(); i++) {
			if(_buffer_count[0]!=_buffer_count[i+1]){
				//If we are waiting, now is a good time to look at the fifo level.

				if(_flowcontrol_mutex.try_lock()){
					// calculate the error
					_fifo_lvl[i] = ((CRIMSON_BUFF_SIZE/2)- _fifo_lvl[i]) / (CRIMSON_BUFF_SIZE/2);
					//apply correction
					_samp_rate[i]=_samp_rate[i]+(_fifo_lvl[i]*_samp_rate[i])/10000000;
					//Limit the correction -magical numbers
					if(_samp_rate[i] > (_samp_rate_usr[i] + _samp_rate_usr[i]/200000)){
						_samp_rate[i] = _samp_rate_usr[i] + _samp_rate_usr[i]/200000;
					}else if(_samp_rate[i] < (_samp_rate_usr[i] - _samp_rate_usr[i]/200000)){
						_samp_rate[i] = _samp_rate_usr[i] - _samp_rate_usr[i]/200000;
					}

					//Buffer is now handled
					_buffer_count[1] = _buffer_count[0];
					_flowcontrol_mutex.unlock();

					//DEBUG: Print out adjusted sample rate
				//	std::cout  << std::setprecision(18)<< "After Adjust" <<_samp_rate[i]<< std::endl;
				}
			}
		}

	}
	// helper function to swap bytes, within 32-bits
	void _32_align(uint32_t* data) {
		*data = (*data & 0x000000ff) << 24 |
			(*data & 0x0000ff00) << 8  |
			(*data & 0x00ff0000) >> 8  |
			(*data & 0xff000000) >> 24;
	}

	std::vector<uhd::transport::udp_stream::sptr> _udp_stream;
	std::vector<size_t> _channels;
	std::vector<double> _samp_rate;
	std::vector<double> _samp_rate_usr;
	std::vector<time_spec_t> _last_time;
	property_tree::sptr _tree;
	size_t _pay_len;
	uhd::wb_iface::sptr _flow_iface;
	boost::mutex _flowcontrol_mutex;
	double _fifo_lvl[4];
	uint32_t _buffer_count[5];
	bool _flow_running;
};

/***********************************************************************
 * Async Data
 **********************************************************************/
// async messages are currently disabled and are deprecated according to UHD
bool crimson_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    return false;
}

/***********************************************************************
 * Receive streamer
 **********************************************************************/
rx_streamer::sptr crimson_impl::get_rx_stream(const uhd::stream_args_t &args){
	// Crimson currently only supports cpu_format of "sc16" (complex<int16_t>) stream
	if (strcmp(args.cpu_format.c_str(), "sc16") != 0 && strcmp(args.cpu_format.c_str(), "") != 0 ) {
		UHD_MSG(error) << "CRIMSON Stream only supports cpu_format of \
			\"sc16\" complex<int16_t>" << std::endl;
	}

	// Crimson currently only supports (over the wire) otw_format of "sc16" - Q16 I16 if specified
	if (strcmp(args.otw_format.c_str(), "sc16") != 0 && strcmp(args.otw_format.c_str(), "") != 0 ) {
		UHD_MSG(error) << "CRIMSON Stream only supports otw_format of \
			\"sc16\" Q16 I16" << std::endl;
	}

	// Warning for preference to set the MTU size to 9000 to support Jumbo Frames
        boost::format base_message (
            "\nCrimson Warning:\n"
            "   Please set the MTU size for SFP ports to 9000.\n"
            "   The device has been optimized for Jumbo Frames\n"
	    "   to lower overhead.\n");
	UHD_MSG(status) << base_message.str();

	// TODO firmware support for other otw_format, cpu_format
	return rx_streamer::sptr(new crimson_rx_streamer(this->_addr, this->_tree, args.channels));
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr crimson_impl::get_tx_stream(const uhd::stream_args_t &args){
	// Crimson currently only supports cpu_format of "sc16" (complex<int16_t>) stream
	if (strcmp(args.cpu_format.c_str(), "sc16") != 0 && strcmp(args.cpu_format.c_str(), "") != 0 ) {
		UHD_MSG(error) << "CRIMSON Stream only supports cpu_format of \
			\"sc16\" complex<int16_t>" << std::endl;
	}

	// Crimson currently only supports (over the wire) otw_format of "sc16" - Q16 I16 if specified
	if (strcmp(args.otw_format.c_str(), "sc16") != 0 && strcmp(args.otw_format.c_str(), "") != 0 ) {
		UHD_MSG(error) << "CRIMSON Stream only supports otw_format of \
			\"sc16\" Q16 I16" << std::endl;
	}

	// Warning for preference to set the MTU size to 9000 to support Jumbo Frames
        boost::format base_message (
            "\nCrimson Warning:\n"
            "   Please set the MTU size for SFP ports to 9000.\n"
            "   The device has been optimized for Jumbo Frames\n"
	    "   to lower overhead.\n");
	UHD_MSG(status) << base_message.str();

	// TODO firmware support for other otw_format, cpu_format
	return tx_streamer::sptr(new crimson_tx_streamer(this->_addr, this->_tree, args.channels));
}
