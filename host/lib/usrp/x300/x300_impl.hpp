//
// Copyright 2013-2016 Ettus Research LLC
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

#ifndef INCLUDED_X300_IMPL_HPP
#define INCLUDED_X300_IMPL_HPP

#include <uhd/property_tree.hpp>
#include "../device3/device3_impl.hpp"
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/types/sensors.hpp>
#include "x300_radio_ctrl_impl.hpp"
#include "x300_clock_ctrl.hpp"
#include "x300_fw_common.h"
#include <uhd/transport/udp_simple.hpp> //mtu
#include "i2c_core_100_wb32.hpp"
#include <boost/weak_ptr.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/muxed_zero_copy_if.hpp>
#include "recv_packet_demuxer_3000.hpp"
#include "x300_regs.hpp"
///////////// RFNOC /////////////////////
#include <uhd/rfnoc/block_ctrl.hpp>
///////////// RFNOC /////////////////////
#include <boost/dynamic_bitset.hpp>

static const std::string X300_FW_FILE_NAME  = "usrp_x300_fw.bin";
static const std::string X300_DEFAULT_CLOCK_SOURCE  = "internal";

static const double X300_DEFAULT_TICK_RATE          = 200e6;        //Hz
static const double X300_DEFAULT_DBOARD_CLK_RATE    = 50e6;         //Hz
static const double X300_BUS_CLOCK_RATE             = 166.666667e6; //Hz

static const size_t X300_RX_SW_BUFF_SIZE_ETH        = 0x2000000;//32MiB    For an ~8k frame size any size >32MiB is just wasted buffer space
static const size_t X300_RX_SW_BUFF_SIZE_ETH_MACOS  = 0x100000; //1Mib

//The FIFO closest to the DMA controller is 1023 elements deep for RX and 1029 elements deep for TX
//where an element is 8 bytes. The buffers (number of frames * frame size) must be aligned to the
//memory page size.  For the control, we are getting lucky because 64 frames * 256 bytes each aligns
//with the typical page size of 4096 bytes.  Since most page sizes are 4096 bytes or some multiple of
//that, keep the number of frames * frame size aligned to it.
static const size_t X300_PCIE_RX_DATA_FRAME_SIZE        = 4096;     //bytes
static const size_t X300_PCIE_RX_DATA_NUM_FRAMES        = 4096;
static const size_t X300_PCIE_TX_DATA_FRAME_SIZE        = 4096;     //bytes
static const size_t X300_PCIE_TX_DATA_NUM_FRAMES	    = 4096;
static const size_t X300_PCIE_MSG_FRAME_SIZE            = 256;      //bytes
static const size_t X300_PCIE_MSG_NUM_FRAMES            = 64;
static const size_t X300_PCIE_MAX_CHANNELS              = 6;
static const size_t X300_PCIE_MAX_MUXED_CTRL_XPORTS     = 32;
static const size_t X300_PCIE_MAX_MUXED_ASYNC_XPORTS    = 4;

static const size_t X300_10GE_DATA_FRAME_MAX_SIZE   = 8000;     // CHDR packet size in bytes
static const size_t X300_1GE_DATA_FRAME_MAX_SIZE    = 1472;     // CHDR packet size in bytes
static const size_t X300_ETH_MSG_FRAME_SIZE         = uhd::transport::udp_simple::mtu;  //bytes
// MTU throttling for ethernet/TX (see above):
static const size_t X300_ETH_DATA_FRAME_MAX_TX_SIZE = 8000;

static const double X300_THREAD_BUFFER_TIMEOUT      = 0.1;   // Time in seconds

static const size_t X300_ETH_MSG_NUM_FRAMES         = 64;
static const size_t X300_ETH_DATA_NUM_FRAMES        = 32;
static const double X300_DEFAULT_SYSREF_RATE        = 10e6;

static const size_t X300_MAX_RATE_PCIE              = 800000000; // bytes/s
static const size_t X300_MAX_RATE_10GIGE            = (size_t)(  // bytes/s
        10e9 / 8 *                                               // wire speed multiplied by percentage of packets that is sample data
        ( float(X300_10GE_DATA_FRAME_MAX_SIZE - uhd::usrp::DEVICE3_TX_MAX_HDR_LEN) /
          float(X300_10GE_DATA_FRAME_MAX_SIZE + 8 /* UDP header */ + 20 /* Ethernet header length */ )));
static const size_t X300_MAX_RATE_1GIGE            = (size_t)(  // bytes/s
        1e9 / 8 *                                               // wire speed multiplied by percentage of packets that is sample data
        ( float(X300_1GE_DATA_FRAME_MAX_SIZE - uhd::usrp::DEVICE3_TX_MAX_HDR_LEN) /
          float(X300_1GE_DATA_FRAME_MAX_SIZE + 8 /* UDP header */ + 20 /* Ethernet header length */ )));

#define X300_RADIO_DEST_PREFIX_TX 0

#define X300_XB_DST_E0  0
#define X300_XB_DST_E1  1
#define X300_XB_DST_PCI 2
#define X300_XB_DST_R0  3 // Radio 0 -> Slot A
#define X300_XB_DST_R1  4 // Radio 1 -> Slot B
#define X300_XB_DST_CE0 5

#define X300_SRC_ADDR0  0
#define X300_SRC_ADDR1  1
#define X300_DST_ADDR   2

// Ethernet ports
enum x300_eth_iface_t
{
    X300_IFACE_NONE = 0,
    X300_IFACE_ETH0 = 1,
    X300_IFACE_ETH1 = 2,
};

struct x300_eth_conn_t
{
    std::string addr;
    x300_eth_iface_t type;
};


uhd::uart_iface::sptr x300_make_uart_iface(uhd::wb_iface::sptr iface);

uhd::wb_iface::sptr x300_make_ctrl_iface_enet(uhd::transport::udp_simple::sptr udp, bool enable_errors = true);
uhd::wb_iface::sptr x300_make_ctrl_iface_pcie(uhd::niusrprio::niriok_proxy::sptr drv_proxy, bool enable_errors = true);

uhd::device_addrs_t x300_find(const uhd::device_addr_t &hint_);

class x300_impl : public uhd::usrp::device3_impl
{
public:

    x300_impl(const uhd::device_addr_t &);
    void setup_mb(const size_t which, const uhd::device_addr_t &);
    ~x300_impl(void);

    // device claim functions
    enum claim_status_t {UNCLAIMED, CLAIMED_BY_US, CLAIMED_BY_OTHER};
    static claim_status_t claim_status(uhd::wb_iface::sptr iface);
    static void claim(uhd::wb_iface::sptr iface);
    static bool try_to_claim(uhd::wb_iface::sptr iface, long timeout = 2000);
    static void release(uhd::wb_iface::sptr iface);

    enum x300_mboard_t {
        USRP_X300_MB, USRP_X310_MB, UNKNOWN
    };
    static x300_mboard_t get_mb_type_from_pcie(const std::string& resource, const std::string& rpc_port);
    static x300_mboard_t get_mb_type_from_eeprom(const uhd::usrp::mboard_eeprom_t& mb_eeprom);

protected:
    void subdev_to_blockid(
            const uhd::usrp::subdev_spec_pair_t &spec, const size_t mb_i,
            uhd::rfnoc::block_id_t &block_id, uhd::device_addr_t &block_args
    );
    uhd::usrp::subdev_spec_pair_t blockid_to_subdev(
            const uhd::rfnoc::block_id_t &blockid, const uhd::device_addr_t &block_args
    );

private:

    //vector of member objects per motherboard
    struct mboard_members_t
    {
        bool initialization_done;
        uhd::task::sptr claimer_task;
        std::string xport_path;

        std::vector<x300_eth_conn_t> eth_conns;
        size_t next_src_addr;
        size_t next_tx_src_addr;
        size_t next_rx_src_addr;

        // Discover the ethernet connections per motherboard
        void discover_eth(const uhd::usrp::mboard_eeprom_t mb_eeprom,
                          const std::vector<std::string> &ip_addrs);

        // Get the primary ethernet connection
        inline const x300_eth_conn_t& get_pri_eth() const
        {
            return eth_conns[0];
        }

        uhd::device_addr_t send_args;
        uhd::device_addr_t recv_args;
        bool if_pkt_is_big_endian;
        uhd::niusrprio::niusrprio_session::sptr  rio_fpga_interface;

        //perifs in the zpu
        uhd::wb_iface::sptr zpu_ctrl;
        spi_core_3000::sptr zpu_spi;
        i2c_core_100_wb32::sptr zpu_i2c;

        //other perifs on mboard
        x300_clock_ctrl::sptr clock;
        uhd::gps_ctrl::sptr gps;

        uhd::usrp::x300::fw_regmap_t::sptr fw_regmap;

        //which FPGA image is loaded
        std::string loaded_fpga_image;

        size_t hw_rev;
        std::string current_refclk_src;

        std::vector<uhd::rfnoc::x300_radio_ctrl_impl::sptr> radios;

        // PCIe specific components:

        //! Maps SID -> DMA channel
        std::map<uint32_t, uint32_t> _dma_chan_pool;
        //! Control transport for one PCIe connection
        uhd::transport::muxed_zero_copy_if::sptr ctrl_dma_xport;
        //! Async message transport
        uhd::transport::muxed_zero_copy_if::sptr async_msg_dma_xport;

        /*! Allocate or return a previously allocated PCIe channel pair
         *
         * Note the SID is always the transmit SID (i.e. from host to device).
         */
        uint32_t allocate_pcie_dma_chan(const uhd::sid_t &tx_sid, const xport_type_t xport_type);
    };
    std::vector<mboard_members_t> _mb;

    //task for periodically reclaiming the device from others
    void claimer_loop(uhd::wb_iface::sptr);

    size_t _sid_framer;

    uhd::sid_t allocate_sid(
        mboard_members_t &mb,
        const uhd::sid_t &address,
        const uint32_t src_addr,
        const uint32_t src_dst);
    uhd::both_xports_t make_transport(
        const uhd::sid_t &address,
        const xport_type_t xport_type,
        const uhd::device_addr_t& args
    );

    struct frame_size_t
    {
        size_t recv_frame_size;
        size_t send_frame_size;
    };
    frame_size_t _max_frame_sizes;

    /*!
     * Automatically determine the maximum frame size available by sending a UDP packet
     * to the device and see which packet sizes actually work. This way, we can take
     * switches etc. into account which might live between the device and the host.
     */
    frame_size_t determine_max_frame_size(const std::string &addr, const frame_size_t &user_mtu);

    ////////////////////////////////////////////////////////////////////
    //
    //Caching for transport interface re-use -- like sharing a DMA.
    //The cache is optionally used by make_transport by use-case.
    //The cache maps an ID string to a transport-ish object.
    //The ID string identifies a purpose for the transport.
    //
    //For recv, there is a demux cache, which maps a ID string
    //to a recv demux object. When a demux is used, the underlying transport
    //must never be used outside of the demux. Use demux->make_proxy(sid).
    //
    uhd::dict<std::string, uhd::usrp::recv_packet_demuxer_3000::sptr> _demux_cache;
    //
    //For send, there is a shared send xport, which maps an ID string
    //to a transport capable of sending buffers. Send transports
    //can be shared amongst multiple callers, unlike recv.
    //
    uhd::dict<std::string, uhd::transport::zero_copy_if::sptr> _send_cache;
    //
    ////////////////////////////////////////////////////////////////////

    uhd::dict<std::string, uhd::usrp::dboard_manager::sptr> _dboard_managers;

    bool _ignore_cal_file;

    void update_clock_control(mboard_members_t&);
    void initialize_clock_control(mboard_members_t &mb);
    void set_time_source_out(mboard_members_t&, const bool);
    void update_clock_source(mboard_members_t&, const std::string &);
    void update_time_source(mboard_members_t&, const std::string &);
    void sync_times(mboard_members_t&, const uhd::time_spec_t&);

    uhd::sensor_value_t get_ref_locked(mboard_members_t& mb);
    bool wait_for_clk_locked(mboard_members_t& mb, uint32_t which, double timeout);
    bool is_pps_present(mboard_members_t& mb);

    void set_mb_eeprom(uhd::i2c_iface::sptr i2c, const uhd::usrp::mboard_eeprom_t &);

    void check_fw_compat(const uhd::fs_path &mb_path, uhd::wb_iface::sptr iface);
    void check_fpga_compat(const uhd::fs_path &mb_path, const mboard_members_t &members);

    /// More IO stuff
    uhd::device_addr_t get_tx_hints(size_t mb_index);
    uhd::device_addr_t get_rx_hints(size_t mb_index);
    uhd::endianness_t get_transport_endianness(size_t mb_index) {
        return _mb[mb_index].if_pkt_is_big_endian ? uhd::ENDIANNESS_BIG : uhd::ENDIANNESS_LITTLE;
    };

    void post_streamer_hooks(uhd::direction_t dir);
};

#endif /* INCLUDED_X300_IMPL_HPP */
// vim: sw=4 expandtab:
