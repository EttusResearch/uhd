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

#ifndef INCLUDED_UHD_USRP_MIMO_USRP_HPP
#define INCLUDED_UHD_USRP_MIMO_USRP_HPP

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/tune_result.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>

namespace uhd{ namespace usrp{

/*!
 * The MIMO USRP device class:
 * A mimo usrp facilitates ease-of-use for multi-usrp scenarios.
 * The wrapper provides convenience functions to control the group
 * of underlying devices as if they consisted of a single device.
 */
class UHD_API mimo_usrp : boost::noncopyable{
public:
    typedef boost::shared_ptr<mimo_usrp> sptr;

    /*!
     * Make a new mimo usrp from the device address.
     * \param dev_addr the device address
     * \return a new mimo usrp object
     */
    static sptr make(const device_addr_t &dev_addr);

    /*!
     * Get the underlying device object.
     * This is needed to get access to the streaming API and properties.
     * \return the device object within this simple usrp
     */
    virtual device::sptr get_device(void) = 0;

    /*!
     * Get a printable name for this mimo usrp.
     * \return a printable string
     */
    virtual std::string get_name(void) = 0;

    //TODO

};

}}

#endif /* INCLUDED_UHD_USRP_MIMO_USRP_HPP */
