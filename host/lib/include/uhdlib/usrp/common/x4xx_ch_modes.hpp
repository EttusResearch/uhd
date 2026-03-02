//
// Copyright 2026 Ettus Research, an NI Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

namespace uhd { namespace usrp { namespace x400 {

/*!
 * Lists all possible channel mixer modes for the X4xx series. This enum must be in sync
 * with the MPM equivalent MixerMode in x4xx_rfdc_ctrl.py.
 */
enum class ch_mode : int { REAL = 0, I = 1, Q = 2, DISABLED = 3, IQ = 4, ALL = 5 };

}}} // namespace uhd::usrp::x400
