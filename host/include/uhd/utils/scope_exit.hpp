//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <functional>
#include <memory>

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
        // When we have C++14, use make_unique instead (TODO)
        return uptr(new scope_exit(std::forward<exit_cb_t>(exit_cb)));
    }

    ~scope_exit()
    {
        _exit_cb();
    }

private:
    scope_exit(std::function<void(void)>&& exit_cb)
        : _exit_cb(std::forward<std::function<void(void)>>(exit_cb))
    {
        // nop
    }

    std::function<void(void)> _exit_cb;
};

}} // namespace uhd::utils
