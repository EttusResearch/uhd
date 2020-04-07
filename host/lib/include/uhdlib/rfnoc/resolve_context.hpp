//
//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

namespace uhd { namespace rfnoc {

/*! Describe situations out of which property propagation is called
 */
enum class resolve_context {
    //! Property propagation was called during an initialization process (e.g.,
    // the graph was committed)
    INIT,
    //! Property propagation was called because a property on a node was
    // updated
    NODE_PROP
};

}} // namespace uhd::rfnoc
