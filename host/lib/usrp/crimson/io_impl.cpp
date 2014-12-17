//
// Copyright 2010-2012 Ettus Research LLC
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
	crimson_rx_streamer(device_addr_t addr) {
		_iface = crimson_str_iface::make( uhd::transport::udp_simple::make_connected(
    			addr["addr"], BOOST_STRINGIZE(CRIMSON_SFP0_STREAM_UDP_PORT)) );
	}

	~crimson_rx_streamer() {
		// nothing
	}

	// number of channels for streamer
	size_t get_num_channels(void) const {
		// insert code to call crimson_impl.cpp function get_enabled_chan();
		return 4;
	}

	// max samples per buffer per packet
	size_t get_max_num_samps(void) const {
		// insert code to call crimson_impl.cpp function get_sample_rate();
		return 1472/4;	// 32-bit wide samples, (16 I, 16 Q)
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
	crimson_str_iface::sptr _iface;
};

class crimson_tx_streamer : public uhd::tx_streamer {
public:
	crimson_tx_streamer(device_addr_t addr) {
		_iface = crimson_str_iface::make( uhd::transport::udp_simple::make_connected(
    			addr["addr"], BOOST_STRINGIZE(CRIMSON_SFP1_STREAM_UDP_PORT)) );
	}

	~crimson_tx_streamer() {
		// nothing
	}

	// number of channels for streamer
	size_t get_num_channels(void) const {
		// insert code to call crimson_impl.cpp function get_enabled_chan();
		return 4;
	}

	// max samples per buffer per packet
	size_t get_max_num_samps(void) const {
		// insert code to call crimson_impl.cpp function get_sample_rate();
		return 1472/4;	// 32-bit wide samples, (16 I, 16 Q)
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
	crimson_str_iface::sptr _iface;
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
	// insert code to process the args and set internal registers
	return rx_streamer::sptr(new crimson_rx_streamer(this->_addr));
}

/***********************************************************************
 * Transmit streamer
 **********************************************************************/
tx_streamer::sptr crimson_impl::get_tx_stream(const uhd::stream_args_t &args){
	// insert code to process the args and set internal registers
	return tx_streamer::sptr(new crimson_tx_streamer(this->_addr));
}
