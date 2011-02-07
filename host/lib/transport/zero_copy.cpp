//
// Copyright 2010-2011 Ettus Research LLC
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

using namespace uhd::transport;

/***********************************************************************
 * Safe managed receive buffer
 **********************************************************************/
static void release_nop(void){
    /* NOP */
}

class safe_managed_receive_buffer : public managed_recv_buffer{
public:
    safe_managed_receive_buffer(
        const void *buff, size_t size, const release_fcn_t &release_fcn
    ):
        _buff(buff), _size(size), _release_fcn(release_fcn)
    {
        /* NOP */
    }

    ~safe_managed_receive_buffer(void){
        _release_fcn();
    }

    void release(void){
        release_fcn_t release_fcn = _release_fcn;
        _release_fcn = &release_nop;
        return release_fcn();
    }

private:
    const void *get_buff(void) const{
        return _buff;
    }

    size_t get_size(void) const{
        return _size;
    }

    const void *_buff;
    size_t _size;
    release_fcn_t _release_fcn;
};

managed_recv_buffer::sptr managed_recv_buffer::make_safe(
    const void *buff, size_t size, const release_fcn_t &release_fcn
){
    return sptr(new safe_managed_receive_buffer(buff, size, release_fcn));
}

/***********************************************************************
 * Safe managed send buffer
 **********************************************************************/
static void commit_nop(size_t){
    /* NOP */
}

class safe_managed_send_buffer : public managed_send_buffer{
public:
    safe_managed_send_buffer(
        void *buff, size_t size, const commit_fcn_t &commit_fcn
    ):
        _buff(buff), _size(size), _commit_fcn(commit_fcn)
    {
        /* NOP */
    }

    ~safe_managed_send_buffer(void){
        _commit_fcn(0);
    }

    void commit(size_t num_bytes){
        commit_fcn_t commit_fcn = _commit_fcn;
        _commit_fcn = &commit_nop;
        return commit_fcn(num_bytes);
    }

private:
    void *get_buff(void) const{
        return _buff;
    }

    size_t get_size(void) const{
        return _size;
    }

    void *_buff;
    size_t _size;
    commit_fcn_t _commit_fcn;
};

safe_managed_send_buffer::sptr managed_send_buffer::make_safe(
    void *buff, size_t size, const commit_fcn_t &commit_fcn
){
    return sptr(new safe_managed_send_buffer(buff, size, commit_fcn));
}
