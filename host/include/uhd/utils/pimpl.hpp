//
// Copyright 2010 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <memory>

/*! Make a declaration for a pimpl in a header file.
 *
 * \deprecated This macro is deprecated and will be removed in a future release.
 *             Instead, directly declare the pimpl struct in the header file.
 */
#define UHD_PIMPL_DECL(_name) \
    struct _name;             \
    std::shared_ptr<_name>

/*! Make an instance of a pimpl in a source file.
 *
 * \deprecated This macro is deprecated and will be removed in a future release.
 *             Instead, directly instantiate the pimpl struct in the source file.
 */
#define UHD_PIMPL_MAKE(_name, _args) std::make_shared<_name> _args
