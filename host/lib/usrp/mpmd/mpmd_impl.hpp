//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_IMPL_HPP
#define INCLUDED_MPMD_IMPL_HPP

#include "mpmd_xport_mgr.hpp"
#include "../device3/device3_impl.hpp"
#include <uhd/stream.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/transport/muxed_zero_copy_if.hpp>
#include <uhd/property_tree.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <boost/optional.hpp>
#include <map>
#include <memory>

namespace uhd { namespace mpmd {

/*! Stores all attributes specific to a single MPM device
 */
class mpmd_mboard_impl
{
  public:
    /*** Types ***************************************************************/
    using uptr = std::unique_ptr<mpmd_mboard_impl>;
    using dev_info = std::map<std::string, std::string>;

    /*** Static helper *******************************************************/
    /*! Will run some checks to determine if this device can be reached from
     *  the current UHD session
     *
     *  \param device_addr Device args. Must contain an mgmt_addr.
     */
    static boost::optional<device_addr_t> is_device_reachable(
        const device_addr_t& device_addr
    );

    /*** Structors ***********************************************************/
    /*! Ctor: Claim device or throw an exception on failure.
     *
     * Does not initialize the device.
     *
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

    /*** Init ****************************************************************/
    void init();

    /*** Public attributes ***************************************************/
    //! These are the args given by the user, with some filtering/preprocessing
    uhd::device_addr_t mb_args;

    //! Device information is read back via MPM and stored here.
    uhd::device_addr_t device_info;

    //! Dboard info is read back via MPM and stored here. There will be one
    // dictionary per dboard; but there's no requirement for the dictionary
    // to be populated at all.
    std::vector<uhd::device_addr_t> dboard_info;

    /*! Reference to the RPC client for this motherboard
     *
     * We store a shared ptr, because we might share it with some of the RFNoC
     * blocks.
     */
    uhd::rpc_client::sptr rpc;

    //! Number of RFNoC crossbars on this device
    const size_t num_xbars;

    /*************************************************************************
     * API
     ************************************************************************/
    //! Configure a crossbar to have a certain local address
    void set_xbar_local_addr(const size_t xbar_index, const size_t local_addr);

    //! Return the local address of a given crossbar
    size_t get_xbar_local_addr(const size_t xbar_index) const {
        return xbar_local_addrs.at(xbar_index);
    }

    //! Device-specific make_transport implementation
    //
    // A major difference to the mpmd_impl::make_transport() is the meaning of
    // the first argument (\p sid). mpmd_impl::make_transport() will add a
    // source part to the SID which needs to be taken into account in this
    // function.
    //
    // \param sid The full SID of this transport (UHD to device)
    // \param xport_type Transport type (CTRL, RX_DATA, ...)
    // \param args Any kind of args passed in via get_?x_stream()
    uhd::both_xports_t make_transport(
        const sid_t& sid,
        usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& args
    );

    size_t get_mtu(const uhd::direction_t dir) const;


    uhd::device_addr_t get_rx_hints() const;
    uhd::device_addr_t get_tx_hints() const;

    //! Set the RPC call timeout to the default value
    void set_timeout_default();

  private:
    /*! Reference to the RPC client that handles claiming
     */
    uhd::rpc_client::sptr _claim_rpc;
    /*************************************************************************
     * Private methods
     ************************************************************************/
    /*! Renew the claim onto the device.
     *
     * This is meant to be called repeatedly, e.g., using a UHD task. See also
     * _claimer_task.
     */
    bool claim();

     /*! Set RPC client timeout value
      *
      * \param timeout_ms time limit (in ms) that a rpc client waits for a single call
      */
     void set_rpcc_timeout(
        const uint64_t timeout_ms
    );

    uhd::task::sptr claim_device_and_make_task(
    );

    /*! Read out the log buffer from the MPM device and send it to native
     * logging system.
     */
    void dump_logs(const bool dump_to_null=false);

    /*************************************************************************
     * Private attributes
     ************************************************************************/
    //! Stores a list of local addresses of the crossbars. The local address is
    // what we use when addressing a crossbar in a CHDR header.
    std::vector<size_t> xbar_local_addrs;

    /*! Continuously reclaims the device.
     */
    uhd::task::sptr _claimer_task;

    uhd::mpmd::xport::mpmd_xport_mgr::uptr _xport_mgr;
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
    //! Device arg key which will allow finding all devices, even those not
    // reachable via CHDR.
    static const std::string MPM_FINDALL_KEY;
    //! Port on which the discovery process is listening (default value, it is
    //  user-overridable)
    static const size_t MPM_DISCOVERY_PORT;
    //! Device arg key to override the discovery port
    static const std::string MPM_DISCOVERY_PORT_KEY;
    //! Port on which the RPC process is listening (default value, it is user-
    //  overridable)
    static const size_t MPM_RPC_PORT;
    //! Device arg key to override the RPC port
    static const std::string MPM_RPC_PORT_KEY;
    //! This is the command that needs to be sent to the discovery port to
    // trigger a response.
    static const std::string MPM_DISCOVERY_CMD;
    //! This is the command that will let you measure ping responses from the
    // device via the discovery process. Useful for MTU discovery.
    static const std::string MPM_ECHO_CMD;
    //! This is the RPC command that will return the last known error from MPM.
    static const std::string MPM_RPC_GET_LAST_ERROR_CMD;

    /**************************************************************************
     * Structors
     ************************************************************************/
    mpmd_impl(const uhd::device_addr_t& device_addr);
    ~mpmd_impl();

    /**************************************************************************
     * API
     ************************************************************************/
    uhd::both_xports_t make_transport(const uhd::sid_t&,
                                      uhd::usrp::device3_impl::xport_type_t,
                                      const uhd::device_addr_t&);

  private:
    uhd::device_addr_t get_rx_hints(size_t mb_index);
    uhd::device_addr_t get_tx_hints(size_t mb_index);

    /*************************************************************************
     * Private methods/helpers
     ************************************************************************/
    /*! Claim a device and create a reference to the mpmd_mboard_impl object.
     *
     * Does not initialize the device (see setup_mb() for that).
     */
    mpmd_mboard_impl::uptr claim_and_make(
        const uhd::device_addr_t& dev_args
    );

    /*! Initialize a single motherboard
     *
     * This is where mpmd_mboard_impl::init() is called.
     * Also assigns the local crossbar addresses.
     *
     * \param mb Reference to the mboard class
     * \param mb_index Index number of the mboard that's being initialized
     * \param device_args Device args
     *
     */
    void setup_mb(
        mpmd_mboard_impl *mb,
        const size_t mb_index,
        const size_t base_xport_addr
    );

    //! Setup all RFNoC blocks running on mboard \p mb_i
    void setup_rfnoc_blocks(
        mpmd_mboard_impl* mb,
        const size_t mb_i,
        const uhd::device_addr_t& block_args
    );

    //! Configure all blocks that require access to an RPC client
    void setup_rpc_blocks(
        const uhd::device_addr_t &block_args,
        const bool serialize_init
    );

    /*! Return the index of the motherboard given the local address of a
     * crossbar
     */
    size_t identify_mboard_by_xbar_addr(const size_t xbar_addr) const;

    /*! Initialize property tree for a single device.
     *
     * \param tree Property tree reference (to the whole tree)
     * \param mb_path Subtree path for this device
     * \param mb Reference to the actual device
     */
    static void init_property_tree(
                uhd::property_tree::sptr tree,
                fs_path mb_path,
                mpmd_mboard_impl *mb
    );


    /*************************************************************************
     * Private attributes
     ************************************************************************/
    //! Stores the args with which the device was originally initialized
    uhd::device_addr_t _device_args;
    //! Stores a list of mboard references
    std::vector<mpmd_mboard_impl::uptr> _mb;
};

}} /* namespace uhd::mpmd */

uhd::device_addrs_t mpmd_find(const uhd::device_addr_t& hint_);

#endif /* INCLUDED_MPMD_IMPL_HPP */
// vim: sw=4 expandtab:
