//
// Copyright 2010 Ettus Research LLC
//

#ifndef INCLUDED_USRP_UHD_USRP_MBOARD_BASE_HPP
#define INCLUDED_USRP_UHD_USRP_MBOARD_BASE_HPP

#include <usrp_uhd/wax.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace usrp_uhd{ namespace usrp{ namespace mboard{

/*!
 * A base class for usrp mboard objects.
 */
class base : boost::noncopyable, public wax::obj{
public:
    typedef boost::shared_ptr<base> sptr;
    base(void);
    ~base(void);

    //TODO other api calls

private:
    virtual void get(const wax::type &, wax::type &) = 0;
    virtual void set(const wax::type &, const wax::type &) = 0;
};

}}} //namespace

#endif /* INCLUDED_USRP_UHD_USRP_MBOARD_BASE_HPP */
