//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_claim.hpp"
#include "x300_fw_common.h"
#include <uhd/utils/platform.hpp>
#include <chrono>
#include <thread>

using namespace uhd;
using namespace uhd::usrp::x300;

/***********************************************************************
 * claimer logic
 **********************************************************************/

void uhd::usrp::x300::claimer_loop(wb_iface::sptr iface)
{
    claim(iface);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

claim_status_t uhd::usrp::x300::claim_status(wb_iface::sptr iface)
{
    claim_status_t claim_status = CLAIMED_BY_OTHER; // Default to most restrictive
    auto timeout_time = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < timeout_time) {
        // If timed out, then device is definitely unclaimed
        if (iface->peek32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_STATUS)) == 0) {
            claim_status = UNCLAIMED;
            break;
        }

        // otherwise check claim src to determine if another thread with the same src has
        // claimed the device
        uint32_t hash = iface->peek32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_SRC));
        if (hash == 0) {
            // A non-zero claim status and an empty hash means the claim might
            // be in the process of being released.  This is possible because
            // older firmware takes a long time to update the status.  Wait and
            // check status again.
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        claim_status = (hash == get_process_hash() ? CLAIMED_BY_US : CLAIMED_BY_OTHER);
        break;
    }
    return claim_status;
}

void uhd::usrp::x300::claim(wb_iface::sptr iface)
{
    iface->poke32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_TIME), uint32_t(time(NULL)));
    iface->poke32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_SRC), get_process_hash());
}

bool uhd::usrp::x300::try_to_claim(wb_iface::sptr iface, long timeout_ms)
{
    const auto timeout_time =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (1) {
        claim_status_t status = claim_status(iface);
        if (status == UNCLAIMED) {
            claim(iface);
            // It takes the claimer 10ms to update status, so wait 20ms before verifying
            // claim
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }
        if (status == CLAIMED_BY_US) {
            break;
        }
        if (std::chrono::steady_clock::now() > timeout_time) {
            // Another process owns the device - give up
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

void uhd::usrp::x300::release(wb_iface::sptr iface)
{
    iface->poke32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_TIME), 0);
    iface->poke32(X300_FW_SHMEM_ADDR(X300_FW_SHMEM_CLAIM_SRC), 0);
}
