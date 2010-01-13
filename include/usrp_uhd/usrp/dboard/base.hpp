//
// Copyright 2010 Ettus Research LLC
//

#ifndef INCLUDED_USRP_UHD_USRP_DBOARD_BASE_H
#define INCLUDED_USRP_UHD_USRP_DBOARD_BASE_H

#include <usrp_uhd/wax.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <usrp_uhd/usrp/dboard/interface.hpp>

namespace usrp_uhd{ namespace usrp{ namespace dboard{

/*!
 * A daughter board base class for all dboards.
 * Sub classes for xcvr boards should inherit this.
 */
class xcvr_base : boost::noncopyable{
public:
    typedef boost::shared_ptr<xcvr_base> sptr;

    //structors
    xcvr_base(size_t subdev_index, interface::sptr dboard_interface);
    ~xcvr_base(void);

    //interface
    virtual void rx_get(const wax::type &key, wax::type &val) = 0;
    virtual void rx_set(const wax::type &key, const wax::type &val) = 0;
    virtual void tx_get(const wax::type &key, wax::type &val) = 0;
    virtual void tx_set(const wax::type &key, const wax::type &val) = 0;

protected:
    size_t get_subdev_index(void);
    interface::sptr get_interface(void);

private:
    size_t             _subdev_index;
    interface::sptr    _dboard_interface;
};

/*!
 * A rx daughter board only implements rx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class rx_base : public xcvr_base{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    rx_base(size_t subdev_index, interface::sptr sptr_interface);

    virtual ~rx_base(void);

    //override here so the derived classes cannot
    void tx_get(const wax::type &key, wax::type &val);
    void tx_set(const wax::type &key, const wax::type &val);
};

/*!
 * A tx daughter board only implements tx methods.
 * Sub classes for rx-only boards should inherit this.
 */
class tx_base : public xcvr_base{
public:
    /*!
     * Create a new rx dboard object, override in subclasses.
     */
    tx_base(size_t subdev_index, interface::sptr sptr_interface);

    virtual ~tx_base(void);

    //override here so the derived classes cannot
    void rx_get(const wax::type &key, wax::type &val);
    void rx_set(const wax::type &key, const wax::type &val);
};

}}} //namespace

#endif /* INCLUDED_USRP_UHD_USRP_DBOARD_BASE_H */
