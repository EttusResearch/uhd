//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Declares the device3_impl class which is a layer between device3 and
// the different 3-rd gen device impls (e.g. x300_impl)

#ifndef INCLUDED_DEVICE3_IMPL_HPP
#define INCLUDED_DEVICE3_IMPL_HPP

#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/device3.hpp>
#include <uhdlib/rfnoc/xports.hpp>

namespace uhd { namespace usrp {

/***********************************************************************
 * Default settings (any device3 may override these)
 **********************************************************************/
static const size_t DEVICE3_RX_FC_REQUEST_FREQ         = 32;    //per flow-control window
static const size_t DEVICE3_TX_FC_RESPONSE_FREQ        = 8;
static const size_t DEVICE3_TX_FC_RESPONSE_CYCLES      = 0;     // Cycles: Off.

static const size_t DEVICE3_TX_MAX_HDR_LEN             = uhd::transport::vrt::chdr::max_if_hdr_words64 * sizeof(uint64_t);    // Bytes
static const size_t DEVICE3_RX_MAX_HDR_LEN             = uhd::transport::vrt::chdr::max_if_hdr_words64 * sizeof(uint64_t);    // Bytes

class device3_impl : public uhd::device3, public boost::enable_shared_from_this<device3_impl>
{
public:
    /***********************************************************************
     * device3-specific Types
     **********************************************************************/
    typedef uhd::transport::bounded_buffer<uhd::async_metadata_t> async_md_type;

    //! The purpose of a transport
    enum xport_type_t {
        CTRL = 0,
        ASYNC_MSG,
        TX_DATA,
        RX_DATA
    };

    enum xport_t {AXI, ETH, PCIE};

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

    /***********************************************************************
     * Other public APIs
     **********************************************************************/
    rfnoc::graph::sptr create_graph(const std::string &name="");

protected:
    /***********************************************************************
     * Structors
     **********************************************************************/
    device3_impl();
    virtual ~device3_impl() {};

    /***********************************************************************
     * Streaming-related
     **********************************************************************/
    // The 'rate' argument is so we can use these as subscribers to rate changes
public: // TODO make these protected again
    void update_rx_streamers(double rate=-1.0);
    void update_tx_streamers(double rate=-1.0);
protected:

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
    virtual uhd::both_xports_t make_transport(
        const uhd::sid_t &address,
        const xport_type_t xport_type,
        const uhd::device_addr_t& args
    ) = 0;

    virtual uhd::device_addr_t get_tx_hints(size_t) { return uhd::device_addr_t(); };
    virtual uhd::device_addr_t get_rx_hints(size_t) { return uhd::device_addr_t(); };

    //! Is called after a streamer is generated
    virtual void post_streamer_hooks(uhd::direction_t) {};

    /***********************************************************************
     * Channel-related
     **********************************************************************/
    /*! Merge a list of channels into the existing channel definition.
     *
     * Intelligently merge the channels described in \p chan_ids
     * into the current channel definition. If none of the channels in
     * \p chan_ids is in the current definition, they simply get appended.
     * Otherwise, they get overwritten in the order of \p chan_ids.
     *
     * \param chan_ids List of block IDs for the channels.
     * \param chan_args New channel args. Must have same length as chan_ids.
     *
     */
    void merge_channel_defs(
            const std::vector<rfnoc::block_id_t> &chan_ids,
            const std::vector<uhd::device_addr_t> &chan_args,
            const uhd::direction_t dir
    );

    /***********************************************************************
     * RFNoC-Specific
     **********************************************************************/
    void enumerate_rfnoc_blocks(
            size_t device_index,
            size_t n_blocks,
            size_t base_port,
            const uhd::sid_t &base_sid,
            uhd::device_addr_t transport_args
    );

    /***********************************************************************
     * Members
     **********************************************************************/
    // TODO: Maybe move these to private
    uhd::dict<std::string, boost::weak_ptr<uhd::rx_streamer> > _rx_streamers;
    uhd::dict<std::string, boost::weak_ptr<uhd::tx_streamer> > _tx_streamers;

private:
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
