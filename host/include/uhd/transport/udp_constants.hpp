//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

// Constants related to UDP (over Ethernet)

static const size_t IP_PROTOCOL_MIN_MTU_SIZE = 576; // bytes
static const size_t IP_PROTOCOL_UDP_PLUS_IP_HEADER =
    28; // bytes. Note that this is the minimum value!
