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

#ifndef INCLUDED_UHD_TRANSPORT_SMART_BUFFER_HPP
#define INCLUDED_UHD_TRANSPORT_SMART_BUFFER_HPP

#include <boost/asio.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace transport{

/*!
 * A buffer that knows how to free itself:
 *
 * This is just the smart buffer interface.
 * A transport implementation will have its own
 * internal (custom) smart buffer implementation.
 *
 * A smart buffer contains a boost asio const buffer.
 * On destruction, the buffer contents will be freed.
 */
class smart_buffer : boost::noncopyable{
public:
    typedef boost::shared_ptr<smart_buffer> sptr;
    virtual const boost::asio::const_buffer &get(void) const = 0;
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_SMART_BUFFER_HPP */
