//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


//@TODO: Move the register defs required by the class to a common location
#include "../../usrp/x300/x300_regs.hpp"
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/transport/nirio/nirio_fifo.h>
#include <uhd/transport/nirio/status.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <chrono>
#include <thread>
#include <stdio.h>

namespace {
    constexpr uint32_t FPGA_READY_TIMEOUT_IN_MS      = 1000;
}

namespace uhd { namespace niusrprio {

niusrprio_session::niusrprio_session(const std::string& resource_name, const std::string& rpc_port_name) :
    _resource_name(resource_name),
    _session_open(false),
    _resource_manager(),
    _rpc_client("localhost", rpc_port_name)
{
   _riok_proxy = create_kernel_proxy(resource_name,rpc_port_name);
   _resource_manager.set_proxy(_riok_proxy);
}

niusrprio_session::~niusrprio_session()
{
    close();
}

nirio_status niusrprio_session::enumerate(const std::string& rpc_port_name, device_info_vtr& device_info_vtr)
{
    usrprio_rpc::usrprio_rpc_client temp_rpc_client("localhost", rpc_port_name);
    nirio_status status = temp_rpc_client.get_ctor_status();
    nirio_status_chain(temp_rpc_client.niusrprio_enumerate(device_info_vtr), status);
    return status;
}

nirio_status niusrprio_session::open(
    nifpga_lvbitx::sptr lvbitx,
    bool force_download)
{
    boost::unique_lock<boost::recursive_mutex> lock(_session_mutex);

    _lvbitx = lvbitx;

    nirio_status status = NiRio_Status_Success;
    std::string bitfile_path(_lvbitx->get_bitfile_path());
    std::string signature(_lvbitx->get_signature());

    //Make sure that the RPC client connected to the server properly
    nirio_status_chain(_rpc_client.get_ctor_status(), status);
    //Get a handle to the kernel driver
    nirio_status_chain(_rpc_client.niusrprio_get_interface_path(_resource_name, _interface_path), status);
    nirio_status_chain(_riok_proxy->open(_interface_path), status);

    if (nirio_status_not_fatal(status)) {
        //Bitfile build for a particular LVFPGA interface will have the same signature
        //because the API of the bitfile does not change. Two files with the same signature
        //can however have different bitstreams because of non-LVFPGA code differences.
        //That is why we need another identifier to qualify the signature. The BIN
        //checksum is a good candidate.
        std::string lvbitx_checksum(_lvbitx->get_bitstream_checksum());
        uint16_t download_fpga = (force_download || (_read_bitstream_checksum() != lvbitx_checksum)) ? 1 : 0;

        nirio_status_chain(_ensure_fpga_ready(), status);

        nirio_status_chain(_rpc_client.niusrprio_open_session(
            _resource_name, bitfile_path, signature, download_fpga), status);
        _session_open = nirio_status_not_fatal(status);

        if (nirio_status_not_fatal(status)) {
            nirio_register_info_vtr reg_vtr;
            nirio_fifo_info_vtr fifo_vtr;
            _lvbitx->init_register_info(reg_vtr);
            _lvbitx->init_fifo_info(fifo_vtr);
            _resource_manager.initialize(reg_vtr, fifo_vtr);

            nirio_status_chain(_verify_signature(), status);
            nirio_status_chain(_write_bitstream_checksum(lvbitx_checksum), status);
        }
    }

    return status;
}

void niusrprio_session::close(bool skip_reset)
{
    boost::unique_lock<boost::recursive_mutex> lock(_session_mutex);

    if (_session_open) {
        nirio_status status = NiRio_Status_Success;
        if (!skip_reset) reset();
        nirio_status_chain(_rpc_client.niusrprio_close_session(_resource_name), status);
        _session_open = false;
    }
}

nirio_status niusrprio_session::reset()
{
    boost::unique_lock<boost::recursive_mutex> lock(_session_mutex);
    return _rpc_client.niusrprio_reset_device(_resource_name);
}

nirio_status niusrprio_session::download_bitstream_to_flash(const std::string& bitstream_path)
{
    boost::unique_lock<boost::recursive_mutex> lock(_session_mutex);
    return _rpc_client.niusrprio_download_fpga_to_flash(_resource_name, bitstream_path);
}

niriok_proxy::sptr niusrprio_session::create_kernel_proxy(
    const std::string& resource_name,
    const std::string& rpc_port_name)
{
    usrprio_rpc::usrprio_rpc_client temp_rpc_client("localhost", rpc_port_name);
    nirio_status status = temp_rpc_client.get_ctor_status();

    std::string interface_path;
    nirio_status_chain(temp_rpc_client.niusrprio_get_interface_path(resource_name, interface_path), status);

    niriok_proxy::sptr proxy = niriok_proxy::make_and_open(interface_path);
	
    return proxy;
}

nirio_status niusrprio_session::_verify_signature()
{
    //Validate the signature using the kernel proxy
    nirio_status status = NiRio_Status_Success;
    uint32_t sig_offset = 0;
    nirio_status_chain(_riok_proxy->get_attribute(RIO_FPGA_DEFAULT_SIGNATURE_OFFSET, sig_offset), status);
    niriok_scoped_addr_space(_riok_proxy, FPGA, status);
    std::string signature;
    for (uint32_t i = 0; i < 8; i++) {
        uint32_t quarter_sig;
        nirio_status_chain(_riok_proxy->peek(sig_offset, quarter_sig), status);
        signature += boost::str(boost::format("%08x") % quarter_sig);
    }

    std::string expected_signature(_lvbitx->get_signature());
    boost::to_upper(signature);
    boost::to_upper(expected_signature);
    if (signature.find(expected_signature) == std::string::npos) {
        nirio_status_chain(NiRio_Status_SignatureMismatch, status);
    }

    return status;
}

std::string niusrprio_session::_read_bitstream_checksum()
{
    nirio_status status = NiRio_Status_Success;
    niriok_scoped_addr_space(_riok_proxy, BUS_INTERFACE, status);
    std::string usr_signature;
    for (uint32_t i = 0; i < FPGA_USR_SIG_REG_SIZE; i+=4) {
        uint32_t quarter_sig;
        nirio_status_chain(_riok_proxy->peek(FPGA_USR_SIG_REG_BASE + i, quarter_sig), status);
        usr_signature += boost::str(boost::format("%08x") % quarter_sig);
    }
    boost::to_upper(usr_signature);

    return usr_signature;
}

nirio_status niusrprio_session::_write_bitstream_checksum(const std::string& checksum)
{
    nirio_status status = NiRio_Status_Success;
    niriok_scoped_addr_space(_riok_proxy, BUS_INTERFACE, status);
    for (uint32_t i = 0; i < FPGA_USR_SIG_REG_SIZE; i+=4) {
        uint32_t quarter_sig;
        try {
            std::stringstream ss;
            ss << std::hex << checksum.substr(i*2,8);
            ss >> quarter_sig;
        } catch (std::exception&) {
            quarter_sig = 0;
        }
        nirio_status_chain(_riok_proxy->poke(FPGA_USR_SIG_REG_BASE + i, quarter_sig), status);
    }
    return status;
}

nirio_status niusrprio_session::_ensure_fpga_ready()
{
    nirio_status status = NiRio_Status_Success;
    niriok_scoped_addr_space(_riok_proxy, BUS_INTERFACE, status);

    //Verify that the Ettus FPGA loaded in the device. This may not be true if the
    //user is switching to UHD after using LabVIEW FPGA. In that case skip this check.
    uint32_t pcie_fpga_signature = 0;
    nirio_status_chain(_riok_proxy->peek(FPGA_PCIE_SIG_REG, pcie_fpga_signature), status);
    //@TODO: Remove X300 specific constants for future products
    if (pcie_fpga_signature != FPGA_X3xx_SIG_VALUE) {
        return status;
    }

    uint32_t reg_data = 0xffffffff;
    nirio_status_chain(_riok_proxy->peek(FPGA_STATUS_REG, reg_data), status);
    if (nirio_status_not_fatal(status) && (reg_data & FPGA_STATUS_DMA_ACTIVE_MASK))
    {
        //In case this session was re-initialized *immediately* after the previous
        //there is a small chance that the server is still finishing up cleaning up
        //the DMA FIFOs. We currently don't have any feedback from the driver regarding
        //this state so just wait.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(FPGA_READY_TIMEOUT_IN_MS));

        //Disable all FIFOs in the FPGA
        for (size_t i = 0; i < _lvbitx->get_input_fifo_count(); i++) {
            _riok_proxy->poke(PCIE_RX_DMA_REG(DMA_CTRL_STATUS_REG, i), DMA_CTRL_DISABLED);
        }
        for (size_t i = 0; i < _lvbitx->get_output_fifo_count(); i++) {
            _riok_proxy->poke(PCIE_TX_DMA_REG(DMA_CTRL_STATUS_REG, i), DMA_CTRL_DISABLED);
        }

        //Disable all FIFOs in the kernel driver
        _riok_proxy->stop_all_fifos();

        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration elapsed;
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); //Avoid flooding the bus
            elapsed = boost::posix_time::microsec_clock::local_time() - start_time;
            nirio_status_chain(_riok_proxy->peek(FPGA_STATUS_REG, reg_data), status);
        } while (
            nirio_status_not_fatal(status) &&
            (reg_data & FPGA_STATUS_DMA_ACTIVE_MASK) &&
            elapsed.total_milliseconds() < FPGA_READY_TIMEOUT_IN_MS);

        nirio_status_chain(_riok_proxy->peek(FPGA_STATUS_REG, reg_data), status);
        if (nirio_status_not_fatal(status) && (reg_data & FPGA_STATUS_DMA_ACTIVE_MASK)) {
            return NiRio_Status_FifoReserved;
        }
    }

    return status;
}

}}
