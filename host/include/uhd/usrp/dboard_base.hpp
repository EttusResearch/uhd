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

#include <uhd/config.hpp>
#include <uhd/wax.hpp>
#include <uhd/utils/pimpl.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/dboard_iface.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Possible subdev connection types:
     *
     * A complex subdevice is physically connected to both channels,
     * which may be connected in one of two ways: IQ or QI (swapped).
     *
     * A real subdevice is only physically connected one channel,
     * either only the I channel or only the Q channel.
     */
    enum subdev_conn_t{
        SUBDEV_CONN_COMPLEX_IQ = 'C',
        SUBDEV_CONN_COMPLEX_QI = 'c',
        SUBDEV_CONN_REAL_I     = 'R',
        SUBDEV_CONN_REAL_Q     = 'r'
    };

    /*!
     * Possible device subdev properties
     */
    enum subdev_prop_t{
        SUBDEV_PROP_NAME,               //ro, std::string
        SUBDEV_PROP_OTHERS,             //ro, prop_names_t
        SUBDEV_PROP_SENSOR,             //ro, sensor_value_t
        SUBDEV_PROP_SENSOR_NAMES,       //ro, prop_names_t
        SUBDEV_PROP_GAIN,               //rw, double
        SUBDEV_PROP_GAIN_RANGE,         //ro, gain_range_t
        SUBDEV_PROP_GAIN_NAMES,         //ro, prop_names_t
        SUBDEV_PROP_FREQ,               //rw, double
        SUBDEV_PROP_FREQ_RANGE,         //ro, freq_range_t
        SUBDEV_PROP_ANTENNA,            //rw, std::string
        SUBDEV_PROP_ANTENNA_NAMES,      //ro, prop_names_t
        SUBDEV_PROP_CONNECTION,         //ro, subdev_conn_t
        SUBDEV_PROP_ENABLED,            //rw, bool
        SUBDEV_PROP_USE_LO_OFFSET,      //ro, bool
        SUBDEV_PROP_BANDWIDTH           //rw, double
    };

/*!
 * A daughter board dboard_base class for all dboards.
 * Only other dboard dboard_base classes should inherit this.
 */
class UHD_API dboard_base : boost::noncopyable{
public:
    typedef boost::shared_ptr<dboard_base> sptr;
    /*!
     * An opaque type for the dboard constructor args.
     * Derived classes should pass the args into the base class,
     * but should not deal with the internals of the args.
     */
    typedef void * ctor_args_t;

    //structors
    dboard_base(ctor_args_t);
    virtual ~dboard_base(void);

    //interface
    virtual void rx_get(const wax::obj &key, wax::obj &val) = 0;
    virtual void rx_set(const wax::obj &key, const wax::obj &val) = 0;
    virtual void tx_get(const wax::obj &key, wax::obj &val) = 0;
    virtual void tx_set(const wax::obj &key, const wax::obj &val) = 0;

protected:
    std::string get_subdev_name(void);
    dboard_iface::sptr get_iface(void);
    dboard_id_t get_rx_id(void);
    dboard_id_t get_tx_id(void);

private:
    UHD_PIMPL_DECL(impl) _impl;
};

/*!
 * A xcvr daughter board implements rx and tx methods
 * Sub classes for xcvr boards should inherit this.
 */
class UHD_API xcvr_dboard_base : public dboard_base{
public:
    /*!
     * Create a new xcvr dboard object, override in subclasses.
     */
    xcvr_dboard_base(ctor_args_t);

    virtual ~xcvr_dboard_base(void);
};

/*!
 * A rx daughter board only implements rx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class UHD_API rx_dboard_base : public dboard_base{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    rx_dboard_base(ctor_args_t);

    virtual ~rx_dboard_base(void);

    //override here so the derived classes cannot
    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);
};

/*!
 * A tx daughter board only implements tx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class UHD_API tx_dboard_base : public dboard_base{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    tx_dboard_base(ctor_args_t);

    virtual ~tx_dboard_base(void);

    //override here so the derived classes cannot
    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_BASE_HPP */
