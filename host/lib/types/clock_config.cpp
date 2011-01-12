//
// Copyright 2011 Ettus Research LLC
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

#include <uhd/types/clock_config.hpp>

using namespace uhd;

clock_config_t clock_config_t::external(void){
    clock_config_t clock_config;
    clock_config.ref_source = clock_config_t::REF_SMA;
    clock_config.pps_source = clock_config_t::PPS_SMA;
    clock_config.pps_polarity = clock_config_t::PPS_POS;
    return clock_config;
}

clock_config_t clock_config_t::internal(void){
    clock_config_t clock_config;
    clock_config.ref_source = clock_config_t::REF_INT;
    clock_config.pps_source = clock_config_t::PPS_SMA;
    clock_config.pps_polarity = clock_config_t::PPS_POS;
    return clock_config;
}

clock_config_t::clock_config_t(void):
    ref_source(REF_INT),
    pps_source(PPS_SMA),
    pps_polarity(PPS_POS)
{
    /* NOP */
}
