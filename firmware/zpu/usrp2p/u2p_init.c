/*
 * Copyright 2011 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "u2p_init.h"
#include "i2c.h"
#include "ethernet.h"

void u2p_init(void){
    //we do this to see if we should set a default ip addr or not
    bool safe_fw = find_safe_booted_flag();
    set_safe_booted_flag(0);
    if (safe_fw) {
        eth_addrs_set_default();
    }
}
