//
// Copyright 2010-2013 Ettus Research LLC
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

#include "xport_benchmarker.hpp"

namespace uhd { namespace transport {

const device_addr_t& xport_benchmarker::benchmark_throughput_chdr
(
    zero_copy_if::sptr tx_transport,
    zero_copy_if::sptr rx_transport,
    boost::uint32_t sid,
    bool big_endian,
    boost::uint32_t duration_ms)
{
    vrt::if_packet_info_t pkt_info;
    _initialize_chdr(tx_transport, rx_transport, sid, pkt_info);
    _reset_counters();
    boost::posix_time::ptime start_time(boost::posix_time::microsec_clock::local_time());

    _tx_thread.reset(new boost::thread(boost::bind(&xport_benchmarker::_stream_tx, this, tx_transport.get(), &pkt_info, big_endian)));
    _rx_thread.reset(new boost::thread(boost::bind(&xport_benchmarker::_stream_rx, this, rx_transport.get(), &pkt_info, big_endian)));

    boost::this_thread::sleep(boost::posix_time::milliseconds(duration_ms));

    _tx_thread->interrupt();
    _rx_thread->interrupt();
    _tx_thread->join();
    _rx_thread->join();

    boost::posix_time::ptime stop_time(boost::posix_time::microsec_clock::local_time());
    double duration_s = ((double)(stop_time-start_time).total_microseconds())/1e6;

    boost::uint64_t tx_bytes = pkt_info.num_payload_words32*sizeof(uint32_t)*_num_tx_packets;
    boost::uint64_t rx_bytes = pkt_info.num_payload_words32*sizeof(uint32_t)*_num_rx_packets;
    double tx_rate = (((double)tx_bytes)/duration_s);
    double rx_rate = (((double)rx_bytes)/duration_s);

    _results["TX-Bytes"] = (boost::format("%.2fMB") % (tx_bytes/(1024*1024))).str();
    _results["RX-Bytes"] = (boost::format("%.2fMB") % (rx_bytes/(1024*1024))).str();
    _results["TX-Throughput"] = (boost::format("%.2fMB/s") % (tx_rate/(1024*1024))).str();
    _results["RX-Throughput"] = (boost::format("%.2fMB/s") % (rx_rate/(1024*1024))).str();
    _results["TX-Timeouts"] = boost::lexical_cast<std::string>(_num_tx_timeouts);
    _results["RX-Timeouts"] = boost::lexical_cast<std::string>(_num_rx_timeouts);
    _results["Data-Errors"] = boost::lexical_cast<std::string>(_num_data_errors);

    return _results;
}

void xport_benchmarker::_stream_tx(zero_copy_if* transport, vrt::if_packet_info_t* pkt_info, bool big_endian)
{
    while (not boost::this_thread::interruption_requested()) {
        managed_send_buffer::sptr buff = transport->get_send_buff(_tx_timeout);
        if (buff) {
            boost::uint32_t *packet_buff = buff->cast<boost::uint32_t *>();
            //Populate packet
            if (big_endian) {
                vrt::if_hdr_pack_be(packet_buff, *pkt_info);
            } else {
                vrt::if_hdr_pack_le(packet_buff, *pkt_info);
            }
            //send the buffer over the interface
            buff->commit(sizeof(boost::uint32_t)*(pkt_info->num_packet_words32));
            _num_tx_packets++;
        } else {
            _num_tx_timeouts++;
        }
    }
}

void xport_benchmarker::_stream_rx(zero_copy_if* transport, const vrt::if_packet_info_t* exp_pkt_info, bool big_endian)
{
    while (not boost::this_thread::interruption_requested()) {
        managed_recv_buffer::sptr buff = transport->get_recv_buff(_rx_timeout);
        if (buff) {
            //Extract packet info
            vrt::if_packet_info_t pkt_info;
            pkt_info.link_type = exp_pkt_info->link_type;
            pkt_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            const boost::uint32_t *packet_buff = buff->cast<const boost::uint32_t *>();

            _num_rx_packets++;

            //unpacking can fail
            try {
                if (big_endian) {
                    vrt::if_hdr_unpack_be(packet_buff, pkt_info);
                } else {
                    vrt::if_hdr_unpack_le(packet_buff, pkt_info);
                }

                if (exp_pkt_info->packet_type != pkt_info.packet_type ||
                    exp_pkt_info->num_payload_bytes != pkt_info.num_payload_bytes) {
                    _num_data_errors++;
                }
            } catch(const std::exception &ex) {
                _num_data_errors++;
            }
        } else {
            _num_rx_timeouts++;
        }
    }
}

void xport_benchmarker::_reset_counters(void)
{
    _num_tx_packets = 0;
    _num_rx_packets = 0;
    _num_tx_timeouts = 0;
    _num_rx_timeouts = 0;
    _num_data_errors = 0;
}

void xport_benchmarker::_initialize_chdr(
    zero_copy_if::sptr tx_transport,
    zero_copy_if::sptr rx_transport,
    boost::uint32_t sid,
    vrt::if_packet_info_t& pkt_info)
{
    _tx_timeout = 0.5;
    _rx_timeout = 0.5;

    size_t frame_size = std::min(tx_transport->get_send_frame_size(), rx_transport->get_recv_frame_size());

    pkt_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    pkt_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    pkt_info.num_packet_words32 = (frame_size/sizeof(boost::uint32_t));
    pkt_info.num_payload_words32 = pkt_info.num_packet_words32 - 2;
    pkt_info.num_payload_bytes = pkt_info.num_payload_words32*sizeof(boost::uint32_t);
    pkt_info.packet_count = 0;
    pkt_info.sob = false;
    pkt_info.eob = false;
    pkt_info.sid = sid;
    pkt_info.has_sid = true;
    pkt_info.has_cid = false;
    pkt_info.has_tsi = false;
    pkt_info.has_tsf = false;
    pkt_info.has_tlr = false;
}

}}
