//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0+
//

#include <uhd/types/time_spec.hpp>

namespace uhd {

    /*!
     * Get the system time in time_spec_t format.
     * Uses the highest precision clock available.
     * \return the system time as a time_spec_t
     */
    time_spec_t get_system_time(void);

}; /* namespace uhd */
