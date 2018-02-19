//
// Copyright 2010-2012,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_ZERO_COPY_HPP
#define INCLUDED_UHD_TRANSPORT_ZERO_COPY_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/atomic_count.hpp>

namespace uhd{ namespace transport{

    //! Simple managed buffer with release interface
    class UHD_API managed_buffer{
    public:
        managed_buffer(void):_ref_count(0),_buffer(NULL),_length(0)
		{
#ifdef UHD_TXRX_DEBUG_PRINTS
            _mb_num = s_buffer_count;
            // From Boost website: atomic_count seems only to have precrement operator.
            ++s_buffer_count;
#endif
        }

        virtual ~managed_buffer(void) {}

        /*!
         * Signal to the transport that we are done with the buffer.
         * This should be called to release the buffer to the transport object.
         * After calling, the referenced memory should be considered invalid.
         */
        virtual void release(void) = 0;

        /*!
         * Use commit() to re-write the length (for use with send buffers).
         * \param num_bytes the number of bytes written into the buffer
         */
        UHD_INLINE void commit(size_t num_bytes){
            _length = num_bytes;
        }

        /*!
         * Get a pointer to the underlying buffer.
         * \return a pointer into memory
         */
        template <class T> UHD_INLINE T cast(void) const{
            return static_cast<T>(_buffer);
        }

        /*!
         * Get the size of the underlying buffer.
         * \return the number of bytes
         */
        UHD_INLINE size_t size(void) const{
            return _length;
        }

        //! Create smart pointer to a reusable managed buffer
        template <typename T> UHD_INLINE boost::intrusive_ptr<T> make(
            T *p, void *buffer, size_t length
        ){
            _buffer = buffer;
            _length = length;
            return boost::intrusive_ptr<T>(p);
        }

        boost::detail::atomic_count _ref_count;
        typedef boost::intrusive_ptr<managed_buffer> sptr;

        int ref_count(){
        	return (int) _ref_count;
        }

#ifdef UHD_TXRX_DEBUG_PRINTS
        int num() const{
        	return _mb_num;
        }
#endif

    protected:
        void *_buffer;
        size_t _length;
#ifdef UHD_TXRX_DEBUG_PRINTS
        int _mb_num;
#endif

    private:
#ifdef UHD_TXRX_DEBUG_PRINTS
        static boost::detail::atomic_count s_buffer_count;
#endif
    };

    UHD_INLINE void intrusive_ptr_add_ref(managed_buffer *p){
        ++(p->_ref_count);
    }

    UHD_INLINE void intrusive_ptr_release(managed_buffer *p){
        if (--(p->_ref_count) == 0) p->release();
    }

    /*!
     * A managed receive buffer:
     * Contains a reference to transport-managed memory,
     * and a method to release the memory after reading.
     */
    class UHD_API managed_recv_buffer : public managed_buffer{
    public:
        typedef boost::intrusive_ptr<managed_recv_buffer> sptr;
    };

    /*!
     * A managed send buffer:
     * Contains a reference to transport-managed memory,
     * and a method to commit the memory after writing.
     */
    class UHD_API managed_send_buffer : public managed_buffer{
    public:
        typedef boost::intrusive_ptr<managed_send_buffer> sptr;
    };

    /*!
     * Transport parameters
     */
    struct zero_copy_xport_params {
        size_t recv_frame_size;
        size_t send_frame_size;
        size_t num_recv_frames;
        size_t num_send_frames;
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
         * Clean up tasks before releasing the transport object.
         */
        virtual ~zero_copy_if() {};

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
