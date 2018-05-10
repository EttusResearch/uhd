//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_USRPRIO_RPC_CLIENT_HPP
#define INCLUDED_USRPRIO_RPC_CLIENT_HPP

#include <uhd/transport/nirio/rpc/rpc_common.hpp>
#include <uhd/transport/nirio/rpc/rpc_client.hpp>
#include <uhd/transport/nirio/rpc/usrprio_rpc_common.hpp>
#include <uhd/transport/nirio/status.h>

namespace uhd { namespace usrprio_rpc {

class UHD_API usrprio_rpc_client {
public:
    usrprio_rpc_client(
        std::string server,
        std::string port);
    ~usrprio_rpc_client();

    inline void set_rpc_timeout(boost::posix_time::milliseconds timeout_in_ms) {
        _timeout = timeout_in_ms;
    }

    inline nirio_status get_ctor_status() {
        return _ctor_status;
    }

    nirio_status niusrprio_enumerate(
        NIUSRPRIO_ENUMERATE_ARGS);
    nirio_status niusrprio_open_session(
        NIUSRPRIO_OPEN_SESSION_ARGS);
    nirio_status niusrprio_close_session(
        NIUSRPRIO_CLOSE_SESSION_ARGS);
    nirio_status niusrprio_reset_device(
        NIUSRPRIO_RESET_SESSION_ARGS);
    nirio_status niusrprio_download_bitstream_to_fpga(
        NIUSRPRIO_DOWNLOAD_BITSTREAM_TO_FPGA_ARGS);
    nirio_status niusrprio_get_interface_path(
         NIUSRPRIO_GET_INTERFACE_PATH_ARGS);
    nirio_status niusrprio_download_fpga_to_flash(
        NIUSRPRIO_DOWNLOAD_FPGA_TO_FLASH_ARGS);

private:
    static nirio_status _boost_error_to_nirio_status(const boost::system::error_code& err);

    rpc_client                      _rpc_client;
    boost::posix_time::milliseconds _timeout;
    nirio_status                    _ctor_status;
};

}}

#endif /* INCLUDED_USRPRIO_RPC_CLIENT_HPP */
