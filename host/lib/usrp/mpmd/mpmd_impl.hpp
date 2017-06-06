//
// Copyright 2017 Ettus Research (National Instruments)
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

#ifndef INCLUDED_MPMD_IMPL_HPP
#define INCLUDED_MPMD_IMPL_HPP
#include "../../utils/rpc.hpp"
#include "../device3/device3_impl.hpp"
#include <uhd/stream.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/tasks.hpp>
#include <map>

static const size_t MPMD_RX_SW_BUFF_SIZE_ETH        = 0x2000000;//32MiB    For an ~8k frame size any size >32MiB is just wasted buffer space
static const size_t MPMD_RX_SW_BUFF_SIZE_ETH_MACOS  = 0x100000; //1Mib

static const size_t MPM_DISCOVERY_PORT = 49600;
static const size_t MPM_RPC_PORT = 49601;
static const char MPM_DISCOVERY_CMD[] = "MPM-DISC";
static const char MPM_ECHO_CMD[] = "MPM-ECHO";
static const size_t MPMD_10GE_DATA_FRAME_MAX_SIZE = 8000; // CHDR packet size in bytes


struct frame_size_t
{
    size_t recv_frame_size;
    size_t send_frame_size;
};


/*! Stores all attributes specific to a single MPM device
 */
class mpmd_mboard_impl
{
  public:
    /*** Types ***************************************************************/
    using uptr = std::unique_ptr<mpmd_mboard_impl>;
    using dev_info = std::map<std::string, std::string>;

    /*** Structors ***********************************************************/
    /*!
     * \param mb_args Device args that pertain to this motherboard
     * \param ip_addr RPC client will attempt to connect to this IP address
     */
    mpmd_mboard_impl(
            const uhd::device_addr_t &mb_args,
            const std::string& ip_addr
    );
    ~mpmd_mboard_impl();

    /*** Factory *************************************************************/
    /*!
     * \param mb_args Device args that pertain to this motherboard
     * \param ip_addr RPC client will attempt to connect to this IP address
     */
    static uptr make(
            const uhd::device_addr_t &mb_args,
            const std::string& addr
    );

    /*** Public attributes ***************************************************/
    //! These are the args given by the user, with some filtering/preprocessing
    uhd::device_addr_t mb_args;

    //! Device information is read back via MPM and stored here.
    uhd::device_addr_t device_info;

    //! Dboard info is read back via MPM and stored here. There will be one
    // dictionary per dboard; but there's no requirement for the dictionary
    // to be populated at all.
    std::vector<uhd::device_addr_t> dboard_info;

    //! Number of RFNoC crossbars on this device
    size_t num_xbars = 0;

    /*! Reference to the RPC client for this motherboard
     *
     * We store a shared ptr, because we might share it with some of the RFNoC
     * blocks.
     */
    uhd::rpc_client::sptr rpc;


    /*************************************************************************
     * API
     ************************************************************************/
    uhd::sid_t allocate_sid(const uint16_t port,
                            const uhd::sid_t address,
                            const uint32_t xbar_src_addr,
                            const uint32_t xbar_src_dst,
                            const uint32_t dst_addr
                            );

    //! Configure a crossbar to have a certain local address
    void set_xbar_local_addr(const size_t xbar_index, const size_t local_addr);

    //! Return the local address of a given crossbar
    size_t get_xbar_local_addr(const size_t xbar_index) {
        return xbar_local_addrs.at(xbar_index);
    }

  private:
    /*************************************************************************
     * Private methods
     ************************************************************************/
    /*! Renew the claim onto the device.
     *
     * This is meant to be called repeatedly, e.g., using a UHD task.
     */
    bool claim();

    uhd::task::sptr claim_device_and_make_task(
            uhd::rpc_client::sptr rpc,
            const uhd::device_addr_t mb_args
    );

    /*************************************************************************
     * Private attributes
     ************************************************************************/
    //! Stores a list of local addresses of the crossbars. The local address is
    // what we use when addressing a crossbar in a CHDR header.
    std::vector<size_t> xbar_local_addrs;


    /*! Continuously reclaims the device.
     */
    uhd::task::sptr _claimer_task;
};


/*! Parent class of an MPM device
 *
 * An MPM device is a USRP running MPM. Because most of the hardware controls
 * are taken care of by MPM itself, it is not necessary to write a specific
 * derived class for every single type of MPM device.
 */
class mpmd_impl : public uhd::usrp::device3_impl
{
  public:
    mpmd_impl(const uhd::device_addr_t& device_addr);
    ~mpmd_impl();

    uhd::both_xports_t make_transport(const uhd::sid_t&,
                                      uhd::usrp::device3_impl::xport_type_t,
                                      const uhd::device_addr_t&);

  private:
    uhd::device_addr_t get_rx_hints(size_t mb_index);

    /*************************************************************************
     * Private methods/helpers
     ************************************************************************/
    /*! Initialize a single motherboard
     *
     * - See mpmd_mboard_impl ctor for details
     * - Also allocates the local crossbar addresses
     */
    mpmd_mboard_impl::uptr setup_mb(
            const size_t mb_i,
            const uhd::device_addr_t& dev_addr
    );

    //! Setup all RFNoC blocks running on mboard \p mb_i
    void setup_rfnoc_blocks(
            const size_t mb_i,
            const uhd::device_addr_t& block_args
    );

    //! Configure all blocks that require access to an RPC client
    void setup_rpc_blocks(const uhd::device_addr_t &block_args);

    /*! Returns a valid local address for a crossbar
     *
     * \returns Valid local address
     * \throws uhd::runtime_error if there are no more local addresses
     */
    size_t allocate_xbar_local_addr();

    /*! Return the index of the motherboard carrying the crossbar at \p remote_addr
     */
    size_t identify_mboard_by_sid(const size_t remote_addr);


    /*************************************************************************
     * Private attributes
     ************************************************************************/
    uhd::dict<std::string, std::string> recv_args;
    uhd::dict<std::string, std::string> send_args;

    //! Stores the args with which the device was originally initialized
    uhd::device_addr_t _device_args;
    //! Stores a list of mboard references
    std::vector<mpmd_mboard_impl::uptr> _mb;

    //! A counter for distributing local addresses to crossbars
    // No-one touches this except allocate_xbar_local_addr(), gotcha?
    size_t  _xbar_local_addr_ctr = 2;

    size_t _sid_framer;
};

uhd::device_addrs_t mpmd_find(const uhd::device_addr_t& hint_);

#endif /* INCLUDED_MPMD_IMPL_HPP */
// vim: sw=4 expandtab:
