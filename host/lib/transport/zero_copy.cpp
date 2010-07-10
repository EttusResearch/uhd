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

#include <uhd/transport/zero_copy.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace uhd::transport;

/***********************************************************************
 * The pure-virtual deconstructor needs an implementation to be happy
 **********************************************************************/
managed_recv_buffer::~managed_recv_buffer(void){
    /* NOP */
}

/***********************************************************************
 * Phony zero-copy recv interface implementation
 **********************************************************************/

//! phony zero-copy recv buffer implementation
class managed_recv_buffer_impl : public managed_recv_buffer{
public:
    managed_recv_buffer_impl(const boost::asio::const_buffer &buff) : _buff(buff){
        /* NOP */
    }

    ~managed_recv_buffer_impl(void){
        delete [] this->cast<const boost::uint8_t *>();
    }

private:
    const boost::asio::const_buffer &get(void) const{
        return _buff;
    }

    const boost::asio::const_buffer _buff;
};

//! phony zero-copy recv interface implementation
struct phony_zero_copy_recv_if::impl{
    size_t max_buff_size;
};

phony_zero_copy_recv_if::phony_zero_copy_recv_if(size_t max_buff_size){
    _impl = UHD_PIMPL_MAKE(impl, ());
    _impl->max_buff_size = max_buff_size;
}

phony_zero_copy_recv_if::~phony_zero_copy_recv_if(void){
    /* NOP */
}

managed_recv_buffer::sptr phony_zero_copy_recv_if::get_recv_buff(void){
    //allocate memory
    boost::uint8_t *recv_mem = new boost::uint8_t[_impl->max_buff_size];

    //call recv() with timeout option
    ssize_t num_bytes = this->recv(boost::asio::buffer(recv_mem, _impl->max_buff_size));

    if (num_bytes <= 0) return managed_recv_buffer::sptr(); //NULL sptr

    //create a new managed buffer to house the data
    return managed_recv_buffer::sptr(
        new managed_recv_buffer_impl(boost::asio::buffer(recv_mem, num_bytes))
    );
}

/***********************************************************************
 * Phony zero-copy send interface implementation
 **********************************************************************/

//! phony zero-copy send buffer implementation
class managed_send_buffer_impl : public managed_send_buffer{
public:
    typedef boost::function<ssize_t(const boost::asio::const_buffer &)> send_fcn_t;

    managed_send_buffer_impl(
        const boost::asio::mutable_buffer &buff,
        const send_fcn_t &send_fcn
    ):
        _buff(buff),
        _send_fcn(send_fcn)
    {
        /* NOP */
    }

    ~managed_send_buffer_impl(void){
        /* NOP */
    }

    ssize_t commit(size_t num_bytes){
        return _send_fcn(boost::asio::buffer(_buff, num_bytes));
    }

private:
    const boost::asio::mutable_buffer &get(void) const{
        return _buff;
    }

    const boost::asio::mutable_buffer _buff;
    const send_fcn_t                  _send_fcn;
};

//! phony zero-copy send interface implementation
struct phony_zero_copy_send_if::impl{
    boost::uint8_t *send_mem;
    managed_send_buffer::sptr send_buff;
};

phony_zero_copy_send_if::phony_zero_copy_send_if(size_t max_buff_size){
    _impl = UHD_PIMPL_MAKE(impl, ());
    _impl->send_mem = new boost::uint8_t[max_buff_size];
    _impl->send_buff = managed_send_buffer::sptr(new managed_send_buffer_impl(
        boost::asio::buffer(_impl->send_mem, max_buff_size),
        boost::bind(&phony_zero_copy_send_if::send, this, _1)
    ));
}

phony_zero_copy_send_if::~phony_zero_copy_send_if(void){
    delete [] _impl->send_mem;
}

managed_send_buffer::sptr phony_zero_copy_send_if::get_send_buff(void){
    return _impl->send_buff; //FIXME there is only ever one send buff, we assume that the caller doesnt hang onto these
}
