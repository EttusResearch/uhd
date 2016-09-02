/*
 * Dissector for ZPU packets (communication with X300 firmware)
 *
 * Copyright 2013-2014 Ettus Research LLC
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
 *
 */

/* TODO: autogenerate this file */

/* Names of SHMEM registers: */
static const value_string X300_SHMEM_NAMES[] = {
    { 0, "X300_FW_SHMEM_COMPAT_NUM" },
    { 1, "X300_FW_SHMEM_GPSDO_STATUS" },
    { 2, "X300_FW_SHMEM_UART_RX_INDEX" },
    { 3, "X300_FW_SHMEM_UART_TX_INDEX" },
    { 5, "X300_FW_SHMEM_CLAIM_STATUS" },
    { 6, "X300_FW_SHMEM_CLAIM_TIME" },
    { 7, "X300_FW_SHMEM_CLAIM_SRC" },
    { 8, "X300_FW_SHMEM_UART_RX_ADDR" },
    { 9, "X300_FW_SHMEM_UART_TX_ADDR" },
    { 10, "X300_FW_SHMEM_UART_WORDS32" },
    { 11, "X300_FW_SHMEM_ROUTE_MAP_ADDR" },
    { 12, "X300_FW_SHMEM_ROUTE_MAP_LEN" }
};

