
// Copyright 2013 Ettus Research LLC

#include <link_state_route_proto.h>
#include <u3_net_stack.h>
#include <ethernet.h>
#include <string.h>
#include <printf.h>
#include <print_addrs.h>

#define lengthof(a) (sizeof(a)/sizeof(*(a)))

/***********************************************************************
 * global constants
 **********************************************************************/
#define LS_PROTO_VERSION 6

//shift the proto version into the ID so only matching fw responds
#define LS_ID_DISCOVER  (0 | (8 << LS_PROTO_VERSION))
#define LS_ID_INFORM    (1 | (8 << LS_PROTO_VERSION))

#define LS_PAYLOAD_MTU 1024
#define LS_NUM_NBOR_ENTRIES 16
#define LS_NUM_NODE_ENTRIES 64
#define LS_NUM_MAP_ENTRIES 128

#define NETHS 4 //max eths supported in this file

/***********************************************************************
 * wire format for table communication
 **********************************************************************/
typedef struct
{
    uint32_t num_nbors; //number of entries in neighbors list
    uint32_t num_ports; //first few neighbors are local ports
    struct ip_addr node;
    struct ip_addr nbors[];
} ls_data_t;

static inline size_t sizeof_ls_data(const ls_data_t *ls_data)
{
    return 0
        + sizeof(uint32_t)/*num neighbors*/
        + sizeof(uint32_t)/*num ports*/
        + sizeof(struct ip_addr)/*source node*/
        + sizeof(struct ip_addr)*ls_data->num_nbors;
}

/***********************************************************************
 * sequence and tick counter monitor
 **********************************************************************/
static uint16_t ticker = 0;

void link_state_route_proto_tick(void)
{
    ticker++;
}

static inline bool is_tick_expired(const uint16_t tick)
{
    const uint16_t delta = ticker - tick;
    return delta > 2; //have not talked in a while, you are deaf to me
}

static uint16_t current_seq = 0;

static inline bool is_seq_newer(const uint16_t seq, const uint16_t entry_seq)
{
    if (seq == entry_seq) return false; //not newer if equal
    const uint16_t delta = seq - entry_seq;
    return (delta & (1 << 15)) == 0; //newer when subtraction did not overflow
}

/***********************************************************************
 * node entry api
 **********************************************************************/
typedef struct
{
    uint16_t seq;
    uint16_t tick;
    uint8_t ethno;
    struct ip_addr ip_addr;
} ls_node_entry_t;

static bool ls_node_entry_valid(const ls_node_entry_t *entry)
{
    return entry->ip_addr.addr != 0 && !is_tick_expired(entry->tick);
}

static void ls_node_entry_update(ls_node_entry_t *entry, const int8_t ethno, const uint16_t seq, const struct ip_addr *ip_addr)
{
    entry->seq = seq;
    entry->tick = ticker;
    entry->ethno = ethno;
    entry->ip_addr.addr = ip_addr->addr;
}

static bool ls_node_entries_update(
    ls_node_entry_t *entries, const size_t num_entries,
    const int8_t ethno, const uint16_t seq, const struct ip_addr *ip_addr
)
{
    for (size_t i = 0; i < num_entries; i++)
    {
        if (!ls_node_entry_valid(&entries[i]))
        {
            ls_node_entry_update(entries+i, ethno, seq, ip_addr);
            return true;
        }

        if (entries[i].ip_addr.addr == ip_addr->addr && entries[i].ethno == ethno)
        {
            if (is_seq_newer(seq, entries[i].seq))
            {
                ls_node_entry_update(entries+i, ethno, seq, ip_addr);
                return true;
            }
            return false;
        }
    }

    //no space, shift the table down and take entry 0
    memmove(entries+1, entries, (num_entries-1)*sizeof(ls_node_entry_t));
    ls_node_entry_update(entries+0, ethno, seq, ip_addr);
    return true;
}

/***********************************************************************
 * storage for nodes in the network
 **********************************************************************/
static ls_node_entry_t ls_nbors[LS_NUM_NBOR_ENTRIES];
static ls_node_entry_t ls_nodes[LS_NUM_NODE_ENTRIES];

/***********************************************************************
 * node table
 **********************************************************************/
static ls_node_mapping_t ls_node_maps[LS_NUM_MAP_ENTRIES];

const ls_node_mapping_t *link_state_route_get_node_mapping(size_t *length)
{
    *length = lengthof(ls_node_maps);
    return ls_node_maps;
}

static void add_node_mapping(const struct ip_addr *node, const struct ip_addr *nbor)
{
    //printf("add_node_mapping: %s -> %s\n", ip_addr_to_str(node), ip_addr_to_str(nbor));

    //write into the first available slot
    for (size_t i = 0; i < lengthof(ls_node_maps); i++)
    {
        if (ls_node_maps[i].node.addr == 0)
        {
            ls_node_maps[i].node.addr = node->addr;
            ls_node_maps[i].nbor.addr = nbor->addr;
            return;
        }
    }

    //otherwise, shift down the table and take slot0
    memmove(ls_node_maps+1, ls_node_maps, sizeof(ls_node_maps) - sizeof(ls_node_mapping_t));
    ls_node_maps[0].node.addr = node->addr;
    ls_node_maps[0].nbor.addr = nbor->addr;
}

static void remove_node_matches(const struct ip_addr *node)
{
    //printf("remove_node_matches: %s\n", ip_addr_to_str(node));

    for (size_t j = 0; j < lengthof(ls_node_maps); j++)
    {
        //if the address is a match, clear the entry
        if (ls_node_maps[j].node.addr == node->addr)
        {
            ls_node_maps[j].node.addr = 0;
            ls_node_maps[j].nbor.addr = 0;
        }
    }
}

static void update_node_mappings(const ls_data_t *ls_data)
{
    //printf("update_node_mappings: %s\n", ip_addr_to_str(&ls_data->node));

    //remove any expired entries
    for (size_t i = 0; i < lengthof(ls_nodes); i++)
    {
        if (ls_nodes[i].ip_addr.addr != 0 && is_tick_expired(ls_nodes[i].tick))
        {
            remove_node_matches(&ls_nodes[i].ip_addr);
        }
    }

    //remove any matches for the current node
    remove_node_matches(&ls_data->node);

    //is this a local packet?
    bool is_local = false;
    for (size_t e = 0; e < ethernet_ninterfaces(); e++)
    {
        if (ls_data->node.addr == u3_net_stack_get_ip_addr(e)->addr) is_local = true;
    }

    //load entries from ls data into array
    for (size_t i = 0; i < ls_data->num_nbors; i++)
    {
        if (is_local && i < ls_data->num_ports) continue; //ignore local ports
        add_node_mapping(&ls_data->node, &ls_data->nbors[i]);
    }
}

/***********************************************************************
 * forward link state data onto all neighbors on the given port
 **********************************************************************/
static void send_link_state_data_to_all_neighbors(
    const uint8_t ethno, const uint16_t seq, const ls_data_t *ls_data
){
    //exit and dont forward if the information is stale
    if (!ls_node_entries_update(ls_nodes, lengthof(ls_nodes), ethno, seq, &ls_data->node)) return;

    //update the mappings with new info
    update_node_mappings(ls_data);

    //forward to all neighbors
    for (size_t i = 0; i < lengthof(ls_nbors); i++)
    {
        if (ls_nbors[i].ip_addr.addr == ls_data->node.addr) continue; //dont forward to sender
        if (ls_node_entry_valid(&ls_nbors[i]))
        {
            if (ethernet_get_link_up(ls_nbors[i].ethno)) u3_net_stack_send_icmp_pkt(
                ls_nbors[i].ethno, ICMP_IRQ, 0,
                LS_ID_INFORM, seq,
                &(ls_nbors[i].ip_addr), ls_data, sizeof_ls_data(ls_data)
            );
        }
    }

    //a change may have occured, update the cache
    link_state_route_proto_update_cycle_cache(ethno);
}

/***********************************************************************
 * handler for information reply
 **********************************************************************/
static void handle_icmp_ir(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t id, const uint16_t seq,
    const void *buff, const size_t num_bytes
){
    switch (id)
    {
    //received a reply directly from the neighbor, add to neighbor list
    case LS_ID_DISCOVER:
        //printf("GOT LS_ID_DISCOVER REPLY - ID 0x%x - IP%u: %s\n", id, (int)ethno, ip_addr_to_str(u3_net_stack_get_ip_addr(ethno)));
        if (ls_node_entries_update(ls_nbors, lengthof(ls_nbors), ethno, seq, src)) link_state_route_proto_flood(ethno);
        break;
    }
}

/***********************************************************************
 * handler for information request
 **********************************************************************/
static void handle_icmp_irq(
    const uint8_t ethno,
    const struct ip_addr *src, const struct ip_addr *dst,
    const uint16_t id, const uint16_t seq,
    const void *buff, const size_t num_bytes
){
    switch (id)
    {
    //replies to discovery packets
    case LS_ID_DISCOVER:
        //printf("GOT LS_ID_DISCOVER REQ - IP%u: %s\n", (int)ethno, ip_addr_to_str(u3_net_stack_get_ip_addr(ethno)));
        //printf("SEND LS_ID_DISCOVER REPLY - IP%u: %s\n", (int)ethno, ip_addr_to_str(u3_net_stack_get_ip_addr(ethno)));
        u3_net_stack_send_icmp_pkt(ethno, ICMP_IR, 0, id, seq, src, buff, num_bytes);
        break;

    //handle and forward information
    case LS_ID_INFORM:
        //printf("GOT LS_ID_INFORM REQ - IP%u: %s\n", (int)ethno, ip_addr_to_str(u3_net_stack_get_ip_addr(ethno)));
        send_link_state_data_to_all_neighbors(ethno, seq, (const ls_data_t *)buff);
        break;
    };
}

/***********************************************************************
 * initiate a periodic update to the table
 **********************************************************************/
void link_state_route_proto_update(const uint8_t ethno)
{
    //send a discovery packet
    //printf("SEND LS_ID_DISCOVER REQ - IP%u: %s\n", (int)ethno, ip_addr_to_str(u3_net_stack_get_ip_addr(ethno)));
    u3_net_stack_send_icmp_pkt(
        ethno, ICMP_IRQ, 0,
        LS_ID_DISCOVER, current_seq++,
        u3_net_stack_get_bcast(ethno), NULL, 0
    );
}

void link_state_route_proto_flood(const uint8_t ethno)
{
    for (size_t e = 0; e < ethernet_ninterfaces(); e++)
    {
        //fill link state data buffer
        uint8_t buff[LS_PAYLOAD_MTU] = {};
        ls_data_t *ls_data = (ls_data_t *)buff;
        ls_data->node.addr = u3_net_stack_get_ip_addr(e)->addr;
        ls_data->num_nbors = 0;
        ls_data->num_ports = 0;

        //first the local port links
        for (size_t ej = 0; ej < ethernet_ninterfaces(); ej++)
        {
            if (e == ej) continue; //dont include our own port
            ls_data->nbors[ls_data->num_nbors++].addr = u3_net_stack_get_ip_addr(ej)->addr;
            ls_data->num_ports++;
        }

        //now list the neighbors
        for (size_t i = 0; i < lengthof(ls_nbors); i++)
        {
            if ((sizeof_ls_data(ls_data) + 4) >= LS_PAYLOAD_MTU) break;
            if (ls_node_entry_valid(&ls_nbors[i]) && ls_nbors[i].ethno == e)
            {
                ls_data->nbors[ls_data->num_nbors++].addr = ls_nbors[i].ip_addr.addr;
            }
        }

        //send this data to all neighbors
        send_link_state_data_to_all_neighbors(ethno, current_seq++, ls_data);
    }
}

/***********************************************************************
 * cycle detection logic
 **********************************************************************/
static void follow_links(const size_t current, struct ip_addr *nodes, bool *visited, const size_t num_nodes)
{
    if (visited[current]) return; //end the recursion
    visited[current] = true;

    //follow all links where current node is the source
    for (size_t i = 0; i < lengthof(ls_node_maps); i++)
    {
        if (ls_node_maps[i].node.addr != nodes[current].addr) continue;

        //find the index of the neighbor in the node list to recurse
        for (size_t j = 0; j < num_nodes; j++)
        {
            if (nodes[j].addr != ls_node_maps[i].nbor.addr) continue;
            follow_links(j, nodes, visited, num_nodes);
        }
    }
}

bool link_state_route_proto_causes_cycle(const struct ip_addr *src, const struct ip_addr *dst)
{
    //printf("is there a cycle? %s -> %s: \n", ip_addr_to_str(src), ip_addr_to_str(dst));

    //make a set of all nodes
    size_t num_nodes = 0;
    struct ip_addr nodes[LS_NUM_MAP_ENTRIES];
    for (size_t i = 0; i < lengthof(ls_node_maps); i++)
    {
        if (ls_node_maps[i].node.addr == 0 || ls_node_maps[i].nbor.addr == 0) continue;
        //printf("  Link %s -> %s\n", ip_addr_to_str(&ls_node_maps[i].node), ip_addr_to_str(&ls_node_maps[i].nbor));
        const struct ip_addr *node = &ls_node_maps[i].node;

        //check if we have an entry
        for (size_t j = 0; j < num_nodes; j++)
        {
            if (nodes[j].addr == node->addr) goto skip_add;
        }

        //otherwise, we add the node
        nodes[num_nodes++].addr = node->addr;
        //printf("  Add to node set: %s\n", ip_addr_to_str(node));
        skip_add: continue;
    }

    //and stateful tracking info for each node
    bool visited[LS_NUM_MAP_ENTRIES];
    for (size_t i = 0; i < num_nodes; i++) visited[i] = false;

    //find our src node in the set and follow
    for (size_t i = 0; i < num_nodes; i++)
    {
        if (nodes[i].addr == src->addr) follow_links(i, nodes, visited, num_nodes);
    }

    //did we visit the destination? if so, there is a cycle
    for (size_t i = 0; i < num_nodes; i++)
    {
        if (nodes[i].addr == dst->addr && visited[i])
        {
            //printf("CAUSES CYCLE!\n");
            return true;
        }
    }

    //printf("no cycle found.\n");
    return false;
}

static bool ls_causes_cycle[NETHS][NETHS];

void link_state_route_proto_update_cycle_cache(const uint8_t eth_src)
{
    for (size_t eth_dst = 0; eth_dst < ethernet_ninterfaces(); eth_dst++)
    {
        if (eth_src == eth_dst) continue;
        ls_causes_cycle[eth_src][eth_dst] = link_state_route_proto_causes_cycle(
            u3_net_stack_get_ip_addr(eth_src),
            u3_net_stack_get_ip_addr(eth_dst)
        );
    }
}

bool link_state_route_proto_causes_cycle_cached(const uint8_t eth_src, const uint8_t eth_dst)
{
    return ls_causes_cycle[eth_src][eth_dst];
}

/***********************************************************************
 * init and registration code
 **********************************************************************/
void link_state_route_proto_init(void)
{
    u3_net_stack_register_icmp_handler(ICMP_IRQ, 0, &handle_icmp_irq);
    u3_net_stack_register_icmp_handler(ICMP_IR, 0, &handle_icmp_ir);

    //default to causing a cycle, let the algorithm set this correctly
    for (size_t i = 0; i < NETHS; i++)
    {
        for (size_t j = 0; j < NETHS; j++)
        {
            ls_causes_cycle[i][j] = true;
        }
    }
}
