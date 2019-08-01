//
//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_RFNOC_RESOLVE_CONTEXT_HPP
#define INCLUDED_UHD_RFNOC_RESOLVE_CONTEXT_HPP

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

#endif /* INCLUDED_UHD_RFNOC_RESOLVE_CONTEXT_HPP */
