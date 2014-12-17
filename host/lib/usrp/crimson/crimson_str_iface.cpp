//
// Copyright 2014 Per Vices Corporation
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
#include "crimson_str_iface.hpp"

using namespace uhd;
using namespace uhd::transport;

/***********************************************************************
 * Structors
 **********************************************************************/
crimson_str_iface::crimson_str_iface():
    _ctrl_seq_num(0),
    _protocol_compat(0)
{
    
}

crimson_str_iface::crimson_str_iface(udp_simple::sptr ctrl_transport):
    _ctrl_transport(ctrl_transport),
    _ctrl_seq_num(0),
    _protocol_compat(0)
{
    memset(_buff, '\0', CRIMSON_MTU_SIZE);
}

/***********************************************************************
 * Stream In/Out
 **********************************************************************/


/***********************************************************************
 * Public make function for crimson interface
 **********************************************************************/
crimson_str_iface::sptr crimson_str_iface::make(udp_simple::sptr ctrl_transport){
    return crimson_str_iface::sptr(new crimson_str_iface(ctrl_transport));
}

