/* packet-chdr.c
 * Dissector for UHD CHDR packets
 *
 *
 * Copyright 2010-2013 Ettus Research LLC
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

#define CHDR_PORT 49153

#include "config.h"

#include <glib.h>
#include <epan/packet.h>
#include <ctype.h>
#include <stdio.h>

void proto_register_chdr(void);
void proto_reg_handoff_chdr(void);

static int proto_chdr = -1;
static int hf_chdr = -1;
static int hf_chdr_hdr = -1;
static int hf_chdr_is_extension = -1;
static int hf_chdr_reserved = -1;
static int hf_chdr_has_time = -1;
static int hf_chdr_eob = -1;
static int hf_chdr_sequence = -1;
static int hf_chdr_packet_size = -1;
static int hf_chdr_stream_id = -1;
static int hf_chdr_src_dev = -1;
static int hf_chdr_src_ep = -1;
static int hf_chdr_dst_dev = -1;
static int hf_chdr_dst_ep = -1;
static int hf_chdr_timestamp = -1;
static int hf_chdr_payload = -1;

/* the heuristic dissector is called on every packet with payload.
 * The warning printed for this should only be printed once.
 */
static int heur_warning_printed = 0;

/* Forward-declare the dissector functions */
static void dissect_chdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

/* Subtree handles: set by register_subtree_array */
static gint ett_chdr = -1;
static gint ett_chdr_id = -1;


/* heuristic dissector call. Will always return. */
static gboolean heur_dissect_chdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	if(heur_warning_printed < 1){
		printf("heuristic dissector for UHD CHDR always returns true!\n");
		heur_warning_printed++;
	}
	dissect_chdr(tvb, pinfo, tree); 
    return (TRUE);
}

static void byte_swap(guint8 *bytes, gint len)
{
	guint8 tmp[4];
	
	if(len != 4){
		printf("FATAL! number of bytes don't match 32 bit!\n");
		return;
	}
	
	memcpy(tmp, bytes, 4);
	bytes[0] = tmp[3];
	bytes[1] = tmp[2];
	bytes[2] = tmp[1];
	bytes[3] = tmp[0];
}

static guint64 get_timestamp(guint8 *bytes, gint len)
{
	guint64 ts;
	guint64 trans;
	int it;
	
	if(len != 8){
		printf("FATAL! timestamps always consist of 64 bits!\n");
	}
	
	byte_swap(bytes, 4);
	byte_swap(bytes+4, 4);

	ts = 0;
	for(it = 0 ; it < 8 ; it++){
		ts = ts << 8;
		trans = (guint64) bytes[it];
		ts = ts | trans;
	}

	return (ts);
}

/* The dissector itself */
static void dissect_chdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	// Here are all the variables	
	proto_item *item;
	proto_item *stream_item;
    proto_tree *chdr_tree;
    proto_tree *stream_tree;
    gint len;
    
    gint flag_offset;
	guint8 *bytes;
	gboolean flag_has_time;
	guint64 timestamp;
	gboolean is_network;
	gint endianness;
	gint id_pos_usb[4] = {7, 6, 5, 4};
	gint id_pos_net[4] = {4, 5, 6, 7};
	gint id_pos[4] = {7, 6, 5, 4};
	
	if(pinfo->match_uint == CHDR_PORT){
		is_network = TRUE;
		flag_offset = 0;
		endianness = ENC_BIG_ENDIAN;
		memcpy(id_pos, id_pos_net, 4 * sizeof(gint));
	}
	else{
		is_network = FALSE;
		flag_offset = 3;
		endianness = ENC_LITTLE_ENDIAN;
		memcpy(id_pos, id_pos_usb, 4 * sizeof(gint));
	}

	len = tvb_reported_length(tvb);

    col_append_str(pinfo->cinfo, COL_PROTOCOL, "/CHDR");
    col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "CHDR",
        tvb_format_text_wsp(tvb, 0, len));

    if (tree) {
        /* Start with a top-level item to add everything else to */
        item = proto_tree_add_item(tree, proto_chdr, tvb, 0, 16, ENC_NA);
        chdr_tree = proto_item_add_subtree(item, ett_chdr);

		bytes = tvb_get_string(tvb, 0, 4);
		flag_has_time = bytes[flag_offset] & 0x20;

		/* These lines add flag info to tree */
		proto_tree_add_item(chdr_tree, hf_chdr_is_extension, tvb, flag_offset, 1, ENC_NA);
		proto_tree_add_item(chdr_tree, hf_chdr_reserved, tvb, flag_offset, 1, ENC_NA);
		proto_tree_add_item(chdr_tree, hf_chdr_has_time, tvb, flag_offset, 1, ENC_NA);
		proto_tree_add_item(chdr_tree, hf_chdr_eob, tvb, flag_offset, 1, ENC_NA);

		/* These lines add sequence, packet_size and stream ID */
		proto_tree_add_item(chdr_tree, hf_chdr_sequence, tvb, is_network ? 0:2, 2, endianness);
		proto_tree_add_item(chdr_tree, hf_chdr_packet_size, tvb, is_network ? 2:0, 2, endianness);
		
		/* stream id can be broken down to 4 sections. these are collapsed in a subtree */
		stream_item = proto_tree_add_item(chdr_tree, hf_chdr_stream_id, tvb, 4, 4, endianness);
		stream_tree = proto_item_add_subtree(stream_item, ett_chdr_id);
		proto_tree_add_item(stream_tree, hf_chdr_src_dev, tvb, id_pos[0], 1, ENC_NA);
		proto_tree_add_item(stream_tree, hf_chdr_src_ep, tvb, id_pos[1], 1, ENC_NA);
		proto_tree_add_item(stream_tree, hf_chdr_dst_dev, tvb, id_pos[2], 1, ENC_NA);
		proto_tree_add_item(stream_tree, hf_chdr_dst_ep, tvb, id_pos[3], 1, ENC_NA);
		
		/* if has_time flag is present interpret timestamp */
		if(flag_has_time){
			item = proto_tree_add_item(chdr_tree, hf_chdr_timestamp, tvb, 8, 8, endianness);
			
			if(!is_network){
				bytes = (guint8*) tvb_get_string(tvb, 8, 8);
				timestamp = get_timestamp(bytes, 8);
			}
		}
    }
}

void
proto_register_chdr(void)
{
    static hf_register_info hf[] = {
        { &hf_chdr,
            { "CHDR", "chdr.hdr", FT_STRING, BASE_NONE,
              NULL, 0x0, NULL, HFILL }
		},
		{ &hf_chdr_is_extension,
            { "CHDR is extension", "chdr.is_extension",
            FT_BOOLEAN, BASE_NONE,
            NULL, 0x80,
            NULL, HFILL }
        },
		{ &hf_chdr_reserved,
            { "CHDR reserved", "chdr.reserved",
            FT_BOOLEAN, BASE_NONE,
            NULL, 0x40,
            NULL, HFILL }
        },
		{ &hf_chdr_has_time,
            { "CHDR has time", "chdr.has_time",
            FT_BOOLEAN, BASE_NONE,
            NULL, 0x20,
            NULL, HFILL }
        },
		{ &hf_chdr_eob,
            { "CHDR end of burst", "chdr.eob",
            FT_BOOLEAN, BASE_NONE,
            NULL, 0x10,
            NULL, HFILL }
        },
		{ &hf_chdr_sequence,
            { "CHDR sequence", "chdr.sequence",
            FT_UINT16, BASE_DEC,
            NULL, 0x0FFF,
            NULL, HFILL }
        },
		{ &hf_chdr_packet_size,
            { "CHDR packet size", "chdr.packet_size",
            FT_UINT16, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
		{ &hf_chdr_stream_id,
            { "CHDR stream id", //name
				"chdr.stream_id", //abbr
            	FT_IPv4, //type
				BASE_NONE, //display
            	NULL, //strings
				0x0, //bitmask
            	NULL, //brief description
				HFILL // id
			}
        },
        { &hf_chdr_src_dev,
            { "CHDR source device", //name
				"chdr.src_dev", //abbr
            	FT_UINT8, //type
				BASE_DEC, //display
            	NULL, //strings
				0x0, //bitmask
            	NULL, //brief description
				HFILL // id
			}
        },
        { &hf_chdr_src_ep,
            { "CHDR source endpoint", //name
				"chdr.src_ep", //abbr
            	FT_UINT8, //type
				BASE_DEC, //display
            	NULL, //strings
				0x0, //bitmask
            	NULL, //brief description
				HFILL // id
			}
        },
        { &hf_chdr_dst_dev,
            { "CHDR destination device", //name
				"chdr.dst_dev", //abbr
            	FT_UINT8, //type
				BASE_DEC, //display
            	NULL, //strings
				0x0, //bitmask
            	NULL, //brief description
				HFILL // id
			}
        },
        { &hf_chdr_dst_ep,
            { "CHDR destination endpoint", //name
				"chdr.dst_ep", //abbr
            	FT_UINT8, //type
				BASE_DEC, //display
            	NULL, //strings
				0x0, //bitmask
            	NULL, //brief description
				HFILL // id
			}
        },

		{ &hf_chdr_timestamp,
            { "CHDR timestamp", //name
				"chdr.timestamp", //abbr
            	FT_UINT64, //type
				BASE_DEC, //display
            	NULL, //strings
				0x0, //bitmask
            	NULL, //brief description
				HFILL
			}
        },
    };

    static gint *ett[] = {
        &ett_chdr,
        &ett_chdr_id
    };

    proto_chdr = proto_register_protocol("UHD CHDR", "CHDR", "chdr");
    proto_register_field_array(proto_chdr, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    register_dissector("chdr", dissect_chdr, proto_chdr);
}

/* Handler registration */
void
proto_reg_handoff_chdr(void)
{
    /* register heuristic dissector for use with USB */
    heur_dissector_add("usb.bulk", heur_dissect_chdr, proto_chdr);

	/* register dissector for UDP packets */
    static dissector_handle_t chdr_handle;
    chdr_handle = create_dissector_handle(dissect_chdr, proto_chdr);
    dissector_add_uint("udp.port", CHDR_PORT, chdr_handle);
}

