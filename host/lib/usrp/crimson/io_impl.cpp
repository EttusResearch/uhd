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
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

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
		// nothing
	}

	// number of channels for streamer
	size_t get_num_channels(void) const {
		const fs_path rx_path = "/mboards/0/rx";
		size_t num = 0;
		for (int i = 0; i < 4; i++) {
			std::string ch = boost::lexical_cast<std::string>((char)(i + 65));
			std::string pwr = _tree->access<std::string>(rx_path / "Channel_"+ch / "pwr").get();
			if ( strcmp(pwr.c_str(), "1") == 0 ) num++;
		}
		return num;
	}

	// max samples per buffer per packet for sfpa
	// it is HIGHLY recommended that the user keep the payload len equal for both SFP ports
	size_t get_max_num_samps(void) const {
		int sample_size = boost::lexical_cast<int>(
			_tree->access<std::string>("/mboards/0/link/sfpa/pay_len").get());

		return sample_size/4;	// 32-bit (4-bytes) wide samples, (16 I, 16 Q)
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
		uint32_t vita_buf[vita_pck];	// buffer to read in data plus room for vita
		memset(vita_buf, 0, vita_pck * 4);
		memset(buffs[0], 0, nsamps_per_buff * 4);

		// defaults to read from the first channel because OpenBTS only requires one RX, one TX
		// One sample is 32 bits wide, thus multiply nsamps by 4 to get number of bytes
		// Buffs is an array of buffer, this accesses the first buffer in the array of buffers
		const size_t nbytes = _udp_stream[0] -> stream_in(vita_buf, vita_pck * 4, timeout);
		if (nbytes == 0) return 0;

		memcpy(buffs[0], vita_buf + vita_hdr , nsamps_per_buff * 4);

		/* VITA Header
		 * ------------------
		 * -1. Trailer
		 *  1. Header
		 *  2. Stream
		 *  3. Fractional Time Stamp (1)
		 *  4. Fractional Time Stamp (2)
		 */
		// parse the VITA time stamp
		// fix endianess for time
		//this -> _32_align(&(vita_buf[0]));
		//this -> _32_align(&(vita_buf[1]));
		//this -> _32_align(&(vita_buf[2]));
		//this -> _32_align(&(vita_buf[3]));

		// get the time
		uint64_t time_ticks = ((uint64_t)vita_buf[2] << 32) | ((uint64_t)vita_buf[3]);

		// vita counter increments according to the sample rate
		double time = time_ticks / 390625.0;//(int)_rate[0];

		// determine the beginning of time
		if (_start_time == 0) {
			_start_time = time;
		}

		//printf("0x%08x  0x%08x\n", vita_buf[2], vita_buf[3]);
		time = time - _start_time;

		metadata.time_spec = time_spec_t((time_t)time, time - (time_t)time);
		//printf("time: %lfs\n", time);

		// parse VITA for sequencing
		uint32_t header = vita_buf[0];
		if (_prev_frame > (header & 0xf0000) >> 16)
			metadata.out_of_sequence = true;
		else
			metadata.out_of_sequence = false;
		_prev_frame = (header & 0xf0000) >> 16;

		// Debugging code
		//printf("Received %li bytes\n", nbytes);
		//for (size_t i = 0; i < nsamps_per_buff; i++) {
		//	if (i % 4 == 0) printf("\n");
		//	printf("0x%08x ", ((uint32_t*)buffs[0])[i]);
		//}

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
		const fs_path prop_path = mb_path / "rx_link";

		// if no channels specified, default to channel 1 (0)
		_channels = _channels.empty()? std::vector<size_t>(1, 0) : _channels;

		for (int i = 0; i < _channels.size(); i++) {
			//printf("streaming rx channel: %i\n", _channels[i]);

			std::string ch       = boost::lexical_cast<std::string>((char)(_channels[i] + 65));
			std::string udp_port = tree->access<std::string>(prop_path / "Channel_"+ch / "port").get();
			std::string sink     = tree->access<std::string>(prop_path / "Channel_"+ch / "iface").get();

         // power on the channel
         tree->access<std::string>(mb_path / "rx" / "Channel_"+ch / "pwr").set("1");
	 sleep(5);

			// vita enable
			tree->access<std::string>(prop_path / "Channel_"+ch / "vita_en").set("1");

			// SFPA
			if (strcmp(sink.c_str(), "sfpa") == 0) {
				//std::cout << __func__ << "(): opened UDP[ IP: "
				//	<< tree->access<std::string>( mb_path / "link" / "sfpa" / "ip_addr").get()
				//	<< " PORT: " << udp_port << " ]" << std::endl;
				_udp_stream.push_back(uhd::transport::udp_stream::make_rx_stream(
					"10.10.10.10", udp_port));

			// SFPB
			} else if (strcmp(sink.c_str(), "sfpb") == 0) {
				//std::cout << __func__ << "(): opened UDP[ IP: "
				//	<< tree->access<std::string>( mb_path / "link" / "sfpb" / "ip_addr").get()
				//	<< " PORT: " << udp_port << " ]" << std::endl;
				_udp_stream.push_back(uhd::transport::udp_stream::make_rx_stream(
					"10.10.11.10", udp_port));

			// MANAGEMENT
			} else {
				//std::cout << __func__ << "(): opened UDP[ IP: " << addr["addr"] << " PORT: "
				//	<< udp_port << " ]" << std::endl;
				_udp_stream.push_back(uhd::transport::udp_stream::make_rx_stream( addr["addr"], udp_port ));
			}
		}
	}

	// helper function to swap bytes, within 32-bits
	// (conversion from 8-bit allignment to 32-bit allignment
	void _32_align(uint32_t* data) {
		*data = (*data & 0x000000ff) << 24 |
			(*data & 0x0000ff00) << 8  |
			(*data & 0x00ff0000) >> 8  |
			(*data & 0xff000000) >> 24;
	}

	std::vector<uhd::transport::udp_stream::sptr> _udp_stream;
	property_tree::sptr _tree;
	size_t _prev_frame;
	std::vector<size_t> _channels;
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
		// nothing
	}

	// number of channels for streamer
	size_t get_num_channels(void) const {
		const fs_path tx_path = "/mboards/0/tx";
		size_t num = 0;
		for (int i = 0; i < 4; i++) {
			std::string ch = boost::lexical_cast<std::string>((char)(_channels[i] + 65));
			std::string pwr = _tree->access<std::string>(tx_path / "Channel_"+ch / "pwr").get();
			if ( strcmp(pwr.c_str(), "1") == 0 ) num++;
		}
		return num;
	}

	// max samples per buffer per packet for sfpa
	// it is HIGHLY recommended that the user keep the payload len equal for both SFP ports
	size_t get_max_num_samps(void) const {
		int sample_size = boost::lexical_cast<int>(
			_tree->access<std::string>("/mboards/0/link/sfpa/pay_len").get());

		return sample_size/4;	// 32-bit (4-bytes) wide samples, (16 I, 16 Q)
	}

	size_t send(
        	const buffs_type &buffs,
        	const size_t nsamps_per_buff,
        	const tx_metadata_t &metadata,
        	const double timeout = 0.1)
	{
		const size_t vita_hdr = 4;
		const size_t vita_tlr = 1;
		const size_t vita_pck = nsamps_per_buff;// + vita_hdr + vita_tlr;	// vita is disabled
		uint32_t vita_buf[vita_pck];						// buffer to read in data plus room for VITA
		memcpy((void*)vita_buf, buffs[0], nsamps_per_buff * 4);			// copy over data to VITA buffer

		// Debug data
		/*for (size_t i = 0; i < vita_pck; i++) {
			if (i % 8 == 0) vita_buf[i] = 0x11223344;
			if (i % 8 == 1) vita_buf[i] = 0x55667788;
			if (i % 8 == 2) vita_buf[i] = 0x99aabbcc;
			if (i % 8 == 3) vita_buf[i] = 0xddeeff00;
			if (i % 8 == 4) vita_buf[i] = 0x01234567;
			if (i % 8 == 5) vita_buf[i] = 0x89abcdef;
			if (i % 8 == 6) vita_buf[i] = 0xdeadbeef;
			if (i % 8 == 7) vita_buf[i] = 0xbadeb01a;
		}*/

		// sending samples, restricted to a jumbo frame of 9000 bytes at a time
		size_t ret = 0;
		while ((ret / 4) < nsamps_per_buff) {
			size_t remaining_bytes = (nsamps_per_buff*4) - ret;

			if (remaining_bytes >= 9000)
				ret += _udp_stream[0] -> stream_out((void*)vita_buf, 9000);
			else
				ret += _udp_stream[0] -> stream_out((void*)vita_buf, remaining_bytes);
		}
		//printf("send nsamps_per_buff: %li ret: %i\n", nsamps_per_buff, ret);

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

		// get the property root path
		const fs_path mb_path   = "/mboards/0";
		const fs_path prop_path = mb_path / "tx_link";

		// if no channels specified, default to channel 1 (0)
		_channels = _channels.empty()? std::vector<size_t>(1, 0) : _channels;

		for (int i = 0; i < _channels.size(); i++) {
			//printf("streaming tx channel: %i\n", _channels[i]);

			std::string ch       = boost::lexical_cast<std::string>((char)(i + 65));
			std::string udp_port = tree->access<std::string>(prop_path / "Channel_"+ch / "port").get();
			std::string sink     = tree->access<std::string>(prop_path / "Channel_"+ch / "iface").get();

         // power on the channel
         tree->access<std::string>(mb_path / "tx" / "Channel_"+ch / "pwr").set("1");
	 sleep(5);

			// vita disable
			tree->access<std::string>(prop_path / "Channel_"+ch / "vita_en").set("0");

			// SFPA (all the same because this is a sink)
			if (strcmp(sink.c_str(), "sfpa") == 0) {
				//std::cout << __func__ << "(): opened UDP[ IP: "
				//	<< tree->access<std::string>( mb_path / "link" / "sfpa" / "ip_addr").get()
				//	<< " PORT: " << udp_port << " ]" << std::endl;
				_udp_stream.push_back(uhd::transport::udp_stream::make_tx_stream(
					tree->access<std::string>( mb_path / "link" / "sfpa" / "ip_addr").get(), udp_port));

			// SFPB (all the same because this is a sink)
			} else if (strcmp(sink.c_str(), "sfpb") == 0) {
				//std::cout << __func__ << "(): opened UDP[ IP: "
				//	<< tree->access<std::string>( mb_path / "link" / "sfpb" / "ip_addr").get()
				//	<< " PORT: " << udp_port << " ]" << std::endl;
				_udp_stream.push_back(uhd::transport::udp_stream::make_tx_stream(
					tree->access<std::string>( mb_path / "link" / "sfpb" / "ip_addr").get(), udp_port));

			// MANAGEMENT (all the same because this is a sink)
			} else {
				//std::cout << __func__ << "(): opened UDP[ IP: " << addr["addr"] << " PORT: "
				//	<< udp_port << " ]" << std::endl;
				_udp_stream.push_back(uhd::transport::udp_stream::make_tx_stream( addr["addr"], udp_port ));
			}
		}
	}

	// helper function to swap bytes, within 32-bits
	// (conversion from 8-bit allignment to 32-bit allignment
	void _32_align(uint32_t* data) {
		*data = (*data & 0x000000ff) << 24 |
			(*data & 0x0000ff00) << 8  |
			(*data & 0x00ff0000) >> 8  |
			(*data & 0xff000000) >> 24;
	}

	std::vector<uhd::transport::udp_stream::sptr> _udp_stream;
	property_tree::sptr _tree;
	std::vector<size_t> _channels;
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
