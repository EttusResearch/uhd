//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/rfnoc/rfnoc_common.hpp>

namespace uhd { namespace rfnoc {

//! Return a new, unique device ID.
//
// This function will never return the same device ID twice.
device_id_t allocate_device_id();

}} /* namespace uhd::rfnoc */
