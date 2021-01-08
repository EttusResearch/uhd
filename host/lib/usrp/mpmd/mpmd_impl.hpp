//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_IMPL_HPP
#define INCLUDED_MPMD_IMPL_HPP

#include <uhd/property_tree.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <uhdlib/rfnoc/rfnoc_device.hpp>
#include <uhdlib/usrp/common/mpmd_mb_controller.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <boost/optional.hpp>
#include <map>
#include <memory>

/*************************************************************************
 * RPC timeout constants for MPMD
 ************************************************************************/
//! Time between reclaims (ms)
static constexpr size_t MPMD_RECLAIM_INTERVAL_MS = 1000;
//! Default timeout value for the init() RPC call (ms)
static constexpr size_t MPMD_DEFAULT_INIT_TIMEOUT = 120000;
//! Default timeout value for RPC calls (ms)
static constexpr size_t MPMD_DEFAULT_RPC_TIMEOUT = 2000;
//! Short timeout value for RPC calls (ms), used for calls that shouldn't
// take long. This value can be used to quickly determine a link status.
static constexpr size_t MPMD_SHORT_RPC_TIMEOUT = 2000;
//! Claimer loop timeout value for RPC calls (ms).
static constexpr size_t MPMD_CLAIMER_RPC_TIMEOUT = 10000;
//! Ethernet address for management and RPC communication
static const std::string MGMT_ADDR_KEY = "mgmt_addr";

namespace uhd { namespace mpmd {

/*! Stores all attributes specific to a single MPM device
 */
class mpmd_mboard_impl
{
public:
    /*** Types ***************************************************************/
    using uptr     = std::unique_ptr<mpmd_mboard_impl>;
    using dev_info = std::map<std::string, std::string>;

    //! MPMD-specific implementation of the mb_iface
    //
    // This handles the transport management
    class mpmd_mb_iface;

    /*** Static helper *******************************************************/
    /*! Will run some checks to determine if this device can be reached from
     *  the current UHD session
     *
     *  \param device_addr Device args. Must contain an mgmt_addr.
     */
    static boost::optional<device_addr_t> is_device_reachable(
        const device_addr_t& device_addr);

    /*** Structors ***********************************************************/
    /*! Ctor: Claim device or throw an exception on failure.
     *
     * Does not initialize the device.
     *
     * \param mb_args Device args that pertain to this motherboard
     * \param ip_addr RPC client will attempt to connect to this IP address
     */
    mpmd_mboard_impl(const uhd::device_addr_t& mb_args, const std::string& ip_addr);
    ~mpmd_mboard_impl();

    /*** Factory *************************************************************/
    /*!
     * \param mb_args Device args that pertain to this motherboard
     * \param ip_addr RPC client will attempt to connect to this IP address
     */
    static uptr make(const uhd::device_addr_t& mb_args, const std::string& addr);

    /*** API *****************************************************************/
    void init();

    uhd::rfnoc::mb_iface& get_mb_iface();

    /*** Public attributes ***************************************************/
    //! These are the args given by the user, with some filtering/preprocessing
    uhd::device_addr_t mb_args;

    //! Device information is read back via MPM and stored here.
    uhd::device_addr_t device_info;

    //! Dboard info is read back via MPM and stored here. There will be one
    // dictionary per dboard; but there's no requirement for the dictionary
    // to be populated at all.
    std::vector<uhd::device_addr_t> dboard_info;

    //! Reference to this motherboards mb_iface
    std::unique_ptr<mpmd_mb_iface> mb_iface;

    //! Reference to this motherboards mb_controller
    uhd::rfnoc::mpmd_mb_controller::sptr mb_ctrl;

    /*! Reference to the RPC client for this motherboard
     *
     * We store a shared ptr, because we might share it with some of the RFNoC
     * blocks.
     */
    uhd::rpc_client::sptr rpc;

    /*************************************************************************
     * API
     ************************************************************************/
    /*! Setting this flag will enable a mode where a reclaim failure is
     *  acceptable.
     *
     * The only legitimate time to do this is when a procedure is called that
     * can cause communication with the RPC server to be interrupted
     * legitimately, but non-critically. For example, when updating the FPGA
     * image, the RPC server gets rebooted, but the claim loop is running in a
     * separate thread, and needs some kind of flag to be notified that
     * something is up.
     */
    void allow_claim_failure(const bool allow)
    {
        if (allow) {
            _allow_claim_failure_latch = true;
        }
        _allow_claim_failure_flag = allow;
    }

    //! Read the device access token
    std::string get_token()
    {
        return _token;
    }

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
    void set_rpcc_timeout(const uint64_t timeout_ms);

    uhd::task::sptr claim_device_and_make_task();

    /*! Read out the log buffer from the MPM device and send it to native
     * logging system.
     */
    void dump_logs(const bool dump_to_null = false);

    /*************************************************************************
     * Private attributes
     ************************************************************************/
    /*! Continuously reclaims the device.
     */
    uhd::task::sptr _claimer_task;

    /*! A copy of the device access token
     */
    std::string _token;

    /*! This flag is only used within the claim() function. Go look there if you
     * really need to know what it does.
     */
    std::atomic<bool> _allow_claim_failure_flag{false};

    /*! This flag is only used within the claim() function. Go look there if you
     * really need to know what it does.
     */
    std::atomic<bool> _allow_claim_failure_latch{false};
};


/*! Parent class of an MPM device
 *
 * An MPM device is a USRP running MPM. Because most of the hardware controls
 * are taken care of by MPM itself, it is not necessary to write a specific
 * derived class for every single type of MPM device.
 */
class mpmd_impl : public uhd::rfnoc::detail::rfnoc_device
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
    ~mpmd_impl() override;

    /**************************************************************************
     * API
     ************************************************************************/
    uhd::rfnoc::mb_iface& get_mb_iface(const size_t mb_idx) override
    {
        if (mb_idx >= _mb.size()) {
            throw uhd::index_error(
                std::string("Cannot get mb_iface, invalid motherboard index: ")
                + std::to_string(mb_idx));
        }
        return _mb.at(mb_idx)->get_mb_iface();
    }

protected:
    //! Destroys the mboard_impls and the device_tree
    void _deinit();

private:
    /*************************************************************************
     * Private methods/helpers
     ************************************************************************/
    /*! Claim a device and create a reference to the mpmd_mboard_impl object.
     *
     * Does not initialize the device (see setup_mb() for that).
     */
    mpmd_mboard_impl::uptr claim_and_make(const uhd::device_addr_t& dev_args);

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
    void setup_mb(mpmd_mboard_impl* mb, const size_t mb_index);

    /*! Initialize property tree for a single device.
     *
     * \param tree Property tree reference (to the whole tree)
     * \param mb_path Subtree path for this device
     * \param mb Reference to the actual device
     */
    static void init_property_tree(
        uhd::property_tree::sptr tree, fs_path mb_path, mpmd_mboard_impl* mb);

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
