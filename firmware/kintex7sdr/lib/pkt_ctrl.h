/*
 * Copyright 2010-2011 Ettus Research LLC
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

#ifndef INCLUDED_PKT_CTRL_H
#define INCLUDED_PKT_CTRL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <lwip/ip_addr.h>

typedef enum {
    PKT_CTRL_ROUTING_MODE_SLAVE = 0,
    PKT_CTRL_ROUTING_MODE_MASTER = 1
} pkt_ctrl_routing_mode_t;

//! Program the decision values into the packet inspector
void pkt_ctrl_program_inspector(
    const struct ip_addr *ip_addr, uint16_t data_port
);

//! Set the routing mode for this device
void pkt_ctrl_set_routing_mode(pkt_ctrl_routing_mode_t mode);

/*!
 * Try to claim an incomming buffer.
 * \param num_lines filled with the buffer size
 * \return a pointer to the buffer memory or NULL
 */
void *pkt_ctrl_claim_incoming_buffer(size_t *num_lines);

/*!
 * Release the incoming buffer. Call when done.
 */
void pkt_ctrl_release_incoming_buffer(void);

/*!
 * Claim an outgoing buffer.
 * \return a pointer to the buffer
 */
void *pkt_ctrl_claim_outgoing_buffer(void);

/*!
 * Commit the outgoing buffer.
 * \param num_lines how many lines written.
 */
void pkt_ctrl_commit_outgoing_buffer(size_t num_lines);

#endif /* INCLUDED_PKT_CTRL_H */
