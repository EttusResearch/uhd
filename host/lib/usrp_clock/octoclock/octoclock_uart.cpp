//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <string.h>
#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>

#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>

#include "common.h"
#include "octoclock_uart.hpp"

namespace asio = boost::asio;
using namespace uhd::transport;

#define NUM_WRAPS_EQUAL   (_state.num_wraps == _device_state.num_wraps)
#define POS_EQUAL         (_state.pos == _device_state.pos)
#define STATES_EQUAL      (NUM_WRAPS_EQUAL && POS_EQUAL)
#define MAX_CACHE_AGE     256   //seconds

namespace uhd{
    octoclock_uart_iface::octoclock_uart_iface(udp_simple::sptr udp, uint32_t proto_ver): uart_iface(){
        _udp = udp;
        _state.num_wraps = 0;
        _state.pos = 0;
        _device_state.num_wraps = 0;
        _device_state.pos = 0;
        _proto_ver = proto_ver;
        // To avoid replicating sequence numbers between sessions
        _sequence = uint32_t(std::rand());
        size_t len = 0;

        //Get pool size from device
        octoclock_packet_t pkt_out;
        pkt_out.sequence = uhd::htonx<uint32_t>(_sequence);
        pkt_out.len = 0;

        uint8_t octoclock_data[udp_simple::mtu];
        const octoclock_packet_t *pkt_in = reinterpret_cast<octoclock_packet_t*>(octoclock_data);

        UHD_OCTOCLOCK_SEND_AND_RECV(_udp, _proto_ver, SEND_POOLSIZE_CMD, pkt_out, len, octoclock_data);
        if(UHD_OCTOCLOCK_PACKET_MATCHES(SEND_POOLSIZE_ACK, pkt_out, pkt_in, len)){
            _poolsize = pkt_in->poolsize;
            _cache.resize(_poolsize);
        }
        else throw uhd::runtime_error("Failed to communicate with GPSDO.");
    }

    void octoclock_uart_iface::write_uart(const std::string &buf){
        size_t len = 0;

        octoclock_packet_t pkt_out;
        pkt_out.sequence = uhd::htonx<uint32_t>(++_sequence);
        pkt_out.len = buf.size();
        memcpy(pkt_out.data, buf.c_str(), buf.size());

        uint8_t octoclock_data[udp_simple::mtu];
        const octoclock_packet_t *pkt_in = reinterpret_cast<octoclock_packet_t*>(octoclock_data);

        UHD_OCTOCLOCK_SEND_AND_RECV(_udp, _proto_ver, HOST_SEND_TO_GPSDO_CMD, pkt_out, len, octoclock_data);
        if(not UHD_OCTOCLOCK_PACKET_MATCHES(HOST_SEND_TO_GPSDO_ACK, pkt_out, pkt_in, len)){
            throw uhd::runtime_error("Failed to send commands to GPSDO.");
        }
    }

    std::string octoclock_uart_iface::read_uart(double timeout){
        std::string result;
        boost::system_time exit_time = boost::get_system_time() + boost::posix_time::milliseconds(long(timeout*1e3));

        while(true)
        {
            _update_cache();

            for(char ch = _getchar(); ch != 0; ch = _getchar()){
                _rxbuff += ch;

                //If newline found, return string
                if(ch == '\n'){
                    result.swap(_rxbuff);
                    return result;
                }
            }
            if (boost::get_system_time() > exit_time)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return result;
    }

    void octoclock_uart_iface::_update_cache(){
        octoclock_packet_t pkt_out;
        pkt_out.len = 0;
        size_t len = 0;

        uint8_t octoclock_data[udp_simple::mtu];
        const octoclock_packet_t *pkt_in = reinterpret_cast<octoclock_packet_t*>(octoclock_data);

        if(STATES_EQUAL){
            boost::system_time time = boost::get_system_time();
            boost::posix_time::time_duration age = time - _last_cache_update;
            bool cache_expired = (age > boost::posix_time::seconds(MAX_CACHE_AGE));

            pkt_out.sequence = uhd::htonx<uint32_t>(++_sequence);
            UHD_OCTOCLOCK_SEND_AND_RECV(_udp, _proto_ver, SEND_GPSDO_CACHE_CMD, pkt_out, len, octoclock_data);
            if(UHD_OCTOCLOCK_PACKET_MATCHES(SEND_GPSDO_CACHE_ACK, pkt_out, pkt_in, len)){
                memcpy(&_cache[0], pkt_in->data, _poolsize);
                _device_state = pkt_in->state;
                _last_cache_update = time;
            }

            uint8_t delta_wraps = (_device_state.num_wraps - _state.num_wraps);
            if(cache_expired or delta_wraps > 1 or
               ((delta_wraps == 1) and (_device_state.pos > _state.pos))){

                _state.pos = _device_state.pos;
                _state.num_wraps = (_device_state.num_wraps-1);
                _rxbuff.clear();

                while((_cache[_state.pos] != '\n')){
                    _state.pos = (_state.pos+1) % _poolsize;
                    //We may have wrapped around locally
                    if(_state.pos == 0) _state.num_wraps++;
                    if(STATES_EQUAL) break;
                }
                if (_cache[_state.pos] == '\n'){
                    _state.pos = (_state.pos+1) % _poolsize;
                    //We may have wrapped around locally
                    if(_state.pos == 0) _state.num_wraps++;
                }
            }
        }
    }

    char octoclock_uart_iface::_getchar(){
        if(STATES_EQUAL){
            return 0;
        }

        char ch = _cache[_state.pos];
        _state.pos = ((_state.pos+1) % _poolsize);
        //We may have wrapped around locally
        if(_state.pos == 0) _state.num_wraps++;

        return ch;
    }

    uart_iface::sptr octoclock_make_uart_iface(udp_simple::sptr udp, uint32_t proto_ver){
        return uart_iface::sptr(new octoclock_uart_iface(udp, proto_ver));
    }
}
