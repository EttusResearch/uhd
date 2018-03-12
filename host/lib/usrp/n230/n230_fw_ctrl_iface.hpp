//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_N230_UDP_FW_CTRL_IFACE_HPP
#define INCLUDED_LIBUHD_USRP_N230_UDP_FW_CTRL_IFACE_HPP

#include <uhd/types/wb_iface.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>

namespace uhd { namespace usrp { namespace n230 {

class n230_fw_ctrl_iface : public uhd::wb_iface
{
public:
    n230_fw_ctrl_iface(
        uhd::transport::udp_simple::sptr udp_xport,
        const uint16_t product_id,
        const bool verbose);
    virtual ~n230_fw_ctrl_iface();

    // -- uhd::wb_iface --
    void poke32(const wb_addr_type addr, const uint32_t data);
    uint32_t peek32(const wb_addr_type addr);
    void flush();

    static uhd::wb_iface::sptr make(
        uhd::transport::udp_simple::sptr udp_xport,
        const uint16_t product_id,
        const bool verbose = true);
    // -- uhd::wb_iface --

    static std::vector<std::string> discover_devices(
        const std::string& addr_hint, const std::string& port,
        uint16_t product_id);

    static uint32_t get_iface_id(
        const std::string& addr, const std::string& port,
        uint16_t product_id);

private:
    void _poke32(const wb_addr_type addr, const uint32_t data);
    uint32_t _peek32(const wb_addr_type addr);
    void _flush(void);

    const uint16_t               _product_id;
    const bool                          _verbose;
    uhd::transport::udp_simple::sptr    _udp_xport;
    uint32_t                     _seq_num;
    boost::mutex                        _mutex;

    static const size_t NUM_RETRIES = 3;
};

}}} //namespace

#endif // INCLUDED_LIBUHD_USRP_N230_UDP_FW_CTRL_IFACE_HPP
