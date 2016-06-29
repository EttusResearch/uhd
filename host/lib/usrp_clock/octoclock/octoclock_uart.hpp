//
// Copyright 2014,2016 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the uart_ifaceied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_OCTOCLOCK_UART_HPP
#define INCLUDED_OCTOCLOCK_UART_HPP

#include <vector>

#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/serial.hpp>

/*!
 * The OctoClock doesn't take UART input per se but reads a specific
 * packet type and sends the string from there through its own serial
 * functions.
 */
namespace uhd{
class octoclock_uart_iface : public uhd::uart_iface{
public:
    octoclock_uart_iface(uhd::transport::udp_simple::sptr udp, uint32_t proto_ver);
    ~octoclock_uart_iface(void) {};

    void write_uart(const std::string &buf);
    std::string read_uart(double timeout);

private:
    uhd::transport::udp_simple::sptr _udp;

    boost::uint16_t _poolsize;
    gpsdo_cache_state_t _state;
    gpsdo_cache_state_t _device_state;
    std::vector<boost::uint8_t> _cache;
    std::string _rxbuff;
    boost::uint32_t _sequence;
	boost::uint32_t _proto_ver;
    boost::system_time _last_cache_update;

    void _update_cache();
    char _getchar();
};

uart_iface::sptr octoclock_make_uart_iface(uhd::transport::udp_simple::sptr udp, uint32_t proto_ver);

}

#endif /* INCLUDED_OCTOCLOCK_UART_HPP */
