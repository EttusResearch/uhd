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

#include "n230_eth_handlers.h"

#include <wb_utils.h>
#include <string.h> //memcmp
#include <u3_net_stack.h>
#include <print_addrs.h>
#include <trace.h>
#include "../../../host/lib/usrp/n230/n230_fw_comm_protocol.h"
#include "../../../host/lib/usrp/n230/n230_fw_defs.h"
#include "../n230/n230_fw_host_iface.h"
#include "../../../host/lib/usrp/n230/n230_eeprom.h"

static n230_host_shared_mem_t* host_shared_mem_ptr;

static const soft_reg_field_t LED_REG_FIELD_ETH_LINK2   = {.num_bits=1, .shift=0};
static const soft_reg_field_t LED_REG_FIELD_ETH_LINK1   = {.num_bits=1, .shift=1};
static const soft_reg_field_t LED_REG_FIELD_ETH_ACT2    = {.num_bits=1, .shift=2};
static const soft_reg_field_t LED_REG_FIELD_ETH_ACT1    = {.num_bits=1, .shift=3};

/***********************************************************************
 * Handler for host <-> firmware communication
 **********************************************************************/

static inline void n230_poke32(const uint32_t addr, const uint32_t data)
{
    if (addr >= N230_FW_HOST_SHMEM_RW_BASE_ADDR && addr <= N230_FW_HOST_SHMEM_MAX_ADDR) {
        host_shared_mem_ptr->buff[(addr - N230_FW_HOST_SHMEM_BASE_ADDR)/sizeof(uint32_t)] = data;
    } else if (addr < N230_FW_HOST_SHMEM_BASE_ADDR) {
        wb_poke32(addr, data);
    }
}

static inline uint32_t n230_peek32(const uint32_t addr)
{
    if (addr >= N230_FW_HOST_SHMEM_BASE_ADDR && addr <= N230_FW_HOST_SHMEM_MAX_ADDR) {
        return host_shared_mem_ptr->buff[(addr - N230_FW_HOST_SHMEM_BASE_ADDR)/sizeof(uint32_t)];
    } else if (addr < N230_FW_HOST_SHMEM_BASE_ADDR) {
        return wb_peek32(addr);
    } else {
        return 0;
    }
}

void n230_handle_udp_fw_comms(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t src_port, const uint16_t dst_port,
    const void *buff, const size_t num_bytes)
{
    if (buff == NULL) {
        UHD_FW_TRACE(WARN, "n230_handle_udp_fw_comms got an ICMP_DUR");
        /* We got here from ICMP_DUR undeliverable packet */
        /* Future space for hooks to tear down streaming radios etc */
    } else if (num_bytes != sizeof(fw_comm_pkt_t)) {
        UHD_FW_TRACE(WARN, "n230_handle_udp_fw_comms got an unknown request (bad size).");
    } else {
        const fw_comm_pkt_t *request = (const fw_comm_pkt_t *)buff;
        fw_comm_pkt_t response;
        bool send_response = process_fw_comm_protocol_pkt(
            request, &response,
            N230_FW_PRODUCT_ID,
            (uint32_t)ethno,
            n230_poke32, n230_peek32);

        if (send_response) {
            u3_net_stack_send_udp_pkt(ethno, src, dst_port, src_port, &response, sizeof(response));
        }
    }
}

void n230_register_udp_fw_comms_handler(n230_host_shared_mem_t* shared_mem_ptr)
{
    host_shared_mem_ptr = shared_mem_ptr;
    u3_net_stack_register_udp_handler(N230_FW_COMMS_UDP_PORT, &n230_handle_udp_fw_comms);
}


/***********************************************************************
 * Handler for UDP framer program packets
 **********************************************************************/
void program_udp_framer(
    const uint8_t ethno,
    const uint32_t sid,
    const struct ip_addr *dst_ip,
    const uint16_t dst_port,
    const uint16_t src_port)
{
    const eth_mac_addr_t *dst_mac = u3_net_stack_arp_cache_lookup(dst_ip);
    const size_t vdest = (sid >> 16) & 0xff;

    uint32_t framer_base =
        ((ethno == 1) ? SR_ZPU_ETHINT1 : SR_ZPU_ETHINT0) + SR_ZPU_ETHINT_FRAMER_BASE;

    //setup source framer
    const eth_mac_addr_t *src_mac = u3_net_stack_get_mac_addr(ethno);
    wb_poke32(SR_ADDR(WB_SBRB_BASE, framer_base + ETH_FRAMER_SRC_MAC_HI),
        (((uint32_t)src_mac->addr[0]) << 8) | (((uint32_t)src_mac->addr[1]) << 0));
    wb_poke32(SR_ADDR(WB_SBRB_BASE, framer_base + ETH_FRAMER_SRC_MAC_LO),
        (((uint32_t)src_mac->addr[2]) << 24) | (((uint32_t)src_mac->addr[3]) << 16) |
        (((uint32_t)src_mac->addr[4]) << 8) | (((uint32_t)src_mac->addr[5]) << 0));
    wb_poke32(SR_ADDR(WB_SBRB_BASE, framer_base + ETH_FRAMER_SRC_IP_ADDR), u3_net_stack_get_ip_addr(ethno)->addr);
    wb_poke32(SR_ADDR(WB_SBRB_BASE, framer_base + ETH_FRAMER_SRC_UDP_PORT), src_port);

    //setup destination framer
    wb_poke32(SR_ADDR(WB_SBRB_BASE, framer_base + ETH_FRAMER_DST_RAM_ADDR), vdest);
    wb_poke32(SR_ADDR(WB_SBRB_BASE, framer_base + ETH_FRAMER_DST_IP_ADDR), dst_ip->addr);
    wb_poke32(SR_ADDR(WB_SBRB_BASE, framer_base + ETH_FRAMER_DST_UDP_MAC),
        (((uint32_t)dst_port) << 16) |
        (((uint32_t)dst_mac->addr[0]) << 8) | (((uint32_t)dst_mac->addr[1]) << 0));
    wb_poke32(SR_ADDR(WB_SBRB_BASE, framer_base + ETH_FRAMER_DST_MAC_LO),
        (((uint32_t)dst_mac->addr[2]) << 24) | (((uint32_t)dst_mac->addr[3]) << 16) |
        (((uint32_t)dst_mac->addr[4]) << 8) | (((uint32_t)dst_mac->addr[5]) << 0));
}

void handle_udp_prog_framer(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t src_port, const uint16_t dst_port,
    const void *buff, const size_t num_bytes)
{
    if (buff == NULL) {
        /* We got here from ICMP_DUR undeliverable packet */
        /* Future space for hooks to tear down streaming radios etc */
    } else {
        const uint32_t sid = ((const uint32_t *)buff)[1];
        program_udp_framer(ethno, sid, src, src_port, dst_port);
        UHD_FW_TRACE_FSTR(INFO, "Reprogrammed eth%d framer. Src=%s:%d, Dest=%s:%d",
            ethno,ip_addr_to_str(src),src_port,ip_addr_to_str(dst),dst_port);
    }
}

void n230_register_udp_prog_framer()
{
    u3_net_stack_register_udp_handler(N230_FW_COMMS_CVITA_PORT, &handle_udp_prog_framer);
}


/***********************************************************************
 * Handler for flash programming interface over UDP
 **********************************************************************/

void n230_handle_flash_prog_comms(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t src_port, const uint16_t dst_port,
    const void *buff, const size_t num_bytes)
{
    if (buff == NULL) {
        UHD_FW_TRACE(WARN, "n230_handle_flash_prog_comms got an ICMP_DUR");
        /* We got here from ICMP_DUR undeliverable packet */
        /* Future space for hooks to tear down streaming radios etc */
    } else if (num_bytes != sizeof(n230_flash_prog_t)) {
        UHD_FW_TRACE(WARN, "n230_handle_flash_prog_comms got an unknown request (bad size).");
    } else {
        const n230_flash_prog_t *request = (const n230_flash_prog_t *)buff;
        n230_flash_prog_t response;
        bool ack_requested = request->flags & N230_FLASH_COMM_FLAGS_ACK;

        //Request is valid. Copy it into the reply.
        memcpy(&response, request, sizeof(n230_flash_prog_t));

        switch (request->flags & N230_FLASH_COMM_FLAGS_CMD_MASK) {
            case N230_FLASH_COMM_CMD_READ_NV_DATA: {
                UHD_FW_TRACE(DEBUG, "n230_handle_flash_prog_comms::read_nv_data()");
                //Offset ignored because all non-volatile data fits in a packet.
                if (is_n230_eeprom_cache_dirty()) {
                    read_n230_eeprom();
                }
                //EEPROM cache is up-to-date. Copy it into the packet.
                //Assumption: Cache size < 256. If this is no longer true, the offset field
                //will have to be used.
                memcpy(response.data, get_n230_const_eeprom_map(), sizeof(n230_eeprom_map_t));
                ack_requested = true;
            } break;

            case N230_FLASH_COMM_CMD_WRITE_NV_DATA: {
                UHD_FW_TRACE(DEBUG, "n230_handle_flash_prog_comms::write_nv_data()");
                //Offset ignored because all non-volatile data fits in a packet.
                memcpy(get_n230_eeprom_map(), request->data, sizeof(n230_eeprom_map_t));
                if (!write_n230_eeprom()) {
                    response.flags |= N230_FLASH_COMM_ERR_CMD_ERROR;
                }
            } break;

            case N230_FLASH_COMM_CMD_READ_FPGA: {
                UHD_FW_TRACE_FSTR(DEBUG, "n230_handle_flash_prog_comms::read_fpga_page(offset=0x%x, size=%d)",
                    request->offset, request->size);
                read_n230_fpga_image_page(request->offset, response.data, request->size);
                ack_requested = true;
            } break;

            case N230_FLASH_COMM_CMD_WRITE_FPGA: {
                UHD_FW_TRACE_FSTR(DEBUG, "n230_handle_flash_prog_comms::write_fpga_page(offset=0x%x, size=%d)",
                    request->offset, request->size);
                if (!write_n230_fpga_image_page(request->offset, request->data, request->size)) {
                    response.flags |= N230_FLASH_COMM_ERR_CMD_ERROR;
                }
            } break;

            case N230_FLASH_COMM_CMD_ERASE_FPGA: {
                UHD_FW_TRACE_FSTR(DEBUG, "n230_handle_flash_prog_comms::erase_fpga_sector(offset=0x%x)",
                    request->offset);
                if (!erase_n230_fpga_image_sector(request->offset)) {
                    response.flags |= N230_FLASH_COMM_ERR_CMD_ERROR;
                }
            } break;

            default :{
                UHD_FW_TRACE(ERROR, "n230_handle_flash_prog_comms got an invalid command.");
                response.flags |= FW_COMM_ERR_CMD_ERROR;
            }
        }
        //Send a reply if ack requested
        if (ack_requested) {
            u3_net_stack_send_udp_pkt(ethno, src, dst_port, src_port, &response, sizeof(response));
        }
    }
}

void n230_register_flash_comms_handler()
{
    u3_net_stack_register_udp_handler(N230_FW_COMMS_FLASH_PROG_PORT, &n230_handle_flash_prog_comms);
}

/***********************************************************************
 * Handler for SFP state changes
 **********************************************************************/
#define SFPP_STATUS_MODABS_CHG     (1 << 5)    // Has MODABS changed since last read?
#define SFPP_STATUS_TXFAULT_CHG    (1 << 4)    // Has TXFAULT changed since last read?
#define SFPP_STATUS_RXLOS_CHG      (1 << 3)    // Has RXLOS changed since last read?
#define SFPP_STATUS_MODABS         (1 << 2)    // MODABS state
#define SFPP_STATUS_TXFAULT        (1 << 1)    // TXFAULT state
#define SFPP_STATUS_RXLOS          (1 << 0)    // RXLOS state

static bool     links_up[N230_MAX_NUM_ETH_PORTS] = {};
static uint32_t packet_count[N230_MAX_NUM_ETH_PORTS] = {};

void n230_poll_sfp_status(const uint32_t eth, bool force, bool* state_updated)
{
    // Has MODDET/MODAbS changed since we last looked?
    uint32_t rb = wb_peek32(SR_ADDR(WB_SBRB_BASE, (eth==0) ? RB_ZPU_SFP_STATUS0 : RB_ZPU_SFP_STATUS1));

    if (rb & SFPP_STATUS_RXLOS_CHG)
        UHD_FW_TRACE_FSTR(DEBUG, "eth%1d RXLOS changed state: %d", eth, (rb & SFPP_STATUS_RXLOS));
    if (rb & SFPP_STATUS_TXFAULT_CHG)
        UHD_FW_TRACE_FSTR(DEBUG, "eth%1d TXFAULT changed state: %d", eth, ((rb & SFPP_STATUS_TXFAULT) >> 1));
    if (rb & SFPP_STATUS_MODABS_CHG)
        UHD_FW_TRACE_FSTR(DEBUG, "eth%1d MODABS changed state: %d", eth, ((rb & SFPP_STATUS_MODABS) >> 2));

    //update the link up status
    if ((rb & SFPP_STATUS_RXLOS_CHG) || (rb & SFPP_STATUS_TXFAULT_CHG) || (rb & SFPP_STATUS_MODABS_CHG) || force)
    {
        const bool old_link_up = links_up[eth];
        const uint32_t status_reg_addr = (eth==0) ? RB_ZPU_SFP_STATUS0 : RB_ZPU_SFP_STATUS1;

        uint32_t sfpp_status = wb_peek32(SR_ADDR(WB_SBRB_BASE, status_reg_addr)) & 0xFFFF;
        if ((sfpp_status & (SFPP_STATUS_RXLOS|SFPP_STATUS_TXFAULT|SFPP_STATUS_MODABS)) == 0) {
            int8_t timeout = 100;
            bool link_up = false;
            do {
                link_up = ((wb_peek32(SR_ADDR(WB_SBRB_BASE, status_reg_addr)) >> 16) & 0x1) != 0;
            } while (!link_up && timeout-- > 0);

            links_up[eth] = link_up;
        } else {
            links_up[eth] = false;
        }

        if (!old_link_up && links_up[eth]) u3_net_stack_send_arp_request(eth, u3_net_stack_get_ip_addr(eth));
        UHD_FW_TRACE_FSTR(INFO, "The link on eth port %u is %s", eth, links_up[eth]?"up":"down");
        if (rb & SFPP_STATUS_MODABS_CHG) {
            // MODDET has changed state since last checked
            if (rb & SFPP_STATUS_MODABS) {
                // MODDET is high, module currently removed.
                UHD_FW_TRACE_FSTR(INFO, "An SFP+ module has been removed from eth port %d.", eth);
            } else {
                // MODDET is low, module currently inserted.
                // Return status.
                UHD_FW_TRACE_FSTR(INFO, "A new SFP+ module has been inserted into eth port %d.", eth);
            }
        }
        *state_updated = true;
    } else {
        *state_updated = false;
    }
}

void n230_update_link_act_state(soft_reg_t* led_reg)
{
    static bool first_poll = 1;
    static uint32_t poll_cnt;

    bool activity[N230_MAX_NUM_ETH_PORTS] = {};
    for (uint32_t i = 0; i < N230_NUM_ETH_PORTS; i++) {
        if (first_poll) {
            links_up[i] = 0;
            packet_count[i] = 0;
            poll_cnt = 0;
        }

        //Check SFP status and update links_up
        bool link_state_from_sfp = false;
        n230_poll_sfp_status(i, first_poll, &link_state_from_sfp);

        //Check packet counters less frequently to keep the LED on for a visible duration
        uint32_t cnt = wb_peek32(SR_ADDR(WB_SBRB_BASE, (i==0)?RB_ZPU_ETH0_PKT_CNT:RB_ZPU_ETH1_PKT_CNT));
        activity[i] = (cnt != packet_count[i]);
        packet_count[i] = cnt;

        //Update links_up if there is activity only if the SFP
        //handler has not updated it
        if (activity[i] && !link_state_from_sfp) links_up[i] = true;
    }

    //TODO: Swap this when Ethernet port swap issues is fixed
    soft_reg_write(led_reg, LED_REG_FIELD_ETH_LINK2, links_up[0]?1:0);
    soft_reg_write(led_reg, LED_REG_FIELD_ETH_LINK1, links_up[1]?1:0);
    soft_reg_write(led_reg, LED_REG_FIELD_ETH_ACT2,  activity[0]?1:0);
    soft_reg_write(led_reg, LED_REG_FIELD_ETH_ACT1,  activity[1]?1:0);

    first_poll = 0;
}

