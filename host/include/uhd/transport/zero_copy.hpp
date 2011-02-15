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

#ifndef INCLUDED_UHD_TRANSPORT_ZERO_COPY_HPP
#define INCLUDED_UHD_TRANSPORT_ZERO_COPY_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

namespace uhd{ namespace transport{

    /*!
     * A managed receive buffer:
     * Contains a reference to transport-managed memory,
     * and a method to release the memory after reading.
     */
    class UHD_API managed_recv_buffer{
    public:
        typedef boost::shared_ptr<managed_recv_buffer> sptr;
        typedef boost::function<void(void)> release_fcn_t;

        /*!
         * Make a safe managed receive buffer:
         * A safe managed buffer ensures that release is called once,
         * either by the user or automatically upon deconstruction.
         * \param buff a pointer into read-only memory
         * \param size the length of the buffer in bytes
         * \param release_fcn callback to release the memory
         * \return a new managed receive buffer
         */
        static sptr make_safe(
            const void *buff, size_t size, const release_fcn_t &release_fcn
        );

        /*!
         * Signal to the transport that we are done with the buffer.
         * This should be called to release the buffer to the transport object.
         * After calling, the referenced memory should be considered invalid.
         */
        virtual void release(void) = 0;

        /*!
         * Get a pointer to the underlying buffer.
         * \return a pointer into memory
         */
        template <class T> inline T cast(void) const{
            return static_cast<T>(this->get_buff());
        }

        /*!
         * Get the size of the underlying buffer.
         * \return the number of bytes
         */
        inline size_t size(void) const{
            return this->get_size();
        }

    private:
        virtual const void *get_buff(void) const = 0;
        virtual size_t get_size(void) const = 0;
    };

    /*!
     * A managed send buffer:
     * Contains a reference to transport-managed memory,
     * and a method to commit the memory after writing.
     */
    class UHD_API managed_send_buffer{
    public:
        typedef boost::shared_ptr<managed_send_buffer> sptr;
        typedef boost::function<void(size_t)> commit_fcn_t;

        /*!
         * Make a safe managed send buffer:
         * A safe managed buffer ensures that commit is called once,
         * either by the user or automatically upon deconstruction.
         * In the later case, the deconstructor will call commit(0).
         * \param buff a pointer into writable memory
         * \param size the length of the buffer in bytes
         * \param commit_fcn callback to commit the memory
         * \return a new managed send buffer
         */
        static sptr make_safe(
            void *buff, size_t size, const commit_fcn_t &commit_fcn
        );

        /*!
         * Signal to the transport that we are done with the buffer.
         * This should be called to commit the write to the transport object.
         * After calling, the referenced memory should be considered invalid.
         * \param num_bytes the number of bytes written into the buffer
         */
        virtual void commit(size_t num_bytes) = 0;

        /*!
         * Get a pointer to the underlying buffer.
         * \return a pointer into memory
         */
        template <class T> inline T cast(void) const{
            return static_cast<T>(this->get_buff());
        }

        /*!
         * Get the size of the underlying buffer.
         * \return the number of bytes
         */
        inline size_t size(void) const{
            return this->get_size();
        }

    private:
        virtual void *get_buff(void) const = 0;
        virtual size_t get_size(void) const = 0;
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
         * \param timeout the timeout to get the buffer in seconds
         * \return a managed buffer, or null sptr on timeout/error
         */
        virtual managed_recv_buffer::sptr get_recv_buff(double timeout = 0.1) = 0;

        /*!
         * Get the number of receive frames:
         * The number of simultaneous receive buffers in use.
         * \return number of frames
         */
        virtual size_t get_num_recv_frames(void) const = 0;

        /*!
         * Get the size of a receive frame:
         * The maximum capacity of a single receive buffer.
         * \return frame size in bytes
         */
        virtual size_t get_recv_frame_size(void) const = 0;

        /*!
         * Get a new send buffer from this transport object.
         * \param timeout the timeout to get the buffer in seconds
         * \return a managed buffer, or null sptr on timeout/error
         */
        virtual managed_send_buffer::sptr get_send_buff(double timeout = 0.1) = 0;

        /*!
         * Get the number of send frames:
         * The number of simultaneous send buffers in use.
         * \return number of frames
         */
        virtual size_t get_num_send_frames(void) const = 0;

        /*!
         * Get the size of a send frame:
         * The maximum capacity of a single send buffer.
         * \return frame size in bytes
         */
        virtual size_t get_send_frame_size(void) const = 0;

    };

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_ZERO_COPY_HPP */
