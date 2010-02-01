//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_USRP_UHD_DEVICE_HPP
#define INCLUDED_USRP_UHD_DEVICE_HPP

#include <usrp_uhd/device_addr.hpp>
#include <usrp_uhd/props.hpp>
#include <usrp_uhd/wax.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio/buffer.hpp>
#include <vector>

namespace usrp_uhd{

/*!
 * The usrp device interface represents the usrp hardware.
 * The api allows for discovery, configuration, and streaming.
 */
class device : boost::noncopyable, public wax::obj{

public:
    typedef boost::shared_ptr<device> sptr;

    //argument types for send and recv raw methods
    //the send args is a vector of the boost asio buffers
    //the recv args is a callback that takes a boost asio buffer
    typedef std::vector<boost::asio::const_buffer>                   send_args_t;
    typedef boost::function<bool(const boost::asio::const_buffer &)> recv_args_t;

    //structors
    device(void);
    virtual ~device(void);

    /*!
     * \brief Discover usrp devices attached to the host.
     *
     * The hint device address should be used to narrow down the search
     * to particular transport types and/or transport arguments.
     *
     * \param hint a partially (or fully) filled in device address
     * \return a vector of device addresses for all usrps on the system
     */
    static std::vector<device_addr_t> discover(const device_addr_t & hint);

    /*!
     * \brief Create a new usrp device from the device address hint.
     *
     * The make routine will call discover and pick one of the results.
     * By default, the first result will be used to create a new device.
     * Use the which parameter as an index into the list of results.
     *
     * \param hint a partially (or fully) filled in device address
     * \param which which address to use when multiple are discovered
     * \return a shared pointer to a new device instance
     */
    static sptr make(const device_addr_t & hint, size_t which = 0);

    /*!
     * Get the device address for this board.
     */
    device_addr_t get_device_addr(void);

    //the io interface
    virtual void send_raw(const send_args_t &) = 0;
    virtual void recv_raw(const recv_args_t &) = 0;

    //connect dsps and subdevs
    void connect(const wax::obj &src, const wax::obj &sink);
};

} //namespace usrp_uhd

#endif /* INCLUDED_USRP_UHD_DEVICE_HPP */
