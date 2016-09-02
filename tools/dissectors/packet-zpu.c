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

/* Format of ZPU packets is defined in x300_fw_commons.h,
 * x300_fw_comms_t.
 *
 * Reminder:
 *
 *  uint32_t flags; (ack, error, peek, poke)
 *  uint32_t sequence;
 *  uint32_t addr;
 *  uint32_t data;
 */

#include "config.h"

#include <glib.h>
#include <epan/packet.h>
#include <ctype.h>
#include <stdio.h>
#include <endian.h>

#include "../../host/lib/usrp/x300/x300_fw_common.h"
#include "zpu_addr_names.h"

#define LOG_HEADER  "[ZPU] "

#ifndef min
#define min(a,b)    ((a<b)?a:b)
#endif // min

const unsigned int FW_PORT = X300_FW_COMMS_UDP_PORT;

static int proto_zpu = -1;
static int hf_zpu_flags = -1;
static int hf_zpu_flags_ack = -1;
static int hf_zpu_flags_error = -1;
static int hf_zpu_flags_poke = -1;
static int hf_zpu_flags_peek = -1;
static int hf_zpu_seq = -1;
static int hf_zpu_addr = -1;
static int hf_zpu_data = -1;
static int hf_zpu_shmem_addr = -1;
static int hf_zpu_shmem_addr_name = -1;

/* Subtree handles: set by register_subtree_array */
static gint ett_zpu = -1;
static gint ett_zpu_flags = -1;
//static gint ett_zpu_shmem = -1;

/* Forward-declare the dissector functions */
void proto_register_zpu(void);
void proto_reg_handoff_zpu(void);
static void dissect_zpu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

/* The dissector itself */
static void dissect_zpu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_item *item;
    proto_tree *zpu_tree;
    proto_item *flags_item;
    proto_tree *flags_tree;
    gint len;

    gboolean is_network;
    gint endianness;

    if (pinfo->match_uint == FW_PORT) {
        is_network = TRUE;
        endianness = ENC_BIG_ENDIAN;
    }
    else {
        is_network = FALSE;
        endianness = ENC_LITTLE_ENDIAN;
    }

    len = tvb_reported_length(tvb);

    col_append_str(pinfo->cinfo, COL_PROTOCOL, "/ZPU");
    /*col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "ZPU", tvb_format_text_wsp(tvb, 0, len));*/
    col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "ZPU");

    if (tree)
    {
        item = proto_tree_add_item(tree, proto_zpu, tvb, 0, min(16, len), ENC_NA);

	// Dissect 'flags'
        if (len >= 4)
        {
            zpu_tree = proto_item_add_subtree(item, ett_zpu);

            flags_item = proto_tree_add_item(zpu_tree, hf_zpu_flags, tvb, 0, 4, endianness);
            flags_tree = proto_item_add_subtree(flags_item, ett_zpu_flags);

            proto_tree_add_item(flags_tree, hf_zpu_flags_ack, tvb, 0, 4, ENC_NA);
            proto_tree_add_item(flags_tree, hf_zpu_flags_error, tvb, 0, 4, ENC_NA);
            proto_tree_add_item(flags_tree, hf_zpu_flags_poke, tvb, 0, 4, ENC_NA);
            proto_tree_add_item(flags_tree, hf_zpu_flags_peek, tvb, 0, 4, ENC_NA);

            // Dissect 'sequence number'
            if (len >= 8)
            {
                proto_tree_add_item(zpu_tree, hf_zpu_seq, tvb, 4, 4, ENC_NA);

                // Dissect 'address'
                if (len >= 12)
                {
                    proto_tree_add_item(zpu_tree, hf_zpu_addr, tvb, 8, 4, ENC_NA);
#if VERSION_MAJOR == 1
                    guint8 *bytes = tvb_get_string(tvb, 8, 4);
#elif VERSION_MAJOR == 2
                    guint8 *bytes = tvb_get_string(wmem_packet_scope(), tvb, 8, 4);
#else
#error Wireshark version not found or not compatible
#endif
                    unsigned int addr = 0;
                    memcpy(&addr, bytes, 4);
                    /* TODO proper endianness handling */
                    addr = (addr >> 24) | ((addr & 0x00FF0000) >> 8) | ((addr & 0x0000FF00) << 8) | ((addr & 0x000000FF) << 24);
                    /* TODO check the actual size of shmem instead of this constant */
                    if (addr >= X300_FW_SHMEM_BASE && addr <= X300_FW_SHMEM_BASE + 0x2000)
                    {
                        proto_item *shmem_addr_item = NULL;

                        // Convert the address to a register number (32-bit registers == 4-byte registers)
                        addr -= X300_FW_SHMEM_BASE;
                        addr /= 4;

                        shmem_addr_item = proto_tree_add_uint(zpu_tree, hf_zpu_shmem_addr, tvb, 8, 4, addr);
                        proto_item_append_text(shmem_addr_item, ", Register name: %s",
                                val_to_str(addr, X300_SHMEM_NAMES, "Unknown (0x%04x)")
                        );

                    }

                    // Dissect 'data'
                    if (len >= 16)
                    {
                        proto_tree_add_item(zpu_tree, hf_zpu_data, tvb, 12, 4, ENC_NA);
                    }
                }
            }
        }
    }
}

void proto_register_zpu(void)
{
    static hf_register_info hf[] = {
        { &hf_zpu_flags,
            { "Flags", "zpu.flags",
                FT_UINT32, BASE_HEX,
                NULL, 0x0,
                NULL, HFILL }
        },
            { &hf_zpu_flags_ack,
                { "ACK", "zpu.flags.ack",
                    FT_BOOLEAN, BASE_NONE,
                    NULL, 0x1,
                    NULL, HFILL }
            },
            { &hf_zpu_flags_error,
                { "Error", "zpu.flags.error",
                    FT_BOOLEAN, BASE_NONE,
                    NULL, 0x2,
                    NULL, HFILL }
            },
            { &hf_zpu_flags_poke,
                { "Poke", "zpu.flags.poke",
                    FT_BOOLEAN, BASE_NONE,
                    NULL, 0x4,
                    NULL, HFILL }
            },
            { &hf_zpu_flags_peek,
                { "Peek", "zpu.flags.peek",
                    FT_BOOLEAN, BASE_NONE,
                    NULL, 0x8,
                    NULL, HFILL }
            },
        { &hf_zpu_seq,
            { "Sequence ID", "zpu.seq",
                FT_UINT32, BASE_HEX,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_zpu_addr,
            { "Address", "zpu.addr",
                FT_UINT32, BASE_HEX,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_zpu_shmem_addr,
            { "SHMEM section", "zpu.shmem",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_zpu_data,
            { "Data", "zpu.data",
                FT_UINT32, BASE_HEX,
                NULL, 0x0,
                NULL, HFILL }
        },
    };

    static gint *ett[] = {
        &ett_zpu,
        &ett_zpu_flags,
        //&ett_zpu_shmem
    };

    proto_zpu = proto_register_protocol("ZPU FW", "ZPU", "zpu");
    proto_register_field_array(proto_zpu, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    register_dissector("zpu", dissect_zpu, proto_zpu);
}

/* Handler registration */
void proto_reg_handoff_zpu(void)
{
    /* register dissector for UDP packets */
    static dissector_handle_t zpu_handle;
    zpu_handle = create_dissector_handle(dissect_zpu, proto_zpu);
    dissector_add_uint("udp.port", FW_PORT, zpu_handle);
}
