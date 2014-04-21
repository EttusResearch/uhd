//
// Copyright 2013-2014 Ettus Research LLC
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

#ifndef _UHD_DUMP_H_
#define _UHD_DUMP_H_

#define FALSE 0
#define TRUE 1
#define UNKNOWN 2

// Define directions for Host->USRP & USRP->Host
#define H2U 0
#define U2H 1

// Endpoint encodings for USRP3 from SID LSB's
#define RADIO 0
#define RADIO_CTRL 1
#define SRC_FLOW_CTRL 2
#define RESERVED 3

/* // VRT Type definitions */
/* #define IF_DATA_NO_SID 0 */
/* #define IF_DATA_WITH_SID 1 */
/* #define EXT_DATA_NO_SID 2 */
/* #define EXT_DATA_WITH_SID 3 */
/* #define IF_CONTEXT 4 */
/* #define EXT_CONTEXT 5 */

// CHDR bit masks
#define EXT_CONTEXT 1<<31
#define HAS_TIME 1<<29
#define EOB 1<<28
#define SIZE (1<<16)-1


// UDP used as source for all CHDR comms.
#define CHDR_PORT 49153

typedef unsigned char bool;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

union ip_addr_decoder {
  unsigned long s_addr;
  unsigned char octet[4];
};

/*
Ethernet header structure
*/

struct ethernet_header {
  u8 eth_dst[6]; // MAC addr of destination
  u8 eth_src[6]; // MAC addr of source
  u16 eth_typ;   // Payload protocol type
};

#define ETH_SIZE 14

/*
  IP Header strcture
  NOTE: Network byte order (Big Endian)
*/

struct ip_header {
  u8 ip_vhl;   // [7:4] Version, [3:0] Header Length
  u8 ip_tos;   // Type of Service/DIff Serv/ECN
  u16 ip_len;  // Total Length
  u16 ip_id;   // Identification
  u16 ip_off;  // [15:14] Flags, [13:0] Fragment Offset
  u8 ip_ttl;   // Time To Live
  u8 ip_pro;   // Protocol
  u16 ip_sum;  // Checksum
  struct in_addr ip_src; // IP Source address
  struct in_addr ip_dst; // IP Destination address
};

#define IP_SIZE 20

/*
  UDP Header Structure
  NOTE: Network byte order (Big Endian)
*/

struct udp_header {
  u16 udp_src;  // Source Port
  u16 udp_dst;  // Destination Port
  u16 udp_len;  // Length
  u16 udp_sum;  // Checksum
};

#define UDP_SIZE 8

/* /\* */
/* VITA49 VRLP Header */
/* NOTE: Network byte order (Big Endian) */
/* *\/ */

/* struct vrlp_header { */
/*   u32 vrlp_start; // Hardcoded to ASCII "VRLP" */
/*   u32 vrlp_size;  // [31:20] Frame Count, [19:0] Frame Size */
/* }; */

/* #define VRLP_SIZE 8 */

/* #define VRLP_SEQID(x)  (((x & 0xff)<<4) | ((x & 0xf000) >> 12)) */

/* /\* */
/* VITA49 VRLP Trailer */
/* NOTE: Network byte order (Big Endian) */
/* *\/ */

/* struct vrlp_trailer { */
/*   u32 vrlp_end; // Hardcoded to ASCII "VEND" */
/* }; */

/* #define VRLP_TRAILER_SIZE 4 */

/* /\* */
/* VITA49 VRT Header  */
/* NOTE: Network byte order (Big Endian) */
/* *\/ */

/* struct vrt_header { */
/*   u8 vrt_type; // [7:4] type, [3] Class ID flag, [2] Trailer flag, [1] SOB, [0] EOB  */
/*   u8 vrt_count; // [7:6] TSI, [5:4] TSF, [3:0] Packet Count modulo 16 */
/*   u16 vrt_size; // Number of 32bit words in VRT packet including headers and payload. */
/*   u32 vrt_sid; // Stream ID */
/* }; */

/* #define VRT_SIZE 8 */


/*
Ettus Research CHDR header
NOTE: Little endian byte order (must be unswizzled)
*/

struct chdr_header {
  u32 chdr_type;// [31] Ext Context, [30] RSVD, [29] Has_time, [28] EOB], [27:16] SEQ_ID, [15:0] Size
  u32 chdr_sid; // Stream ID
};

#define CHDR_SIZE 8

/*
Break down SID into CHDR defined fields
*/
struct chdr_sid {
  u8 src_device;
  u8 src_endpoint;
  u8 dst_device;
  u8 dst_endpoint;
};

struct radio_ctrl_payload {
  u32 addr;
  u32 data;
};

struct radio_response {
  u64 data;
};

struct tx_response {
  u32 error_code;
  u32 seq_id;
};

#define TX_ACK 0x00
#define TX_EOB 0x01
#define TX_UNDERRUN 0x02
#define TX_SEQ_ERROR 0x04
#define TX_TIME_ERROR 0x08
#define TX_MIDBURST_SEQ_ERROR 0x20

struct src_flow_ctrl {
  u32 unused;
  u32 seq_id;
};

struct vita_time {
  u64 time;
};

#define VITA_TIME_SIZE 8
#define RADIO_CTRL_SIZE 8
#define RADIO_RESPONSE_SIZE 4


/*
  Packet storage
*/

struct pbuf {
  struct pbuf *next;
  struct pbuf *last;
  struct timeval ts;
  int size;                 // Size stored in pcap file
  int orig_size;            // Original capture size on the wire
  char *payload;
};

struct pbuf_info {
  struct pbuf *start;
  struct pbuf *current;
  struct pbuf *end;
};

struct radio_ctrl_names {
  u32 addr;
  char *name;
};


//
// Prototypes
//

unsigned long swaplong (unsigned long);
unsigned int swapint (unsigned int);
unsigned short swapshort (unsigned short);
char *format_gmt(const struct timeval *, char *);
double timeval2double(struct timeval *);
double relative_time(struct timeval *, struct timeval *);
void get_packet(struct pbuf_info * , const struct pcap_pkthdr *, const u_char *);
void get_start_time(struct timeval * , const struct pcap_pkthdr *, const u_char *);
void get_udp_port_from_file(u16, const char *, struct pbuf_info *, struct timeval *);
void get_everything_from_file(const char *, struct pbuf_info *, struct timeval *);
void get_connection_endpoints( struct pbuf_info *, struct in_addr *, struct in_addr *);
void print_direction(const struct pbuf_info *, const struct in_addr *, const struct in_addr *);
void print_size( const struct pbuf_info *);
void print_sid( const struct pbuf_info *);
void print_vita_header( const struct pbuf_info *, const struct in_addr *);

#endif
