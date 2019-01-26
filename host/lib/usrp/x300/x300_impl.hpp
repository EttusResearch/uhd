//
// Copyright 2013-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_IMPL_HPP
#define INCLUDED_X300_IMPL_HPP

#include "../device3/device3_impl.hpp"
#include "x300_clock_ctrl.hpp"
#include "x300_defaults.hpp"
#include "x300_device_args.hpp"
#include "x300_fw_common.h"
#include "x300_radio_ctrl_impl.hpp"
#include "x300_regs.hpp"
#include <uhd/property_tree.hpp>
#include <uhd/transport/muxed_zero_copy_if.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/transport/udp_simple.hpp> //mtu
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
///////////// RFNOC /////////////////////
#include <uhd/rfnoc/block_ctrl.hpp>
///////////// RFNOC /////////////////////

#include <uhdlib/usrp/common/recv_packet_demuxer_3000.hpp>
#include <uhdlib/usrp/cores/i2c_core_100_wb32.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/weak_ptr.hpp>
#include <atomic>
#include <functional>

// Ethernet ports
enum x300_eth_iface_t {
    X300_IFACE_NONE = 0,
    X300_IFACE_ETH0 = 1,
    X300_IFACE_ETH1 = 2,
};

struct x300_eth_conn_t
{
    std::string addr;
    x300_eth_iface_t type;
    size_t link_rate;
};

uhd::uart_iface::sptr x300_make_uart_iface(uhd::wb_iface::sptr iface);

uhd::wb_iface::sptr x300_make_ctrl_iface_enet(
    uhd::transport::udp_simple::sptr udp, bool enable_errors = true);
uhd::wb_iface::sptr x300_make_ctrl_iface_pcie(
    uhd::niusrprio::niriok_proxy::sptr drv_proxy, bool enable_errors = true);

uhd::device_addrs_t x300_find(const uhd::device_addr_t& hint_);

class x300_impl : public uhd::usrp::device3_impl
{
public:
    //! Function to create a udp_simple::sptr (kernel-based or DPDK-based)
    using udp_simple_factory_t =
        std::function<uhd::transport::udp_simple::sptr(const std::string&, const std::string&)>;

    x300_impl(const uhd::device_addr_t&);
    void setup_mb(const size_t which, const uhd::device_addr_t&);
    ~x300_impl(void);

    // device claim functions
    enum claim_status_t { UNCLAIMED, CLAIMED_BY_US, CLAIMED_BY_OTHER };
    static claim_status_t claim_status(uhd::wb_iface::sptr iface);
    static void claim(uhd::wb_iface::sptr iface);
    static bool try_to_claim(uhd::wb_iface::sptr iface, long timeout = 2000);
    static void release(uhd::wb_iface::sptr iface);

    enum x300_mboard_t { USRP_X300_MB, USRP_X310_MB, USRP_X310_MB_NI_2974, UNKNOWN };
    static x300_mboard_t get_mb_type_from_pcie(
        const std::string& resource, const std::string& rpc_port);
    static x300_mboard_t get_mb_type_from_eeprom(
        const uhd::usrp::mboard_eeprom_t& mb_eeprom);

    //! Read out the on-board EEPROM, convert to dict, and return
    static uhd::usrp::mboard_eeprom_t get_mb_eeprom(uhd::i2c_iface::sptr i2c);

protected:
    void subdev_to_blockid(const uhd::usrp::subdev_spec_pair_t& spec,
        const size_t mb_i,
        uhd::rfnoc::block_id_t& block_id,
        uhd::device_addr_t& block_args);
    uhd::usrp::subdev_spec_pair_t blockid_to_subdev(
        const uhd::rfnoc::block_id_t& blockid, const uhd::device_addr_t& block_args);

private:
    // vector of member objects per motherboard
    struct mboard_members_t
    {
        uhd::usrp::x300::x300_device_args_t args;

        bool initialization_done;
        uhd::task::sptr claimer_task;
        std::string xport_path;

        std::vector<x300_eth_conn_t> eth_conns;
        size_t next_src_addr;
        size_t next_tx_src_addr;
        size_t next_rx_src_addr;

        // Discover the ethernet connections per motherboard
        void discover_eth(const uhd::usrp::mboard_eeprom_t mb_eeprom,
            const std::vector<std::string>& ip_addrs);

        // Get the primary ethernet connection
        inline const x300_eth_conn_t& get_pri_eth() const
        {
            return eth_conns[0];
        }

        uhd::device_addr_t send_args;
        uhd::device_addr_t recv_args;
        bool if_pkt_is_big_endian;
        uhd::niusrprio::niusrprio_session::sptr rio_fpga_interface;

        // perifs in the zpu
        uhd::wb_iface::sptr zpu_ctrl;
        spi_core_3000::sptr zpu_spi;
        i2c_core_100_wb32::sptr zpu_i2c;

        // other perifs on mboard
        x300_clock_ctrl::sptr clock;
        uhd::gps_ctrl::sptr gps;

        uhd::usrp::x300::fw_regmap_t::sptr fw_regmap;

        // which FPGA image is loaded
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
        uint32_t allocate_pcie_dma_chan(
            const uhd::sid_t& tx_sid, const xport_type_t xport_type);
    };
    std::vector<mboard_members_t> _mb;

    // task for periodically reclaiming the device from others
    void claimer_loop(uhd::wb_iface::sptr);

    std::atomic<size_t> _sid_framer;

    uhd::sid_t allocate_sid(mboard_members_t& mb,
        const uhd::sid_t& address,
        const uint32_t src_addr,
        const uint32_t src_dst);
    uhd::both_xports_t make_transport(const uhd::sid_t& address,
        const xport_type_t xport_type,
        const uhd::device_addr_t& args);

    //! get mtu
    size_t get_mtu(const size_t, const uhd::direction_t);

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
    frame_size_t determine_max_frame_size(
        const std::string& addr, const frame_size_t& user_mtu);

    ////////////////////////////////////////////////////////////////////
    //
    // Caching for transport interface re-use -- like sharing a DMA.
    // The cache is optionally used by make_transport by use-case.
    // The cache maps an ID string to a transport-ish object.
    // The ID string identifies a purpose for the transport.
    //
    // For recv, there is a demux cache, which maps a ID string
    // to a recv demux object. When a demux is used, the underlying transport
    // must never be used outside of the demux. Use demux->make_proxy(sid).
    //
    uhd::dict<std::string, uhd::usrp::recv_packet_demuxer_3000::sptr> _demux_cache;
    //
    // For send, there is a shared send xport, which maps an ID string
    // to a transport capable of sending buffers. Send transports
    // can be shared amongst multiple callers, unlike recv.
    //
    uhd::dict<std::string, uhd::transport::zero_copy_if::sptr> _send_cache;
    //
    ////////////////////////////////////////////////////////////////////

    uhd::dict<std::string, uhd::usrp::dboard_manager::sptr> _dboard_managers;

    bool _ignore_cal_file;

    void update_clock_control(mboard_members_t&);
    void initialize_clock_control(mboard_members_t& mb);
    void set_time_source_out(mboard_members_t&, const bool);
    void update_clock_source(mboard_members_t&, const std::string&);
    void update_time_source(mboard_members_t&, const std::string&);
    void sync_times(mboard_members_t&, const uhd::time_spec_t&);

    uhd::sensor_value_t get_ref_locked(mboard_members_t& mb);
    bool wait_for_clk_locked(mboard_members_t& mb, uint32_t which, double timeout);
    bool is_pps_present(mboard_members_t& mb);

    //! Write the contents of an EEPROM dict to the on-board EEPROM
    void set_mb_eeprom(uhd::i2c_iface::sptr i2c, const uhd::usrp::mboard_eeprom_t&);

    void check_fw_compat(const uhd::fs_path& mb_path, const mboard_members_t& members);
    void check_fpga_compat(const uhd::fs_path& mb_path, const mboard_members_t& members);

    /// More IO stuff
    uhd::device_addr_t get_tx_hints(size_t mb_index);
    uhd::device_addr_t get_rx_hints(size_t mb_index);

    void post_streamer_hooks(uhd::direction_t dir);

    udp_simple_factory_t _x300_make_udp_connected;
};

#endif /* INCLUDED_X300_IMPL_HPP */
