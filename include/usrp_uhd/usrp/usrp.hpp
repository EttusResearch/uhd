//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/device.hpp>
#include <usrp_uhd/usrp/mboard/base.hpp>
#include <vector>

#ifndef INCLUDED_USRP_UHD_USRP_USRP_HPP
#define INCLUDED_USRP_UHD_USRP_USRP_HPP

namespace usrp_uhd{ namespace usrp{

/*!
 * A usrp device provides a device-level interface to usrp mboards.
 * In most cases, a usrp device will have only one mboard.
 * In the usrp2 mimo case, this device will have two mboards,
 * where one talks through the other's control port.
 */
class usrp : public device{
public:
    usrp(const device_addr_t & device_addr);
    ~usrp(void);

    void send_raw(const send_args_t &);
    void recv_raw(const recv_args_t &);

private:
    void get(const wax::type &, wax::type &);
    void set(const wax::type &, const wax::type &);

    std::vector<mboard::base::sptr> _mboards;
    boost::function<void(const device::send_args_t &)> _send_raw_cb;
    boost::function<void(const device::recv_args_t &)> _recv_raw_cb;
};

}} //namespace

#endif /* INCLUDED_USRP_UHD_USRP_USRP_HPP */
