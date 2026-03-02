//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>

namespace uhd::usrp::cal {

/*! Clear the front-end correction cache.
 *  This will ensure that any available data is reloaded.
 */
UHD_API void clear_fe_correction_cache();

} // namespace uhd::usrp::cal
