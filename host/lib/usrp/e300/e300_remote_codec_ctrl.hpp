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

#ifndef INCLUDED_E300_REMOTE_CODEC_CTRL_HPP
#define INCLUDED_E300_REMOTE_CODEC_CTRL_HPP

#include "ad9361_ctrl.hpp"
#include <uhd/transport/zero_copy.hpp>

namespace uhd { namespace usrp { namespace e300 {

class e300_remote_codec_ctrl : public uhd::usrp::ad9361_ctrl
{
public:
    struct transaction_t {
        boost::uint32_t     action;
        boost::uint32_t     which;
        union {
            double          rate;
            double          gain;
            double          freq;
            boost::uint64_t bits;
        };

        //Actions
        static const boost::uint32_t ACTION_SET_GAIN            = 10;
        static const boost::uint32_t ACTION_SET_CLOCK_RATE      = 11;
        static const boost::uint32_t ACTION_SET_ACTIVE_CHANS    = 12;
        static const boost::uint32_t ACTION_TUNE                = 13;
        static const boost::uint32_t ACTION_SET_LOOPBACK        = 14;

        //Values for "which"
        static const boost::uint32_t CHAIN_NONE = 0;
        static const boost::uint32_t CHAIN_TX1  = 1;
        static const boost::uint32_t CHAIN_TX2  = 2;
        static const boost::uint32_t CHAIN_RX1  = 3;
        static const boost::uint32_t CHAIN_RX2  = 4;
    };

    static sptr make(uhd::transport::zero_copy_if::sptr xport);
};

}}};

#endif /* INCLUDED_E300_REMOTE_CODEC_CTRL_HPP */
