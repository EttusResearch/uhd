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
#include <uhd/usrp/dboard/interface.hpp>

namespace uhd{ namespace usrp{ namespace dboard{

/*!
 * A daughter board base class for all dboards.
 * Only other dboard base classes should inherit this.
 */
class base : boost::noncopyable{
public:
    typedef boost::shared_ptr<base> sptr;
    //the constructor args consist of a subdev name and an interface
    //derived classes should pass the args into the base class ctor
    //but should not have to deal with the internals of the args
    typedef boost::tuple<std::string, interface::sptr> ctor_args_t;

    //structors
    base(ctor_args_t const&);
    virtual ~base(void);

    //interface
    virtual void rx_get(const wax::obj &key, wax::obj &val) = 0;
    virtual void rx_set(const wax::obj &key, const wax::obj &val) = 0;
    virtual void tx_get(const wax::obj &key, wax::obj &val) = 0;
    virtual void tx_set(const wax::obj &key, const wax::obj &val) = 0;

protected:
    std::string get_subdev_name(void);
    interface::sptr get_interface(void);

private:
    std::string        _subdev_name;
    interface::sptr    _dboard_interface;
};

/*!
 * A xcvr daughter board implements rx and tx methods
 * Sub classes for xcvr boards should inherit this.
 */
class xcvr_base : public base{
public:
    /*!
     * Create a new xcvr dboard object, override in subclasses.
     */
    xcvr_base(ctor_args_t const&);
    virtual ~xcvr_base(void);
};

/*!
 * A rx daughter board only implements rx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class rx_base : public base{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    rx_base(ctor_args_t const&);

    virtual ~rx_base(void);

    //override here so the derived classes cannot
    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);
};

/*!
 * A tx daughter board only implements tx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class tx_base : public base{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    tx_base(ctor_args_t const&);

    virtual ~tx_base(void);

    //override here so the derived classes cannot
    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);
};

}}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_BASE_HPP */
