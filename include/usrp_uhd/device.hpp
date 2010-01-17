//
// Copyright 2010 Ettus Research LLC
//

#ifndef INCLUDED_USRP_UHD_DEVICE_HPP
#define INCLUDED_USRP_UHD_DEVICE_HPP

#include <usrp_uhd/device_addr.hpp>
#include <usrp_uhd/wax.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>
#include <sys/uio.h>

namespace usrp_uhd{

/*!
 * The usrp device interface represents the usrp hardware.
 * The api allows for discovery, configuration, and streaming.
 */
class device{

public:
    typedef boost::shared_ptr<device> sptr;
    typedef boost::function<bool(void *data, size_t len)> recv_hdlr_t;

    /*!
     * \brief Discover usrp devices attached to the host.
     *
     * The hint device address should be used to narrow down the search
     * to particular transport types and/or transport arguments.
     *
     * \param hint a partially (or fully) filled in device address
     * \return a vector of device addresses for all usrps on the system
     */
    static std::vector<device_addr_t> discover(const device_addr_t& hint);

    /*!
     * \brief Create a new usrp device from the device address hint.
     *
     * The make routine will call discover and pick one of the results.
     * By default, the first result will be used to create a new device.
     * Use the which parameter as an index into the list of results.
     *
     * \param hint a partially (or fully) filled in device address
     * \param which which address to use when multiple are discovered
     * \return a shared pointer to a new device instance
     */
    static sptr make(const device_addr_t& hint, size_t which = 0);

    /*!
     * Deconstructor: called automatically by the shared pointer.
     */
    ~device(void);

    //the io interface
    void send_raw(const std::vector<iovec> &iovs);
    void recv_raw(const recv_hdlr_t &recv_hdlr);

    //connect dsps and subdevs
    void connect(const wax::type &src, const wax::type &sink);

    //the properties interface
    wax::proxy props(void);

private:
    device(const device_addr_t& hint);

    wax::type d_mboard;
};

} //namespace usrp_uhd

#endif /* INCLUDED_USRP_UHD_DEVICE_HPP */
