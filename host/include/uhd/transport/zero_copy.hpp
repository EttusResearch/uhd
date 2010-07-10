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

#ifndef INCLUDED_UHD_TRANSPORT_ZERO_COPY_HPP
#define INCLUDED_UHD_TRANSPORT_ZERO_COPY_HPP

#include <uhd/config.hpp>
#include <uhd/utils/pimpl.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace transport{

    /*!
     * A managed receive buffer:
     * Contains a reference to transport-managed memory,
     * and a method to release the memory after reading.
     */
    class UHD_API managed_recv_buffer : boost::noncopyable{
    public:
        typedef boost::shared_ptr<managed_recv_buffer> sptr;

        /*!
         * Managed recv buffer destructor:
         * Signal to the transport that we are done with the buffer.
         * This should be called to release the buffer to the transport.
         * After calling, the referenced memory should be considered invalid.
         */
        virtual ~managed_recv_buffer(void) = 0;

        /*!
         * Get the size of the underlying buffer.
         * \return the number of bytes
         */
        inline size_t size(void) const{
            return boost::asio::buffer_size(this->get());
        }

        /*!
         * Get a pointer to the underlying buffer.
         * \return a pointer into memory
         */
        template <class T> inline T cast(void) const{
            return boost::asio::buffer_cast<T>(this->get());
        }

    private:
        /*!
         * Get a reference to the internal const buffer.
         * The buffer has a reference to memory and a size.
         * \return a boost asio const buffer
         */
        virtual const boost::asio::const_buffer &get(void) const = 0;
    };

    /*!
     * A managed send buffer:
     * Contains a reference to transport-managed memory,
     * and a method to release the memory after writing.
     */
    class UHD_API managed_send_buffer : boost::noncopyable{
    public:
        typedef boost::shared_ptr<managed_send_buffer> sptr;

        /*!
         * Signal to the transport that we are done with the buffer.
         * This should be called to commit the write to the transport object.
         * After calling, the referenced memory should be considered invalid.
         * \param num_bytes the number of bytes written into the buffer
         * \return the number of bytes written, 0 for timeout, negative for error
         */
        virtual ssize_t commit(size_t num_bytes) = 0;

        /*!
         * Get the size of the underlying buffer.
         * \return the number of bytes
         */
        inline size_t size(void) const{
            return boost::asio::buffer_size(this->get());
        }

        /*!
         * Get a pointer to the underlying buffer.
         * \return a pointer into memory
         */
        template <class T> inline T cast(void) const{
            return boost::asio::buffer_cast<T>(this->get());
        }

    private:
        /*!
         * Get a reference to the internal mutable buffer.
         * The buffer has a reference to memory and a size.
         * \return a boost asio mutable buffer
         */
        virtual const boost::asio::mutable_buffer &get(void) const = 0;
    };

    /*!
     * A zero-copy interface for transport objects.
     * Provides a way to get send and receive buffers
     * with memory managed by the transport object.
     */
    class UHD_API zero_copy_if : boost::noncopyable{
    public:
        typedef boost::shared_ptr<zero_copy_if> sptr;

        /*!
         * Get a new receive buffer from this transport object.
         * \return a managed buffer, or null sptr on timeout/error
         */
        virtual managed_recv_buffer::sptr get_recv_buff(void) = 0;

        /*!
         * Get the maximum number of receive frames:
         *   The maximum number of valid managed recv buffers,
         *   or the maximum number of frames in the ring buffer,
         *   depending upon the underlying implementation.
         * \return number of frames
         */
        virtual size_t get_num_recv_frames(void) const = 0;

        /*!
         * Get a new send buffer from this transport object.
         * \return a managed buffer, or null sptr on timeout/error
         */
        virtual managed_send_buffer::sptr get_send_buff(void) = 0;

        /*!
         * Get the maximum number of send frames:
         *   The maximum number of valid managed send buffers,
         *   or the maximum number of frames in the ring buffer,
         *   depending upon the underlying implementation.
         * \return number of frames
         */
        virtual size_t get_num_send_frames(void) const = 0;

    };

    /*!
     * A phony-zero-copy interface for transport objects that
     * provides a zero-copy interface on top of copying transport.
     * This interface implements the get managed recv buffer,
     * the base class must implement the private recv method.
     */
    class UHD_API phony_zero_copy_recv_if : public virtual zero_copy_if{
    public:
        /*!
         * Create a phony zero copy recv interface.
         * \param max_buff_size max buffer size in bytes
         */
        phony_zero_copy_recv_if(size_t max_buff_size);

        //! destructor
        virtual ~phony_zero_copy_recv_if(void);

        /*!
         * Get a new receive buffer from this transport object.
         */
        managed_recv_buffer::sptr get_recv_buff(void);

    private:
        /*!
         * Perform a private copying recv.
         * \param buff the buffer to write data into
         * \return the number of bytes written to buff, 0 for timeout, negative for error
         */
        virtual ssize_t recv(const boost::asio::mutable_buffer &buff) = 0;

        UHD_PIMPL_DECL(impl) _impl;
    };

    /*!
     * A phony-zero-copy interface for transport objects that
     * provides a zero-copy interface on top of copying transport.
     * This interface implements the get managed send buffer,
     * the base class must implement the private send method.
     */
    class UHD_API phony_zero_copy_send_if : public virtual zero_copy_if{
    public:
        /*!
         * Create a phony zero copy send interface.
         * \param max_buff_size max buffer size in bytes
         */
        phony_zero_copy_send_if(size_t max_buff_size);

        //! destructor
        virtual ~phony_zero_copy_send_if(void);

        /*!
         * Get a new send buffer from this transport object.
         */
        managed_send_buffer::sptr get_send_buff(void);

    private:
        /*!
         * Perform a private copying send.
         * \param buff the buffer to read data from
         * \return the number of bytes read from buff, 0 for timeout, negative for error
         */
        virtual ssize_t send(const boost::asio::const_buffer &buff) = 0;

        UHD_PIMPL_DECL(impl) _impl;
    };

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_ZERO_COPY_HPP */
