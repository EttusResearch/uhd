//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <cstddef>

/*************************************************************************
 * RPC timeout constants for MPMD
 *
 * Single authoritative definition for all MPM/MPMD RPC timeout values.
 * Included by:
 *   - mpmd_impl.hpp  (host C++ MPMD implementation layer)
 *   - mpm_client.cpp (generated gRPC C++ client, via mpm_client.cpp.mako)
 *   - mpmd_mb_controller.cpp
 *
 * Python mirrors are in host/python/uhd/utils/mpmtools.py — update those
 * comments when changing values here.
 ************************************************************************/

//! Time between reclaims (ms)
static constexpr size_t MPMD_RECLAIM_INTERVAL_MS = 1000;
//! Default timeout for long-running RPC calls: init(), set_device_id(), etc. (ms)
static constexpr size_t MPMD_DEFAULT_INIT_TIMEOUT = 120000;
//! Default timeout for the reset_timer_and_mgr() RPC call (ms)
static constexpr size_t MPMD_DEFAULT_REBOOT_TIMEOUT = 200000;
//! Default timeout for ordinary RPC calls (ms)
static constexpr size_t MPMD_DEFAULT_RPC_TIMEOUT = 2000;
//! Short timeout for calls that must return quickly; also used to probe link status (ms)
static constexpr size_t MPMD_SHORT_RPC_TIMEOUT = 2000;
//! Long timeout for RPC calls that may block on slow device operations,
//! e.g. set_time_source() / set_clock_source() which trigger PLL re-locking (ms)
static constexpr size_t MPMD_DEFAULT_LONG_TIMEOUT = 30000;
//! Extended timeout for RPC calls that probe network interfaces after FPGA reload (ms)
static constexpr size_t MPMD_LONG_RPC_TIMEOUT = 4000;
//! Claimer loop timeout for RPC calls (ms)
//! Python mirror: MPM_CLAIMER_RPC_TIMEOUT_S = 10 in mpmtools.py
static constexpr size_t MPMD_CLAIMER_RPC_TIMEOUT = 10000;
