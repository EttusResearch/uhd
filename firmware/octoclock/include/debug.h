/*
 * Copyright 2014 Ettus Research LLC
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

//Only expose these macros to the firmware, and only if specified
#if defined(DEBUG) && !defined(__BOOTLOADER__)

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>

#include <serial.h>

#define DEBUG_INIT() serial_init(&PORTF, 0)
#define DEBUG_LOG(msg) serial_tx_P(PSTR(msg), &PORTF, 0, true)
#define DEBUG_LOG_NNL(msg) serial_tx_P(PSTR(msg), &PORTF, 0, false)
#define DEBUG_LOG_BYTE(byte) serial_tx_byte(byte, &PORTF, 0, true)
#define DEBUG_LOG_BYTE_NNL(byte) serial_tx_byte(byte, &PORTF, 0, false)
#define DEBUG_LOG_HEX(byte) serial_tx_hex(byte, &PORTF, 0, true)
#define DEBUG_LOG_HEX_NNL(byte) serial_tx_hex(byte, &PORTF, 0, false)

#define DEBUG_LOG_CHAR_ARR_NNL(arr,len) for(uint8_t i = 0; i < len; i++){ \
                                            DEBUG_LOG_BYTE_NNL(arr[i]); \
                                            DEBUG_LOG_NNL(" "); \
                                        }
#define DEBUG_LOG_CHAR_ARR(arr,len) DEBUG_LOG_CHAR_ARR_NNL(arr,len); \
                                    DEBUG_LOG(" ")

#define DEBUG_LOG_MAC(mac_addr) DEBUG_LOG_HEX_NNL(mac_addr[0]); \
                                DEBUG_LOG_NNL(":"); \
                                DEBUG_LOG_HEX_NNL(mac_addr[1]); \
                                DEBUG_LOG_NNL(":"); \
                                DEBUG_LOG_HEX_NNL(mac_addr[2]); \
                                DEBUG_LOG_NNL(":"); \
                                DEBUG_LOG_HEX_NNL(mac_addr[3]); \
                                DEBUG_LOG_NNL(":"); \
                                DEBUG_LOG_HEX_NNL(mac_addr[4]); \
                                DEBUG_LOG_NNL(":"); \
                                DEBUG_LOG_HEX(mac_addr[5]);

#define DEBUG_LOG_IP(ip_addr) DEBUG_LOG_BYTE_NNL(ip4_addr1(&ip_addr)); \
                              DEBUG_LOG_NNL("."); \
                              DEBUG_LOG_BYTE_NNL(ip4_addr2(&ip_addr)); \
                              DEBUG_LOG_NNL("."); \
                              DEBUG_LOG_BYTE_NNL(ip4_addr3(&ip_addr)); \
                              DEBUG_LOG_NNL("."); \
                              DEBUG_LOG_BYTE(ip4_addr4(&ip_addr));

#define DEBUG_LOG_SHORT(num) DEBUG_LOG_HEX_NNL(((uint8_t*)&num)[1]); \
                             DEBUG_LOG_HEX(((uint8_t*)&num)[0]);

#define DEBUG_LOG_INT(num) DEBUG_LOG_HEX_NNL(((uint8_t*)&num)[3]); \
                           DEBUG_LOG_HEX_NNL(((uint8_t*)&num)[2]); \
                           DEBUG_LOG_HEX_NNL(((uint8_t*)&num)[1]); \
                           DEBUG_LOG_HEX(((uint8_t*)&num)[0]);

#else

#define DEBUG_INIT()
#define DEBUG_LOG(msg)
#define DEBUG_LOG_NNL(msg)
#define DEBUG_LOG_CHAR(byte)
#define DEBUG_LOG_CHAR_NNL(byte)
#define DEBUG_LOG_BYTE(byte)
#define DEBUG_LOG_BYTE_NNL(byte)
#define DEBUG_LOG_HEX(byte)
#define DEBUG_LOG_HEX_NNL(byte)
#define DEBUG_LOG_CHAR_ARR(arr,len)
#define DEBUG_LOG_CHAR_ARR_NNL(arr,len)

#define DEBUG_LOG_MAC(mac_addr)
#define DEBUG_LOG_IP(ip_addr)
#define DEBUG_LOG_SHORT(num)
#define DEBUG_LOG_INT(num)

#endif

#endif /* _DEBUG_H_ */
