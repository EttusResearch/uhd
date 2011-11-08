//
// Copyright 2010-2011 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TYPES_CLOCK_CONFIG_HPP
#define INCLUDED_UHD_TYPES_CLOCK_CONFIG_HPP

#include <uhd/config.hpp>

namespace uhd{

    /*!
     * The DEPRECATED Clock configuration settings:
     * The source for the 10MHz reference clock.
     * The source and polarity for the PPS clock.
     *
     * Deprecated in favor of set time/clock source calls.
     * Its still in this file for the sake of gr-uhd swig.
     *
     * Use the convenience functions external() and internal(),
     * unless you have a special purpose and cannot use them.
     */
    struct UHD_API clock_config_t{
        //------ simple usage --------//

        //! A convenience function to create an external clock configuration
        static clock_config_t external(void);

        //! A convenience function to create an internal clock configuration
        static clock_config_t internal(void);

        //------ advanced usage --------//
        enum ref_source_t {
            REF_AUTO = int('a'), //automatic (device specific)
            REF_INT  = int('i'), //internal reference
            REF_SMA  = int('s'), //external sma port
            REF_MIMO = int('m'), //reference from mimo cable
        } ref_source;
        enum pps_source_t {
            PPS_INT  = int('i'), //there is no internal
            PPS_SMA  = int('s'), //external sma port
            PPS_MIMO = int('m'), //time sync from mimo cable
        } pps_source;
        enum pps_polarity_t {
            PPS_NEG = int('n'), //negative edge
            PPS_POS = int('p')  //positive edge
        } pps_polarity;
        clock_config_t(void);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_CLOCK_CONFIG_HPP */
