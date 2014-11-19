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

#ifndef INCLUDED_CRIMSON_IFACE_HPP
#define INCLUDED_CRIMSON_IFACE_HPP

#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/serial.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <uhd/types/wb_iface.hpp>
#include <string>
#include "crimson_fw_common.h"

/*!
 * The crimson interface class:
 * Provides a set of functions to UDP implementation layer.
 */
class crimson_iface : public uhd::wb_iface
{
public:
    typedef boost::shared_ptr<crimson_iface> sptr;
    /*!
     * Make a new crimson interface with the control transport.
     * \param ctrl_transport the udp transport object
     * \return a new crimson interface object
     */
    crimson_iface(uhd::transport::udp_simple::sptr ctrl_transport);

    static crimson_iface::sptr make(uhd::transport::udp_simple::sptr ctrl_transport);

    // Send/write a data packet (string), null terminated
    virtual void poke_str(std::string data);

    // Recieve/read a data packet (string), null terminated
    virtual std::string peek_str(void);

private:
    //this lovely lady makes it all possible
    uhd::transport::udp_simple::sptr _ctrl_transport;

    // add another transport for streaming

    // internal function for tokenizing the inputs
    void parse(std::vector<std::string> &tokens, char* data, const char delim);

    //used in send/recv
    boost::uint32_t _ctrl_seq_num;
    boost::uint32_t _protocol_compat;

    // buffer for in and out
    char _buff[CRIMSON_MTU_SIZE];
};

#endif /* INCLUDED_CRIMSON_IFACE_HPP */
