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
    //a dboard can be identified by a 16 bit integer
    typedef uint16_t dboard_id_t;

    //dboard constructor (each dboard should have a ::make with this signature)
    typedef base::sptr(*dboard_ctor_t)(base::ctor_args_t const&);

    /*!
     * Register subdevices for a given dboard id.
     *
     * \param dboard_id the dboard id (rx or tx)
     * \param dboard_ctor the dboard constructor function pointer
     * \param num_subdevs the number of subdevs in this dboard
     */
    static void register_subdevs(
        dboard_id_t dboard_id,
        dboard_ctor_t dboard_ctor,
        size_t num_subdevs
    );

public:
    typedef boost::shared_ptr<manager> sptr;
    //structors
    manager(
        dboard_id_t rx_dboard_id,
        dboard_id_t tx_dboard_id,
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
    std::vector<base::sptr> _rx_dboards;
    std::vector<base::sptr> _tx_dboards;
};

}}} //namespace

#endif /* INCLUDED_USRP_UHD_USRP_DBOARD_MANAGER_HPP */
