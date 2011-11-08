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

#include <uhd/config.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

namespace uhd{ namespace usrp{

/*!
 * A daughter board subdev dboard_manager class.
 * Create subdev instances for each subdev on a dboard.
 * Provide wax::obj access to the subdevs inside.
 */
class UHD_API dboard_manager : boost::noncopyable{
public:
    typedef boost::shared_ptr<dboard_manager> sptr;

    //dboard constructor (each dboard should have a ::make with this signature)
    typedef dboard_base::sptr(*dboard_ctor_t)(dboard_base::ctor_args_t);

    /*!
     * Register a rx or tx dboard into the system.
     * For single subdevice boards, omit subdev_names.
     * \param dboard_id the dboard id (rx or tx)
     * \param dboard_ctor the dboard constructor function pointer
     * \param name the canonical name for the dboard represented
     * \param subdev_names the names of the subdevs on this dboard
     */
    static void register_dboard(
        const dboard_id_t &dboard_id,
        dboard_ctor_t dboard_ctor,
        const std::string &name,
        const std::vector<std::string> &subdev_names = std::vector<std::string>(1, "0")
    );

    /*!
     * Register an xcvr dboard into the system.
     * For single subdevice boards, omit subdev_names.
     * \param rx_dboard_id the rx unit dboard id
     * \param tx_dboard_id the tx unit dboard id
     * \param dboard_ctor the dboard constructor function pointer
     * \param name the canonical name for the dboard represented
     * \param subdev_names the names of the subdevs on this dboard
     */
    static void register_dboard(
        const dboard_id_t &rx_dboard_id,
        const dboard_id_t &tx_dboard_id,
        dboard_ctor_t dboard_ctor,
        const std::string &name,
        const std::vector<std::string> &subdev_names = std::vector<std::string>(1, "0")
    );

    /*!
     * Make a new dboard manager.
     * \param rx_dboard_id the id of the rx dboard
     * \param tx_dboard_id the id of the tx dboard
     * \param gdboard_id the id of the grand-dboard
     * \param iface the custom dboard interface
     * \param subtree the subtree to load with props
     * \return an sptr to the new dboard manager
     */
    static sptr make(
        dboard_id_t rx_dboard_id,
        dboard_id_t tx_dboard_id,
        dboard_id_t gdboard_id,
        dboard_iface::sptr iface,
        property_tree::sptr subtree
    );
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_MANAGER_HPP */
