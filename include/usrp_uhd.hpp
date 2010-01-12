//
// Copyright 2010 Ettus Research LLC
//

#ifndef INCLUDED_USRP_UHD_HPP
#define INCLUDED_USRP_UHD_HPP

#include <usrp_uhd/device_addr.hpp>
#include <usrp_uhd/wax.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>
#include <sys/uio.h>

namespace usrp_uhd{

    class usrp_uhd{

    public:
        typedef boost::shared_ptr<usrp_uhd> sptr;
        typedef boost::function<bool(void *data, size_t len)> recv_hdlr_t;
        usrp_uhd(device_addr_t device_addr);
        ~usrp_uhd(void);

        //the io interface
        void send(const std::vector<iovec> &iovs);
        void send(void* data, size_t len); //wrapper
        void recv(const recv_hdlr_t &recv_hdlr);
        void recv(void* &data, size_t &len); //wrapper

        //connect dsps and subdevs
        void connect(const wax::type &src, const wax::type &sink);

        //the properties interface
        wax::proxy props(void);

    private:
        wax::type d_mboard;
    };

} //namespace usrp_uhd

#endif /* INCLUDED_USRP_UHD_HPP */
