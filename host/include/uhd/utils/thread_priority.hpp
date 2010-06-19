//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_UTILS_THREAD_PRIORITY_HPP
#define INCLUDED_UHD_UTILS_THREAD_PRIORITY_HPP

#include <uhd/config.hpp>

namespace uhd{

    static const float default_thread_priority = float(0.5);

    /*!
     * Set the scheduling priority on the current thread.
     *
     * A new thread or calling process should make this call
     * with the defailts this to enable realtime scheduling.
     *
     * A priority of zero corresponds to normal priority.
     * Positive priority values are higher than normal.
     * Negative priority values are lower than normal.
     *
     * \param priority a value between -1 and 1
     * \param realtime true to use realtime mode
     * \throw exception on set priority failure
     */
    UHD_API void set_thread_priority(
        float priority = default_thread_priority,
        bool realtime = true
    );

    /*!
     * Set the scheduling priority on the current thread.
     * Same as set_thread_priority but does not throw on failure.
     * \return true on success, false on failure
     */
    UHD_API bool set_thread_priority_safe(
        float priority = default_thread_priority,
        bool realtime = true
    );

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_THREAD_PRIORITY_HPP */
