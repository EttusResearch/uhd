//
// Copyright 2014 Ettus Research LLC
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

// Declares the device3_impl class which is a layer between device3 and
// the different 3-rd gen device impls (e.g. x300_impl)

#ifndef INCLUDED_DEVICE3_IMPL_HPP
#define INCLUDED_DEVICE3_IMPL_HPP

#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/device3.hpp>

namespace uhd { namespace usrp {

class device3_impl : public uhd::device3
{
public:
    typedef uhd::transport::bounded_buffer<uhd::async_metadata_t> async_md_type;

    // TODO: Move out of public when get_tx_stream is merged
    struct tx_fc_cache_t
    {
        tx_fc_cache_t(void):
            stream_channel(0),
            device_channel(0),
            last_seq_out(0),
            last_seq_ack(0),
            seq_queue(1){}
        size_t stream_channel;
        size_t device_channel;
        size_t last_seq_out;
        size_t last_seq_ack;
        uhd::transport::bounded_buffer<size_t> seq_queue;
        boost::shared_ptr<async_md_type> async_queue;
        boost::shared_ptr<async_md_type> old_async_queue;
    };

    // TODO: Move out of public when get_tx_stream is merged
    //! Defines the transport type.
    enum xport_type_t {
        CTRL = 0,
        TX_DATA,
        RX_DATA
    };

protected:
    device3_impl();

    /***********************************************************************
     * Transports
     **********************************************************************/
    /*! Holds all necessary items for a bidirectional link
     */
    struct both_xports_t
    {
        uhd::transport::zero_copy_if::sptr recv;
        uhd::transport::zero_copy_if::sptr send;
        size_t recv_buff_size;
        size_t send_buff_size;
        uhd::sid_t send_sid;
        uhd::sid_t recv_sid;
    };

    /*! \brief Create a transport to a given endpoint.
     *
     * \param address The endpoint address of the block we're creating a transport to.
     *                The source address in this value is not considered, only the
     *                destination address.
     * \param xport_type Specify which kind of transport this is.
     * \param args Additional arguments for the transport generation. See \ref page_transport
     *             for valid arguments.
     */
    virtual both_xports_t make_transport(
        const uhd::sid_t &address,
        const xport_type_t xport_type,
        const uhd::device_addr_t& args
    ) = 0;


    /***********************************************************************
     * Members
     **********************************************************************/
    //! A counter, designed to create unique SIDs
    size_t _sid_framer;

    //! Buffer for async metadata
    boost::shared_ptr<async_md_type> _async_md;

private:

public:
    /**********************************************************************
     *        ATTENTION!
     * Most of these static functions should be static in (local to)
     * device3_io_impl.cpp. When the get_?x_stream functions get merged
     * into device3_impl, we can move them there, out of the way.
     ***********************************************************************/

    /***********************************************************************
     * Helper functions for get_?x_stream()
     **********************************************************************/
    static uhd::stream_args_t sanitize_stream_args(const uhd::stream_args_t &args_);

    /*! \brief Returns a list of rx or tx channels for a streamer.
     *
     * If the given stream args contain instructions to set up channels,
     * those are used. Otherwise, the current device's channel definition
     * is consulted.
     *
     * \param args_ Stream args.
     * \param[out] chan_list The list of channels in the correct order.
     * \param[out] chan_args Channel args for every channel. `chan_args.size() == chan_list.size()`
     * \param xx Either "tx" or "rx".
     */
    void generate_channel_list(
            const uhd::stream_args_t &args_,
            std::vector<uhd::rfnoc::block_id_t> &chan_list,
            std::vector<device_addr_t> &chan_args,
            const std::string &xx
    );

    /***********************************************************************
     * RX Flow Control Functions
     **********************************************************************/
    /*! Determine the size of the flow control window in number of packets.
     *
     * This value depends on three things:
     * - The packet size (in bytes), P
     * - The size of the software buffer (in bytes), B
     * - The desired buffer fullness, F
     *
     * The FC window size is thus X = floor(B*F/P).
     *
     * \param pkt_size Packet size in bytes
     * \param sw_buff_size Software buffer size in bytes
     * \param rx_args If this has a key 'recv_buff_fullness', this value will
     *                be used for said fullness. Must be between 0.01 and 1.
     * \param fullness If specified, this value will override the value in
     *                 \p rx_args.
     */
    static size_t get_rx_flow_control_window(
            size_t pkt_size,
            size_t sw_buff_size,
            const uhd::device_addr_t& rx_args,
            double fullness=-1
    );

    /*! Send out RX flow control packets.
     *
     * For an rx stream, this function takes care of sending back
     * a flow control packet to the source telling it which
     * packets have been consumed.
     *
     * This function should only be called by the function handling
     * the rx stream, usually recv() in super_recv_packet_handler.
     *
     * \param sid The SID that goes into this packet. This is the reversed()
     *            version of the data stream's SID.
     * \param xport A transport object over which to send the data
     * \param big_endian Endianness of the transport
     * \param seq32_state Pointer to a variable that saves the 32-Bit state
     *                    of the sequence numbers, since we only have 12 Bit
     *                    sequence numbers in CHDR.
     * \param last_seq The value to send: The last consumed packet's sequence number.
     */
    static void handle_rx_flowctrl(
            const uhd::sid_t &sid,
            uhd::transport::zero_copy_if::sptr xport,
            bool big_endian,
            boost::shared_ptr<boost::uint32_t> seq32_state,
            const size_t last_seq
    );

    /***********************************************************************
     * TX Flow Control Functions
     **********************************************************************/
    /*! 
     *
     * send_buff_size beats hw_buff_size_!
     */
    static size_t get_tx_flow_control_window(
            size_t pkt_size,
            const double hw_buff_size_,
            const uhd::device_addr_t& tx_args
    );

    static uhd::transport::managed_send_buffer::sptr get_tx_buff_with_flowctrl(
        uhd::task::sptr /*holds ref*/,
        boost::shared_ptr<device3_impl::tx_fc_cache_t> guts,
        uhd::transport::zero_copy_if::sptr xport,
        size_t fc_pkt_window,
        const double timeout
    );

    /***********************************************************************
     * Async Data
     **********************************************************************/
    bool recv_async_msg(
        uhd::async_metadata_t &async_metadata, double timeout
    );

    /***********************************************************************
     * CHDR packer/unpacker (TODO: Remove once new chdr class is in)
     **********************************************************************/
    static void if_hdr_unpack_be(
        const boost::uint32_t *packet_buff,
        uhd::transport::vrt::if_packet_info_t &if_packet_info
    );

    static void if_hdr_unpack_le(
        const boost::uint32_t *packet_buff,
        uhd::transport::vrt::if_packet_info_t &if_packet_info
    );

    static void if_hdr_pack_be(
        boost::uint32_t *packet_buff,
        uhd::transport::vrt::if_packet_info_t &if_packet_info
    );

    static void if_hdr_pack_le(
        boost::uint32_t *packet_buff,
        uhd::transport::vrt::if_packet_info_t &if_packet_info
    );

};

}} /* namespace uhd::usrp */

#endif /* INCLUDED_DEVICE3_IMPL_HPP */
// vim: sw=4 expandtab:
