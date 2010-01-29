//
// Copyright 2010 Ettus Research LLC
//

#include <stdint.h>

#ifndef INCLUDED_USRP_UHD_TIME_SPEC_HPP
#define INCLUDED_USRP_UHD_TIME_SPEC_HPP

namespace usrp_uhd{

    /*!
     * A time_spec_t holds a seconds and ticks time value.
     * The temporal width of a tick depends on the device's clock rate.
     * The time_spec_t can be used when setting the time on devices
     * and for controlling the start of streaming for applicable dsps.
     */
    struct time_spec_t{
        uint32_t secs;
        uint32_t ticks;

        /*!
         * Create a time_spec_t that holds a wildcard time.
         * This will have implementation-specific meaning.
         */
        time_spec_t(void){
            secs = ~0;
            ticks = ~0;
        }

        /*!
         * Create a time_spec_t from seconds and ticks.
         * \param new_secs the new seconds
         * \param new_ticks the new ticks (default = 0)
         */
        time_spec_t(uint32_t new_secs, uint32_t new_ticks = 0){
            secs = new_secs;
            ticks = new_ticks;
        }
    };

} //namespace usrp_uhd

#endif /* INCLUDED_USRP_UHD_TIME_SPEC_HPP */
