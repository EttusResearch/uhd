//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <cstddef>

namespace uhd { namespace transport {

//! Host transport adapter ID
using adapter_id_t = size_t;
//! NULL/unassigned host transport adapter ID
static const adapter_id_t NULL_ADAPTER_ID = 0;

}} // namespace uhd::transport
