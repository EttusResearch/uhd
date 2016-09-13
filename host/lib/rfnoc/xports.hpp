//
// Copyright 2016 Ettus Research LLC
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

#include <uhd/types/sid.hpp>
#include <uhd/transport/zero_copy.hpp>

namespace uhd {

    /*! Holds all necessary items for a bidirectional link
     */
    struct both_xports_t
    {
        uhd::transport::zero_copy_if::sptr recv;
        uhd::transport::zero_copy_if::sptr send;
        size_t recv_buff_size;
        size_t send_buff_size;
        uhd::sid_t send_sid;
        uhd::sid_t recv_sid;
    };

};

