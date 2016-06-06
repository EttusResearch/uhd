/*
 * Dissector for Ettus Octoclock packets
 *
 * Copyright 2016 Ettus Research
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
#include <stddef.h>

#include "../../host/lib/usrp_clock/octoclock/common.h"

#define LOG_HEADER  "[Octoclock] "
#define size_mem(t,m) sizeof(((t*)0)->m)
#define packet_elem_size(m) (size_mem(octoclock_packet_t,m))
#define packet_offset(m) (offsetof(octoclock_packet_t, m))
const unsigned int OCTOCLOCK_PORT = OCTOCLOCK_UDP_CTRL_PORT;

static int 	proto_octo 		= -1;
static int 	hf_octo_proto_version 	= -1;
static int 	hf_octo_proto_sequence 	= -1;
static int 	hf_octo_proto_code 	= -1;
static int 	hf_octo_proto_poolsize 	= -1;
static int 	hf_octo_proto_data 	= -1;
static int 	hf_octo_proto_payload 	= -1;
static int 	hf_octo_proto_len 	= -1;
static gint 	ett_octo 		= -1;
static const value_string packetcodes[] = {
	{1, "OCTOCLOCK_QUERY_CMD"},
	{2, "OCTOCLOCK_QUERY_ACK"},
	{3, "SEND_EEPROM_CMD"},
	{4, "SEND_EEPROM_ACK"},
	{5, "BURN_EEPROM_CMD"},
	{6, "BURN_EEPROM_SUCCESS_ACK"},
	{7, "BURN_EEPROM_FAILURE_ACK"},
	{8, "CLEAR_EEPROM_CMD"},
	{9, "CLEAR_EEPROM_ACK"},
	{10, "SEND_STATE_CMD"},
	{11, "SEND_STATE_ACK"},
	{12, "RESET_CMD"},
	{13, "RESET_ACK"},
	{14, "HOST_SEND_TO_GPSDO_CMD"},
	{15, "HOST_SEND_TO_GPSDO_ACK"},
	{16, "SEND_POOLSIZE_CMD"},
	{17, "SEND_POOLSIZE_ACK"},
	{18, "SEND_CACHE_STATE_CMD"},
	{19, "SEND_CACHE_STATE_ACK"},
	{20, "SEND_GPSDO_CACHE_CMD"},
	{21, "SEND_GPSDO_CACHE_ACK"},
	{22, "PREPARE_FW_BURN_CMD"},
	{23, "FW_BURN_READY_ACK"},
	{24, "FILE_TRANSFER_CMD"},
	{25, "FILE_TRANSFER_ACK"},
	{26, "READ_FW_CMD"},
	{27, "READ_FW_ACK"},
	{28, "FINALIZE_BURNING_CMD"},
	{29, "FINALIZE_BURNING_ACK"},
	{ 0, NULL }
};
/* Forward-declare the dissector functions */
void proto_register_octo(void);
void proto_reg_handoff_octo(void);
static void dissect_octo(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

static void dissect_octo(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "OCTO");
	col_clear(pinfo->cinfo, COL_INFO);
	proto_item *ti = proto_tree_add_item(tree, proto_octo, tvb, 0, -1, ENC_NA);
	proto_tree *octo_tree = proto_item_add_subtree(ti, ett_octo);
	proto_tree_add_item(octo_tree,
			hf_octo_proto_version,
			tvb,
			packet_offset(proto_ver),
			packet_elem_size(proto_ver),
			ENC_LITTLE_ENDIAN);
	guint32 rev = tvb_get_letohl(tvb, packet_offset(proto_ver));
	if(rev==3)
	{
		proto_tree_add_item(octo_tree,
				hf_octo_proto_sequence,
				tvb,
				packet_offset(sequence),
				packet_elem_size(sequence),
				ENC_LITTLE_ENDIAN);
		proto_tree_add_item(octo_tree,
				hf_octo_proto_code,
				tvb,
				packet_offset(code),
				packet_elem_size(code),
				ENC_LITTLE_ENDIAN);
		proto_tree_add_item(octo_tree,
				hf_octo_proto_poolsize,
				tvb,
				packet_offset(poolsize),
				packet_elem_size(poolsize),
				ENC_LITTLE_ENDIAN);

		//packet_code_t code = (packet_code_t)(tvb_get_guint8(tvb, packet_offset(code)));

		guint16 len = tvb_get_letohs(tvb, packet_offset(len));
		if(len && len <= packet_elem_size(data))
		{
			proto_tree_add_item(octo_tree,
					hf_octo_proto_payload,
					tvb,
					packet_offset(data),
					len,
					ENC_LITTLE_ENDIAN);
		}
		proto_tree_add_item(octo_tree,
				hf_octo_proto_data,
				tvb,
				packet_offset(data),
				packet_elem_size(data),
				ENC_LITTLE_ENDIAN);
		proto_tree_add_item(octo_tree,
				hf_octo_proto_len,
				tvb,
				packet_offset(len),
				packet_elem_size(len),
				ENC_LITTLE_ENDIAN);
	}
}


void proto_register_octo(void)
{
	static hf_register_info hf[] = {
		{ &hf_octo_proto_version,
			{ "version", "octo.rev",
				FT_UINT32, BASE_DEC,
				NULL, 0x0,
				"Protocol Revision", HFILL }
		},
		{ &hf_octo_proto_sequence,
			{ "sequence number", "octo.seq",
				FT_UINT32, BASE_DEC,
				NULL, 0x0,
				NULL, HFILL }
		},
		{ &hf_octo_proto_code,
			{ "code", "octo.code",
				FT_UINT8, BASE_DEC,
				VALS(packetcodes), 0x0,
				NULL, HFILL }
		},
		{ &hf_octo_proto_poolsize,
			{ "poolsize", "octo.poolsize",
				FT_UINT16, BASE_DEC,
				NULL, 0x0,
				NULL, HFILL }
		},
		{ &hf_octo_proto_payload,
			{ "payload", "octo.payload",
				FT_STRING, BASE_NONE,
				NULL, 0x0,
				NULL, HFILL }
		},
		{ &hf_octo_proto_data,
			{ "data", "octo.data",
				FT_BYTES, BASE_NONE,
				NULL, 0x0,
				NULL, HFILL }
		},
		{ &hf_octo_proto_len,
			{ "length", "octo.len",
				FT_UINT16, BASE_DEC,
				NULL, 0x0,
				NULL, HFILL }
		}
	};
	//protocol subtree array
	static gint *ett[] = {
		&ett_octo
	};
	proto_octo = proto_register_protocol(
			"Octoclock",
			"Octoclock",
			"octo"
			);

	proto_register_field_array(proto_octo, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	register_dissector("octo", dissect_octo, proto_octo);
}

void proto_reg_handoff_octo(void)
{
	static dissector_handle_t octo_handle;
	octo_handle = create_dissector_handle(dissect_octo, proto_octo);
	dissector_add_uint("udp.port", OCTOCLOCK_PORT, octo_handle);
	dissector_add_uint("udp.port", OCTOCLOCK_UDP_GPSDO_PORT, octo_handle);
}
