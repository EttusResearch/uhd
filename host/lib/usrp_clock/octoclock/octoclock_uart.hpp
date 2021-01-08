//
// Copyright 2014,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_OCTOCLOCK_UART_HPP
#define INCLUDED_OCTOCLOCK_UART_HPP

#include "common.h"
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/serial.hpp>
#include <boost/thread.hpp>
#include <string>
#include <vector>

/*!
 * The OctoClock doesn't take UART input per se but reads a specific
 * packet type and sends the string from there through its own serial
 * functions.
 */
namespace uhd {
class octoclock_uart_iface : public uhd::uart_iface
{
public:
    octoclock_uart_iface(uhd::transport::udp_simple::sptr udp, uint32_t proto_ver);
    ~octoclock_uart_iface(void) override{};

    void write_uart(const std::string& buf) override;
    std::string read_uart(double timeout) override;

private:
    uhd::transport::udp_simple::sptr _udp;

    uint16_t _poolsize;
    gpsdo_cache_state_t _state;
    gpsdo_cache_state_t _device_state;
    std::vector<uint8_t> _cache;
    std::string _rxbuff;
    uint32_t _sequence;
    uint32_t _proto_ver;
    boost::system_time _last_cache_update;

    void _update_cache();
    char _getchar();
};

uart_iface::sptr octoclock_make_uart_iface(
    uhd::transport::udp_simple::sptr udp, uint32_t proto_ver);

} // namespace uhd

#endif /* INCLUDED_OCTOCLOCK_UART_HPP */
