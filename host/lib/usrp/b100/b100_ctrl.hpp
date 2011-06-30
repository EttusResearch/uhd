//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_B100_CTRL_HPP
#define INCLUDED_B100_CTRL_HPP

#include "wb_iface.hpp"
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/serial.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include "ctrl_packet.hpp"
#include <boost/thread.hpp>

class b100_ctrl : boost::noncopyable, public wb_iface{
public:
    typedef boost::shared_ptr<b100_ctrl> sptr;

    /*!
     * Make a USRP control object from a data transport
     * \param ctrl_transport a USB data transport
     * \return a new b100 control object
     */
    static sptr make(uhd::transport::zero_copy_if::sptr ctrl_transport);

    /*!
     * Write a byte vector to an FPGA register
     * \param addr the FPGA register address
     * \param bytes the data to write
     * \return 0 on success, error code on failure
     */
    virtual int write(boost::uint32_t addr, const ctrl_data_t &data) = 0;

    /*!
     * Read a byte vector from an FPGA register (blocking read)
     * \param addr the FPGA register address
     * \param len the length of the read
     * \return a vector of bytes from the register(s) in question
     */
    virtual ctrl_data_t read(boost::uint32_t addr, size_t len) = 0;

    /*!
     * Get a sync ctrl packet (blocking)
     * \param the packet data buffer
     * \param the timeout value
     * \return true if it got something
     */
    virtual bool get_ctrl_data(ctrl_data_t &pkt_data, double timeout) = 0;

    virtual bool recv_async_msg(uhd::async_metadata_t &async_metadata, double timeout) = 0;

};

#endif /* INCLUDED_B100_CTRL_HPP */
