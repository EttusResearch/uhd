//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <mpm_client.hpp>

namespace uhd { namespace usrp {
// Type aliases in usrp namespace for backward compatibility
using mpmd_rpc_iface = uhd::rpc_client;
using dio_rpc_iface  = uhd::rpc_client;
}} // namespace uhd::usrp
