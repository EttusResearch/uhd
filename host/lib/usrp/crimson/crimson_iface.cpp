//
// Copyright 2014-2015 Per Vices Corporation
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

#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/utils/images.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/types/dict.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <iostream>
#include <inttypes.h>
#include <uhd/utils/platform.hpp>
#include "crimson_fw_common.h"
#include "crimson_iface.hpp"

using namespace uhd;
using namespace uhd::transport;

static uint32_t seq = 1;

/***********************************************************************
 * Structors
 **********************************************************************/
crimson_iface::crimson_iface(udp_simple::sptr ctrl_transport):
    _ctrl_transport(ctrl_transport),
    _ctrl_seq_num(0),
    _protocol_compat(0)
{
    memset(_buff, '\0', CRIMSON_MTU_SIZE);
}

/***********************************************************************
 * Peek and Poke
 **********************************************************************/
// Never call this function by itself, always call through crimson_impl::get/set()
// else it will mess up the protocol with the sequencing and will contian no error checks.
void crimson_iface::poke_str(std::string data) {
    // populate the command string with sequence number
    data = data.insert(0, (boost::lexical_cast<std::string>(seq++) + ","));
    _ctrl_transport->send( boost::asio::buffer(data, data.length()) );
    return;
}

// Never call this function by itself, always call through crimson_impl::get/set(),
// else it will mess up the protocol with the sequencing and will contian no error checks.
std::string crimson_iface::peek_str(void) {
    uint32_t iseq;
    std::vector<std::string> tokens;
    uint8_t tries = 0;
    uint8_t num_tries = 5;
    bool flow_cntrl = false;

    do {
        // clears the buffer and receives the message
        memset(_buff, 0, CRIMSON_MTU_SIZE);
        const size_t nbytes = _ctrl_transport -> recv(boost::asio::buffer(_buff), 6.250);
        if (nbytes == 0) return "TIMEOUT";


        // parses it through tokens: seq, status, [data]
        this -> parse(tokens, _buff, ',');

		// if parameter was not initialized
		if (tokens.size() < 3) return "0";


		// if flow control
		if (tokens[0] == "flow") flow_cntrl = true;

        // If the message has an error, return ERROR
        if((flow_cntrl==false)&&(tokens[1].c_str()[0] == CMD_ERROR)) return "ERROR";


        // if seq is incorrect, return an error
        sscanf(tokens[0].c_str(), "%"SCNd32, &iseq);


    } while(iseq != seq - 1 && tries++ < num_tries && !flow_cntrl);

    // exits with an error if can't find a matching sequence
    if (tries == num_tries) return "INVLD_SEQ";

    // Return the message, tokens[1] is the sequence number
    if (flow_cntrl) 	{
		//std::cout <<"Read Buffer: "<< _buff<< std::endl;


    	return _buff;
    }
    else		return tokens[2];
}

/***********************************************************************
 * Public make function for crimson interface
 **********************************************************************/
crimson_iface::sptr crimson_iface::make(udp_simple::sptr ctrl_transport){
    return crimson_iface::sptr(new crimson_iface(ctrl_transport));
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void crimson_iface::parse(std::vector<std::string> &tokens, char* data, const char delim) {
	int i = 0;
	while (data[i]) {
		std::string token = "";
		while (data[i] && data[i] != delim) {
			token.push_back(data[i]);
			if (data[i+1] == 0 || data[i+1] == delim)
				tokens.push_back(token);
			i++;
		}
		i++;
	}
	return;
}
