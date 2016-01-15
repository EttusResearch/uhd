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

#include <cron.h>
#include <printf.h>
#include <wb_utils.h>
#include <wb_uart.h>
#include <wb_i2c.h>
#include <wb_pkt_iface64.h>
#include <u3_net_stack.h>
#include <print_addrs.h>
#include <trace.h>
#include "../../../host/lib/usrp/n230/n230_eeprom.h"
#include "n230_init.h"
#include "../../../host/lib/usrp/n230/n230_fw_defs.h"

static wb_pkt_iface64_config_t pkt_config;

static void putc(void *p, char c)
{
//If FW_TRACE_LEVEL is defined, then the trace level is set
//to a non-zero number. Turn on the debug UART to enable tracing
#ifdef UHD_FW_TRACE_LEVEL
    wb_uart_putc(WB_DBG_UART_BASE, c);
#endif
}

static uint32_t get_counter_val()
{
    return wb_peek32(SR_ADDR(WB_SBRB_BASE, RB_ZPU_COUNTER));
}

void n230_init(void)
{
    //TODO: We may need to remove the debug UART before we release.
    //Initialize the debug UART first.
    wb_uart_init(WB_DBG_UART_BASE, CPU_CLOCK_FREQ/DBG_UART_BAUD);
    init_printf(NULL, putc);

    //Now we can init the rest with prints
    UHD_FW_TRACE_FSTR(INFO, "[ZPU Init Begin -- CPU CLOCK is %d MHz]", (CPU_CLOCK_FREQ/1000000));

    //Initialize cron and the per millisecond cron job
    UHD_FW_TRACE(INFO, "Initializing cron...");
    cron_init(get_counter_val, CPU_CLOCK_FREQ);
    cron_job_init(PER_MILLISEC_CRON_JOBID, 1);
    cron_job_init(PER_SECOND_CRON_JOBID, 1000);

    //Initialize rate for I2C cores
    UHD_FW_TRACE(INFO, "Initializing I2C...");
    for (uint32_t i = 0; i < N230_NUM_ETH_PORTS; i++) {
        wb_i2c_init((i==1)?WB_ETH1_I2C_BASE:WB_ETH0_I2C_BASE, CPU_CLOCK_FREQ);
    }

    //Initialize eeprom
    read_n230_eeprom();

    UHD_FW_TRACE(INFO, "Initializing network stack...");
    init_network_stack();
}

void init_network_stack(void)
{
    //Hold Ethernet PHYs in reset
    wb_poke32(SR_ADDR(WB_SBRB_BASE, SR_ZPU_SW_RST), SR_ZPU_SW_RST_PHY);

    //Initialize ethernet packet interface
    pkt_config = wb_pkt_iface64_init(WB_PKT_RAM_BASE, WB_PKT_RAM_CTRL_OFFSET);
    u3_net_stack_init(&pkt_config);

    //Initialize MACs
    for (uint32_t i = 0; i < N230_NUM_ETH_PORTS; i++) {
        init_ethernet_mac(i);
    }

    //Pull Ethernet PHYs out of reset
    wb_poke32(SR_ADDR(WB_SBRB_BASE, SR_ZPU_SW_RST), SR_ZPU_SW_RST_NONE);
}

void init_ethernet_mac(uint32_t iface_num)
{
    UHD_FW_TRACE_FSTR(INFO, "Initializing eth%d...", iface_num);

    //Get interface info from the EEPROM (or defaults otherwise)
    const n230_eth_eeprom_map_t* eth_eeprom_map = get_n230_ethernet_info(iface_num);
    const eth_mac_addr_t *my_mac = (const eth_mac_addr_t *) &(eth_eeprom_map->mac_addr);
    const struct ip_addr *my_ip  = (const struct ip_addr *) &(eth_eeprom_map->ip_addr);
    const struct ip_addr *subnet = (const struct ip_addr *) &(eth_eeprom_map->subnet);

    //Init software fields related to ethernet
    u3_net_stack_init_eth(iface_num, my_mac, my_ip, subnet);

    uint32_t dispatcher_base =
        ((iface_num == 1) ? SR_ZPU_ETHINT1 : SR_ZPU_ETHINT0) + SR_ZPU_ETHINT_DISPATCHER_BASE;

    //Program dispatcher
    wb_poke32(SR_ADDR(WB_SBRB_BASE, dispatcher_base + 0),
          (my_mac->addr[5] << 0) | (my_mac->addr[4] << 8) | (my_mac->addr[3] << 16) | (my_mac->addr[2] << 24));
    wb_poke32(SR_ADDR(WB_SBRB_BASE, dispatcher_base + 1), (my_mac->addr[1] << 0) | (my_mac->addr[0] << 8));
    wb_poke32(SR_ADDR(WB_SBRB_BASE, dispatcher_base + 2), my_ip->addr);
    wb_poke32(SR_ADDR(WB_SBRB_BASE, dispatcher_base + 4), 0/*nofwd*/);
    wb_poke32(SR_ADDR(WB_SBRB_BASE, dispatcher_base + 5), (ICMP_IRQ << 8) | 0); //no fwd: type, code

    //DEBUG: Print initialized info
    UHD_FW_TRACE_FSTR(INFO, "-- MAC%u:     %s", iface_num, mac_addr_to_str(u3_net_stack_get_mac_addr(iface_num)));
    UHD_FW_TRACE_FSTR(INFO, "-- IP%u:      %s", iface_num, ip_addr_to_str(u3_net_stack_get_ip_addr(iface_num)));
    UHD_FW_TRACE_FSTR(INFO, "-- SUBNET%u:  %s", iface_num, ip_addr_to_str(u3_net_stack_get_subnet(iface_num)));
    UHD_FW_TRACE_FSTR(INFO, "-- BCAST%u:   %s", iface_num, ip_addr_to_str(u3_net_stack_get_bcast(iface_num)));
}


