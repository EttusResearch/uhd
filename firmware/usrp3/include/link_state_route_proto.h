
// Copyright 2013 Ettus Research LLC

#ifndef INCLUDED_LINK_STATE_ROUTE_PROTO_H
#define INCLUDED_LINK_STATE_ROUTE_PROTO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <lwip/ip_addr.h>

//http://en.wikipedia.org/wiki/Link-state_routing_protocol

//! Initialize internals and handler registration
void link_state_route_proto_init(void);

/*!
 * Advances the internal counter to determine expired entries.
 * Call this periodically, along with the other periodic calls.
 */
void link_state_route_proto_tick(void);

//! Initiate a periodic update to the neighbor table
void link_state_route_proto_update(const uint8_t ethno);

//! Flood the network with information about routes
void link_state_route_proto_flood(const uint8_t ethno);

/*!
 * Updates the causes cycle cache for the given source ethno.
 */
void link_state_route_proto_update_cycle_cache(const uint8_t ethno);

/*!
 * Determine if the given link is the cause of a cycle (aka routing loop)
 * This call does not run the algorithm, but rather checks the cache.
 * This call also differs from the one below, takes the ethnos.
 */
bool link_state_route_proto_causes_cycle_cached(const uint8_t eth_src, const uint8_t eth_dst);

//! Determine if the given link is the cause of a cycle (aka routing loop)
bool link_state_route_proto_causes_cycle(const struct ip_addr *src, const struct ip_addr *dst);

typedef struct
{
    struct ip_addr node;
    struct ip_addr nbor;
} ls_node_mapping_t;

/*!
 * Get a pointer to the node mapping table.
 * The table length will be set to *length.
 */
const ls_node_mapping_t *link_state_route_get_node_mapping(size_t *length);

#endif /* INCLUDED_LINK_STATE_ROUTE_PROTO_H */
