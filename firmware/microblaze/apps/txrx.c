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

#define DEBUG_MODE 0 //0 for normal operation

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
#include <vrt/bits.h>
#include "usrp2/fw_common.h"
#include <db.h>
#include <i2c.h>
#include <lsdac.h>
#include <lsadc.h>
#include <ethertype.h>
#include <arp_cache.h>

#define FW_SETS_SEQNO	1	// define to 0 or 1 (FIXME must be 1 for now)

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

// DSP Rx writes ethernet header words
#define DSP_RX_FIRST_LINE 1 //1 = control stuff to udp sm

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

//controls continuous streaming...
static bool   auto_reload_command = false;
static size_t streaming_items_per_frame = 0;
static int    streaming_frame_count = 0;
#define FRAMES_PER_CMD	2

static void setup_network(void);
static void setup_vrt(void);

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
    //store the 2nd word as the following:
    streaming_items_per_frame = ((uint32_t *)payload)[1];

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
    setup_vrt();

    // kick off the state machine
    dbsm_start(&dsp_rx_sm);

}

static void inline issue_stream_command(size_t nsamps, bool now, bool chain, uint32_t secs, uint32_t ticks, bool start){
    //printf("Stream cmd: nsamps %d, now %d, chain %d, secs %u, ticks %u\n", (int)nsamps, now, chain, secs, ticks);
    sr_rx_ctrl->cmd = MK_RX_CMD(nsamps, now, chain);

    if (start) dbsm_start(&dsp_rx_sm);

    sr_rx_ctrl->time_secs = secs;
    sr_rx_ctrl->time_ticks = ticks;	// enqueue command
}

#define OTW_GPIO_BANK_TO_NUM(bank) \
    (((bank) == USRP2_DIR_RX)? (GPIO_RX_BANK) : (GPIO_TX_BANK))

void handle_udp_ctrl_packet(
    struct socket_address src, struct socket_address dst,
    unsigned char *payload, int payload_len
){
    //printf("Got ctrl packet #words: %d\n", (int)payload_len);
    if (payload_len < sizeof(usrp2_ctrl_data_t)){
        //TODO send err packet
        return;
    }

    //setup the input and output data
    usrp2_ctrl_data_t *ctrl_data_in = (usrp2_ctrl_data_t *)payload;
    usrp2_ctrl_data_t ctrl_data_out = {
        .id=USRP2_CTRL_ID_HUH_WHAT,
        .seq=ctrl_data_in->seq
    };

    //handle the data based on the id
    switch(ctrl_data_in->id){

    /*******************************************************************
     * Addressing
     ******************************************************************/
    case USRP2_CTRL_ID_GIVE_ME_YOUR_IP_ADDR_BRO:
        ctrl_data_out.id = USRP2_CTRL_ID_THIS_IS_MY_IP_ADDR_DUDE;
        memcpy(&ctrl_data_out.data.ip_addr, get_ip_addr(), sizeof(struct ip_addr));
        break;

    case USRP2_CTRL_ID_HERE_IS_A_NEW_IP_ADDR_BRO:
        ctrl_data_out.id = USRP2_CTRL_ID_THIS_IS_MY_IP_ADDR_DUDE;
        set_ip_addr((struct ip_addr *)&ctrl_data_in->data.ip_addr);
        memcpy(&ctrl_data_out.data.ip_addr, get_ip_addr(), sizeof(struct ip_addr));
        break;

    case USRP2_CTRL_ID_GIVE_ME_YOUR_MAC_ADDR_BRO:
        ctrl_data_out.id = USRP2_CTRL_ID_THIS_IS_MY_MAC_ADDR_DUDE;
        memcpy(&ctrl_data_out.data.mac_addr, ethernet_mac_addr(), sizeof(eth_mac_addr_t));
        break;

    case USRP2_CTRL_ID_HERE_IS_A_NEW_MAC_ADDR_BRO:
        ctrl_data_out.id = USRP2_CTRL_ID_THIS_IS_MY_MAC_ADDR_DUDE;
        ethernet_set_mac_addr((eth_mac_addr_t *)&ctrl_data_in->data.mac_addr);
        memcpy(&ctrl_data_out.data.mac_addr, ethernet_mac_addr(), sizeof(eth_mac_addr_t));
        break;

    case USRP2_CTRL_ID_GIVE_ME_YOUR_DBOARD_IDS_BRO:
        ctrl_data_out.id = USRP2_CTRL_ID_THESE_ARE_MY_DBOARD_IDS_DUDE;
        ctrl_data_out.data.dboard_ids.tx_id = read_dboard_eeprom(I2C_ADDR_TX_A);
        ctrl_data_out.data.dboard_ids.rx_id = read_dboard_eeprom(I2C_ADDR_RX_A);
        break;

    /*******************************************************************
     * Clock Config
     ******************************************************************/
    case USRP2_CTRL_ID_HERES_A_NEW_CLOCK_CONFIG_BRO:
        //TODO handle MC_PROVIDE_CLK_TO_MIMO when we do MIMO setup
        ctrl_data_out.id = USRP2_CTRL_ID_GOT_THE_NEW_CLOCK_CONFIG_DUDE;

        //handle the 10 mhz ref source
        uint32_t ref_flags = 0;
        switch(ctrl_data_out.data.clock_config.ref_source){
        case USRP2_REF_SOURCE_INT:
            ref_flags = MC_WE_DONT_LOCK; break;
        case USRP2_REF_SOURCE_SMA:
            ref_flags = MC_WE_LOCK_TO_SMA; break;
        case USRP2_REF_SOURCE_MIMO:
            ref_flags = MC_WE_LOCK_TO_MIMO; break;
        }
        clocks_mimo_config(ref_flags & MC_REF_CLK_MASK);

        //handle the pps config
        uint32_t pps_flags = 0;

        //fill in the pps polarity flags
        switch(ctrl_data_out.data.clock_config.pps_polarity){
        case USRP2_PPS_POLARITY_POS:
            pps_flags |= 0x01 << 0; break;
        case USRP2_PPS_POLARITY_NEG:
            pps_flags |= 0x00 << 0; break;
        }

        //fill in the pps source flags
        switch(ctrl_data_out.data.clock_config.pps_source){
        case USRP2_PPS_SOURCE_SMA:
            pps_flags |= 0x00 << 1; break;
        case USRP2_PPS_SOURCE_MIMO:
            pps_flags |= 0x01 << 1; break;
        }
        sr_time64->flags = pps_flags;

        break;

    /*******************************************************************
     * GPIO
     ******************************************************************/
    case USRP2_CTRL_ID_USE_THESE_GPIO_DDR_SETTINGS_BRO:
        if (!DEBUG_MODE) hal_gpio_set_ddr(
            OTW_GPIO_BANK_TO_NUM(ctrl_data_in->data.gpio_config.bank),
            ctrl_data_in->data.gpio_config.value,
            ctrl_data_in->data.gpio_config.mask
        );
        ctrl_data_out.id = USRP2_CTRL_ID_GOT_THE_GPIO_DDR_SETTINGS_DUDE;
        break;

    case USRP2_CTRL_ID_SET_YOUR_GPIO_PIN_OUTS_BRO:
        if (!DEBUG_MODE) hal_gpio_write(
            OTW_GPIO_BANK_TO_NUM(ctrl_data_in->data.gpio_config.bank),
            ctrl_data_in->data.gpio_config.value,
            ctrl_data_in->data.gpio_config.mask
        );
        ctrl_data_out.id = USRP2_CTRL_ID_I_SET_THE_GPIO_PIN_OUTS_DUDE;
        break;

    case USRP2_CTRL_ID_GIVE_ME_YOUR_GPIO_PIN_VALS_BRO:
        ctrl_data_out.data.gpio_config.value = hal_gpio_read(
            OTW_GPIO_BANK_TO_NUM(ctrl_data_in->data.gpio_config.bank)
        );
        ctrl_data_out.id = USRP2_CTRL_ID_HERE_IS_YOUR_GPIO_PIN_VALS_DUDE;
        break;

    case USRP2_CTRL_ID_USE_THESE_ATR_SETTINGS_BRO:{
            //setup the atr registers for this bank
            int bank = OTW_GPIO_BANK_TO_NUM(ctrl_data_in->data.atr_config.bank);
            if (!DEBUG_MODE) set_atr_regs(
                bank,
                ctrl_data_in->data.atr_config.rx_value,
                ctrl_data_in->data.atr_config.tx_value
            );

            //setup the sels based on the atr config mask
            int mask = ctrl_data_in->data.atr_config.mask;
            for (int i = 0; i < 16; i++){
                // set to either GPIO_SEL_SW or GPIO_SEL_ATR
                if (!DEBUG_MODE) hal_gpio_set_sel(bank, i, (mask & (1 << i)) ? 'a' : 's');
            }
            ctrl_data_out.id = USRP2_CTRL_ID_GOT_THE_ATR_SETTINGS_DUDE;
        }
        break;

    /*******************************************************************
     * SPI
     ******************************************************************/
    case USRP2_CTRL_ID_TRANSACT_ME_SOME_SPI_BRO:{
            uint8_t num_bytes = ctrl_data_in->data.spi_args.bytes;

            //load the data from the array of bytes
            uint32_t data = 0x0;
            for (size_t i = 0; i < num_bytes; i++){
                data = (data << 8) | ctrl_data_in->data.spi_args.data[i];
            }

            //transact
            uint32_t result = spi_transact(
                (ctrl_data_in->data.spi_args.readback == 0)? SPI_TXONLY : SPI_TXRX,
                (ctrl_data_in->data.spi_args.dev == USRP2_DIR_RX)? SPI_SS_RX_DB : SPI_SS_TX_DB,
                data, num_bytes*8, //length in bits
                (ctrl_data_in->data.spi_args.push == USRP2_CLK_EDGE_RISE)? SPIF_PUSH_RISE : SPIF_PUSH_FALL |
                (ctrl_data_in->data.spi_args.latch == USRP2_CLK_EDGE_RISE)? SPIF_LATCH_RISE : SPIF_LATCH_FALL
            );

            //load the result into the array of bytes
            for (size_t i = 0; i < num_bytes; i++){
                uint8_t byte_shift = num_bytes - i - 1;
                ctrl_data_out.data.spi_args.data[i] = (result >> (byte_shift*8)) & 0xff;
            }
            ctrl_data_out.data.spi_args.bytes = num_bytes;
            ctrl_data_out.id = USRP2_CTRL_ID_OMG_TRANSACTED_SPI_DUDE;
        }
        break;

    /*******************************************************************
     * I2C
     ******************************************************************/
    case USRP2_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO:{
            uint8_t num_bytes = ctrl_data_in->data.i2c_args.bytes;
            i2c_read(
                ctrl_data_in->data.i2c_args.addr,
                ctrl_data_out.data.i2c_args.data,
                num_bytes
            );
            ctrl_data_out.id = USRP2_CTRL_ID_HERES_THE_I2C_DATA_DUDE;
            ctrl_data_out.data.i2c_args.bytes = num_bytes;
        }
        break;

    case USRP2_CTRL_ID_WRITE_THESE_I2C_VALUES_BRO:{
            uint8_t num_bytes = ctrl_data_in->data.i2c_args.bytes;
            i2c_write(
                ctrl_data_in->data.i2c_args.addr,
                ctrl_data_in->data.i2c_args.data,
                num_bytes
            );
            ctrl_data_out.id = USRP2_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE;
            ctrl_data_out.data.i2c_args.bytes = num_bytes;
        }
        break;

    /*******************************************************************
     * AUX DAC/ADC
     ******************************************************************/
    case USRP2_CTRL_ID_WRITE_THIS_TO_THE_AUX_DAC_BRO:
        if (ctrl_data_in->data.aux_args.dir == USRP2_DIR_RX){
            lsdac_write_rx(
                ctrl_data_in->data.aux_args.which,
                ctrl_data_in->data.aux_args.value
            );
        }

        if (ctrl_data_in->data.aux_args.dir == USRP2_DIR_TX){
            lsdac_write_tx(
                ctrl_data_in->data.aux_args.which,
                ctrl_data_in->data.aux_args.value
            );
        }

        ctrl_data_out.id = USRP2_CTRL_ID_DONE_WITH_THAT_AUX_DAC_DUDE;
        break;

    case USRP2_CTRL_ID_READ_FROM_THIS_AUX_ADC_BRO:
        if (ctrl_data_in->data.aux_args.dir == USRP2_DIR_RX){
            ctrl_data_out.data.aux_args.value = lsadc_read_rx(
                ctrl_data_in->data.aux_args.which
            );
        }

        if (ctrl_data_in->data.aux_args.dir == USRP2_DIR_TX){
            ctrl_data_out.data.aux_args.value = lsadc_read_tx(
                ctrl_data_in->data.aux_args.which
            );
        }

        ctrl_data_out.id = USRP2_CTRL_ID_DONE_WITH_THAT_AUX_ADC_DUDE;
        break;

    /*******************************************************************
     * DDC
     ******************************************************************/
    case USRP2_CTRL_ID_SETUP_THIS_DDC_FOR_ME_BRO:
        dsp_rx_regs->freq = ctrl_data_in->data.ddc_args.freq_word;
        dsp_rx_regs->scale_iq = ctrl_data_in->data.ddc_args.scale_iq;

        //setup the interp and half band filters
        {
            uint32_t decim = ctrl_data_in->data.ddc_args.decim;
            uint32_t hb1 = 0;
            uint32_t hb2 = 0;
            if (!(decim & 1)){
              hb2 = 1;
              decim = decim >> 1;
            }
            if (!(decim & 1)){
              hb1 = 1;
              decim = decim >> 1;
            }
            uint32_t decim_word = (hb1<<9) | (hb2<<8) | decim;
            dsp_rx_regs->decim_rate = decim_word;
            printf("Decim: %d, register %d\n", ctrl_data_in->data.ddc_args.decim, decim_word);
        }

        ctrl_data_out.id = USRP2_CTRL_ID_TOTALLY_SETUP_THE_DDC_DUDE;
        break;

    /*******************************************************************
     * Streaming
     ******************************************************************/
    case USRP2_CTRL_ID_SEND_STREAM_COMMAND_FOR_ME_BRO:{

        //issue two commands and set the auto-reload flag
        if (ctrl_data_in->data.stream_cmd.continuous){
            printf("Setting up continuous streaming...\n");
            auto_reload_command = true;
            streaming_frame_count = FRAMES_PER_CMD;

            issue_stream_command(
                streaming_items_per_frame * FRAMES_PER_CMD,
                (ctrl_data_in->data.stream_cmd.now == 0)? false : true, //now
                true, //chain
                ctrl_data_in->data.stream_cmd.secs,
                ctrl_data_in->data.stream_cmd.ticks,
                true //start
            );

            issue_stream_command(
                streaming_items_per_frame * FRAMES_PER_CMD,
                true, //now
                true, //chain
                0, 0, //time does not matter
                false
            );

        }

        //issue regular stream commands (split commands if too large)
        else{
            auto_reload_command = false;
            size_t num_samps = ctrl_data_in->data.stream_cmd.num_samps;
            if (num_samps == 0) num_samps = 1; //FIXME hack, zero is used when stopping continuous streaming but it somehow makes it inifinite

            bool chain = num_samps > MAX_SAMPLES_PER_CMD;
            issue_stream_command(
                (chain)? streaming_items_per_frame : num_samps, //nsamps
                (ctrl_data_in->data.stream_cmd.now == 0)? false : true, //now
                chain, //chain
                ctrl_data_in->data.stream_cmd.secs,
                ctrl_data_in->data.stream_cmd.ticks,
                false
            );

            //handle rest of the samples that did not fit into one cmd
            while(chain){
                num_samps -= MAX_SAMPLES_PER_CMD;
                chain = num_samps > MAX_SAMPLES_PER_CMD;
                issue_stream_command(
                    (chain)? streaming_items_per_frame : num_samps, //nsamps
                    true, //now
                    chain, //chain
                    0, 0, //time does not matter
                    false
                );
            }
        }
        ctrl_data_out.id = USRP2_CTRL_ID_GOT_THAT_STREAM_COMMAND_DUDE;
        break;
    }

    /*******************************************************************
     * DUC
     ******************************************************************/
    case USRP2_CTRL_ID_SETUP_THIS_DUC_FOR_ME_BRO:
        dsp_tx_regs->freq = ctrl_data_in->data.duc_args.freq_word;
        dsp_tx_regs->scale_iq = ctrl_data_in->data.duc_args.scale_iq;

        //setup the interp and half band filters
        {
            uint32_t interp = ctrl_data_in->data.duc_args.interp;
            uint32_t hb1 = 0;
            uint32_t hb2 = 0;
            if (!(interp & 1)){
              hb2 = 1;
              interp = interp >> 1;
            }
            if (!(interp & 1)){
              hb1 = 1;
              interp = interp >> 1;
            }
            uint32_t interp_word = (hb1<<9) | (hb2<<8) | interp;
            dsp_tx_regs->interp_rate = interp_word;
            printf("Interp: %d, register %d\n", ctrl_data_in->data.duc_args.interp, interp_word);
        }

        ctrl_data_out.id = USRP2_CTRL_ID_TOTALLY_SETUP_THE_DUC_DUDE;
        break;

    /*******************************************************************
     * Time Config
     ******************************************************************/
    case USRP2_CTRL_ID_GOT_A_NEW_TIME_FOR_YOU_BRO:
        sr_time64->imm = (ctrl_data_in->data.time_args.now == 0)? 0 : 1;
        sr_time64->ticks = ctrl_data_in->data.time_args.ticks;
        sr_time64->secs = ctrl_data_in->data.time_args.secs; //set this last to latch the regs
        ctrl_data_out.id = USRP2_CTRL_ID_SWEET_I_GOT_THAT_TIME_DUDE;
        break;

    /*******************************************************************
     * MUX Config
     ******************************************************************/
    case USRP2_CTRL_ID_UPDATE_THOSE_MUX_SETTINGS_BRO:
        dsp_rx_regs->rx_mux = ctrl_data_in->data.mux_args.rx_mux;
        dsp_tx_regs->tx_mux = ctrl_data_in->data.mux_args.tx_mux;
        ctrl_data_out.id = USRP2_CTRL_ID_UPDATED_THE_MUX_SETTINGS_DUDE;
        break;

    default:
        ctrl_data_out.id = USRP2_CTRL_ID_HUH_WHAT;

    }
    send_udp_pkt(USRP2_UDP_CTRL_PORT, src, &ctrl_data_out, sizeof(ctrl_data_out));
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

static size_t get_vrt_packet_words(void){
    return streaming_items_per_frame + \
        USRP2_HOST_RX_VRT_HEADER_WORDS32 + \
        USRP2_HOST_RX_VRT_TRAILER_WORDS32;
}

static bool vrt_has_trailer(void){
    return USRP2_HOST_RX_VRT_TRAILER_WORDS32 > 0;
}

static void setup_vrt(void){
  // setup RX DSP regs
  sr_rx_ctrl->nsamples_per_pkt = streaming_items_per_frame;
  sr_rx_ctrl->nchannels = 1;
  sr_rx_ctrl->clear_overrun = 1;			// reset
  sr_rx_ctrl->vrt_header = (0
     | VRTH_PT_IF_DATA_WITH_SID
     | (vrt_has_trailer()? VRTH_HAS_TRAILER : 0)
     | VRTH_TSI_OTHER
     | VRTH_TSF_SAMPLE_CNT
  );
  sr_rx_ctrl->vrt_stream_id = 0;
  sr_rx_ctrl->vrt_trailer = 0;
}

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
  //setup header and load into two buffers
  struct {
    uint32_t ctrl_word;
  } mem _AL4;

  memset(&mem, 0, sizeof(mem));
  printf("items per frame: %d\n", (int)streaming_items_per_frame);
  printf("words in a vrt packet %d\n", (int)get_vrt_packet_words());
  mem.ctrl_word = get_vrt_packet_words()*sizeof(uint32_t) | 1 << 16;

  memcpy_wa(buffer_ram(DSP_RX_BUF_0), &mem, sizeof(mem));
  memcpy_wa(buffer_ram(DSP_RX_BUF_1), &mem, sizeof(mem));

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

#if (FW_SETS_SEQNO)
/*
 * Debugging ONLY.  This will be handled by the tx_protocol_engine.
 *
 * This is called when the DSP Rx chain has filled in a packet.
 * We set and increment the seqno, then return false, indicating
 * that we didn't handle the packet.  A bit of a kludge
 * but it should work.
 */
bool 
fw_sets_seqno_inspector(dbsm_t *sm, int buf_this)	// returns false
{
  // queue up another rx command when required
  if (auto_reload_command && --streaming_frame_count == 0){
    streaming_frame_count = FRAMES_PER_CMD;
    sr_rx_ctrl->time_secs = 0;
    sr_rx_ctrl->time_ticks = 0; //enqueue last command
  }

  return false;		// we didn't handle the packet
}
#endif


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

  ethernet_register_link_changed_callback(link_changed_callback);
  ethernet_init();

  register_mac_addr(ethernet_mac_addr());
  register_ip_addr(get_ip_addr());

  register_udp_listener(USRP2_UDP_CTRL_PORT, handle_udp_ctrl_packet);
  register_udp_listener(USRP2_UDP_DATA_PORT, handle_udp_data_packet);

#if 0
  // make bit 15 of Tx gpio's be a s/w output
  hal_gpio_set_sel(GPIO_TX_BANK, 15, 's');
  hal_gpio_set_ddr(GPIO_TX_BANK, 0x8000, 0x8000);
#endif

  output_regs->debug_mux_ctrl = 1;
#if DEBUG_MODE
  hal_gpio_set_sels(GPIO_TX_BANK, "0000000000000000");
  hal_gpio_set_sels(GPIO_RX_BANK, "0000000000000000");
  hal_gpio_set_ddr(GPIO_TX_BANK, 0xffff, 0xffff);
  hal_gpio_set_ddr(GPIO_RX_BANK, 0xffff, 0xffff);
#endif


  // initialize double buffering state machine for ethernet -> DSP Tx

  dbsm_init(&dsp_tx_sm, DSP_TX_BUF_0,
	    &dsp_tx_recv_args, &dsp_tx_send_args,
	    eth_pkt_inspector);


  // initialize double buffering state machine for DSP RX -> Ethernet

  if (FW_SETS_SEQNO){
    dbsm_init(&dsp_rx_sm, DSP_RX_BUF_0,
	      &dsp_rx_recv_args, &dsp_rx_send_args,
	      fw_sets_seqno_inspector);
  }
  else {
    dbsm_init(&dsp_rx_sm, DSP_RX_BUF_0,
	      &dsp_rx_recv_args, &dsp_rx_send_args,
	      dbsm_nop_inspector);
  }

  // tell app_common that this dbsm could be sending to the ethernet
  ac_could_be_sending_to_eth = &dsp_rx_sm;

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

    int pending = pic_regs->pending;		// poll for under or overrun

    if (pending & PIC_UNDERRUN_INT){
      dbsm_handle_tx_underrun(&dsp_tx_sm);
      pic_regs->pending = PIC_UNDERRUN_INT;	// clear interrupt
      putchar('U');
    }

    if (pending & PIC_OVERRUN_INT){
      dbsm_handle_rx_overrun(&dsp_rx_sm);
      pic_regs->pending = PIC_OVERRUN_INT;	// clear pending interrupt

      // FIXME Figure out how to handle this robustly.
      // Any buffers that are emptying should be allowed to drain...

      if (auto_reload_command){
	// restart_streaming();
	// FIXME report error
      }
      else {
	// FIXME report error
      }
      putchar('O');
    }
  }
}
