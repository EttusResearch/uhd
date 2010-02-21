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

#ifndef INCLUDED_UHD_USRP_DBOARD_BASE_HPP
#define INCLUDED_UHD_USRP_DBOARD_BASE_HPP

#include <uhd/wax.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/dboard_interface.hpp>

namespace uhd{ namespace usrp{

/*!
 * A daughter board dboard_base class for all dboards.
 * Only other dboard dboard_base classes should inherit this.
 */
class dboard_base : boost::noncopyable{
public:
    typedef boost::shared_ptr<dboard_base> sptr;
    //the constructor args consist of a subdev name and an interface
    //derived classes should pass the args into the dboard_base class ctor
    //but should not have to deal with the internals of the args
    typedef boost::tuple<std::string, dboard_interface::sptr, dboard_id_t, dboard_id_t> ctor_args_t;

    //structors
    dboard_base(ctor_args_t const&);
    virtual ~dboard_base(void);

    //interface
    virtual void rx_get(const wax::obj &key, wax::obj &val) = 0;
    virtual void rx_set(const wax::obj &key, const wax::obj &val) = 0;
    virtual void tx_get(const wax::obj &key, wax::obj &val) = 0;
    virtual void tx_set(const wax::obj &key, const wax::obj &val) = 0;

protected:
    std::string get_subdev_name(void);
    dboard_interface::sptr get_interface(void);
    dboard_id_t get_rx_id(void);
    dboard_id_t get_tx_id(void);

private:
    std::string        _subdev_name;
    dboard_interface::sptr    _dboard_interface;
    dboard_id_t        _rx_id, _tx_id;
};

/*!
 * A xcvr daughter board implements rx and tx methods
 * Sub classes for xcvr boards should inherit this.
 */
class xcvr_dboard_base : public dboard_base{
public:
    /*!
     * Create a new xcvr dboard object, override in subclasses.
     */
    xcvr_dboard_base(ctor_args_t const&);
    virtual ~xcvr_dboard_base(void);
};

/*!
 * A rx daughter board only implements rx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class rx_dboard_base : public dboard_base{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    rx_dboard_base(ctor_args_t const&);

    virtual ~rx_dboard_base(void);

    //override here so the derived classes cannot
    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);
};

/*!
 * A tx daughter board only implements tx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class tx_dboard_base : public dboard_base{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    tx_dboard_base(ctor_args_t const&);

    virtual ~tx_dboard_base(void);

    //override here so the derived classes cannot
    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_BASE_HPP */
