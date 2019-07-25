//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_CLAIM_HPP
#define INCLUDED_X300_CLAIM_HPP

#include <uhd/types/wb_iface.hpp>

namespace uhd { namespace usrp { namespace x300 {

// device claim functions
enum claim_status_t { UNCLAIMED, CLAIMED_BY_US, CLAIMED_BY_OTHER };

claim_status_t claim_status(uhd::wb_iface::sptr iface);
void claimer_loop(uhd::wb_iface::sptr iface);
void claim(uhd::wb_iface::sptr iface);
bool try_to_claim(uhd::wb_iface::sptr iface, long timeout = 2000);
void release(uhd::wb_iface::sptr iface);

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_X300_CLAIM_HPP */
