//
// Copyright 2014 Per Vices Corporation
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

#ifndef INCLUDED_CRIMSON_FW_COMMON_H
#define INCLUDED_CRIMSON_FW_COMMON_H

#include <stdint.h>

/*!
 * Structs and constants for crimson communication.
 * This header is shared by the firmware and host code.
 * Therefore, this header may only contain valid C code.
 */
#ifdef __cplusplus
extern "C" {
#endif

#define CMD_SUCCESS 	'0'
#define CMD_ERROR	'1'

#define CRIMSON_FW_COMPAT_MAJOR 1
#define CRIMSON_FW_COMPAT_MINOR 0
#define CRIMSON_FPGA_COMPAT_MAJOR 1

#define CRIMSON_FW_NUM_BYTES (1 << 15) //64k
#define CRIMSON_FW_COMMS_MTU (1 << 13) //8k

#define CRIMSON_FW_COMMS_UDP_PORT 	 42799
#define CRIMSON_FLOW_CNTRL_UDP_PORT 	 42808

#define CRIMSON_DEFAULT_MAC_ADDR_0         {0x00, 0x50, 0xC2, 0x85, 0x3f, 0xff}
#define CRIMSON_DEFAULT_MAC_ADDR_1         {0x00, 0x50, 0xC2, 0x85, 0x3f, 0x33}

#define CRIMSON_DEFAULT_GATEWAY            (192 << 24 | 168 << 16 | 10  << 8  | 1 << 0)

#define CRIMSON_DEFAULT_IP_ETH0_1G         (192 << 24 | 168 << 16 | 10  << 8  | 2 << 0)
#define CRIMSON_DEFAULT_IP_ETH0_10G        (192 << 24 | 168 << 16 | 30  << 8  | 2 << 0)
#define CRIMSON_DEFAULT_IP_ETH1_10G        (192 << 24 | 168 << 16 | 40  << 8  | 2 << 0)

#define CRIMSON_DEFAULT_NETMASK_ETH0_1G    (255 << 24 | 255 << 16 | 255  << 8  | 0 << 0)
#define CRIMSON_DEFAULT_NETMASK_ETH0_10G   (255 << 24 | 255 << 16 | 255  << 8  | 0 << 0)
#define CRIMSON_DEFAULT_NETMASK_ETH1_10G   (255 << 24 | 255 << 16 | 255  << 8  | 0 << 0)

#define CRIMSON_MTU_SIZE     1472   // 1500 MTU - 20 IPV4 header - 8 UDP header

#define CRIMSON_RX_CHANNELS 4
#define CRIMSON_TX_CHANNELS 4

#define CRIMSON_FW_COMMS_FLAGS_ACK        (1 << 0)
#define CRIMSON_FW_COMMS_FLAGS_ERROR      (1 << 1)
#define CRIMSON_FW_COMMS_FLAGS_POKE32     (1 << 2)
#define CRIMSON_FW_COMMS_FLAGS_PEEK32     (1 << 3)

// Crimson MTU Size
#define CRIMSON_MAX_MTU		3936

// Crimson Flowcontrol Update Per Second
#define CRIMSON_UPDATE_PER_SEC	20

// Crimson Buffer Size
#define CRIMSON_BUFF_SIZE	32766

// Crimson RF Settings
#define CRIMSON_RF_TX_GAIN_RANGE_START	0.0
#define CRIMSON_RF_TX_GAIN_RANGE_STOP	28.0
#define CRIMSON_RF_TX_GAIN_RANGE_STEP	1.0

// Ranges from, 0dB to 95dB
#define CRIMSON_RF_RX_GAIN_RANGE_START	0.0
#define CRIMSON_RF_RX_GAIN_RANGE_STOP	95.0
#define CRIMSON_RF_RX_GAIN_RANGE_STEP	1.0

// Crimson Rate Settings
#define CRIMSON_RATE_RANGE_START	629425.048828
#define CRIMSON_RATE_RANGE_STOP		161132812.5
#define CRIMSON_RATE_RANGE_STEP		1.0

// Crimson Freq Settings
#define CRIMSON_FREQ_RANGE_START	50.0
#define CRIMSON_FREQ_RANGE_STOP		6000000000.0
#define CRIMSON_FREQ_RANGE_STEP		1.0

// Crimson DSP Freq Settings
#define CRIMSON_DSP_FREQ_RANGE_START	0.0
#define CRIMSON_DSP_FREQ_RANGE_STOP	161000000.0
#define CRIMSON_DSP_FREQ_RANGE_STEP	1.0

// Crimson Clk Settings
#define CRIMSON_MASTER_CLOCK_RATE	322265625
#define CRIMSON_EXT_CLK_RATE		10000000	// only 10 MHz input sources allowed

// Crimson VITA settings
#define CRIMSON_VITA_HDR_TYPE	0x1

#define CRIMSON_VITA_HDR	
#define CRIMSON_VITA_STREAM	

#define CRIMSON_VITA_TLR_EN	0xe00
#define CRIMSON_VITA_TLR_IND	0x0
#define CRIMSON_VITA_TLR_E	0x0
#define CRIMSON_VITA_TLR_PCKCNT	0x0
#define CRIMSON_VITA_TLR	((CRIMSON_VITA_TLR_EN     << 20)|\
				( CRIMSON_VITA_TLR_IND    << 8) |\
				( CRIMSON_VITA_TLR_E      << 7) |\
				( CRIMSON_VITA_TLR_PCKCNT << 0) )

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_CRIMSON_FW_COMMON_H */
