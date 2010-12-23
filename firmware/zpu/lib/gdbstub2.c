/* -*- c++ -*- */
/*
 * Copyright 2009 Ettus Research LLC
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

/*
 * Implement a eensy weensy part of the GDB Remote Serial Protocol
 *
 * See Appendix D of the GDB manual
 *
 *   m<addr>,<length> 		-- read <length> bytes of memory starting at <addr>
 *     Reply:
 *     XX...		XX... is memory contents in hex
 *     ENN		ENN   NN is a hex error number
 *
 *   M<addr>,<length>:XX...     -- write memory, data in hex
 *     Reply:
 *     OK		for success
 *     ENN		for an error.  NN is a hex error number
 *
 *   X<addr>,<length>:XX...     -- write memory, data in binary
 *     Reply:
 *     OK		for success
 *     ENN		for an error.  NN is a hex error number
 *
 *   c<addr>			-- continue.  <addr> is the address to resume (goto).
 *     Reply: <none>
 *
 *   \x80 New Format...
 */

#include "gdbstub2.h"
#include "loader_parser.h"
#include "hal_uart.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_PACKET	1024

/*
 * Get raw character from serial port, no echo.
 */
static inline int 
gdb_getc(void)
{
  return hal_uart_getc();
}

/*
 * Put character to serial port.  Raw output.
 */
static inline void
gdb_putc(int ch)
{
  hal_uart_putc(ch);
}

// ------------------------------------------------------------------------

#define	GDB_ESCAPE 0x7d

static unsigned char hex_table[16] = { 
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static int
put_hex8_checksum(int ch, int checksum)
{
  unsigned char t = hex_table[(ch >> 4) & 0xf];
  checksum += t;
  gdb_putc(t);

  t = hex_table[ch & 0xf];
  checksum += t;
  gdb_putc(t);
  return checksum;
}

static void
put_hex8(int ch)
{
  put_hex8_checksum(ch, 0);
}

static bool
hex4_to_bin(int ch, int *value)
{
  if ('0' <= ch && ch <= '9'){
    *value = ch - '0';
    return true;
  }
  if ('a' <= ch && ch <= 'f'){
    *value = ch - 'a' + 10;
    return true;
  }
  if ('A' <= ch && ch <= 'F'){
    *value = ch - 'A' + 10;
    return true;
  }
  *value = 0;
  return false;
}

static bool
hex8_to_bin(const unsigned char *s, int *value)
{
  int v0, v1;
  if (hex4_to_bin(s[0], &v0) && hex4_to_bin(s[1], &v1)){
    *value = (v0 << 4) | v1;
    return true;
  }
  return false;
}

static bool
hex_to_bin_array(unsigned char *binary_data, const unsigned char *hex_data, size_t nbytes)
{
  for (size_t i = 0; i < nbytes; i++){
    int t;
    if (!hex8_to_bin(&hex_data[2*i], &t))
      return false;
    binary_data[i] = t;
  }
  return true;
}

static bool
needs_escaping(int ch)
{
  return ch == '$' || ch == '#' || ch == GDB_ESCAPE;
}

/*
 * \brief Wait for a packet.  
 * \param[out] pkt_buf gets the received packet payload.
 * \param[in]  max_size is the maximum number of bytes to write into \p pkt_buf.
 * \param[out] actual_size is the number of bytes written to \p pkt_buf.
 *
 * \returns true iff the payload fits and the checksum is OK.
 *
 * Packets have this format:
 *
 *  $<packet-data>#<checksum>
 *
 * Where <packet-data> is anything and <checksum> is a two byte hex
 * checksum.  In <packet-data> '$', '#' and 0x7d are escaped with 0x7d.
 * The checksum is computed as the modulo 256 sum of all characters
 * btween the leading '$' and the trailing '#' (an 8-bit unsigned
 * checksum).
 */
static bool
get_packet(unsigned char *pkt_buf, size_t max_size, size_t *actual_size)
{
  typedef enum states {
    LOOKING_FOR_DOLLAR,
    LOOKING_FOR_HASH,
    CSUM1,
    CSUM2,
  } state_t;

  *actual_size = 0;
  unsigned char csum[2] = {0, 0};
  state_t state = LOOKING_FOR_DOLLAR;
  size_t  pi = 0;

  while (1){
    int ch = gdb_getc();

    switch (state){
    case LOOKING_FOR_DOLLAR:
      if (ch == '$'){
	pi = 0;
	state = LOOKING_FOR_HASH;
      }
      else if (ch == '#'){	// most likely missed the $
	return false;
      }
      break;
	
    case LOOKING_FOR_HASH:
      if (ch == '$'){
	return false;
      }
      else if (ch == '#'){
	state = CSUM1;
      }
      else {
	if (pi >= max_size)	// payload too big
	  return false;

	if (ch == GDB_ESCAPE)
	  ch = gdb_getc();

	pkt_buf[pi++] = ch;
      }
      break;
      
    case CSUM1:
      csum[0] = ch;
      state = CSUM2;
      break;

    case CSUM2:
      csum[1] = ch;
      *actual_size = pi;

      // accept .. as a correct checksum
      if (csum[0] == '.' && csum[1] == '.')
	return true;

      int expected_checksum;
      if (!hex8_to_bin(csum, &expected_checksum))
	return false;

      int checksum = 0;
      for (size_t i = 0; i < pi; i++)
	checksum += pkt_buf[i];

      checksum &= 0xff;
      return checksum == expected_checksum;
    }
  }
}

static void
put_packet_trailer(int checksum)
{
  gdb_putc('#');
  put_hex8(checksum & 0xff);
  gdb_putc('\r');
  gdb_putc('\n');
}

static void
put_packet(const unsigned char *pkt_buf, size_t size)
{
  gdb_putc('$');

  int checksum = 0;
  for (size_t i = 0; i < size; i++){
    int ch = pkt_buf[i];
    if (needs_escaping(ch))
      gdb_putc(GDB_ESCAPE);
    gdb_putc(ch);
    checksum += ch;
  }
  put_packet_trailer(checksum);
}

/*!
 * Read a hex number
 *
 * \param[inout] bufptr - pointer to pointer to buffer (updated on return)
 * \param[in] end - one past end of valid data in buf
 * \param[out] value - the parsed value
 *
 * \returns true iff a valid hex number was read from bufptr
 */
static bool
parse_number(const unsigned char **bufptr, const unsigned char *end, unsigned int *value)
{
  const unsigned char *buf = *bufptr;
  unsigned int v = 0;
  bool valid = false;
  int nibble;

  while (buf < end && hex4_to_bin(*buf, &nibble)){
    valid = true;
    v = (v << 4) | nibble;
    buf++;
  }
  
  *value = v;
  *bufptr = buf;
  return valid;
}

static bool
parse_char(const unsigned char **bufptr, const unsigned char *end, unsigned char *ch)
{
  const unsigned char *buf = *bufptr;
  if (buf < end){
    *ch = *buf++;
    *bufptr = buf;
    return true;
  }
  return false;
}

static bool
expect_char(const unsigned char **bufptr, const unsigned char *end, unsigned char expected)
{
  unsigned char ch;
  return parse_char(bufptr, end, &ch) && ch == expected;
}

static bool
expect_end(const unsigned char **bufptr, const unsigned char *end)
{
  return *bufptr == end;
}

static bool
parse_addr_length(const unsigned char **bufptr, const unsigned char *end,
		  unsigned int *addr, unsigned int *length)
{
  return (parse_number(bufptr, end, addr)
	  && expect_char(bufptr, end, ',')
	  && parse_number(bufptr, end, length));
}

static void
put_error(int error)
{
  unsigned char buf[3];
  buf[0] = 'E';
  buf[1] = hex_table[(error >> 4) & 0xf];
  buf[2] = hex_table[error & 0xf];
  
  put_packet(buf, sizeof(buf));
}

static void
put_ok(void)
{
  const unsigned char buf[2] = "OK";
  put_packet(buf, sizeof(buf));
}

/*
 * Read memory and send the reply.
 * We do it on the fly so that our packet size is effectively unlimited
 */
static void
read_memory(unsigned int addr, unsigned int nbytes)
{
  int checksum = 0;
  gdb_putc('$');

  if ((addr & 0x3) == 0 && (nbytes & 0x3) == 0){	// word aligned
    union {
      unsigned int	i;
      unsigned char	c[4];
    } u;

    unsigned int *p = (unsigned int *) addr;
    unsigned int length = nbytes / 4;

    for (unsigned int i = 0; i < length; i++){
      u.i = p[i];	// do a word read
      checksum = put_hex8_checksum(u.c[0], checksum);
      checksum = put_hex8_checksum(u.c[1], checksum);
      checksum = put_hex8_checksum(u.c[2], checksum);
      checksum = put_hex8_checksum(u.c[3], checksum);
    }
  }
  else {						// byte aligned
    unsigned char *p = (unsigned char *) addr;
    for (unsigned int i = 0; i < nbytes; i++)
      checksum = put_hex8_checksum(p[i], checksum);
  }

  put_packet_trailer(checksum);
}

static unsigned int
get_unaligned_int(const unsigned char *p)
{
  // we're bigendian
  return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
}

static bool
write_memory(unsigned int addr, size_t nbytes,
	     const unsigned char *data)
{
  if ((addr & 0x3) == 0 && (nbytes & 0x3) == 0){	// word-aligned dst
    unsigned int *dst = (unsigned int *) addr;
    size_t length = nbytes / 4;
    for (size_t i = 0; i < length; i++){
      unsigned int t = get_unaligned_int(&data[4*i]);
      dst[i] = t;					// word writes
    }
  }
  else {						// non-word-aligned dst
    unsigned char *dst = (unsigned char *) addr;
    for (size_t i = 0; i < nbytes; i++){
      dst[i] = data[i];
    }
  }
  return true;
}

void
gdbstub2_main_loop(void)
{
  unsigned char inpkt[MAX_PACKET + 24];
  unsigned char binary_data[MAX_PACKET/2] __attribute__((aligned (4)));

  hal_uart_set_mode(UART_MODE_RAW); //tell UART HAL not to map \n to \r\n

  while (1){
    size_t	inpkt_len;
    bool ok = get_packet(inpkt, sizeof(inpkt), &inpkt_len);
    if (!ok){
      gdb_putc('-');
      continue;
    }
    gdb_putc('+');

    const unsigned char *buf = inpkt;
    const unsigned char *end = inpkt + inpkt_len;
    unsigned char ch;

    if (!parse_char(&buf, end, &ch)){	// empty packet
      put_packet(0, 0);
      continue;
    }

    unsigned int addr;
    unsigned int length;

    switch(ch){
    case 'm':		// m<addr>,<length>  -- read <length> bytes starting at <addr>
      if (!(parse_addr_length(&buf, end, &addr, &length) && expect_end(&buf, end))){
	put_error(1);
      }
      else {
	read_memory(addr, length);
      }
      break;

    case 'M':		// M<addr>,<length>:XX...  -- write <length> bytes starting at <addr>
			//   XX... is the data in hex
      if (!(parse_addr_length(&buf, end, &addr, &length)
	    && expect_char(&buf, end, ':')
	    && (end - buf) == 2 * length)){
	put_error(1);
      }
      else {
	if (!hex_to_bin_array(binary_data, buf, length))
	  put_error(2);
	else if (!write_memory(addr, length, binary_data))
	  put_error(3);
	else
	  put_ok();
      }
      break;

    case 'X':		// X<addr>,<length>:XX...  -- write <length> bytes starting at <addr>
			//   XX... is the data in binary
      if (!(parse_addr_length(&buf, end, &addr, &length)
	    && expect_char(&buf, end, ':')
	    && (end - buf) == length)){
	put_error(1);
      }
      else {
	if (!write_memory(addr, length, buf))
	  put_error(3);
	else
	  put_ok();
      }
      break;

    case 'c':		// c<addr>	-- continue.  <addr> is the address to resume (goto).
      if (!(parse_number(&buf, end, &addr)
	    && expect_end(&buf, end))){
	put_error(1);
      }
      else {
	typedef void (*fptr_t)(void);
	(*(fptr_t) addr)();	// most likely no return
      }
      break;
/*
    case 0x80:
      {
	unsigned char *output = binary_data;  // reuse
	size_t sizeof_output = sizeof(binary_data);
	size_t actual_olen;
	loader_parser(buf, end-buf,
		      output, sizeof_output, &actual_olen);
	put_packet(output, actual_olen);
      }
      break;
*/
    default:		// unknown packet type
      put_packet(0, 0);
      break;
    }
  }
}
