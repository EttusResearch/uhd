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

#include <uhd/usrp/usrp_e.hpp>
#include <uhd/usrp/dboard_manager.hpp>

#ifndef INCLUDED_USRP_E_IMPL_HPP
#define INCLUDED_USRP_E_IMPL_HPP

class usrp_e_impl; // dummy class declaration

/*!
 * Make a usrp_e dboard interface.
 * \param impl a pointer to the usrp_e impl object
 * \return a sptr to a new dboard interface
 */
uhd::usrp::dboard_interface::sptr make_usrp_e_dboard_interface(usrp_e_impl *impl);

/*!
 * Simple wax obj proxy class:
 * Provides a wax obj interface for a set and a get function.
 * This allows us to create nested properties structures
 * while maintaining flattened code within the implementation.
 */
class wax_obj_proxy : public wax::obj{
public:
    typedef boost::function<void(const wax::obj &, wax::obj &)>       get_t;
    typedef boost::function<void(const wax::obj &, const wax::obj &)> set_t;
    typedef boost::shared_ptr<wax_obj_proxy> sptr;

    static sptr make(const get_t &get, const set_t &set){
        return sptr(new wax_obj_proxy(get, set));
    }

    ~wax_obj_proxy(void){
        /* NOP */
    }

private:
    get_t _get;
    set_t _set;

    wax_obj_proxy(const get_t &get, const set_t &set){
        _get = get;
        _set = set;
    };

    void get(const wax::obj &key, wax::obj &val){
        return _get(key, val);
    }

    void set(const wax::obj &key, const wax::obj &val){
        return _set(key, val);
    }
};

/*!
 * USRP1E implementation guts:
 * The implementation details are encapsulated here.
 * Handles properties on the mboard, dboard, dsps...
 */
class usrp_e_impl : public uhd::device{
public:
    //structors
    usrp_e_impl(const std::string &node);
    ~usrp_e_impl(void);

    //the io interface
    size_t send(const boost::asio::const_buffer &, const uhd::tx_metadata_t &, const uhd::io_type_t &);
    size_t recv(const boost::asio::mutable_buffer &, uhd::rx_metadata_t &, const uhd::io_type_t &);

    /*!
     * Perform an ioctl call on the device node file descriptor.
     * This will throw when the internal ioctl call fails.
     * \param request the control word
     * \param mem pointer to some memory
     */
    void ioctl(int request, void *mem);

private:
    static const size_t _max_num_samples = 2048/sizeof(boost::uint32_t);
    int _node_fd;

    //device functions and settings
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //mboard functions and settings
    void mboard_init(void);
    void mboard_get(const wax::obj &, wax::obj &);
    void mboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _mboard_proxy;

    //xx dboard functions and settings
    void dboard_init(void);
    uhd::usrp::dboard_manager::sptr _dboard_manager;

    //rx dboard functions and settings
    void rx_dboard_get(const wax::obj &, wax::obj &);
    void rx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_dboard_proxy;

    //tx dboard functions and settings
    void tx_dboard_get(const wax::obj &, wax::obj &);
    void tx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _tx_dboard_proxy;

    //rx ddc functions and settings
    void rx_ddc_init(void);
    void rx_ddc_get(const wax::obj &, wax::obj &);
    void rx_ddc_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_ddc_proxy;

    //tx duc functions and settings
    void tx_duc_init(void);
    void tx_duc_get(const wax::obj &, wax::obj &);
    void tx_duc_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _tx_duc_proxy;
};

#endif /* INCLUDED_USRP_E_IMPL_HPP */
