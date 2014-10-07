//
// Copyright 2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "e300_remote_codec_ctrl.hpp"

#include <boost/cstdint.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>
#include <cstring>
#include <iostream>

namespace uhd { namespace usrp { namespace e300 {

class e300_remote_codec_ctrl_impl : public e300_remote_codec_ctrl
{
public:
    e300_remote_codec_ctrl_impl(uhd::transport::zero_copy_if::sptr xport) : _xport(xport)
    {
    }

    virtual ~e300_remote_codec_ctrl_impl(void)
    {
    }

    double set_gain(const std::string &which, const double value)
    {
        _clear();
        _args.action = uhd::htonx<boost::uint32_t>(transaction_t::ACTION_SET_GAIN);
        if (which == "TX1")      _args.which = uhd::htonx<boost::uint32_t>(transaction_t::CHAIN_TX1);
        else if (which == "TX2") _args.which = uhd::htonx<boost::uint32_t>(transaction_t::CHAIN_TX2);
        else if (which == "RX1") _args.which = uhd::htonx<boost::uint32_t>(transaction_t::CHAIN_RX1);
        else if (which == "RX2") _args.which = uhd::htonx<boost::uint32_t>(transaction_t::CHAIN_RX2);
        else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");
        _args.gain = value;

        _transact();
        return _retval.gain;
    }

    double set_clock_rate(const double rate)
    {
        _clear();
        _args.action = uhd::htonx<boost::uint32_t>(
            transaction_t::ACTION_SET_CLOCK_RATE);
        _args.which = uhd::htonx<boost::uint32_t>(
            transaction_t::CHAIN_NONE);  /*Unused*/
        _args.rate = rate;

        _transact();
        return _retval.gain;
    }

    void set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2)
    {
        _clear();
        _args.action = uhd::htonx<boost::uint32_t>(
            transaction_t::ACTION_SET_ACTIVE_CHANS);
        /*Unused*/
        _args.which = uhd::htonx<boost::uint32_t>(
            transaction_t::CHAIN_NONE);
        _args.bits = uhd::htonx<boost::uint32_t>(
                     (tx1 ? (1<<0) : 0) |
                     (tx2 ? (1<<1) : 0) |
                     (rx1 ? (1<<2) : 0) |
                     (rx2 ? (1<<3) : 0));

        _transact();
    }

    double tune(const std::string &which, const double value)
    {
        _clear();
        _args.action = uhd::htonx<boost::uint32_t>(transaction_t::ACTION_TUNE);
        if (which == "TX1")      _args.which = uhd::htonx<boost::uint32_t>(transaction_t::CHAIN_TX1);
        else if (which == "TX2") _args.which = uhd::htonx<boost::uint32_t>(transaction_t::CHAIN_TX2);
        else if (which == "RX1") _args.which = uhd::htonx<boost::uint32_t>(transaction_t::CHAIN_RX1);
        else if (which == "RX2") _args.which = uhd::htonx<boost::uint32_t>(transaction_t::CHAIN_RX2);
        else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");
        _args.freq = value;

        _transact();
        return _retval.freq;
    }

    void data_port_loopback(const bool on)
    {
        _clear();
        _args.action = uhd::htonx<boost::uint32_t>(transaction_t::ACTION_SET_LOOPBACK);
        _args.which  = uhd::htonx<boost::uint32_t>(transaction_t::CHAIN_NONE);  /*Unused*/
        _args.bits = uhd::htonx<boost::uint32_t>(on ? 1 : 0);

        _transact();
    }

private:
    void _transact() {
        {
            uhd::transport::managed_send_buffer::sptr buff = _xport->get_send_buff(10.0);
            if (not buff or buff->size() < sizeof(_args))
                throw std::runtime_error("e300_remote_codec_ctrl_impl send timeout");
            std::memcpy(buff->cast<void *>(), &_args, sizeof(_args));
            buff->commit(sizeof(_args));
        }
        {
            uhd::transport::managed_recv_buffer::sptr buff = _xport->get_recv_buff(10.0);
            if (not buff or buff->size() < sizeof(_retval))
                throw std::runtime_error("e300_remote_codec_ctrl_impl recv timeout");
            std::memcpy(&_retval, buff->cast<const void *>(), sizeof(_retval));
        }

        if (_args.action != _retval.action)
            throw std::runtime_error("e300_remote_codec_ctrl_impl trancation failed.");
    }

    void _clear() {
        _args.action = 0;
        _args.which = 0;
        _args.bits = 0;
        _retval.action = 0;
        _retval.which = 0;
        _retval.bits = 0;
    }

    uhd::transport::zero_copy_if::sptr _xport;
    transaction_t                      _args;
    transaction_t                      _retval;
};

ad9361_ctrl::sptr e300_remote_codec_ctrl::make(uhd::transport::zero_copy_if::sptr xport)
{
    return sptr(new e300_remote_codec_ctrl_impl(xport));
}

}}};
