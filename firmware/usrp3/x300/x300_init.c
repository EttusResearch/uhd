#include "x300_init.h"
#include "ethernet.h"
#include "cron.h"
#include <wb_utils.h>
#include <wb_uart.h>
#include <wb_i2c.h>
#include <stdint.h>
#include <stdbool.h>
#include <trace.h>
#include <wb_pkt_iface64.h>
#include <u3_net_stack.h>
#include <link_state_route_proto.h>
#include <udp_uart.h>
#include "x300_fw_common.h"
#include <print_addrs.h>

static wb_pkt_iface64_config_t pkt_config;

static x300_eeprom_map_t default_map = {
    .mac_addr0 = X300_DEFAULT_MAC_ADDR_0,
    .mac_addr1 = X300_DEFAULT_MAC_ADDR_1,
    .gateway = X300_DEFAULT_GATEWAY,
    .subnet = {
        X300_DEFAULT_NETMASK_ETH0_1G,
        X300_DEFAULT_NETMASK_ETH1_1G,
        X300_DEFAULT_NETMASK_ETH0_10G,
        X300_DEFAULT_NETMASK_ETH1_10G
    },
    .ip_addr = {
        X300_DEFAULT_IP_ETH0_1G,
        X300_DEFAULT_IP_ETH1_1G,
        X300_DEFAULT_IP_ETH0_10G,
        X300_DEFAULT_IP_ETH1_10G
    },
};

const void *pick_inited_field(const void *eeprom, const void *def, const size_t len)
{
    bool all_ones = true;
    bool all_zeros = true;
    for (size_t i = 0; i < len; i++)
    {
        const uint8_t b = ((const uint8_t *)eeprom)[i];
        if (b != 0x00) all_zeros = false;
        if (b != 0xff) all_ones = false;
    }
    if (all_zeros) return def;
    if (all_ones) return def;
    return eeprom;
}

static void init_network(x300_eeprom_map_t *eeprom_map)
{
    pkt_config = wb_pkt_iface64_init(PKT_RAM0_BASE, 0x1ffc);
    u3_net_stack_init(&pkt_config);

    link_state_route_proto_init();

    //read everything from eeprom
    static const uint8_t eeprom_cmd[2] = {0, 0}; //the command is 16 bits of address offset
    wb_i2c_write(I2C1_BASE, MBOARD_EEPROM_ADDR, eeprom_cmd, 2);
    wb_i2c_read(I2C1_BASE, MBOARD_EEPROM_ADDR, (uint8_t *)(eeprom_map), sizeof(x300_eeprom_map_t));

    //determine interface number
    const size_t eth0no = wb_peek32(SR_ADDR(RB0_BASE, RB_SFP0_TYPE))? 2 : 0;
    const size_t eth1no = wb_peek32(SR_ADDR(RB0_BASE, RB_SFP1_TYPE))? 3 : 1;

    //pick the address from eeprom or default
    const eth_mac_addr_t *my_mac0 = (const eth_mac_addr_t *)pick_inited_field(&eeprom_map->mac_addr0, &default_map.mac_addr0, 6);
    const eth_mac_addr_t *my_mac1 = (const eth_mac_addr_t *)pick_inited_field(&eeprom_map->mac_addr1, &default_map.mac_addr1, 6);
    const struct ip_addr *my_ip0 = (const struct ip_addr *)pick_inited_field(&eeprom_map->ip_addr[eth0no], &default_map.ip_addr[eth0no], 4);
    const struct ip_addr *subnet0 = (const struct ip_addr *)pick_inited_field(&eeprom_map->subnet[eth0no], &default_map.subnet[eth0no], 4);
    const struct ip_addr *my_ip1 = (const struct ip_addr *)pick_inited_field(&eeprom_map->ip_addr[eth1no], &default_map.ip_addr[eth1no], 4);
    const struct ip_addr *subnet1 = (const struct ip_addr *)pick_inited_field(&eeprom_map->subnet[eth1no], &default_map.subnet[eth1no], 4);

    //init eth0
    u3_net_stack_init_eth(0, my_mac0, my_ip0, subnet0);
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT0 + 8 + 0), (my_mac0->addr[5] << 0) | (my_mac0->addr[4] << 8) | (my_mac0->addr[3] << 16) | (my_mac0->addr[2] << 24));
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT0 + 8 + 1), (my_mac0->addr[1] << 0) | (my_mac0->addr[0] << 8));
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT0 + 8 + 2), my_ip0->addr);
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT0 + 8 + 4), 0/*nofwd*/);
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT0 + 8 + 5), (ICMP_IRQ << 8) | 0); //no fwd: type, code

    //init eth1
    u3_net_stack_init_eth(1, my_mac1, my_ip1, subnet1);
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT1 + 8 + 0), (my_mac1->addr[5] << 0) | (my_mac1->addr[4] << 8) | (my_mac1->addr[3] << 16) | (my_mac1->addr[2] << 24));
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT1 + 8 + 1), (my_mac1->addr[1] << 0) | (my_mac1->addr[0] << 8));
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT1 + 8 + 2), my_ip1->addr);
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT1 + 8 + 4), 0/*nofwd*/);
    wb_poke32(SR_ADDR(SET0_BASE, SR_ETHINT1 + 8 + 5), (ICMP_IRQ << 8) | 0); //no fwd: type, code
}

static void putc(void *p, char c)
{
//If FW_TRACE_LEVEL is defined, then the trace level is set
//to a non-zero number. Turn on the debug UART to enable tracing
#ifdef UHD_FW_TRACE_LEVEL
    wb_uart_putc(UART1_BASE, c);
#endif
}

static uint32_t get_counter_val()
{
    return wb_peek32(SR_ADDR(RB0_BASE, RB_COUNTER));
}

void x300_init(x300_eeprom_map_t *eeprom_map)
{
    //first - uart
    wb_uart_init(UART0_BASE, CPU_CLOCK/UART0_BAUD);
    wb_uart_init(UART1_BASE, CPU_CLOCK/UART1_BAUD);
    init_printf(NULL,putc);
    //udp_uart_init(UART0_BASE, X300_GPSDO_UDP_PORT);

    //now we can init the rest with prints
    UHD_FW_TRACE(INFO, "[ZPU Initializing]");
    UHD_FW_TRACE_FSTR(INFO, "-- Firmware Compat Number: %u.%u", (int)X300_FW_COMPAT_MAJOR, (int)X300_FW_COMPAT_MINOR);
    uint32_t fpga_compat = wb_peek32(SR_ADDR(SET0_BASE, RB_FPGA_COMPAT));
    UHD_FW_TRACE_FSTR(INFO, "-- FPGA Compat Number: %u.%u", (fpga_compat>>16), (fpga_compat&0xFFFF));
    UHD_FW_TRACE_FSTR(INFO, "-- Clock Frequency: %u MHz", (CPU_CLOCK/1000000));

    //Initialize cron
    cron_init(get_counter_val, CPU_CLOCK);

    //i2c rate init
    wb_i2c_init(I2C0_BASE, CPU_CLOCK);
    wb_i2c_init(I2C1_BASE, CPU_CLOCK);
    wb_i2c_init(I2C2_BASE, CPU_CLOCK);

    //hold phy in reset
    wb_poke32(SR_ADDR(SET0_BASE, SR_SW_RST), SW_RST_PHY);

    //setup net stack and eth state machines
    init_network(eeprom_map);

    //phy reset release
    wb_poke32(SR_ADDR(SET0_BASE, SR_SW_RST), 0);

    //print network summary
    for (uint8_t sfp = 0; sfp < ethernet_ninterfaces(); sfp++)
    {
        uint32_t sfp_type = wb_peek32(SR_ADDR(RB0_BASE, ((sfp==1) ? RB_SFP1_TYPE : RB_SFP0_TYPE)));
        UHD_FW_TRACE_FSTR(INFO, "SFP+ Port %u:", (int)sfp);
        if (sfp_type == RB_SFP_AURORA) {
            UHD_FW_TRACE     (INFO, "-- PHY:    10Gbps Aurora");
        } else {
            UHD_FW_TRACE_FSTR(INFO, "-- PHY:    %s", (sfp_type == RB_SFP_10G_ETH) ? "10Gbps Ethernet" : "1Gbps Ethernet");
            UHD_FW_TRACE_FSTR(INFO, "-- MAC:    %s", mac_addr_to_str(u3_net_stack_get_mac_addr(sfp)));
            UHD_FW_TRACE_FSTR(INFO, "-- IP:     %s", ip_addr_to_str(u3_net_stack_get_ip_addr(sfp)));
            UHD_FW_TRACE_FSTR(INFO, "-- SUBNET: %s", ip_addr_to_str(u3_net_stack_get_subnet(sfp)));
            UHD_FW_TRACE_FSTR(INFO, "-- BCAST:  %s", ip_addr_to_str(u3_net_stack_get_bcast(sfp)));
        }
    }

    // For eth interfaces, initialize the PHY's
    sleep_ms(100);
    ethernet_init(0);
    ethernet_init(1);
}
