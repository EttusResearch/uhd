//
// Copyright 2017 Ettus Research, National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include "mpmd_impl.hpp"
#include "mpmd_xport_mgr.hpp"
#include "mpmd_xport_ctrl_base.hpp"
#include "mpmd_xport_ctrl_udp.hpp"
#ifdef HAVE_LIBERIO
#  include "mpmd_xport_ctrl_liberio.hpp"
#endif

uhd::dict<std::string, std::string> uhd::mpmd::xport::filter_args(
    const uhd::device_addr_t& args,
    const std::string& prefix
) {
    uhd::dict<std::string, std::string> filtered_args;
    for (const std::string& key : args.keys()) {
        if (key.find(prefix) != std::string::npos) {
            filtered_args[key] = args[key];
        }
    }

    return filtered_args;
}

using namespace uhd::mpmd::xport;

class mpmd_xport_mgr_impl : public mpmd_xport_mgr
{
public:
    mpmd_xport_mgr_impl(
        const uhd::device_addr_t& mb_args
    ) : _mb_args(mb_args)
    {
        // nop
    }

    /**************************************************************************
     * API (see mpmd_xport_mgr.hpp)
     *************************************************************************/
    uhd::both_xports_t make_transport(
        const xport_info_list_t &xport_info_list,
        const uhd::usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& xport_args,
        xport_info_t& xport_info_out
    ) {
        for (const auto &xport_info : xport_info_list) {
            require_xport_mgr(xport_info.at("type"));
        }

        // Run our incredibly smart selection algorithm
        xport_info_out = select_xport_option(xport_info_list);
        const std::string xport_medium = xport_info_out.at("type");
        UHD_LOG_TRACE("MPMD",
                __func__ << "(): xport medium is " << xport_medium);

        // When we've picked our preferred option, pass it to the transport
        // implementation for execution:
        return _xport_ctrls[xport_medium]->make_transport(
            xport_info_out,
            xport_type,
            xport_args
       );
    }


private:
    /**************************************************************************
     * Private methods / helpers
     *************************************************************************/
    /*! Picks a transport option based on available data
     *
     * \param xport_info_list List of available options, they all need to be
     *                        valid choices.
     *
     * \returns One element of \p xport_info_list based on a selection
     *          algorithm.
     */
    xport_info_t select_xport_option(
        const xport_info_list_t &xport_info_list
    ) const {
        // Naive xport picking method, this needs fixing (FIXME).
        // Have algorithm run on baseline score, allocation, and random soup
        return xport_info_list.at(0);
    }

    //! Create an instance of an xport manager implementation
    //
    // \param xport_medium "UDP" or "liberio"
    // \param mb_args Device args
    mpmd_xport_ctrl_base::uptr make_mgr_impl(
        const std::string &xport_medium,
        const uhd::device_addr_t& mb_args
    ) const {
        if (xport_medium == "UDP") {
            return mpmd_xport_ctrl_base::uptr(
                new mpmd_xport_ctrl_udp(mb_args)
            );
#ifdef HAVE_LIBERIO
        } else if (xport_medium == "liberio") {
            return mpmd_xport_ctrl_base::uptr(
                new mpmd_xport_ctrl_liberio(mb_args)
            );
#endif
        } else {
            throw uhd::key_error("Unknown transport medium!");
        }
    }

    //! This will make sure that _xport_ctrls contains a valid transport manager
    // for \p xport_medium
    //
    // When this function returns, it will be possible to access
    // this->_xport_ctrls[xport_medium].
    //
    // \param xport_medium Type of transport, e.g. "UDP", "liberio", ...
    //
    // \throws uhd::key_error if \p xport_medium is not known or registered
    void require_xport_mgr(const std::string &xport_medium)
    {
        if (_xport_ctrls.count(xport_medium) == 0) {
            UHD_LOG_TRACE("MPMD",
                "Instantiating transport manager `" << xport_medium << "'");
            _xport_ctrls[xport_medium] = make_mgr_impl(xport_medium, _mb_args);
        }
    }

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    //! Cache available xport manager implementations
    //
    // Should only every be populated by require_xport_mgr()
    std::map<std::string, mpmd_xport_ctrl_base::uptr> _xport_ctrls;

    //! Motherboard args, can contain things like 'recv_buff_size'
    const uhd::device_addr_t _mb_args;
};

mpmd_xport_mgr::uptr mpmd_xport_mgr::make(
    const uhd::device_addr_t& mb_args
) {
    return mpmd_xport_mgr::uptr(new mpmd_xport_mgr_impl(mb_args));
}

