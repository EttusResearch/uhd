//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <cstddef>

namespace uhd { namespace transport {

//! Host transport adapter ID
// The adapter ID is a unique identifier for the physical connection. For example,
// if the UHD session is using two separate Ethernet cables, each plugged into
// its own Ethernet port, then there would be two adapter IDs available (0 and 1).
using adapter_id_t = size_t;
//! NULL/unassigned host transport adapter ID
static const adapter_id_t NULL_ADAPTER_ID = 0;

}} // namespace uhd::transport
