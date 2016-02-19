//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_N230_INIT_H
#define INCLUDED_N230_INIT_H

#include <stdint.h>
#include <stdbool.h>

void n230_init(void);
void init_network_stack(void);
void init_ethernet_mac(uint32_t iface_num);

#endif /* INCLUDED_B250_INIT_H */
