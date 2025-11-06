//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace uhd { namespace utils {

/*! A class that will execute a function on its destruction
 *
 * Similar to Boost.ScopeExit. A useful tool for RAII-style operations.
 *
 * Note: The creation of the object can be costly if converting the exit
 * callback to exit_cb_t incurs copying overhead. Keep this in mind when using
 * this object in a high-performance path.
 */
class scope_exit
{
public:
    using uptr      = std::unique_ptr<scope_exit>;
    using exit_cb_t = std::function<void(void)>;

    // \param exit_b The function object ("exit callback") that gets executed
    //               in the destructor
    static uptr make(exit_cb_t&& exit_cb)
    {
        // Note: We can't use make_unique() here because it requires the
        // constructor to be public.
        return uptr(new scope_exit(std::forward<exit_cb_t>(exit_cb)));
    }

    ~scope_exit()
    {
        _exit_cb();
    }

private:
    scope_exit(exit_cb_t&& exit_cb) : _exit_cb(std::forward<exit_cb_t>(exit_cb))
    {
        // nop
    }

    exit_cb_t _exit_cb;
};

}} // namespace uhd::utils
