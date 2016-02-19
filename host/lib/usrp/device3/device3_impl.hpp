//
// Copyright 2014-2015 Ettus Research LLC
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

#include <uhd/usrp/subdev_spec.hpp>
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
// Common FPGA cores:
#include "radio_ctrl_core_3000.hpp"
#include "rx_dsp_core_3000.hpp"
#include "tx_dsp_core_3000.hpp"
#include "rx_vita_core_3000.hpp"
#include "tx_vita_core_3000.hpp"
#include "rx_frontend_core_200.hpp"
#include "tx_frontend_core_200.hpp"
#include "time_core_3000.hpp"
#include "gpio_atr_3000.hpp"
// RFNoC-specific includes:
#include "../rfnoc/radio_ctrl.hpp"

namespace uhd { namespace usrp {

/***********************************************************************
 * Default settings (any device3 may override these)
 **********************************************************************/
static const size_t DEVICE3_RX_FC_REQUEST_FREQ         = 32;    //per flow-control window
static const size_t DEVICE3_TX_FC_RESPONSE_FREQ        = 8;
static const size_t DEVICE3_TX_FC_RESPONSE_CYCLES      = 0;     // Cycles: Off.

static const size_t DEVICE3_TX_MAX_HDR_LEN             = uhd::transport::vrt::chdr::max_if_hdr_words64 * sizeof(boost::uint64_t);    // Bytes
static const size_t DEVICE3_RX_MAX_HDR_LEN             = uhd::transport::vrt::chdr::max_if_hdr_words64 * sizeof(boost::uint64_t);    // Bytes

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

    //! Common cores used by radio.v
    struct radio_v_perifs_t
    {
        radio_ctrl_core_3000::sptr ctrl;
        time_core_3000::sptr time64;
        rx_vita_core_3000::sptr framer;
        rx_dsp_core_3000::sptr ddc;
        tx_vita_core_3000::sptr deframer;
        tx_dsp_core_3000::sptr duc;
        usrp::gpio_atr::gpio_atr_3000::sptr leds;
        rx_frontend_core_200::sptr rx_fe;
        tx_frontend_core_200::sptr tx_fe;
    };

    /***********************************************************************
     * I/O Interface
     **********************************************************************/
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &);
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &);
    bool recv_async_msg(uhd::async_metadata_t &async_metadata, double timeout);

    /***********************************************************************
     * Subdev-Spec legacy support
     **********************************************************************/
    /*! Translate a subdev spec pair into a block ID and block args.
     */
    virtual void subdev_to_blockid(
            const uhd::usrp::subdev_spec_pair_t &spec, const size_t mb_i,
            rfnoc::block_id_t &block_id, device_addr_t &block_args
    ) = 0;

    /*! Translate a block ID and args into a subdev spec pair.
     */
    virtual uhd::usrp::subdev_spec_pair_t blockid_to_subdev(
            const rfnoc::block_id_t &blockid, const device_addr_t &block_args
    ) = 0;

    /*! Updates a mboards settings from a subdev spec.
     *
     * Since device3's don't actually use subdev specs internally,
     * this will translate the spec into a channel definition and
     * update this devices channels.
     */
    void update_subdev_spec(
            const uhd::usrp::subdev_spec_t &spec,
            const direction_t direction,
            const size_t mb_i=0
    );

    uhd::usrp::subdev_spec_t get_subdev_spec(
            const direction_t direction,
            const size_t mb_i=0
    );


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
    virtual both_xports_t make_transport(
        const uhd::sid_t &address,
        const xport_type_t xport_type,
        const uhd::device_addr_t& args
    ) = 0;

    virtual uhd::device_addr_t get_tx_hints(size_t) { return uhd::device_addr_t(); };
    virtual uhd::device_addr_t get_rx_hints(size_t) { return uhd::device_addr_t(); };
    virtual uhd::endianness_t get_transport_endianness(size_t mb_index) = 0;

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
            uhd::device_addr_t transport_args,
            uhd::endianness_t endianness
    );

    void init_radio_ctrl(
            const radio_v_perifs_t &perifs,
            const uhd::sid_t &address,
            const size_t mb_i,
            const endianness_t endianness,
            const uhd::rfnoc::radio_ctrl::dboard_type_t dboard_type
    );

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
