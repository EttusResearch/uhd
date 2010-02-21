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

#ifndef INCLUDED_UHD_USRP_DBOARD_MANAGER_HPP
#define INCLUDED_UHD_USRP_DBOARD_MANAGER_HPP

#include <uhd/dict.hpp>
#include <uhd/wax.hpp>
#include <uhd/props.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_id.hpp>

namespace uhd{ namespace usrp{

/*!
 * A daughter board subdev dboard_manager class.
 * Create subdev instances for each subdev on a dboard.
 * Provide wax::obj access to the subdevs inside.
 */
class dboard_manager : boost::noncopyable{

public:

    //dboard constructor (each dboard should have a ::make with this signature)
    typedef dboard_base::sptr(*dboard_ctor_t)(dboard_base::ctor_args_t const&);

    /*!
     * Register subdevices for a given dboard id.
     *
     * \param dboard_id the dboard id (rx or tx)
     * \param dboard_ctor the dboard constructor function pointer
     * \param subdev_names the names of the subdevs on this dboard
     */
    static void register_subdevs(
        dboard_id_t dboard_id,
        dboard_ctor_t dboard_ctor,
        const prop_names_t &subdev_names
    );

public:
    typedef boost::shared_ptr<dboard_manager> sptr;
    //structors
    dboard_manager(
        dboard_id_t rx_dboard_id,
        dboard_id_t tx_dboard_id,
        dboard_interface::sptr interface
    );
    ~dboard_manager(void);

    //dboard_interface
    prop_names_t get_rx_subdev_names(void);
    prop_names_t get_tx_subdev_names(void);
    wax::obj get_rx_subdev(const std::string &subdev_name);
    wax::obj get_tx_subdev(const std::string &subdev_name);

private:
    //list of rx and tx dboards in this dboard_manager
    //each dboard here is actually a subdevice proxy
    //the subdevice proxy is internal to the cpp file
    uhd::dict<std::string, wax::obj> _rx_dboards;
    uhd::dict<std::string, wax::obj> _tx_dboards;
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_MANAGER_HPP */
