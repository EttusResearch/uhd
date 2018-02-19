//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_UDP_CONSTANTS_HPP
#define INCLUDED_UHD_TRANSPORT_UDP_CONSTANTS_HPP

// Constants related to UDP (over Ethernet)

static const size_t IP_PROTOCOL_MIN_MTU_SIZE        = 576;      //bytes
static const size_t IP_PROTOCOL_UDP_PLUS_IP_HEADER  = 28;      //bytes. Note that this is the minimum value!

#endif /* INCLUDED_UHD_TRANSPORT_UDP_CONSTANTS_HPP */
