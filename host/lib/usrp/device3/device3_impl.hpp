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
#include <uhd/types/endianness.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/device3.hpp>

namespace uhd { namespace usrp {

/***********************************************************************
 * Default settings (any device3 may override these)
 **********************************************************************/
static const size_t DEVICE3_RX_FC_REQUEST_FREQ         = 32;    //per flow-control window
static const size_t DEVICE3_TX_FC_RESPONSE_FREQ        = 8;
static const size_t DEVICE3_TX_FC_RESPONSE_CYCLES      = 0;     // Cycles: Off.

// TODO Remove the hardcoded 16 once chdr code is merged
static const size_t DEVICE3_TX_MAX_HDR_LEN             = 16;    // Bytes
static const size_t DEVICE3_RX_MAX_HDR_LEN             = 16;    // Bytes

class device3_impl : public uhd::device3
{
public:
    /***********************************************************************
     * device3-specific Types
     **********************************************************************/
    typedef uhd::transport::bounded_buffer<uhd::async_metadata_t> async_md_type;

    //! The purpose of a transport
    enum xport_type_t {
        CTRL = 0,
        TX_DATA,
        RX_DATA
    };

    enum xport_t {AXI, ETH, PCIE};

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

    //! Stores all streaming-related options
    struct stream_options_t
    {
        //! Max size of the header in bytes for TX
        size_t tx_max_len_hdr;
        //! Max size of the header in bytes for RX
        size_t rx_max_len_hdr;
        //! How often we send ACKs to the upstream block per one full FC window
        size_t rx_fc_request_freq;
        //! How often the downstream block should send ACKs per one full FC window
        size_t tx_fc_response_freq;
        //! How often the downstream block should send ACKs in cycles
        size_t tx_fc_response_cycles;
        stream_options_t(void)
            : tx_max_len_hdr(DEVICE3_TX_MAX_HDR_LEN)
            , rx_max_len_hdr(DEVICE3_RX_MAX_HDR_LEN)
            , rx_fc_request_freq(DEVICE3_RX_FC_REQUEST_FREQ)
            , tx_fc_response_freq(DEVICE3_TX_FC_RESPONSE_FREQ)
            , tx_fc_response_cycles(DEVICE3_TX_FC_RESPONSE_CYCLES)
        {};
    };

    /***********************************************************************
     * I/O Interface
     **********************************************************************/
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &);
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &);
    bool recv_async_msg(uhd::async_metadata_t &async_metadata, double timeout);


protected:
    /***********************************************************************
     * Structors
     **********************************************************************/
    device3_impl();
    virtual ~device3_impl() {};

    /***********************************************************************
     * Transport-related
     **********************************************************************/
    stream_options_t stream_options;

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

    virtual uhd::device_addr_t get_tx_hints(size_t) { return uhd::device_addr_t(); };
    virtual uhd::device_addr_t get_rx_hints(size_t) { return uhd::device_addr_t(); };
    virtual uhd::endianness_t get_transport_endianness(size_t mb_index) = 0;
    boost::function<double(size_t)> _tick_rate_retriever;

    //! Is called after a streamer is generated
    virtual void post_streamer_hooks(bool /* is_tx */) {};

    /***********************************************************************
     * Members
     **********************************************************************/
    //! A counter, designed to create unique SIDs
    size_t _sid_framer;

    // TODO: Maybe move these to private
    uhd::dict<std::string, boost::weak_ptr<uhd::rx_streamer> > _rx_streamers;
    uhd::dict<std::string, boost::weak_ptr<uhd::tx_streamer> > _tx_streamers;

private:
    /***********************************************************************
     * Transport related
     **********************************************************************/
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
     * Private Members
     **********************************************************************/
    //! Buffer for async metadata
    boost::shared_ptr<async_md_type> _async_md;

    //! This mutex locks the get_xx_stream() functions.
    boost::mutex _transport_setup_mutex;
};

}} /* namespace uhd::usrp */

#endif /* INCLUDED_DEVICE3_IMPL_HPP */
// vim: sw=4 expandtab:
