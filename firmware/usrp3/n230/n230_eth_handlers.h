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

#ifndef INCLUDED_N230_ETH_HANDLERS_H
#define INCLUDED_N230_ETH_HANDLERS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <lwip/ip_addr.h>
#include <wb_soft_reg.h>
#include "../../../host/lib/usrp/n230/n230_fw_host_iface.h"

/*!
 * Registrar for host firmware communications handler.
 */
void n230_register_udp_fw_comms_handler(n230_host_shared_mem_t* shared_mem_ptr);

/*!
 * Registrar for framer programmer handler.
 */
void n230_register_udp_prog_framer();

/*!
 * Registrar for host firmware communications handler.
 */
void n230_register_flash_comms_handler();

/*!
 * Handle SFP updates.
 */
void n230_update_link_act_state(soft_reg_t* led_reg);

#endif /* INCLUDED_N230_ETH_HANDLERS_H */
