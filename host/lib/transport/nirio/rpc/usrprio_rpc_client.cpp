//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/nirio/rpc/usrprio_rpc_client.hpp>
#include <uhd/utils/platform.hpp>

namespace {
    constexpr int64_t DEFAULT_TIMEOUT_IN_MS = 5000;
}


namespace uhd { namespace usrprio_rpc {

usrprio_rpc_client::usrprio_rpc_client(
    std::string server,
    std::string port
) : _rpc_client(server, port, uhd::get_process_id(), uhd::get_host_id()),
    _timeout(boost::posix_time::milliseconds(long(DEFAULT_TIMEOUT_IN_MS)))
{
   _ctor_status = _rpc_client.status() ? NiRio_Status_RpcConnectionError : NiRio_Status_Success;
}

usrprio_rpc_client::~usrprio_rpc_client()
{
}

nirio_status usrprio_rpc_client::niusrprio_enumerate(NIUSRPRIO_ENUMERATE_ARGS)
/*
#define NIUSRPRIO_ENUMERATE_ARGS         \
    usrprio_device_info_vtr& device_info_vtr
*/
{
    usrprio_rpc::func_args_writer_t in_args;
    usrprio_rpc::func_args_reader_t out_args;
    nirio_status status = NiRio_Status_Success;
    uint32_t vtr_size = 0;

    status = _boost_error_to_nirio_status(
        _rpc_client.call(NIUSRPRIO_ENUMERATE, in_args, out_args, _timeout));

    if (nirio_status_not_fatal(status)) {
        out_args >> status;
        out_args >> vtr_size;
    }
    if (nirio_status_not_fatal(status) && vtr_size > 0) {
        device_info_vtr.resize(vtr_size);
        for (size_t i = 0; i < (size_t)vtr_size; i++) {
            usrprio_device_info info;
            out_args >> info;
            device_info_vtr[i] = info;
        }
    }
    return status;
}

nirio_status usrprio_rpc_client::niusrprio_open_session(NIUSRPRIO_OPEN_SESSION_ARGS)
/*
#define NIUSRPRIO_OPEN_SESSION_ARGS     \
    const std::string& resource,        \
    const std::string& path,            \
    const std::string& signature,       \
    const uint16_t& download_fpga
*/
{
    usrprio_rpc::func_args_writer_t in_args;
    usrprio_rpc::func_args_reader_t out_args;
    nirio_status status = NiRio_Status_Success;

    in_args << resource;
    in_args << path;
    in_args << signature;
    in_args << download_fpga;

    //Open needs a longer timeout because the FPGA download can take upto 6 secs and the NiFpga libload can take 4.
    static const uint32_t OPEN_TIMEOUT = 15000;
    status = _boost_error_to_nirio_status(
        _rpc_client.call(NIUSRPRIO_OPEN_SESSION, in_args, out_args, boost::posix_time::milliseconds(OPEN_TIMEOUT)));

    if (nirio_status_not_fatal(status)) {
        out_args >> status;
    }

    return status;
}

nirio_status usrprio_rpc_client::niusrprio_close_session(NIUSRPRIO_CLOSE_SESSION_ARGS)
/*
#define NIUSRPRIO_CLOSE_SESSION_ARGS    \
    const std::string& resource
*/
{
    usrprio_rpc::func_args_writer_t in_args;
    usrprio_rpc::func_args_reader_t out_args;
    nirio_status status = NiRio_Status_Success;

    in_args << resource;

    status = _boost_error_to_nirio_status(
        _rpc_client.call(NIUSRPRIO_CLOSE_SESSION, in_args, out_args, _timeout));

    if (nirio_status_not_fatal(status)) {
        out_args >> status;
    }

    return status;
}

nirio_status usrprio_rpc_client::niusrprio_reset_device(NIUSRPRIO_RESET_SESSION_ARGS)
/*
#define NIUSRPRIO_RESET_SESSION_ARGS    \
    const std::string& resource
*/
{
    usrprio_rpc::func_args_writer_t in_args;
    usrprio_rpc::func_args_reader_t out_args;
    nirio_status status = NiRio_Status_Success;

    in_args << resource;

    status = _boost_error_to_nirio_status(
        _rpc_client.call(NIUSRPRIO_RESET_SESSION, in_args, out_args, _timeout));

    if (nirio_status_not_fatal(status)) {
        out_args >> status;
    }

    return status;
}

nirio_status usrprio_rpc_client::niusrprio_get_interface_path(NIUSRPRIO_GET_INTERFACE_PATH_ARGS)
/*
#define NIUSRPRIO_GET_INTERFACE_PATH_ARGS   \
    const std::string& resource,            \
    std::string& interface_path
*/
{
    usrprio_rpc::func_args_writer_t in_args;
    usrprio_rpc::func_args_reader_t out_args;
    nirio_status status = NiRio_Status_Success;

    in_args << resource;

    status = _boost_error_to_nirio_status(
        _rpc_client.call(NIUSRPRIO_GET_INTERFACE_PATH, in_args, out_args, _timeout));

    if (nirio_status_not_fatal(status)) {
        out_args >> status;
        out_args >> interface_path;
    }

    return status;
}

nirio_status usrprio_rpc_client::niusrprio_download_fpga_to_flash(NIUSRPRIO_DOWNLOAD_FPGA_TO_FLASH_ARGS)
/*
#define NIUSRPRIO_DOWNLOAD_FPGA_TO_FLASH_ARGS   \
    const uint32_t& interface_num,       \
    const std::string& bitstream_path
*/
{
    usrprio_rpc::func_args_writer_t in_args;
    usrprio_rpc::func_args_reader_t out_args;
    nirio_status status = NiRio_Status_Success;

    in_args << resource;
    in_args << bitstream_path;

    static const uint32_t DOWNLOAD_FPGA_TIMEOUT = 1200000;
    status = _boost_error_to_nirio_status(
        _rpc_client.call(NIUSRPRIO_DOWNLOAD_FPGA_TO_FLASH, in_args, out_args,
            boost::posix_time::milliseconds(DOWNLOAD_FPGA_TIMEOUT)));

    if (nirio_status_not_fatal(status)) {
        out_args >> status;
    }

    return status;
}

nirio_status usrprio_rpc_client::niusrprio_download_bitstream_to_fpga(NIUSRPRIO_DOWNLOAD_BITSTREAM_TO_FPGA_ARGS)
/*
#define NIUSRPRIO_DOWNLOAD_BITSTREAM_TO_FPGA_ARGS    \
    const std::string& resource
*/
{
    usrprio_rpc::func_args_writer_t in_args;
    usrprio_rpc::func_args_reader_t out_args;
    nirio_status status = NiRio_Status_Success;

    in_args << resource;

    status = _boost_error_to_nirio_status(
        _rpc_client.call(NIUSRPRIO_DOWNLOAD_BITSTREAM_TO_FPGA, in_args, out_args, _timeout));

    if (nirio_status_not_fatal(status)) {
        out_args >> status;
    }

    return status;
}

nirio_status usrprio_rpc_client::_boost_error_to_nirio_status(const boost::system::error_code& err) {
    if (err) {
        switch (err.value()) {
            case boost::asio::error::connection_aborted:
            case boost::asio::error::connection_refused:
            case boost::asio::error::eof:
                return NiRio_Status_RpcSessionError;
            case boost::asio::error::timed_out:
            case boost::asio::error::operation_aborted:
                return NiRio_Status_RpcOperationError;
            default:
                return NiRio_Status_SoftwareFault;
        }
    } else {
        return NiRio_Status_Success;
    }
}

}}
