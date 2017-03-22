//
// Copyright 2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../../../host/lib/usrp/n230/n230_fw_comm_protocol.h"

#include <trace.h>
#include <string.h> //memcmp

bool process_fw_comm_protocol_pkt(
    const fw_comm_pkt_t* request,
    fw_comm_pkt_t* response,
    uint8_t product_id,
    uint32_t iface_id,
    poke32_func poke_callback,
    peek32_func peek_callback)
{
    bool send_response = false;

    uint16_t signature = request->id;
    uint8_t version = FW_COMM_GET_PROTOCOL_VER(request->id);
    uint8_t product = FW_COMM_GET_PRODUCT_ID(request->id);
    if (signature == FW_COMM_PROTOCOL_SIGNATURE &&  //Verify protocol
        version   <= FW_COMM_PROTOCOL_VERSION &&    //Verify protocol version (older versions supported)
        product   == product_id)                    //Verify device
    {
        //Request is valid. Copy it into the reply.
        memcpy(response, request, sizeof(fw_comm_pkt_t));

        //Start assuming no error
        response->flags &= ~FW_COMM_FLAGS_ERROR_MASK;

        //Otherwise, run the command set by the flags
        switch (request->flags & FW_COMM_FLAGS_CMD_MASK) {
            case FW_COMM_CMD_ECHO: {
                UHD_FW_TRACE(DEBUG, "fw_comm_protocol::echo()");
                response->data_words = 1;
                response->data[0] = iface_id;
            } break;

            case FW_COMM_CMD_POKE32: {
                UHD_FW_TRACE_FSTR(DEBUG, "fw_comm_protocol::poke32(0x%x)=0x%x",
                    request->addr,*(request->data));
                poke_callback(request->addr, *(request->data));
            } break;

            case FW_COMM_CMD_PEEK32: {
                *(response->data) = peek_callback(request->addr);
                UHD_FW_TRACE_FSTR(DEBUG, "fw_comm_protocol::peek32(0x%x)=0x%x",
                    request->addr,*(response->data));
            } break;

            case FW_COMM_CMD_BLOCK_POKE32: {
                if (request->data_words > FW_COMM_MAX_DATA_WORDS) {
                    response->flags |= FW_COMM_ERR_SIZE_ERROR;
                    response->data_words = FW_COMM_MAX_DATA_WORDS;
                } else {
                    response->data_words = request->data_words;
                }
                UHD_FW_TRACE_FSTR(DEBUG, "fw_comm_protocol::block_poke32(0x%x,%d)",request->addr,response->data_words);
                for (uint32_t i = 0; i < response->data_words; i++) {
                    poke_callback(request->addr + (i * sizeof(uint32_t)), request->data[i]);
                }
            } break;

            case FW_COMM_CMD_BLOCK_PEEK32: {
                if (request->data_words > FW_COMM_MAX_DATA_WORDS) {
                    response->flags |= FW_COMM_ERR_SIZE_ERROR;
                    response->data_words = FW_COMM_MAX_DATA_WORDS;
                } else {
                    response->data_words = request->data_words;
                }
                for (uint32_t i = 0; i < response->data_words; i++) {
                    response->data[i] = peek_callback(request->addr + (i * sizeof(uint32_t)));
                }
                UHD_FW_TRACE_FSTR(DEBUG, "fw_comm_protocol::block_peek32(0x%x,%d)",request->addr,response->data_words);
            } break;

            default: {
                UHD_FW_TRACE(ERROR, "fw_comm_protocol got an invalid command.");
                response->flags |= FW_COMM_ERR_CMD_ERROR;
            }
        }

        //Send a reply if ack requested
        send_response = (request->flags & FW_COMM_FLAGS_ACK);
    } else {    //Size, protocol, product check failed
        UHD_FW_TRACE(WARN, "fw_comm_protocol ignored an unknown request.");
        send_response = false;
    }
    return send_response;
}
