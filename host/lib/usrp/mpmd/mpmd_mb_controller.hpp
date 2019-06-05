//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_MPMD_MB_CONTROLLER_HPP
#define INCLUDED_LIBUHD_MPMD_MB_CONTROLLER_HPP

#include <uhd/rfnoc/mb_controller.hpp>
#include <uhdlib/utils/rpc.hpp>

namespace uhd { namespace rfnoc {

/*! X300-Specific version of the mb_controller
 *
 * Reminder: There is one of these per motherboard.
 */
class mpmd_mb_controller : public mb_controller
{
public:


    //! Return reference to the RPC client
    uhd::rpc_client::sptr get_rpc_client() { return _rpc; }

    //! X300-specific version of the timekeeper controls
    class mpmd_timekeeper : public mb_controller::timekeeper
    {
    public:
        mpmd_timekeeper(const size_t tk_idx, uhd::rpc_client::sptr rpc_client)
            : _tk_idx(tk_idx), _rpc(rpc_client)
        {
            // nop
        }

        uint64_t get_ticks_now();

        uint64_t get_ticks_last_pps();

        void set_ticks_now(const uint64_t ticks);

        void set_ticks_next_pps(const uint64_t ticks);

        void set_period(const uint64_t period_ns);

    private:
        /*! Shorthand to perform an RPC request. Saves some typing.
         */
        template <typename return_type, typename... Args>
        return_type request(std::string const& func_name, Args&&... args)
        {
            UHD_LOG_TRACE("X300MBCTRL", "[RPC] Calling " << func_name);
            return _rpc->request_with_token<return_type>(
                func_name, std::forward<Args>(args)...);
        };

        const size_t _tk_idx;

        uhd::rpc_client::sptr _rpc;
    };

private:
    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Reference to RPC interface
    uhd::rpc_client::sptr _rpc;
};

}} // namespace uhd::rfnoc

#endif /* INCLUDED_LIBUHD_MPMD_MB_CONTROLLER_HPP */
