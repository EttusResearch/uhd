//
// Copyright 2017 Ettus Research
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

#ifndef INCLUDED_ZERO_COPY_FLOW_CTRL_HPP
#define INCLUDED_ZERO_COPY_FLOW_CTRL_HPP

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace transport{

/*!
 * Flow control function.
 * \param buff buffer to be sent or receive buffer being released
 * \return true if OK, false if not
 */
typedef boost::function<bool(managed_buffer::sptr buff)> flow_ctrl_func;

/*!
 * Adds flow control to any zero_copy_if transport.
 */
class UHD_API zero_copy_flow_ctrl : public virtual zero_copy_if {
public:
    typedef boost::shared_ptr<zero_copy_flow_ctrl> sptr;

    /*!
     * Make flow controlled transport.
     *
     * \param transport a shared pointer to the transport interface
     * \param send_flow_ctrl optional send flow control function called before buffer is sent
     * \param recv_flow_ctrl optional receive flow control function called after buffer released
     */
    static sptr make(
        zero_copy_if::sptr transport,
        flow_ctrl_func send_flow_ctrl,
        flow_ctrl_func recv_flow_ctrl
    );
};

}} //namespace

#endif /* INCLUDED_ZERO_COPY_FLOW_CTRL_HPP */
