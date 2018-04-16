//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIUSRPRIO_SESSION_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIUSRPRIO_SESSION_H

#include <stdint.h>
#include <uhd/transport/nirio/rpc/usrprio_rpc_client.hpp>
#include <uhd/transport/nirio/niriok_proxy.h>
#include <uhd/transport/nirio/nirio_resource_manager.h>
#include <uhd/transport/nirio/nifpga_lvbitx.h>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <string>

namespace uhd { namespace niusrprio {

class UHD_API niusrprio_session : private boost::noncopyable
{
public:
    typedef boost::shared_ptr<niusrprio_session> sptr;
    typedef uhd::usrprio_rpc::usrprio_device_info device_info;
    typedef uhd::usrprio_rpc::usrprio_device_info_vtr device_info_vtr;

    static nirio_status enumerate(
        const std::string& rpc_port_name,
        device_info_vtr& device_info_vtr);

    niusrprio_session(
        const std::string& resource_name,
        const std::string& port_name);
    virtual ~niusrprio_session();

    nirio_status open(
        nifpga_lvbitx::sptr lvbitx,
        bool force_download = false);

    void close(bool skip_reset = false);

    nirio_status reset();

    template<typename data_t>
    nirio_status create_tx_fifo(
        const char* fifo_name,
        boost::shared_ptr< nirio_fifo<data_t> >& fifo)
    {
        if (!_session_open) return NiRio_Status_ResourceNotInitialized;
        return _resource_manager.create_tx_fifo(fifo_name, fifo);
    }

    template<typename data_t>
    nirio_status create_tx_fifo(
        uint32_t fifo_instance,
        boost::shared_ptr< nirio_fifo<data_t> >& fifo)
    {
        if ((size_t)fifo_instance >= _lvbitx->get_output_fifo_count()) return NiRio_Status_InvalidParameter;
        return create_tx_fifo(_lvbitx->get_output_fifo_names()[fifo_instance], fifo);
    }

    template<typename data_t>
    nirio_status create_rx_fifo(
        const char* fifo_name,
        boost::shared_ptr< nirio_fifo<data_t> >& fifo)
    {
        if (!_session_open) return NiRio_Status_ResourceNotInitialized;
        return _resource_manager.create_rx_fifo(fifo_name, fifo);
    }

    template<typename data_t>
    nirio_status create_rx_fifo(
        uint32_t fifo_instance,
        boost::shared_ptr< nirio_fifo<data_t> >& fifo)
    {
        if ((size_t)fifo_instance >= _lvbitx->get_input_fifo_count()) return NiRio_Status_InvalidParameter;
        return create_rx_fifo(_lvbitx->get_input_fifo_names()[fifo_instance], fifo);
    }

    UHD_INLINE niriok_proxy::sptr get_kernel_proxy() {
        return _riok_proxy;
    }

    nirio_status download_bitstream_to_flash(const std::string& bitstream_path);

    //Static
    static niriok_proxy::sptr create_kernel_proxy(
        const std::string& resource_name,
        const std::string& rpc_port_name);

private:
    nirio_status _verify_signature();
    std::string _read_bitstream_checksum();
    nirio_status _write_bitstream_checksum(const std::string& checksum);
    nirio_status _ensure_fpga_ready();

    std::string                     _resource_name;
    nifpga_lvbitx::sptr             _lvbitx;
    std::string                     _interface_path;
    bool                            _session_open;
    niriok_proxy::sptr              _riok_proxy;
    nirio_resource_manager          _resource_manager;
    usrprio_rpc::usrprio_rpc_client _rpc_client;
    boost::recursive_mutex          _session_mutex;
};

}}

#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_NIUSRPRIO_SESSION_H */
