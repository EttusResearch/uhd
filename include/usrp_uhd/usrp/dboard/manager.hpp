//
// Copyright 2010 Ettus Research LLC
//

#ifndef INCLUDED_USRP_UHD_USRP_DBOARD_MANAGER_HPP
#define INCLUDED_USRP_UHD_USRP_DBOARD_MANAGER_HPP

#include <vector>
#include <usrp_uhd/wax.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <usrp_uhd/usrp/dboard/base.hpp>

namespace usrp_uhd{ namespace usrp{ namespace dboard{

/*!
 * A daughter board subdev manager class.
 * Create subdev instances for each subdev on a dboard.
 * Provide wax::obj access to the subdevs inside.
 */
class manager : boost::noncopyable{
public:
    typedef boost::shared_ptr<manager> sptr;
    //structors
    manager(
        uint16_t rx_dboard_id,
        uint16_t tx_dboard_id,
        interface::sptr dboard_interface
    );
    ~manager(void);

    //interface
    size_t get_num_rx_subdevs(void);
    size_t get_num_tx_subdevs(void);
    wax::obj::sptr get_rx_subdev(size_t subdev_index);
    wax::obj::sptr get_tx_subdev(size_t subdev_index);

private:
    //list of rx and tx dboards in this manager
    //each dboard here is actually a subdevice
    std::vector<xcvr_base::sptr> _rx_dboards;
    std::vector<xcvr_base::sptr> _tx_dboards;
};

}}} //namespace

#endif /* INCLUDED_USRP_UHD_USRP_DBOARD_MANAGER_HPP */
