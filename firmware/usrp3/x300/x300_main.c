// Copyright 2013-2017 Ettus Research

#include "x300_init.h"
#include "x300_defs.h"
#include "x300_fw_common.h"
#include "xge_phy.h"
#include "ethernet.h"
#include "chinch.h"

#include <wb_utils.h>
#include <wb_uart.h>
#include <udp_uart.h>
#include <u3_net_stack.h>
#include <link_state_route_proto.h>
#include <trace.h>
#include <string.h>
#include <print_addrs.h>

static uint32_t *shmem = (uint32_t *) X300_FW_SHMEM_BASE;

/***********************************************************************
 * Setup call for udp framer
 **********************************************************************/
void program_udp_framer(
    const uint8_t ethno,
    const uint32_t sid,
    const struct ip_addr *dst_ip,
    const uint16_t dst_port,
    const uint16_t src_port
)
{
    const eth_mac_addr_t *dst_mac = u3_net_stack_arp_cache_lookup(dst_ip);
    const size_t ethbase = (ethno == 0)? SR_ETHINT0 : SR_ETHINT1;
    const size_t vdest = (sid >> 16) & 0xff;
    UHD_FW_TRACE_FSTR(INFO, "handle_udp_prog_framer sid %u vdest %u\n", sid, vdest);

    //setup source framer
    const eth_mac_addr_t *src_mac = u3_net_stack_get_mac_addr(ethno);
    wb_poke32(SR_ADDR(SET0_BASE, ethbase + ETH_FRAMER_SRC_MAC_HI),
        (((uint32_t)src_mac->addr[0]) << 8) | (((uint32_t)src_mac->addr[1]) << 0));
    wb_poke32(SR_ADDR(SET0_BASE, ethbase + ETH_FRAMER_SRC_MAC_LO),
        (((uint32_t)src_mac->addr[2]) << 24) | (((uint32_t)src_mac->addr[3]) << 16) |
        (((uint32_t)src_mac->addr[4]) << 8) | (((uint32_t)src_mac->addr[5]) << 0));
    wb_poke32(SR_ADDR(SET0_BASE, ethbase + ETH_FRAMER_SRC_IP_ADDR), u3_net_stack_get_ip_addr(ethno)->addr);
    wb_poke32(SR_ADDR(SET0_BASE, ethbase + ETH_FRAMER_SRC_UDP_PORT), src_port);

    //setup destination framer
    wb_poke32(SR_ADDR(SET0_BASE, ethbase + ETH_FRAMER_DST_RAM_ADDR), vdest);
    wb_poke32(SR_ADDR(SET0_BASE, ethbase + ETH_FRAMER_DST_IP_ADDR), dst_ip->addr);
    wb_poke32(SR_ADDR(SET0_BASE, ethbase + ETH_FRAMER_DST_UDP_MAC),
        (((uint32_t)dst_port) << 16) |
        (((uint32_t)dst_mac->addr[0]) << 8) | (((uint32_t)dst_mac->addr[1]) << 0));
    wb_poke32(SR_ADDR(SET0_BASE, ethbase + ETH_FRAMER_DST_MAC_LO),
        (((uint32_t)dst_mac->addr[2]) << 24) | (((uint32_t)dst_mac->addr[3]) << 16) |
        (((uint32_t)dst_mac->addr[4]) << 8) | (((uint32_t)dst_mac->addr[5]) << 0));
}

/***********************************************************************
 * Handler for UDP framer program packets
 **********************************************************************/
void handle_udp_prog_framer(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t src_port, const uint16_t dst_port,
    const void *buff, const size_t num_bytes
)
{
  if (buff == NULL) {
    /* We got here from ICMP_DUR undeliverable packet */
    /* Future space for hooks to tear down streaming radios etc */
  }
  else {
    const uint32_t sid = ((const uint32_t *)buff)[1];
    program_udp_framer(ethno, sid, src, src_port, dst_port);
  }
}

/***********************************************************************
 * Handler for peek and poke host packets
 **********************************************************************/
void handle_udp_fw_comms(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t src_port, const uint16_t dst_port,
    const void *buff, const size_t num_bytes
)
{
    if (buff == NULL) {
     /* We got here from ICMP_DUR undeliverable packet */
    /* Future space for hooks to tear down streaming radios etc */
    } else {
        const x300_fw_comms_t *request = (const x300_fw_comms_t *)buff;
        x300_fw_comms_t reply; memcpy(&reply, buff, sizeof(reply));

        //check for error and set error flag
        if (num_bytes < sizeof(x300_fw_comms_t)) {
           reply.flags |= X300_FW_COMMS_FLAGS_ERROR;
        }
        //otherwise, run the actions set by the flags
        else {
            if (request->flags & X300_FW_COMMS_FLAGS_PEEK32)
            {
                if (request->addr & 0x00100000) {
                    chinch_peek32(request->addr & 0x000FFFFF, &reply.data);
                } else {
                    reply.data = wb_peek32(request->addr);
                }
            }
            if (request->flags & X300_FW_COMMS_FLAGS_POKE32)
            {
                if (request->addr & 0x00100000) {
                    chinch_poke32(request->addr & 0x000FFFFF, request->data);
                } else {
                    wb_poke32(request->addr, request->data);
                }
            }
        }

        //send a reply if ack requested
        if (request->flags & X300_FW_COMMS_FLAGS_ACK) {
            u3_net_stack_send_udp_pkt(ethno, src, dst_port, src_port, &reply, sizeof(reply));
        }
    }
}

/***********************************************************************
 * Handler for FPGA programming packets
 **********************************************************************/
void handle_udp_fpga_prog(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t src_port, const uint16_t dst_port,
    const void *buff, const size_t num_bytes
)
{
    const x300_fpga_prog_t *request = (const x300_fpga_prog_t *)buff;
    x300_fpga_prog_flags_t reply = {0};
    bool status = true;

    if (buff == NULL) {
        return;
    } else if (num_bytes < offsetof(x300_fpga_prog_t, data)) {
        reply.flags |= X300_FPGA_PROG_FLAGS_ERROR;
    } else {
        if (request->flags & X300_FPGA_PROG_FLAGS_INIT) {
            STATUS_MERGE(chinch_flash_init(), status);
        } else if (request->flags & X300_FPGA_PROG_FLAGS_CLEANUP) {
            chinch_flash_cleanup();
        } else if (request->flags & X300_FPGA_PROG_CONFIGURE) {
            //This is a self-destructive operation and will most likely not return an ack.
            chinch_start_config();
        } else if (request->flags & X300_FPGA_PROG_CONFIG_STATUS) {
            if (chinch_get_config_status() != CHINCH_CONFIG_COMPLETED)
                reply.flags |= X300_FPGA_PROG_FLAGS_ERROR;
        } else {
            STATUS_MERGE(chinch_flash_select_sector(request->sector), status);
            if (request->flags & X300_FPGA_PROG_FLAGS_ERASE)
                STATUS_CHAIN(chinch_flash_erase_sector(), status);

            uint32_t num_buff_writes = (request->size / CHINCH_FLASH_MAX_BUF_WRITES) +
                                       (request->size % CHINCH_FLASH_MAX_BUF_WRITES == 0 ? 0 : 1);
            uint32_t data_idx = 0;
            for (uint32_t buf_wr_i = 0; (buf_wr_i < num_buff_writes) && status; buf_wr_i++) {
                uint32_t wr_len = (request->size - data_idx) >= CHINCH_FLASH_MAX_BUF_WRITES ?
                    CHINCH_FLASH_MAX_BUF_WRITES : (request->size - data_idx);

                STATUS_MERGE(chinch_flash_write_buf((request->index + data_idx)*2,
                    (uint16_t*)request->data+data_idx, wr_len), status);
                data_idx += wr_len;
            }

            if (request->flags & X300_FPGA_PROG_FLAGS_VERIFY) {
                uint16_t data[request->size];
                STATUS_MERGE(chinch_flash_read_buf(request->index*2, data, request->size), status);
                for (uint32_t i = 0; i < request->size; i++) {
                    status &= (data[i] == request->data[i]);
                }
            }
        }
    }
    if (!status) reply.flags |= X300_FPGA_PROG_FLAGS_ERROR;

    //send a reply if ack requested
    if (request->flags & X300_FPGA_PROG_FLAGS_ACK)
    {
        u3_net_stack_send_udp_pkt(ethno, src, dst_port, src_port, &reply, sizeof(reply));
    }
}

/***********************************************************************
 * Handler for FPGA image reading packets
 **********************************************************************/
void handle_udp_fpga_read(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t src_port, const uint16_t dst_port,
    const void *buff, const size_t num_bytes
)
{
    const x300_fpga_read_t *request = (const x300_fpga_read_t *) buff;
    x300_fpga_read_reply_t reply = {0};
    bool status = true;

    if (buff == NULL) {
        return;
    } else if (num_bytes < offsetof(x300_fpga_read_t, size)) {
        reply.flags |= X300_FPGA_READ_FLAGS_ERROR;
    } else {
        if (request->flags & X300_FPGA_READ_FLAGS_INIT) {
            STATUS_MERGE(chinch_flash_init(), status);
        } else if (request->flags & X300_FPGA_READ_FLAGS_CLEANUP) {
            chinch_flash_cleanup();
        } else {
            reply.flags |= X300_FPGA_READ_FLAGS_ACK;
            reply.sector = request->sector;
            reply.index  = request->index;
            reply.size   = request->size;

            STATUS_MERGE(chinch_flash_select_sector(request->sector), status);
            STATUS_MERGE(chinch_flash_read_buf(request->index*2, reply.data, request->size), status);
        }
    }

    if (!status) reply.flags |= X300_FPGA_READ_FLAGS_ERROR;
    u3_net_stack_send_udp_pkt(ethno, src, dst_port, src_port, &reply, sizeof(reply));
}

/***********************************************************************
 * Handler for MTU detection
 **********************************************************************/
void handle_udp_mtu_detect(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t src_port, const uint16_t dst_port,
    const void *buff, const size_t num_bytes
)
{
    const x300_mtu_t *request = (const x300_mtu_t *) buff;
    x300_mtu_t reply;

    if (buff == NULL) {
        return;
    } else if (!(request->flags & X300_MTU_DETECT_ECHO_REQUEST)) {
        UHD_FW_TRACE(WARN, "MTU detect got unknown request");
        reply.flags |= X300_MTU_DETECT_ERROR;
    }

    reply.flags |= X300_MTU_DETECT_ECHO_REPLY;
    reply.size = num_bytes;

    u3_net_stack_send_udp_pkt(ethno, src, dst_port, src_port, &reply, request->size);
}


/***********************************************************************
 * Deal with host claims and claim timeout
 **********************************************************************/
static void handle_claim(uint32_t ticks_now)
{
    static const uint32_t CLAIM_TIMEOUT = 2*CPU_CLOCK;      // 2 seconds
    static uint32_t ticks_last_claim = 0;
    static uint32_t last_time = 0;

    // Claim status can only change if the claim is active or the claim is renewed.
    if (shmem[X300_FW_SHMEM_CLAIM_STATUS] != 0 &&
            (shmem[X300_FW_SHMEM_CLAIM_TIME] == 0 ||
            ticks_now - ticks_last_claim > CLAIM_TIMEOUT))
    {
            // the claim was released or timed out
            shmem[X300_FW_SHMEM_CLAIM_STATUS] = 0;
            last_time = shmem[X300_FW_SHMEM_CLAIM_TIME];
    }
    else if (last_time != shmem[X300_FW_SHMEM_CLAIM_TIME])
    {
        // claim was renewed
        shmem[X300_FW_SHMEM_CLAIM_STATUS] = 1;
        last_time = shmem[X300_FW_SHMEM_CLAIM_TIME];
        ticks_last_claim = ticks_now;
    }
}

/***********************************************************************
 * LED blinky logic and support utilities
 **********************************************************************/
static uint32_t get_xbar_total(const uint32_t port)
{
    static const uint32_t NUM_PORTS = 16;
    uint32_t total = 0;
    for (uint32_t i = 0; i < NUM_PORTS; i++)
    {
        wb_poke32(SET0_BASE + SR_RB_ADDR*4, (NUM_PORTS*port + i));
        total += wb_peek32(RB0_BASE + RB_XBAR*4);
        wb_poke32(SET0_BASE + SR_RB_ADDR*4, (NUM_PORTS*i + port));
        total += wb_peek32(RB0_BASE + RB_XBAR*4);
    }
    if (port < 2) //also netstack if applicable
    {
        total += u3_net_stack_get_stat_counts(port);
    }
    return total;
}

static void update_leds(void)
{
    static uint32_t last_total0 = 0;
    static uint32_t last_total1 = 0;
    const uint32_t total0 = get_xbar_total(0);
    const uint32_t total1 = get_xbar_total(1);
    const bool act0 = (total0 != last_total0);
    const bool act1 = (total1 != last_total1);
    last_total0 = total0;
    last_total1 = total1;

    const bool link0 = ethernet_get_link_up(0);
    const bool link1 = ethernet_get_link_up(1);
    const bool claimed = shmem[X300_FW_SHMEM_CLAIM_STATUS];

    wb_poke32(SET0_BASE + SR_LEDS*4, 0
        | (link0? LED_LINK2 : 0)
        | (link1? LED_LINK1 : 0)
        | (act0? LED_ACT2 : 0)
        | (act1? LED_ACT1 : 0)
        | ((act0 || act1)? LED_LINKACT : 0)
        | (claimed? LED_LINKSTAT : 0)
    );
}

/***********************************************************************
 * Send periodic GARPs to keep network hardware informed
 **********************************************************************/
static void garp(void)
{
    static size_t count = 0;
    if (count++ < 3000) return; //30 seconds
    count = 0;
    for (size_t e = 0; e < ethernet_ninterfaces(); e++)
    {
        if (wb_peek32(SR_ADDR(RB0_BASE, e == 0 ? RB_SFP0_TYPE : RB_SFP1_TYPE)) != RB_SFP_AURORA) {
            if (!ethernet_get_link_up(e)) continue;
            u3_net_stack_send_arp_request(e, u3_net_stack_get_ip_addr(e));
        }
    }
}

/***********************************************************************
 * UART handlers - interacts between UART and SHMEM
 **********************************************************************/
static void handle_uarts(void)
{
    //pool allocations - always update shmem with location
    #define NUM_POOL_WORDS32 64
    static uint32_t rxpool[NUM_POOL_WORDS32];
    static uint32_t txpool[NUM_POOL_WORDS32];
    shmem[X300_FW_SHMEM_UART_RX_ADDR] = (uint32_t)rxpool;
    shmem[X300_FW_SHMEM_UART_TX_ADDR] = (uint32_t)txpool;
    shmem[X300_FW_SHMEM_UART_WORDS32] = NUM_POOL_WORDS32;

    ////////////////////////////////////////////////////////////////////
    // RX UART - get available characters and post to the shmem buffer
    ////////////////////////////////////////////////////////////////////
    static uint32_t rxoffset = 0;
    for (int rxch = wb_uart_getc(UART0_BASE); rxch != -1; rxch = wb_uart_getc(UART0_BASE))
    {
        const int shift = ((rxoffset%4) * 8);
        static uint32_t rxword32 = 0;
        if (shift == 0) rxword32 = 0;
        rxword32 |= ((uint32_t) rxch & 0xFF) << shift;
        rxpool[(rxoffset/4) % NUM_POOL_WORDS32] = rxword32;
        rxoffset++;
        shmem[X300_FW_SHMEM_UART_RX_INDEX] = rxoffset;
    }

    ////////////////////////////////////////////////////////////////////
    // TX UART - check for characters in the shmem buffer and send them
    ////////////////////////////////////////////////////////////////////
    static uint32_t txoffset = 0;
    while (txoffset != shmem[X300_FW_SHMEM_UART_TX_INDEX])
    {
        const int shift = ((txoffset%4) * 8);
        const int txch = txpool[txoffset/4] >> shift;
        wb_uart_putc(UART0_BASE, txch);
        txoffset = (txoffset+1) % (NUM_POOL_WORDS32*4);
    }
}

/***********************************************************************
 * update the link state periodic update
 **********************************************************************/
static void update_forwarding(const uint8_t e)
{
    /* FIXME: This code is broken.
     * It blindly enables forwarding without regard to whether or not
     * packets can be forwarded.  If one of the Ethernet interfaces is not
     * connected, data backs up until the first interface becomes unresponsive.
     *
     * //update forwarding rules
     * uint32_t forward = 0;
     * if (!link_state_route_proto_causes_cycle_cached(e, (e+1)%2))
     * {
     *     forward |= (1 << 0); //forward bcast
     *     forward |= (1 << 1); //forward not mac dest
     * }
     * const uint32_t eth_base = (e == 0)? SR_ETHINT0 : SR_ETHINT1;
     * wb_poke32(SR_ADDR(SET0_BASE, eth_base + 8 + 4), forward);
     */

}

static void handle_link_state(void)
{
    //update shmem entries to keep it persistent
    size_t map_len = 0;
    shmem[X300_FW_SHMEM_ROUTE_MAP_ADDR] = (uint32_t)link_state_route_get_node_mapping(&map_len);
    shmem[X300_FW_SHMEM_ROUTE_MAP_LEN] = map_len;

    static size_t count = 0;
    if (count--) return;
    count = 2000;  //repeat every ~2 seconds

    link_state_route_proto_tick();
    for (size_t e = 0; e < ethernet_ninterfaces(); e++)
    {
        if (ethernet_get_link_up(e))
        {
            link_state_route_proto_update(e);
            link_state_route_proto_flood(e);
        }

        //update forwarding if something changed
        bool before = link_state_route_proto_causes_cycle_cached(e, (e+1)%2);
        link_state_route_proto_update_cycle_cache(e);
        if (before != link_state_route_proto_causes_cycle_cached(e, (e+1)%2))
            update_forwarding(e);
        /*
        printf("is there a cycle %s -> %s? %s\n",
            ip_addr_to_str(u3_net_stack_get_ip_addr(e)),
            ip_addr_to_str(u3_net_stack_get_ip_addr((e+1)%2)),
            link_state_route_proto_causes_cycle_cached(e, (e+1)%2)? "YES" : "no");
        //*/
    }
}

/***********************************************************************
 * Main loop runs all the handlers
 **********************************************************************/
int main(void)
{
    x300_init((x300_eeprom_map_t *)&shmem[X300_FW_SHMEM_IDENT]);
    u3_net_stack_register_udp_handler(X300_FW_COMMS_UDP_PORT, &handle_udp_fw_comms);
    u3_net_stack_register_udp_handler(X300_VITA_UDP_PORT, &handle_udp_prog_framer);
    u3_net_stack_register_udp_handler(X300_FPGA_PROG_UDP_PORT, &handle_udp_fpga_prog);
    u3_net_stack_register_udp_handler(X300_FPGA_READ_UDP_PORT, &handle_udp_fpga_read);
    u3_net_stack_register_udp_handler(X300_MTU_DETECT_UDP_PORT, &handle_udp_mtu_detect);

    uint32_t last_cronjob = 0;

    while(true)
    {
        const uint32_t ticks_now = wb_peek32(SR_ADDR(RB0_BASE, RB_COUNTER));

        // handle the claim every time because any packet processed could
        // have claimed or released the device and we want the claim status
        // to be updated immediately to make it atomic from the host perspective
        handle_claim(ticks_now);

        //jobs that happen once every 10ms
        const uint32_t ticks_passed = ticks_now - last_cronjob;
        static const uint32_t tick_delta = CPU_CLOCK/100;
        if (ticks_passed > tick_delta)
        {
            poll_sfpp_status(0); // Every so often poll XGE Phy to look for SFP+ hotplug events.
            poll_sfpp_status(1); // Every so often poll XGE Phy to look for SFP+ hotplug events.
            //handle_link_state(); //deal with router table update
            update_leds(); //run the link and activity leds
            garp(); //send periodic garps
            last_cronjob = ticks_now;
        }

        //run the network stack - poll and handle
        u3_net_stack_handle_one();

        //run the PCIe listener - poll and fwd to wishbone
        forward_pcie_user_xact_to_wb();

        //run the udp uart handler for incoming serial data
        handle_uarts(); //udp_uart_poll();

        //always reload the compat number into the shmem to keep it persistent
        shmem[X300_FW_SHMEM_COMPAT_NUM] = (X300_FW_COMPAT_MAJOR << 16) | X300_FW_COMPAT_MINOR;
    }
    return 0;
}
