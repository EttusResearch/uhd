/*
 * Dissector for UHD CVITA (CHDR) packets
 *
 * Copyright 2010-2014 Ettus Research LLC
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

#include "config.h"

#include <glib.h>
#include <epan/packet.h>
#include <ctype.h>
#include <stdio.h>

#include "../../host/lib/usrp/x300/x300_fw_common.h"

#define LOG_HEADER  "[UHD CHDR] "

#ifndef min
#define min(a,b)    ((a<b)?a:b)
#endif // min

const unsigned int CHDR_PORT = X300_VITA_UDP_PORT;

static int proto_chdr = -1;
static int hf_chdr_hdr = -1;
static int hf_chdr_type = -1;
static int hf_chdr_has_time = -1;
static int hf_chdr_eob = -1;
static int hf_chdr_error = -1;
static int hf_chdr_sequence = -1;
static int hf_chdr_packet_size = -1;
static int hf_chdr_stream_id = -1;
static int hf_chdr_src_dev = -1;
static int hf_chdr_src_ep = -1;
static int hf_chdr_src_blockport = -1;
static int hf_chdr_dst_dev = -1;
static int hf_chdr_dst_ep = -1;
static int hf_chdr_dst_blockport = -1;
static int hf_chdr_timestamp = -1;
static int hf_chdr_payload = -1;
static int hf_chdr_ext_response = -1;
static int hf_chdr_ext_status_code = -1;
static int hf_chdr_ext_seq_num = -1;
static int hf_chdr_cmd = -1;
static int hf_chdr_cmd_address = -1;
static int hf_chdr_cmd_value = -1;

static const value_string CHDR_PACKET_TYPES[] = {
    { 0, "Data" },
    { 1, "Data (End-of-Burst)" },
    { 4, "Flow Control" },
    { 8, "Command" },
    { 12, "Response" },
    { 13, "Error Response" },
};

static const value_string CHDR_PACKET_TYPES_SHORT[] = {
    { 0, "data" },
    { 1, "data" },
    { 4, "fc" },
    { 8, "cmd" },
    { 12, "resp" },
    { 13, "resp" },
};

/* the heuristic dissector is called on every packet with payload.
 * The warning printed for this should only be printed once.
 */
static int heur_warning_printed = 0;

/* Subtree handles: set by register_subtree_array */
static gint ett_chdr = -1;
static gint ett_chdr_header = -1;
static gint ett_chdr_id = -1;
static gint ett_chdr_response = -1;
static gint ett_chdr_cmd = -1;

/* Forward-declare the dissector functions */
void proto_register_chdr(void);
void proto_reg_handoff_chdr(void);
static void dissect_chdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

/* heuristic dissector call. Will always return. */
static gboolean heur_dissect_chdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* whatislove)
{
    if(heur_warning_printed < 1){
        printf(LOG_HEADER"heuristic dissector always returns true!\n");
        heur_warning_printed++;
    }
    dissect_chdr(tvb, pinfo, tree);
    return (TRUE);
}

static void byte_swap(guint8 *bytes, gint len)
{
    guint8 tmp[4];

    if(len != sizeof(tmp)){
        printf(LOG_HEADER"FATAL! number of bytes don't match 32 bit!\n");
        return;
    }

    memcpy(tmp, bytes, sizeof(tmp));
    bytes[0] = tmp[3];
    bytes[1] = tmp[2];
    bytes[2] = tmp[1];
    bytes[3] = tmp[0];
}

static unsigned long long get_timestamp(guint8 *bytes, gint len)
{
    unsigned long long ts;
    unsigned long long trans;
    int it;

    if(len != sizeof(unsigned long long)){
        printf(LOG_HEADER"FATAL! timestamps always consist of 64 bits!\n");
    }

    byte_swap(bytes + 0, 4);
    byte_swap(bytes + 4, 4);

    ts = 0;
    for(it = 0; it < 8; it++){
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
    proto_item *header_item;
    proto_tree *header_tree;
    proto_tree *stream_tree;
    proto_item *response_item;
    proto_tree *response_tree;
    proto_item *cmd_item;
    proto_tree *cmd_tree;
    gint len;

    gint flag_offset;
    guint8 *bytes;
    guint8 hdr_bits = 0;
    gboolean flag_has_time = 0;
    gboolean flag_is_data = 0;
    gboolean flag_is_fc = 0;
    gboolean flag_is_cmd = 0;
    gboolean flag_is_resp = 0;
    gboolean flag_is_eob = 0;
    gboolean flag_is_error = 0;
    uint64_t timestamp;
    gboolean is_network;
    gint endianness;
    gint id_pos_usb[4] = {3, 2, 1, 0};
    gint id_pos_net[4] = {0, 1, 2, 3};
    gint id_pos[4] = {0, 1, 2, 3};

    if(pinfo->match_uint == CHDR_PORT){
        is_network = TRUE;
        flag_offset = 0;
        endianness = ENC_BIG_ENDIAN;
        memcpy(id_pos, id_pos_net, 4 * sizeof(gint));
    }
    else{   // Parsing a USB capture
        is_network = FALSE;
        flag_offset = 3;
        endianness = ENC_LITTLE_ENDIAN;
        memcpy(id_pos, id_pos_usb, 4 * sizeof(gint));
    }

    len = tvb_reported_length(tvb);

    col_append_str(pinfo->cinfo, COL_PROTOCOL, "/CHDR");
    /* This throws a warning: */
    /*col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "CHDR", tvb_format_text_wsp(tvb, 0, len));*/
    col_append_sep_fstr(pinfo->cinfo, COL_INFO, NULL, "CHDR");

    if (tree){
        int header_size = -1; // Total size of the CHDR header. Either 8 or 16.

        guint16 hdr_info;
        if (len >= 4){
            guint8 pkt_type = 0;
            hdr_info = tvb_get_ntohs(tvb, flag_offset);
            header_size = 8; // We now know the header is at least 8 bytes long.
            hdr_bits = (hdr_info & 0xF000) >> 12;
            pkt_type = hdr_bits >> 2;
            flag_is_data = (pkt_type == 0);
            flag_is_fc = (pkt_type == 1);
            flag_is_cmd = (pkt_type == 2);
            flag_is_resp = (pkt_type == 3);
            flag_is_eob = flag_is_data && (hdr_bits & 0x1);
            flag_is_error = flag_is_resp && (hdr_bits & 0x1);
            flag_has_time = hdr_bits & 0x2;
            if (flag_has_time) {
                header_size += 8; // 64-bit timestamp.
            }
            /* header_size is now final. */
        }

        /* Start with a top-level item to add everything else to */
        item = proto_tree_add_item(tree, proto_chdr, tvb, 0, min(len, header_size), ENC_NA);

        if (len >= 4) {
            chdr_tree = proto_item_add_subtree(item, ett_chdr);

            /* Header info. First, a top-level header tree item: */
            header_item = proto_tree_add_item(chdr_tree, hf_chdr_hdr, tvb, flag_offset, 1, endianness);
            header_tree = proto_item_add_subtree(header_item, ett_chdr_header);
            proto_item_append_text(header_item, ", Packet type: %s %04x",
                val_to_str(hdr_bits & 0xD, CHDR_PACKET_TYPES, "Unknown (0x%x)"), hdr_bits
            );
            /* Let us query hdr.type */
            proto_tree_add_string(
                header_tree, hf_chdr_type, tvb, flag_offset, 1,
                val_to_str(hdr_bits & 0xD, CHDR_PACKET_TYPES_SHORT, "invalid")
            );
            /* And other flags */
            proto_tree_add_boolean(header_tree, hf_chdr_has_time, tvb, flag_offset, 1, flag_has_time);
            if (flag_is_data) {
                proto_tree_add_boolean(header_tree, hf_chdr_eob, tvb, flag_offset, 1, flag_is_eob);
            }
            if (flag_is_resp) {
                proto_tree_add_boolean(header_tree, hf_chdr_error, tvb, flag_offset, 1, flag_is_error);
                /*proto_tree_add_boolean(header_tree, hf_chdr_error, tvb, flag_offset, 1, true);*/
            }

            /* These lines add sequence, packet_size and stream ID */
            proto_tree_add_item(chdr_tree, hf_chdr_sequence, tvb, (is_network ? 0:2), 2, endianness);
            proto_tree_add_item(chdr_tree, hf_chdr_packet_size, tvb, (is_network ? 2:0), 2, endianness);

            if (len >= 8){
                /* stream id can be broken down to 4 sections. these are collapsed in a subtree */
                stream_item = proto_tree_add_item(chdr_tree, hf_chdr_stream_id, tvb, 4, 4, endianness);
                stream_tree = proto_item_add_subtree(stream_item, ett_chdr_id);
                proto_tree_add_item(stream_tree, hf_chdr_src_dev, tvb, 4+id_pos[0], 1, ENC_NA);
                proto_tree_add_item(stream_tree, hf_chdr_src_ep,  tvb, 4+id_pos[1], 1, ENC_NA);
                proto_tree_add_item(stream_tree, hf_chdr_dst_dev, tvb, 4+id_pos[2], 1, ENC_NA);
                proto_tree_add_item(stream_tree, hf_chdr_dst_ep,  tvb, 4+id_pos[3], 1, ENC_NA);

                /* Block ports (only add them if address points to a device) */
                guint32 sid = tvb_get_ntohl(tvb, 4);
                guint8* sid_bytes = (guint8*) &sid;
		if (sid_bytes[3] != 0) {
                    proto_tree_add_item(stream_tree, hf_chdr_src_blockport, tvb, 4+2, 1, ENC_NA);
		}
		if (sid_bytes[1] != 0) {
                    proto_tree_add_item(stream_tree, hf_chdr_dst_blockport, tvb, 4+0, 1, ENC_NA);
		}

		/* Append SID in sid_t hex format */
                proto_item_append_text(stream_item, " (%02X:%02X>%02X:%02X)",
                    sid_bytes[3],
                    sid_bytes[2],
                    sid_bytes[1],
                    sid_bytes[0]
                );
                /*proto_item_append_text(stream_item, "%08X", sid);*/


                /* if has_time flag is present interpret timestamp */
                if ((flag_has_time) && (len >= 16)){
                    if (is_network)
                        item = proto_tree_add_item(chdr_tree, hf_chdr_timestamp, tvb, 8, 8, endianness);
                    else{
                        bytes = (guint8*) tvb_get_string_enc(wmem_packet_scope(), tvb, 8, sizeof(unsigned long long), ENC_ASCII);
                        timestamp = get_timestamp(bytes, sizeof(unsigned long long));
                        proto_tree_add_uint64(chdr_tree, hf_chdr_timestamp, tvb, 8, 8, timestamp);
                    }
                }

                int remaining_bytes = (len - header_size);
                int show_raw_payload = (remaining_bytes > 0);

                if (flag_is_cmd && remaining_bytes == 8) {
                    cmd_item = proto_tree_add_item(chdr_tree, hf_chdr_cmd, tvb, header_size, 8, endianness);
                    cmd_tree = proto_item_add_subtree(cmd_item, ett_chdr_cmd);
                    proto_tree_add_item(cmd_tree, hf_chdr_cmd_address, tvb, header_size,     4, endianness);
                    proto_tree_add_item(cmd_tree, hf_chdr_cmd_value,   tvb, header_size + 4, 4, endianness);
                } else if (flag_is_resp) {
                    response_item = proto_tree_add_item(chdr_tree, hf_chdr_ext_response, tvb, header_size, 8, endianness);
                    response_tree = proto_item_add_subtree(response_item, ett_chdr_response);

                    proto_tree_add_item(response_tree, hf_chdr_ext_status_code, tvb, header_size, 4, endianness);
                    /* This will show the 12-bits of sequence ID in the last 2 bytes */
                    proto_tree_add_item(response_tree, hf_chdr_ext_seq_num, tvb, (header_size + 4 + (is_network ? 2 : 0)), 2, endianness);
                } else if (show_raw_payload) {
                    proto_tree_add_item(chdr_tree, hf_chdr_payload, tvb, header_size, -1, ENC_NA);
                }
            }
        }
    }
}

void proto_register_chdr(void)
{
    static hf_register_info hf[] = {
        { &hf_chdr_hdr,
            { "Header bits", "chdr.hdr",
                FT_UINT8, BASE_HEX,
                NULL, 0xF0,
                NULL, HFILL }
        },
        { &hf_chdr_type,
            { "Packet Type", "chdr.hdr.type",
                FT_STRINGZ, BASE_NONE,
                NULL, 0x00,
                "Packet Type", HFILL }
        },
        { &hf_chdr_has_time,
            { "Has time", "chdr.hdr.has_time",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x20,
                NULL, HFILL }
        },
        { &hf_chdr_eob,
            { "End Of Burst", "chdr.hdr.eob",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x10,
                NULL, HFILL }
        },
        { &hf_chdr_error,
            { "Error Flag", "chdr.hdr.error",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x10,
                NULL, HFILL }
        },
        { &hf_chdr_sequence,
            { "Sequence ID", "chdr.seq",
                FT_UINT16, BASE_DEC,
                NULL, 0x0FFF,
                NULL, HFILL }
        },
        { &hf_chdr_packet_size,
            { "Packet size", "chdr.size",
                FT_UINT16, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_chdr_stream_id,
            { "Stream ID", "chdr.sid",
                FT_IPv4, BASE_NONE,
                NULL, 0x0,
                NULL,  HFILL }
        },
        { &hf_chdr_src_dev,
            { "Source device", "chdr.src_dev",
                FT_UINT8, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_chdr_src_ep,
            { "Source endpoint", "chdr.src_ep",
                FT_UINT8, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_chdr_src_blockport,
            { "Source block port", "chdr.src_bp",
                FT_UINT8, BASE_DEC,
                NULL, 0xF,
                NULL, HFILL }
        },
        { &hf_chdr_dst_dev,
            { "Destination device", "chdr.dst_dev",
                FT_UINT8, BASE_DEC,
                NULL,  0x0,
                NULL, HFILL }
        },
        { &hf_chdr_dst_ep,
            { "Destination endpoint", "chdr.dst_ep",
                FT_UINT8, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_chdr_dst_blockport,
            { "Destination block port", "chdr.dst_bp",
                FT_UINT8, BASE_DEC,
                NULL, 0xF,
                NULL, HFILL }
        },
        { &hf_chdr_timestamp,
            { "Time", "chdr.time",
                FT_UINT64, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_chdr_payload,
            { "Payload", "chdr.payload",
                FT_BYTES, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL
            }
        },
        { &hf_chdr_ext_response,
            { "Response", "chdr.res",
                FT_BYTES, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_chdr_ext_status_code,
            { "Status code", "chdr.res.status",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_chdr_ext_seq_num,
            { "Response to sequence ID", "chdr.res.seq",
                FT_UINT16, BASE_DEC,
                NULL, 0x0FFF,
                NULL, HFILL }
        },
        { &hf_chdr_cmd,
            { "Command", "chdr.cmd",
                FT_BYTES, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_chdr_cmd_address,
            { "Register Address", "chdr.cmd.addr",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
        },
        { &hf_chdr_cmd_value,
            { "Command Value", "chdr.cmd.val",
                FT_UINT32, BASE_HEX,
                NULL, 0x0,
                NULL, HFILL }
        },
    };

    static gint *ett[] = {
        &ett_chdr,
        &ett_chdr_header,
        &ett_chdr_id,
        &ett_chdr_response,
        &ett_chdr_cmd
    };

    proto_chdr = proto_register_protocol("UHD CHDR", "CHDR", "chdr");
    proto_register_field_array(proto_chdr, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    register_dissector("chdr", dissect_chdr, proto_chdr);
}

/* Handler registration */
void proto_reg_handoff_chdr(void)
{
    /* register heuristic dissector for use with USB */
#if VERSION_MAJOR == 1
    heur_dissector_add("usb.bulk", heur_dissect_chdr, proto_chdr);
#elif VERSION_MAJOR == 2
    heur_dissector_add("usb.bulk", heur_dissect_chdr, "USB dissector", "usb_bulk", proto_chdr, HEURISTIC_ENABLE);
#else
#error Wireshark version not found or not compatible
#endif
    /* register dissector for UDP packets */
    static dissector_handle_t chdr_handle;
    chdr_handle = create_dissector_handle(dissect_chdr, proto_chdr);
    dissector_add_uint("udp.port", CHDR_PORT, chdr_handle);
}
