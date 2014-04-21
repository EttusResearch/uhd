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

#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <netinet/in.h>
#include <time.h>
#include <string.h>

#include "uhd_dump.h"
#include "usrp3_regs.h"


// Swap endianness of 64bits
unsigned long swaplong (unsigned long nLongNumber)
{
   union u {unsigned long vi; unsigned char c[sizeof(unsigned long)];};
   union v {unsigned long ni; unsigned char d[sizeof(unsigned long)];};
   union u un;
   union v vn;
   un.vi = nLongNumber;
   vn.d[0]=un.c[7];
   vn.d[1]=un.c[6];
   vn.d[2]=un.c[5];
   vn.d[3]=un.c[4];
   vn.d[4]=un.c[3];
   vn.d[5]=un.c[2];
   vn.d[6]=un.c[1];
   vn.d[7]=un.c[0];

   return (vn.ni);
}

// Swap endianness of 32bits
unsigned int swapint (unsigned int nIntNumber)
{
   union u {unsigned int vi; unsigned char c[sizeof(unsigned long)];};
   union v {unsigned int ni; unsigned char d[sizeof(unsigned long)];};
   union u un;
   union v vn;
   un.vi = nIntNumber;
   vn.d[0]=un.c[3];
   vn.d[1]=un.c[2];
   vn.d[2]=un.c[1];
   vn.d[3]=un.c[0];
   return (vn.ni);
}

// Swap Endianness of 16bits
unsigned short swapshort (unsigned short nShortNumber)
{
   union u {unsigned short vi; unsigned char c[sizeof(unsigned short)];};
   union v {unsigned short ni; unsigned char d[sizeof(unsigned short)];};
   union u un;
   union v vn;
   un.vi = nShortNumber;
   vn.d[0]=un.c[1];
   vn.d[1]=un.c[0];

   return (vn.ni);
}

// Format time from pcap as ascii style.
char *format_gmt(const struct timeval *ts, char *buffer)
{
  time_t seconds;
  struct tm gmt;
  seconds = ts->tv_sec;
  if ((gmtime_r(&seconds, &gmt)) == NULL) {
    fprintf(stderr, "Fatal time format conversion error.\n");
    exit(2);
  }
  sprintf(buffer,
        "%04i-%02i-%02iT%02i:%02i:%02i,%03iZ",
        gmt.tm_year + 1900, gmt.tm_mon + 1, gmt.tm_mday,
        gmt.tm_hour, gmt.tm_min, gmt.tm_sec, (int) (ts->tv_usec / 1000));
  return (buffer);
}

// Takes 2 timeval absolute timevalues, and returns a double value thats the relative time
// difference normalized to seconds.
double relative_time(struct timeval *event_ts, struct timeval *origin_ts)
{
  struct timeval z;
  double x;
  timersub(event_ts,origin_ts,&z);
  x = (double)z.tv_sec + (double)z.tv_usec/1000000;
  return x;
}

// Convert timeval to double, normalized to seconds.
double timeval2double(struct timeval *event_ts)
{
  double x;
  x = (double)event_ts->tv_sec + (double)event_ts->tv_usec/1000000;
  return x;
}

void get_packet(struct pbuf_info *packet_buffer , const struct pcap_pkthdr *header, const u_char *packet)
{
  // Get size of new packet
  packet_buffer->current->size = header->caplen;
  packet_buffer->current->orig_size = header->len;

  // Allocate memory for packet
  packet_buffer->current->payload = (char *)malloc((size_t)packet_buffer->current->size);

  // Copy Packet into buffer
  memcpy(packet_buffer->current->payload,packet,packet_buffer->current->size);
  packet_buffer->current->ts = header->ts;

  // Allocate memory for next pbuf in chain, init it and shift list.
  packet_buffer->current->next = malloc(sizeof (struct pbuf));
  packet_buffer->current->next->last = packet_buffer->current;
  packet_buffer->current = packet_buffer->current->next;
}

// This grabs the (absolute) time stamp of the first packet in the cature file, which can be used to
// derive times relative to the start of the capture file for cross correlation with interactive work
// in Wireshark
void get_start_time(struct timeval *ts , const struct pcap_pkthdr *header, const u_char *packet)
{
  *ts = header->ts;
}

void get_udp_port_from_file(const u16 udp_port, const char *filename, struct pbuf_info *packet_buffer, struct timeval *ts)
{
  pcap_t *handle;			// Session handle
  char errbuf[PCAP_ERRBUF_SIZE];	// Error string
  char filter_exp[256];	                // The ascii filter expression
  struct bpf_program filter;     	// The compiled filter

  // Open PCAP file for read capture time stamp of first packet
  if ((handle = pcap_open_offline(filename,errbuf)) == NULL) {
    fprintf(stderr,"Can't open pcap file for reading: %s\n",errbuf);
    exit(2);
  }

  // Parse PCAP file with no filter to grab the time stamp of the first captured packet, which becomes the time origin
  // local to the capture file.
  if (pcap_dispatch(handle, 1, (pcap_handler) get_start_time, (u_char *)ts) == -1)  {
    fprintf(stderr, "Error parsing PCAP file: %s\n", pcap_geterr(handle));
    exit(2);
  }

  // Close file again because no way to rewind file descriptor.
  pcap_close(handle);

  // Open PCAP file for read.
  if ((handle = pcap_open_offline(filename,errbuf)) == NULL) {
    fprintf(stderr,"Can't open pcap file for reading: %s\n",errbuf);
    exit(2);
  }

  // Build ASCII filter expression from UDP port
  sprintf(filter_exp,"udp port %d",udp_port);
  printf("\nBPF filter is udp port %d\n",udp_port);

  // Compile filter string to BPF
  if (pcap_compile(handle, &filter, filter_exp, 0, 0) == -1) {
    fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
    exit(2);
  }

  // Apply filter
  if (pcap_setfilter(handle, &filter) == -1) {
    fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
    exit(2);
  }

  // Allocate and initialize packet buffer linked list
  packet_buffer->start = packet_buffer->current = malloc(sizeof (struct pbuf));
  packet_buffer->start->last = NULL;

  // Parse PCAP file using filter, collect all interesting packets.
  if (pcap_dispatch(handle, -1, (pcap_handler) get_packet, (u_char *)packet_buffer) == -1)  {
    fprintf(stderr, "Error parsing PCAP file: %s\n", pcap_geterr(handle));
    exit(2);
  }

  // If no packets matched in the capture then linked list should be completely empty.
  if ( packet_buffer->start == packet_buffer->current) {
    free(packet_buffer->current);
    packet_buffer->start = packet_buffer->current = NULL;
  } else {
    // Note the last used buffer in the list. Removed allocated but unused buffer from list and free
    packet_buffer->end = packet_buffer->current->last;
    packet_buffer->end->next = NULL;
    free(packet_buffer->current);
  }
}

//
// Read a pcap file into memory.
//
void get_everything_from_file(const char *filename, struct pbuf_info *packet_buffer, struct timeval *ts)
{
  pcap_t *handle;			// Session handle
  char errbuf[PCAP_ERRBUF_SIZE];	// Error string

  // Open PCAP file for read capture time stamp of first packet
  if ((handle = pcap_open_offline(filename,errbuf)) == NULL) {
    fprintf(stderr,"Can't open pcap file for reading: %s\n",errbuf);
    exit(2);
  }

  // Parse PCAP file with no filter to grab the time stamp of the first captured packet, which becomes the time origin
  // local to the capture file.
  if (pcap_dispatch(handle, 1, (pcap_handler) get_start_time, (u_char *)ts) == -1)  {
    fprintf(stderr, "Error parsing PCAP file: %s\n", pcap_geterr(handle));
    exit(2);
  }

  // Close file again because no way to rewind file descriptor.
  pcap_close(handle);

  // Open PCAP file for read
  if ((handle = pcap_open_offline(filename,errbuf)) == NULL) {
    fprintf(stderr,"Can't open pcap file for reading: %s\n",errbuf);
    exit(2);
  }

  // Allocate and initialize packet buffer linked list
  packet_buffer->start = packet_buffer->current = malloc(sizeof (struct pbuf));
  packet_buffer->start->last = NULL;

  // Parse PCAP file using filter, collect all interesting packets.
  if (pcap_dispatch(handle, -1, (pcap_handler) get_packet, (u_char *)packet_buffer) == -1)  {
    fprintf(stderr, "Error parsing PCAP file: %s\n", pcap_geterr(handle));
    exit(2);
  }

  // If no packets matched in the capture then linked list should be completely empty.
  if ( packet_buffer->start == packet_buffer->current) {
    free(packet_buffer->current);
    packet_buffer->start = packet_buffer->current = NULL;
  } else {
    // Note the last used buffer in the list. Removed allocated but unused buffer from list and free
    packet_buffer->end = packet_buffer->current->last;
    packet_buffer->end->next = NULL;
    free(packet_buffer->current);
  }
}


// Debug
void print_raw(const struct pbuf_info *packet_buffer, const int count)
{
  const u8 *raw;
  int x;
  raw = (u8 *) packet_buffer;
  fprintf(stdout,"  ");
  for (x = 0; x<count; x++)
    fprintf(stdout,"%02x ",*(raw+x));
}

// Print to STDOUT the direction of this packet flow
void print_direction(const struct pbuf_info *packet_buffer, const struct in_addr *host_addr, const struct in_addr *usrp_addr)
{
 const struct ip_header *ip_header;

 // Overlay IP header on packet payload
 ip_header = (struct ip_header *)(packet_buffer->current->payload+ETH_SIZE);

 if ((host_addr->s_addr == ip_header->ip_src.s_addr) && (usrp_addr->s_addr == ip_header->ip_dst.s_addr))
   fprintf(stdout,"Host->USRP");
 else if  ((host_addr->s_addr == ip_header->ip_dst.s_addr) && (usrp_addr->s_addr == ip_header->ip_src.s_addr))
    fprintf(stdout,"USRP->Host");
 else
   fprintf(stdout,"UNKNOWN");
}

// Print to STDOUT the CHDR size in bytes
void print_size(const struct pbuf_info *packet_buffer)
{
  const struct chdr_header *chdr_header;

  // Overlay CHDR header on packet payload
  chdr_header = (struct chdr_header *)(packet_buffer->current->payload+ETH_SIZE+IP_SIZE+UDP_SIZE);

  fprintf(stdout,"Size: %04d ",(swapint(chdr_header->chdr_type) & SIZE));
}

// Print to STDOUT the CHDR SID decode
void print_sid(const struct pbuf_info *packet_buffer)
{
  const struct chdr_header *chdr_header;
  const struct chdr_sid *chdr_sid;

  // Overlay CHDR header on packet payload
  chdr_header = (struct chdr_header *)(packet_buffer->current->payload+ETH_SIZE+IP_SIZE+UDP_SIZE);

  // Overlay CHDR SID definition on CHDR SID.
  chdr_sid = (struct chdr_sid *)&(chdr_header->chdr_sid);

  fprintf(stdout,"%02x.%02x->%02x.%02x",chdr_sid->src_device,chdr_sid->src_endpoint,chdr_sid->dst_device,chdr_sid->dst_endpoint);
}

// Print to STDOUT a decoded tx response packet payload.
void print_tx_response(const struct tx_response *tx_response)
{
  switch(swapint(tx_response->error_code))
    {
    case TX_ACK: fprintf(stdout,"ACK "); break;
    case TX_EOB: fprintf(stdout,"EOB "); break;
    case TX_UNDERRUN: fprintf(stdout,"Underrun "); break;
    case TX_SEQ_ERROR: fprintf(stdout,"Sequence Error "); break;
    case TX_TIME_ERROR: fprintf(stdout,"Time Error "); break;
    case TX_MIDBURST_SEQ_ERROR: fprintf(stdout,"Mid-Burst Seq Errror "); break;
    default: fprintf(stdout,"Unknown Error ");
    }
  fprintf(stdout,"for SeqID = %03x ",swapint(tx_response->seq_id)&0xFFF);
}



// Returns Name of a register from it's address
char *reg_addr_to_name(const u32 addr)
{
  int x;
  x = 0;
  while((reg_list[x].addr != addr) && (reg_list[x].addr != 999))
    x++;
  return(reg_list[x].name);
}

// Print to STDOUT decode of CHDR header including time if present.
void print_vita_header(const struct pbuf_info *packet_buffer, const struct in_addr *host_addr)
{
  const struct ip_header *ip_header;
  const struct chdr_header *chdr_header;
  const struct chdr_sid *chdr_sid;
  const struct radio_ctrl_payload *radio_ctrl_payload;
  const struct radio_response *radio_response;
  const struct tx_response *tx_response;
  const struct src_flow_ctrl *src_flow_ctrl;
  const struct vita_time *vita_time;
  int direction;
  u8 endpoint;
  int has_time;

  // Overlay IP header on packet payload
  ip_header = (struct ip_header *)(packet_buffer->current->payload+ETH_SIZE);

  // Overlay CHDR header on packet payload
  chdr_header = (struct chdr_header *)(packet_buffer->current->payload+ETH_SIZE+IP_SIZE+UDP_SIZE);

  // Overlay CHDR SID definition on CHDR SID.
  chdr_sid = (struct chdr_sid *)&(chdr_header->chdr_sid);

  // Identify packet direction
  if (ip_header->ip_src.s_addr == host_addr->s_addr)
    direction = H2U;
  else
    direction = U2H;

  // Decode packet type


  if ((swapint(chdr_header->chdr_type) & EXT_CONTEXT) == EXT_CONTEXT)  fprintf(stdout,"Context Ext ");
  else  fprintf(stdout,"IF Data     ");

  // Determine USRP Sink/Src Endpoint
  if (direction==H2U)
    endpoint = (chdr_sid->dst_endpoint) & 0x3;
  else if (direction==U2H)
    endpoint = (chdr_sid->src_endpoint) & 0x3;

  // Look for CHDR EOB flags.
  if ((swapint(chdr_header->chdr_type) & EOB) == EOB)  fprintf(stdout,"EOB ");
  else fprintf(stdout,"    ");

  // Is there embeded VITA time?
  if ((swapint(chdr_header->chdr_type) & HAS_TIME) == HAS_TIME) {
    vita_time = (struct vita_time *)(packet_buffer->current->payload+ETH_SIZE+IP_SIZE+UDP_SIZE+CHDR_SIZE);
    fprintf(stdout,"Time=%016lx ",swaplong(vita_time->time));
    has_time = 1;
  } else {
    fprintf(stdout,"                      ");
    has_time = 0;
  }

  fprintf(stdout,"SeqID=%03x  ",(swapint(chdr_header->chdr_type)>>16)&0xFFF);

  // Print Payload
  if (endpoint == RADIO)
    {
      if ((swapint(chdr_header->chdr_type) & EXT_CONTEXT) != EXT_CONTEXT)
	{
	  if (direction == H2U)
	    {
	      fprintf(stdout,"TX IF Data ");
	    }
	  else
	    // U2H
	    {
	      fprintf(stdout,"RX IF Data ");
	    }
	}
      else if ((swapint(chdr_header->chdr_type) & EXT_CONTEXT) == EXT_CONTEXT)
	{
	  if (direction == H2U)
	    {
	      // BAD PACKET
	    }
	  else
	    // U2H
	    {
	      // TX Response packet.
	      tx_response = (struct tx_response *) (packet_buffer->current->payload+ETH_SIZE+IP_SIZE+UDP_SIZE+CHDR_SIZE+(has_time?VITA_TIME_SIZE:0));
	      print_tx_response(tx_response);
	    }
	}
    }
  else if (endpoint == RADIO_CTRL)
    {
      fprintf(stdout,"\t\t\t");
      if ((swapint(chdr_header->chdr_type) & EXT_CONTEXT) != EXT_CONTEXT)
	{
	  // BAD PACKET
	}
      else if ((swapint(chdr_header->chdr_type) & EXT_CONTEXT) == EXT_CONTEXT)
	{
	if (direction == H2U)
	    {
	      radio_ctrl_payload = (struct radio_ctrl_payload *)(packet_buffer->current->payload+ETH_SIZE+IP_SIZE+UDP_SIZE+CHDR_SIZE+(VITA_TIME_SIZE*has_time));
	      fprintf(stdout,"Radio Ctrl %s(0x%02x)=0x%08x",reg_addr_to_name(swapint(radio_ctrl_payload->addr)),(u8)swapint(radio_ctrl_payload->addr),swapint(radio_ctrl_payload->data));
	    }
	  else
	    // U2H
	    {
	      radio_response = (struct radio_response *)(packet_buffer->current->payload+ETH_SIZE+IP_SIZE+UDP_SIZE+CHDR_SIZE+(VITA_TIME_SIZE*has_time));
	      fprintf(stdout,"Radio Response = 0x%016lx",swaplong(radio_response->data));
	    }
	}
    }
  else if (endpoint == SRC_FLOW_CTRL)
    {
      if  ((swapint(chdr_header->chdr_type) & EXT_CONTEXT) != EXT_CONTEXT)
	{
	  // BAD PACKET
	}
      else if  ((swapint(chdr_header->chdr_type) & EXT_CONTEXT) == EXT_CONTEXT)
	{
	  if (direction == H2U)
	    {
	      src_flow_ctrl = (struct src_flow_ctrl *)(packet_buffer->current->payload+ETH_SIZE+IP_SIZE+UDP_SIZE+CHDR_SIZE+(VITA_TIME_SIZE*has_time));
	      fprintf(stdout,"Src Flow Ctrl = 0x%04x",swapint(src_flow_ctrl->seq_id));
	    }
	  else
	    // U2H
	    {
	      // Bad Packet
	    }
	}
    }
  //print_raw((struct pbuf_info *)vrt_header,16);
}


// Find IP addresses for Host and USRP in this Session

void get_connection_endpoints( struct pbuf_info *packet_buffer,  struct in_addr *host_addr, struct in_addr *usrp_addr)
{
  const struct ip_header *ip_header;
  const struct chdr_header *chdr_header;
  const struct chdr_sid *chdr_sid;

  // Determine which side of the stream is Host and which is USRP by probing capture until a
  // CHDR message type is discovered. The SID reveals which direction the packet is traveling.
  // Then record apparent IP addresses of Host and USRP for future packet clasification.
  packet_buffer->current = packet_buffer->start;

  host_addr->s_addr = 0x0;
  usrp_addr->s_addr = 0x0;

  while (packet_buffer->current != NULL) {

    // Overlay IP header on packet payload
    ip_header = (struct ip_header *)(packet_buffer->current->payload+ETH_SIZE);

    // Overlay CHDR header on packet payload
    chdr_header = (struct chdr_header *)(packet_buffer->current->payload+ETH_SIZE+IP_SIZE+UDP_SIZE);

    // Overlay CHDR SID definition on CHDR SID.
    chdr_sid = (struct chdr_sid *)&(chdr_header->chdr_sid);


    // Catagorise stream
    // CHDR is actually quite hard to conclusively detect, the following deductions help...
    // For CHDR v2 bit 62 should always be 0 (reserved)
    // Bit 47 should always be 0 because sizes > 8192 are unsupport be typical ethernet MTU's
    // By convention currently the host uses SID address 0.x so the first packets in a new UHD session
    // should flow from Host to Device hence [31:24] = 0.
    if (
	((swapint(chdr_header->chdr_type) & 0x40000000) != 0x0) ||
	((swapint(chdr_header->chdr_type) & 0x8000) != 0x0) ||
	((swapint(chdr_header->chdr_sid) & 0xFF000000) != 0x0) ||
	((swapint(chdr_header->chdr_sid) & 0x0000FF00) == 0x0)
	)
      fprintf(stderr,"Current packet is not CHDR. Skipping.");
    else
      {
	// Implicitly CHDR (At least that is our best guess)
	// Go take a look at the SID and see who is boss.
	if ((chdr_sid->src_device == 0) && (chdr_sid->dst_device != 0))
	  {
	    // Host->USRP
	    host_addr->s_addr = ip_header->ip_src.s_addr;
	    usrp_addr->s_addr = ip_header->ip_dst.s_addr;
	    break;
	  }
	else if ((chdr_sid->src_device == 0) && (chdr_sid->dst_device != 0))
	  {
	    // USRP->Host
	    usrp_addr->s_addr = ip_header->ip_src.s_addr;
	    host_addr->s_addr = ip_header->ip_dst.s_addr;
	    break;
	  }
	else
	  {
	    fprintf(stderr,"Malformed CHDR packet, SID is unexpected value: 0x%x",swapint(chdr_header->chdr_sid));
	  }
      }
    packet_buffer->current = packet_buffer->current->next;
  }

  if (host_addr->s_addr == 0) {
    fprintf(stderr, "Could not identify Host/USRP direction in capture analysis, exiting.\n");
    exit(2);
  }
}
