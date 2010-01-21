//
// Copyright 2010 Ettus Research LLC
//

#ifndef INCLUDED_USRP_UHD_USRP_MBOARD_TEST_HPP
#define INCLUDED_USRP_UHD_USRP_MBOARD_TEST_HPP

#include <usrp_uhd/usrp/mboard/base.hpp>
#include <usrp_uhd/device_addr.hpp>

namespace usrp_uhd{ namespace usrp{ namespace mboard{

/*!
 * A test usrp mboard object.
 * Exercises access routines for the test suite.
 */
class test : public base{
public:
    test(const device_addr_t &);
    ~test(void);

private:
    void get(const wax::type &, wax::type &);
    void set(const wax::type &, const wax::type &);
};

}}} //namespace

#endif /* INCLUDED_USRP_UHD_USRP_MBOARD_TEST_HPP */
