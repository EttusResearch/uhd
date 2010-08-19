//
// Copyright 2010 Ettus Research LLC
//
/*
 * Copyright 2007,2008 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <lwip/ip.h>
#include <lwip/udp.h>
#include "u2_init.h"
#include "memory_map.h"
#include "spi.h"
#include "hal_io.h"
#include "buffer_pool.h"
#include "pic.h"
#include <stdbool.h>
#include "ethernet.h"
#include "nonstdio.h"
#include "dbsm.h"
#include <net/padded_eth_hdr.h>
#include <net_common.h>
#include "memcpy_wa.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "clocks.h"
#include "usrp2/fw_common.h"
#include <i2c.h>
#include <ethertype.h>
#include <arp_cache.h>

/*
 * Full duplex Tx and Rx between ethernet and DSP pipelines
 *
 * Buffer 1 is used by the cpu to send frames to the host.
 * Buffers 2 and 3 are used to double-buffer the DSP Rx to eth flow
 * Buffers 4 and 5 are used to double-buffer the eth to DSP Tx  eth flow
 */
//#define CPU_RX_BUF	0	// eth -> cpu

#define	DSP_RX_BUF_0	2	// dsp rx -> eth (double buffer)
#define	DSP_RX_BUF_1	3	// dsp rx -> eth
#define	DSP_TX_BUF_0	4	// eth -> dsp tx (double buffer)
#define	DSP_TX_BUF_1	5	// eth -> dsp tx

/*
 * ================================================================
 *   configure DSP TX double buffering state machine (eth -> dsp)
 * ================================================================
 */

// DSP Tx reads ethernet header words
#define DSP_TX_FIRST_LINE ((sizeof(padded_eth_hdr_t) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr))/sizeof(uint32_t))

// Receive from ethernet
buf_cmd_args_t dsp_tx_recv_args = {
  PORT_ETH,
  0,
  BP_LAST_LINE
};

// send to DSP Tx
buf_cmd_args_t dsp_tx_send_args = {
  PORT_DSP,
  DSP_TX_FIRST_LINE,	// starts just past transport header
  0			// filled in from last_line register
};

dbsm_t dsp_tx_sm;	// the state machine

/*
 * ================================================================
 *   configure DSP RX double buffering state machine (dsp -> eth)
 * ================================================================
 */

static const uint32_t rx_ctrl_word = 1 << 16;

// DSP Rx writes ethernet header words
#define DSP_RX_FIRST_LINE sizeof(rx_ctrl_word)/sizeof(uint32_t)

static bool dbsm_rx_inspector(dbsm_t *sm, int buf_this){
    size_t num_lines = buffer_pool_status->last_line[buf_this]-DSP_RX_FIRST_LINE;
    ((uint32_t*)buffer_ram(buf_this))[0] = (num_lines*sizeof(uint32_t)) | (1 << 16);
    return false;
}

// receive from DSP
buf_cmd_args_t dsp_rx_recv_args = {
  PORT_DSP,
  DSP_RX_FIRST_LINE,
  BP_LAST_LINE
};

// send to ETH
buf_cmd_args_t dsp_rx_send_args = {
  PORT_ETH,
  0,		// starts with ethernet header in line 0
  0,		// filled in from list_line register
};

dbsm_t dsp_rx_sm;	// the state machine


// The mac address of the host we're sending to.
eth_mac_addr_t host_mac_addr;

static void setup_network(void);

// ----------------------------------------------------------------
// the fast-path setup global variables
// ----------------------------------------------------------------
static eth_mac_addr_t fp_mac_addr_src, fp_mac_addr_dst;
static struct socket_address fp_socket_src, fp_socket_dst;

// ----------------------------------------------------------------
void start_rx_streaming_cmd(void);
void stop_rx_cmd(void);

static void print_ip_addr(const void *t){
    uint8_t *p = (uint8_t *)t;
    printf("%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
}

void handle_udp_data_packet(
    struct socket_address src, struct socket_address dst,
    unsigned char *payload, int payload_len
){
    //its a tiny payload, load the fast-path variables
    fp_mac_addr_src = *ethernet_mac_addr();
    arp_cache_lookup_mac(&src.addr, &fp_mac_addr_dst);
    fp_socket_src = dst;
    fp_socket_dst = src;
    printf("Storing for fast path:\n");
    printf("  source mac addr: ");
    print_mac_addr(fp_mac_addr_src.addr); newline();
    printf("  source ip addr: ");
    print_ip_addr(&fp_socket_src.addr); newline();
    printf("  source udp port: %d\n", fp_socket_src.port);
    printf("  destination mac addr: ");
    print_mac_addr(fp_mac_addr_dst.addr); newline();
    printf("  destination ip addr: ");
    print_ip_addr(&fp_socket_dst.addr); newline();
    printf("  destination udp port: %d\n", fp_socket_dst.port);
    newline();

    //setup network and vrt
    setup_network();

    // kick off the state machine
    dbsm_start(&dsp_rx_sm);

}

#define OTW_GPIO_BANK_TO_NUM(bank) \
    (((bank) == USRP2_DIR_RX)? (GPIO_RX_BANK) : (GPIO_TX_BANK))

//setup the output data
static usrp2_ctrl_data_t ctrl_data_out;
static struct socket_address i2c_src;
static struct socket_address spi_src;

static volatile bool i2c_done = false;
void i2c_read_done_callback(void) {
  //printf("I2C read done callback\n");
  i2c_async_data_ready(ctrl_data_out.data.i2c_args.data);
  i2c_done = true;
  i2c_register_callback(0);
}

void i2c_write_done_callback(void) {
  //printf("I2C write done callback\n");
  i2c_done = true;
  i2c_register_callback(0);
}

static volatile bool spi_done = false;
static volatile uint32_t spi_readback_data;
void get_spi_readback_data(void) {
  ctrl_data_out.data.spi_args.data = spi_get_data();
  spi_done = true;
  spi_register_callback(0);
}

void handle_udp_ctrl_packet(
    struct socket_address src, struct socket_address dst,
    unsigned char *payload, int payload_len
){
    //printf("Got ctrl packet #words: %d\n", (int)payload_len);
    const usrp2_ctrl_data_t *ctrl_data_in = (usrp2_ctrl_data_t *)payload;
    uint32_t ctrl_data_in_id = ctrl_data_in->id;

    //ensure that the protocol versions match
    if (payload_len >= sizeof(uint32_t) && ctrl_data_in->proto_ver != USRP2_FW_COMPAT_NUM){
        printf("!Error in control packet handler: Expected compatibility number %d, but got %d\n",
            USRP2_FW_COMPAT_NUM, ctrl_data_in->proto_ver
        );
        ctrl_data_in_id = USRP2_CTRL_ID_WAZZUP_BRO;
    }

    //ensure that this is not a short packet
    if (payload_len < sizeof(usrp2_ctrl_data_t)){
        printf("!Error in control packet handler: Expected payload length %d, but got %d\n",
            (int)sizeof(usrp2_ctrl_data_t), payload_len
        );
        ctrl_data_in_id = USRP2_CTRL_ID_HUH_WHAT;
    }

    //setup the output data
    ctrl_data_out.proto_ver = USRP2_FW_COMPAT_NUM;
    ctrl_data_out.id=USRP2_CTRL_ID_HUH_WHAT;
    ctrl_data_out.seq=ctrl_data_in->seq;

    //handle the data based on the id
    switch(ctrl_data_in_id){

    /*******************************************************************
     * Addressing
     ******************************************************************/
    case USRP2_CTRL_ID_WAZZUP_BRO:
        ctrl_data_out.id = USRP2_CTRL_ID_WAZZUP_DUDE;
        memcpy(&ctrl_data_out.data.ip_addr, get_ip_addr(), sizeof(struct ip_addr));
        send_udp_pkt(USRP2_UDP_CTRL_PORT, src, &ctrl_data_out, sizeof(ctrl_data_out));
        break;

    /*******************************************************************
     * SPI
     ******************************************************************/
    case USRP2_CTRL_ID_TRANSACT_ME_SOME_SPI_BRO:{
            //transact
            bool success = spi_async_transact(
                //(ctrl_data_in->data.spi_args.readback == 0)? SPI_TXONLY : SPI_TXRX,
                ctrl_data_in->data.spi_args.dev,      //which device
                ctrl_data_in->data.spi_args.data,     //32 bit data
                ctrl_data_in->data.spi_args.num_bits, //length in bits
                (ctrl_data_in->data.spi_args.mosi_edge == USRP2_CLK_EDGE_RISE)? SPIF_PUSH_FALL : SPIF_PUSH_RISE | //flags
                (ctrl_data_in->data.spi_args.miso_edge == USRP2_CLK_EDGE_RISE)? SPIF_LATCH_RISE : SPIF_LATCH_FALL,
                get_spi_readback_data //callback
            );

            //load output
            ctrl_data_out.id = USRP2_CTRL_ID_OMG_TRANSACTED_SPI_DUDE;
            spi_src = src;
        }
//        send_udp_pkt(USRP2_UDP_CTRL_PORT, src, &ctrl_data_out, sizeof(ctrl_data_out));
        break;

    /*******************************************************************
     * I2C
     ******************************************************************/
    case USRP2_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO:{
            uint8_t num_bytes = ctrl_data_in->data.i2c_args.bytes;
            i2c_register_callback(i2c_read_done_callback);
            i2c_async_read(
                ctrl_data_in->data.i2c_args.addr,
                num_bytes
            );
            i2c_src = src;
//            i2c_dst = dst;
            ctrl_data_out.id = USRP2_CTRL_ID_HERES_THE_I2C_DATA_DUDE;
            ctrl_data_out.data.i2c_args.bytes = num_bytes;
        }
        break;

    case USRP2_CTRL_ID_WRITE_THESE_I2C_VALUES_BRO:{
            uint8_t num_bytes = ctrl_data_in->data.i2c_args.bytes;
            i2c_register_callback(i2c_read_done_callback);
            i2c_async_write(
                ctrl_data_in->data.i2c_args.addr,
                ctrl_data_in->data.i2c_args.data,
                num_bytes
            );
            i2c_src = src;
//            i2c_dst = dst;
            ctrl_data_out.id = USRP2_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE;
            ctrl_data_out.data.i2c_args.bytes = num_bytes;
        }
        break;

    /*******************************************************************
     * Peek and Poke Register
     ******************************************************************/
    case USRP2_CTRL_ID_POKE_THIS_REGISTER_FOR_ME_BRO:
        if (0){//ctrl_data_in->data.poke_args.addr < 0xC000){
            printf("error! tried to poke into 0x%x\n", ctrl_data_in->data.poke_args.addr);
        }
        else switch(ctrl_data_in->data.poke_args.num_bytes){
        case sizeof(uint64_t):
            *((uint32_t *) ctrl_data_in->data.poke_args.addrhi) = (uint32_t)ctrl_data_in->data.poke_args.datahi;
            //continue to uint32_t for low addr:

        case sizeof(uint32_t):
            *((uint32_t *) ctrl_data_in->data.poke_args.addr) = (uint32_t)ctrl_data_in->data.poke_args.data;
            break;

        case sizeof(uint16_t):
            *((uint16_t *) ctrl_data_in->data.poke_args.addr) = (uint16_t)ctrl_data_in->data.poke_args.data;
            break;

        case sizeof(uint8_t):
            *((uint8_t *) ctrl_data_in->data.poke_args.addr) = (uint8_t)ctrl_data_in->data.poke_args.data;
            break;

        }
        ctrl_data_out.id = USRP2_CTRL_ID_OMG_POKED_REGISTER_SO_BAD_DUDE;
        send_udp_pkt(USRP2_UDP_CTRL_PORT, src, &ctrl_data_out, sizeof(ctrl_data_out));
        break;

    case USRP2_CTRL_ID_PEEK_AT_THIS_REGISTER_FOR_ME_BRO:
        switch(ctrl_data_in->data.poke_args.num_bytes){
        case sizeof(uint64_t):
            ctrl_data_out.data.poke_args.datahi = *((uint32_t *) ctrl_data_in->data.poke_args.addrhi);
            //continue to uint32_t for low addr:

        case sizeof(uint32_t):
            ctrl_data_out.data.poke_args.data = *((uint32_t *) ctrl_data_in->data.poke_args.addr);
            break;

        case sizeof(uint16_t):
            ctrl_data_out.data.poke_args.data = *((uint16_t *) ctrl_data_in->data.poke_args.addr);
            break;

        case sizeof(uint8_t):
            ctrl_data_out.data.poke_args.data = *((uint8_t *) ctrl_data_in->data.poke_args.addr);
            break;

        }
        ctrl_data_out.id = USRP2_CTRL_ID_WOAH_I_DEFINITELY_PEEKED_IT_DUDE;
        send_udp_pkt(USRP2_UDP_CTRL_PORT, src, &ctrl_data_out, sizeof(ctrl_data_out));
        break;

    default:
        ctrl_data_out.id = USRP2_CTRL_ID_HUH_WHAT;
        send_udp_pkt(USRP2_UDP_CTRL_PORT, src, &ctrl_data_out, sizeof(ctrl_data_out));
    }
    
}

/*
 * Called when an ethernet packet is received.
 * Return true if we handled it here, otherwise
 * it'll be passed on to the DSP Tx pipe
 */
static bool
eth_pkt_inspector(dbsm_t *sm, int bufno)
{
  //point me to the ethernet frame
  uint32_t *buff = (uint32_t *)buffer_ram(bufno);

  //treat this as fast-path data?
  // We have to do this operation as fast as possible.
  // Therefore, we do not check all the headers,
  // just check that the udp port matches
  // and that the vrt header is non zero.
  // In the future, a hardware state machine will do this...
  if ( //warning! magic numbers approaching....
      (((buff + ((2 + 14 + 20)/sizeof(uint32_t)))[0] & 0xffff) == USRP2_UDP_DATA_PORT) &&
      ((buff + ((2 + 14 + 20 + 8)/sizeof(uint32_t)))[0] != USRP2_INVALID_VRT_HEADER)
  ) return false;

  //test if its an ip recovery packet
  typedef struct{
      padded_eth_hdr_t eth_hdr;
      char code[4];
      union {
        struct ip_addr ip_addr;
      } data;
  }recovery_packet_t;
  recovery_packet_t *recovery_packet = (recovery_packet_t *)buff;
  if (recovery_packet->eth_hdr.ethertype == 0xbeee && strncmp(recovery_packet->code, "addr", 4) == 0){
      printf("Got ip recovery packet: "); print_ip_addr(&recovery_packet->data.ip_addr); newline();
      set_ip_addr(&recovery_packet->data.ip_addr);
      return true;
  }

  //pass it to the slow-path handler
  size_t len = buffer_pool_status->last_line[bufno] - 3;
  handle_eth_packet(buff, len);
  return true;
}

//------------------------------------------------------------------

/*
 * 1's complement sum for IP and UDP headers
 *
 * init chksum to zero to start.
 */
static unsigned int
CHKSUM(unsigned int x, unsigned int *chksum)
{
  *chksum += x;
  *chksum = (*chksum & 0xffff) + (*chksum>>16);
  *chksum = (*chksum & 0xffff) + (*chksum>>16);
  return x;
}

/*
 * Called when eth phy state changes (w/ interrupts disabled)
 */
volatile bool link_is_up = false;	// eth handler sets this
void
link_changed_callback(int speed)
{
  link_is_up = speed != 0;
  hal_set_leds(link_is_up ? LED_RJ45 : 0x0, LED_RJ45);
  printf("\neth link changed: speed = %d\n", speed);
}

static void setup_network(void){

  //setup ethernet header machine
  sr_udp_sm->eth_hdr.mac_dst_0_1 = (fp_mac_addr_dst.addr[0] << 8) | fp_mac_addr_dst.addr[1];
  sr_udp_sm->eth_hdr.mac_dst_2_3 = (fp_mac_addr_dst.addr[2] << 8) | fp_mac_addr_dst.addr[3];
  sr_udp_sm->eth_hdr.mac_dst_4_5 = (fp_mac_addr_dst.addr[4] << 8) | fp_mac_addr_dst.addr[5];
  sr_udp_sm->eth_hdr.mac_src_0_1 = (fp_mac_addr_src.addr[0] << 8) | fp_mac_addr_src.addr[1];
  sr_udp_sm->eth_hdr.mac_src_2_3 = (fp_mac_addr_src.addr[2] << 8) | fp_mac_addr_src.addr[3];
  sr_udp_sm->eth_hdr.mac_src_4_5 = (fp_mac_addr_src.addr[4] << 8) | fp_mac_addr_src.addr[5];
  sr_udp_sm->eth_hdr.ether_type = ETHERTYPE_IPV4;

  //setup ip header machine
  unsigned int chksum = 0;
  sr_udp_sm->ip_hdr.ver_ihl_tos = CHKSUM(0x4500, &chksum);    // IPV4,  5 words of header (20 bytes), TOS=0
  sr_udp_sm->ip_hdr.total_length = UDP_SM_INS_IP_LEN;             // Don't checksum this line in SW
  sr_udp_sm->ip_hdr.identification = CHKSUM(0x0000, &chksum);    // ID
  sr_udp_sm->ip_hdr.flags_frag_off = CHKSUM(0x4000, &chksum);    // don't fragment
  sr_udp_sm->ip_hdr.ttl_proto = CHKSUM(0x2011, &chksum);    // TTL=32, protocol = UDP (17 decimal)
  //sr_udp_sm->ip_hdr.checksum .... filled in below
  uint32_t src_ip_addr = fp_socket_src.addr.addr;
  uint32_t dst_ip_addr = fp_socket_dst.addr.addr;
  sr_udp_sm->ip_hdr.src_addr_high = CHKSUM(src_ip_addr >> 16, &chksum);    // IP src high
  sr_udp_sm->ip_hdr.src_addr_low = CHKSUM(src_ip_addr & 0xffff, &chksum); // IP src low
  sr_udp_sm->ip_hdr.dst_addr_high = CHKSUM(dst_ip_addr >> 16, &chksum);    // IP dst high
  sr_udp_sm->ip_hdr.dst_addr_low = CHKSUM(dst_ip_addr & 0xffff, &chksum); // IP dst low
  sr_udp_sm->ip_hdr.checksum = UDP_SM_INS_IP_HDR_CHKSUM | (chksum & 0xffff);

  //setup the udp header machine
  sr_udp_sm->udp_hdr.src_port = fp_socket_src.port;
  sr_udp_sm->udp_hdr.dst_port = fp_socket_dst.port;
  sr_udp_sm->udp_hdr.length = UDP_SM_INS_UDP_LEN;
  sr_udp_sm->udp_hdr.checksum = UDP_SM_LAST_WORD;		// zero UDP checksum
}

inline static void
buffer_irq_handler(unsigned irq)
{
  uint32_t  status = buffer_pool_status->status;

  dbsm_process_status(&dsp_tx_sm, status);
  dbsm_process_status(&dsp_rx_sm, status);
}

int
main(void)
{
  u2_init();

  putstr("\nTxRx-NEWETH\n");
  print_mac_addr(ethernet_mac_addr()->addr);
  newline();
  print_ip_addr(get_ip_addr()); newline();
  printf("FPGA compatibility number: %d\n", USRP2_FPGA_COMPAT_NUM);
  printf("Firmware compatibility number: %d\n", USRP2_FW_COMPAT_NUM);

  ethernet_register_link_changed_callback(link_changed_callback);
  ethernet_init();

  register_mac_addr(ethernet_mac_addr());
  register_ip_addr(get_ip_addr());

  register_udp_listener(USRP2_UDP_CTRL_PORT, handle_udp_ctrl_packet);
  register_udp_listener(USRP2_UDP_DATA_PORT, handle_udp_data_packet);

  // initialize double buffering state machine for ethernet -> DSP Tx

  dbsm_init(&dsp_tx_sm, DSP_TX_BUF_0,
	    &dsp_tx_recv_args, &dsp_tx_send_args,
	    eth_pkt_inspector);


  // initialize double buffering state machine for DSP RX -> Ethernet

    dbsm_init(&dsp_rx_sm, DSP_RX_BUF_0,
	      &dsp_rx_recv_args, &dsp_rx_send_args,
	      dbsm_rx_inspector);

  sr_tx_ctrl->clear_state = 1;
  bp_clear_buf(DSP_TX_BUF_0);
  bp_clear_buf(DSP_TX_BUF_1);

  // kick off the state machine
  dbsm_start(&dsp_tx_sm);

  //int which = 0;

  while(1){
    // hal_gpio_write(GPIO_TX_BANK, which, 0x8000);
    // which ^= 0x8000;

    buffer_irq_handler(0);

    if(i2c_done) {
      i2c_done = false;
      send_udp_pkt(USRP2_UDP_CTRL_PORT, i2c_src, &ctrl_data_out, sizeof(ctrl_data_out));
      //printf("Sending UDP packet from main loop for I2C...\n");
    }

    if(spi_done) {
      spi_done = false;
      send_udp_pkt(USRP2_UDP_CTRL_PORT, spi_src, &ctrl_data_out, sizeof(ctrl_data_out));
    }

    int pending = pic_regs->pending;		// poll for under or overrun

    if (pending & PIC_UNDERRUN_INT){
      //dbsm_handle_tx_underrun(&dsp_tx_sm);
      pic_regs->pending = PIC_UNDERRUN_INT;	// clear interrupt
      putchar('U');
    }

    if (pending & PIC_OVERRUN_INT){
      dbsm_handle_rx_overrun(&dsp_rx_sm);
      pic_regs->pending = PIC_OVERRUN_INT;	// clear pending interrupt

      // FIXME Figure out how to handle this robustly.
      // Any buffers that are emptying should be allowed to drain...

      putchar('O');
    }
  }
}
