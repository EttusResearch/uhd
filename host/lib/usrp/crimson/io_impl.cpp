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
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/make_shared.hpp>
#include <iostream>
#include "crimson_str_iface.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;
namespace pt = boost::posix_time;

// TODO we are faking timestamps right now, this needs to be updated
static long time_stamp = 0;

class crimson_rx_streamer : public uhd::rx_streamer {
public:
	crimson_rx_streamer(device_addr_t addr, property_tree::sptr tree) {
		// save the tree
		_tree = tree;

		// get the property root path
		const fs_path mb_path   = "/mboards/0";
		const fs_path prop_path = mb_path / "rx_link";

		for (int i = 0; i < 4; i++) {
			std::string ch       = boost::lexical_cast<std::string>((char)(i + 65));
			std::string udp_port = tree->access<std::string>(prop_path / "Channel_"+ch / "port").get();
			std::string sink     = tree->access<std::string>(prop_path / "Channel_"+ch / "iface").get();

			// SFPA
			if (strcmp(sink.c_str(), "sfpa") == 0) {
				_iface[i] = crimson_str_iface::make( uhd::transport::udp_simple::make_connected(
					tree->access<std::string>( mb_path / "sfpa" / "link_ip").get(), udp_port) );
			// SFPB
			} else if (strcmp(sink.c_str(), "sfpb") == 0) {
				_iface[i] = crimson_str_iface::make( uhd::transport::udp_simple::make_connected(
					tree->access<std::string>( mb_path / "sfpb" / "link_ip").get(), udp_port) );
			// MANAGEMENT
			} else {
				_iface[i] = crimson_str_iface::make( uhd::transport::udp_simple::make_connected(
					addr["addr"], udp_port));
			}
		}
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
			_tree->access<std::string>("/mboards/0/fpga/link/sfpa/pay_len").get());

		return sample_size/4;	// 32-bit (4-bytes) wide samples, (16 I, 16 Q)
	}

	size_t recv(
        	const buffs_type &buffs,
        	const size_t nsamps_per_buff,
        	rx_metadata_t &metadata,
        	const double timeout = 0.1,
        	const bool one_packet = false ) {
		// only handles one_packet
		// samples are 32 bit wide, 16-bit I, 16-bit Q
		// populates buffer
		for (int i = 0; i < nsamps_per_buff; i++) buffs[i];

		// populate metadata
		metadata.error_code = rx_metadata_t::ERROR_CODE_NONE;
		metadata.out_of_sequence = false;
		metadata.start_of_burst = true;
		metadata.end_of_burst = true;
		metadata.fragment_offset = 0;
		metadata.more_fragments = false;
		metadata.has_time_spec = true;
		metadata.time_spec = time_spec_t(time_stamp++);
		return nsamps_per_buff;
	}

	void issue_stream_cmd(const stream_cmd_t &stream_cmd) {

	}

private:
	// 0-channel A, 1-channel B, 2-channel C, 3-channel D
	crimson_str_iface::sptr _iface[4];
	property_tree::sptr _tree;
};

class crimson_tx_streamer : public uhd::tx_streamer {
public:
	crimson_tx_streamer(device_addr_t addr, property_tree::sptr tree) {
		// save the tree
		_tree = tree;

		// get the property root path
		const fs_path mb_path   = "/mboards/0";
		const fs_path prop_path = mb_path / "tx_link";

		for (int i = 0; i < 4; i++) {
			std::string ch       = boost::lexical_cast<std::string>((char)(i + 65));
			std::string udp_port = tree->access<std::string>(prop_path / "Channel_"+ch / "port").get();
			std::string sink     = tree->access<std::string>(prop_path / "Channel_"+ch / "iface").get();

			// SFPA
			if (strcmp(sink.c_str(), "sfpa") == 0) {
				_iface[i] = crimson_str_iface::make( uhd::transport::udp_simple::make_connected(
					addr["addr"], udp_port));
			// SFPB
			} else if (strcmp(sink.c_str(), "sfpb") == 0) {
				_iface[i] = crimson_str_iface::make( uhd::transport::udp_simple::make_connected(
					addr["addr"], udp_port));
			// MANAGEMENT
			} else {
				_iface[i] = crimson_str_iface::make( uhd::transport::udp_simple::make_connected(
					addr["addr"], udp_port));
			}
		}
	}

	~crimson_tx_streamer() {
		// nothing
	}

	// number of channels for streamer
	size_t get_num_channels(void) const {
		const fs_path tx_path = "/mboards/0/tx";
		size_t num = 0;
		for (int i = 0; i < 4; i++) {
			std::string ch = boost::lexical_cast<std::string>((char)(i + 65));
			std::string pwr = _tree->access<std::string>(tx_path / "Channel_"+ch / "pwr").get();
			if ( strcmp(pwr.c_str(), "1") == 0 ) num++;
		}
		return num;
	}

	// max samples per buffer per packet for sfpa
	// it is HIGHLY recommended that the user keep the payload len equal for both SFP ports
	size_t get_max_num_samps(void) const {
		int sample_size = boost::lexical_cast<int>(
			_tree->access<std::string>("/mboards/0/fpga/link/sfpa/pay_len").get());

		return sample_size/4;	// 32-bit (4-bytes) wide samples, (16 I, 16 Q)
	}

	size_t send(
        	const buffs_type &buffs,
        	const size_t nsamps_per_buff,
        	const tx_metadata_t &metadata,
        	const double timeout = 0.1) {

		return nsamps_per_buff;
	}

	// async messages are currently disabled
	bool recv_async_msg( async_metadata_t &async_metadata, double timeout = 0.1) {
		return false;
	}

private:
	// 0-channel A, 1-channel B, 2-channel C, 3-channel D
	crimson_str_iface::sptr _iface[4];
	property_tree::sptr _tree;
};

/***********************************************************************
 * Async Data
 **********************************************************************/
// async messages are currently disabled
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
			\"sc16\" complex\<int16_t\>" << std::endl;
	}

	// Crimson currently only supports (over the wire) otw_format of "sc16" - Q16 I16 if specified
	if (strcmp(args.otw_format.c_str(), "sc16") != 0 && strcmp(args.otw_format.c_str(), "") != 0 ) {
		UHD_MSG(error) << "CRIMSON Stream only supports otw_format of \
			\"sc16\" Q16 I16" << std::endl;
	}

	// TODO firmware support for other otw_format, cpu_format
	return rx_streamer::sptr(new crimson_rx_streamer(this->_addr, this->_tree));
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr crimson_impl::get_tx_stream(const uhd::stream_args_t &args){
	// Crimson currently only supports cpu_format of "sc16" (complex<int16_t>) stream
	if (strcmp(args.cpu_format.c_str(), "sc16") != 0 && strcmp(args.cpu_format.c_str(), "") != 0 ) {
		UHD_MSG(error) << "CRIMSON Stream only supports cpu_format of \
			\"sc16\" complex\<int16_t\>" << std::endl;
	}

	// Crimson currently only supports (over the wire) otw_format of "sc16" - Q16 I16 if specified
	if (strcmp(args.otw_format.c_str(), "sc16") != 0 && strcmp(args.otw_format.c_str(), "") != 0 ) {
		UHD_MSG(error) << "CRIMSON Stream only supports otw_format of \
			\"sc16\" Q16 I16" << std::endl;
	}

	// TODO firmware support for other otw_format, cpu_format
	return tx_streamer::sptr(new crimson_tx_streamer(this->_addr, this->_tree));
}
