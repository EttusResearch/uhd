
// Copyright 2012 Ettus Research LLC

#ifndef INCLUDED_WB_PKT_IFACE64_H
#define INCLUDED_WB_PKT_IFACE64_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//! opaque, dont touch insides
typedef struct
{
    uint32_t base;
    uint32_t ctrl;
    uint32_t config_addr;
} wb_pkt_iface64_config_t;

//! Init the wb slave for packet interface
wb_pkt_iface64_config_t wb_pkt_iface64_init(const uint32_t base, const size_t ctrl_offset);

/*!
 * Poll if an packet has been received.
 * If yes, return the pointer to the pkt, num_bytes is set.
 * Otherwise, return null.
 */
const void *wb_pkt_iface64_rx_try_claim(wb_pkt_iface64_config_t *config, size_t *num_bytes);

/*!
 * Release the hold on a received packet.
 */
void wb_pkt_iface64_rx_release(wb_pkt_iface64_config_t *config);

/*!
 * Aquire the buffer for an outgoing packet.
 */
void *wb_pkt_iface64_tx_claim(wb_pkt_iface64_config_t *config);

/*!
 * Submit an outgoing packet from a filled the buffer.
 */
void wb_pkt_iface64_tx_submit(wb_pkt_iface64_config_t *config, const size_t num_bytes);

#endif /* INCLUDED_WB_PKT_IFACE64_H */
