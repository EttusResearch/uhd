//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MPMD_XPORT_ctrl_liberio_HPP
#define INCLUDED_MPMD_XPORT_ctrl_liberio_HPP

#include "mpmd_xport_ctrl_base.hpp"
#include <uhd/types/device_addr.hpp>
#include "../device3/device3_impl.hpp"
#include <uhd/transport/muxed_zero_copy_if.hpp>

namespace uhd { namespace mpmd { namespace xport {

/*! Liberio transport manager
 */
class mpmd_xport_ctrl_liberio : public mpmd_xport_ctrl_base
{
public:
    mpmd_xport_ctrl_liberio(
        const uhd::device_addr_t& mb_args
    );

    /*! Open DMA interface to kernel (and thus to FPGA DMA engine)
     */
    both_xports_t make_transport(
        mpmd_xport_mgr::xport_info_t& xport_info,
        const usrp::device3_impl::xport_type_t xport_type,
        const uhd::device_addr_t& xport_args
    );

    bool is_valid(
        const mpmd_xport_mgr::xport_info_t& xport_info
    ) const;

    size_t get_mtu(
        const uhd::direction_t dir
    ) const ;

private:
    /*! Create a muxed liberio transport for control packets */
    uhd::transport::muxed_zero_copy_if::sptr make_muxed_liberio_xport(
            const std::string &tx_dev,
            const std::string &rx_dev,
            const uhd::transport::zero_copy_xport_params &buff_args,
            const size_t max_muxed_ports
    );

    const uhd::device_addr_t _mb_args;
    const uhd::dict<std::string, std::string> _recv_args;
    const uhd::dict<std::string, std::string> _send_args;

    //! Control transport for one liberio connection
    uhd::transport::muxed_zero_copy_if::sptr _ctrl_dma_xport;
    //! Control transport for one liberio connection
    uhd::transport::muxed_zero_copy_if::sptr _async_msg_dma_xport;
};

}}} /* namespace uhd::mpmd::xport */

#endif /* INCLUDED_MPMD_XPORT_ctrl_liberio_HPP */
