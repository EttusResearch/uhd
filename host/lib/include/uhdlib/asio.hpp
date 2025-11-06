/* -*- c++ -*- */
/*
 * Copyright 2025 Ettus Research / NI / Emerson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// This is an abstraction layer for asio to non-Boost asio. It makes
// asio look like regular asio to allow easier transition.

#pragma once

// If compiler warnings emanate from Boost, we let them through, so they don't
// trip up our preferred -Werror settings.
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    if __GNUC__ > 15 || (__GNUC__ == 15 && (__GNUC_MINOR__ > 0))
#        pragma GCC diagnostic warning "-Warray-bounds"
#    endif
#endif
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

// In regular, non-Boost ASIO these types are part of the library. In regular
// Boost, they're part of boost::system. We therefore patch the boost namespace
// to make them look the same.
namespace boost { namespace asio {
using error_code   = boost::system::error_code;
using system_error = boost::system::system_error;
}} // namespace boost::asio

namespace asio = boost::asio;
