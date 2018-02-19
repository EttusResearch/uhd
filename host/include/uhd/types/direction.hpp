//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_DIRECTION_HPP
#define INCLUDED_UHD_TYPES_DIRECTION_HPP

namespace uhd {

    enum direction_t {
        //! Receive
        RX_DIRECTION,
        //! Transmit
        TX_DIRECTION,
        //! Duplex
        DX_DIRECTION
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_DIRECTION_HPP */
