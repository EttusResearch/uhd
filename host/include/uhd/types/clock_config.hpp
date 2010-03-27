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

#ifndef INCLUDED_UHD_TYPES_CLOCK_CONFIG_HPP
#define INCLUDED_UHD_TYPES_CLOCK_CONFIG_HPP

#include <uhd/config.hpp>

namespace uhd{

    /*!
     * Clock configuration settings:
     * The source for the 10MHz reference clock.
     * The source and polarity for the PPS clock.
     */
    struct UHD_API clock_config_t{
        enum ref_source_t {
            REF_INT, //internal reference
            REF_SMA, //external sma port
            REF_MIMO //mimo cable (usrp2 only)
        } ref_source;
        enum pps_source_t {
            PPS_INT, //there is no internal
            PPS_SMA, //external sma port
            PPS_MIMO //mimo cable (usrp2 only)
        } pps_source;
        enum pps_polarity_t {
            PPS_NEG, //negative edge
            PPS_POS  //positive edge
        } pps_polarity;
        clock_config_t(void);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_CLOCK_CONFIG_HPP */
