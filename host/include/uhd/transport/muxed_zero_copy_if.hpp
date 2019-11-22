//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_TRANSPORT_MUXED_ZERO_COPY_IF_HPP
#define INCLUDED_LIBUHD_TRANSPORT_MUXED_ZERO_COPY_IF_HPP

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <stdint.h>
#include <boost/function.hpp>
#include <uhd/utils/noncopyable.hpp>

namespace uhd { namespace transport {

/*!
 * Implements a software muxed-demuxed zero-copy transport
 * This is a wrapper around a base transport that allows
 * creation of virtual zero_copy_if streams that are
 * indistinguishable from physical transport streams.
 * This class handles demuxing receive streams into the
 * appropriate virtual streams with the given classifier
 * function. A worker therad is spawned to handle the demuxing.
 */
class muxed_zero_copy_if : private uhd::noncopyable
{
public:
    typedef boost::shared_ptr<muxed_zero_copy_if> sptr;

    /*!
     * Function to classify the stream based on the payload.
     * The classifier must return a stream number, an arbitrary
     * identifier for a virtual stream that is consistent with
     * the stream number used in the make_stream and remove_stream
     * fuctions
     * \param buff a pointer to the payload of the frame
     * \param size number of bytes in the frame payload
     * \return stream number
     */
    typedef boost::function<uint32_t(void* buff, size_t size)> stream_classifier_fn;

    //! virtual dtor
    virtual ~muxed_zero_copy_if() {}

    //! Make a virtual transport for the specified stream number
    virtual zero_copy_if::sptr make_stream(const uint32_t stream_num) = 0;

    //! Unregister the stream number. All packets destined to the stream will be dropped.
    virtual void remove_stream(const uint32_t stream_num) = 0;

    //! Get number of frames dropped due to unregistered streams
    virtual size_t get_num_dropped_frames() const = 0;

    //! Make a new demuxer from a transport and parameters
    //
    // \param classify_fn See also stream_classifier_fn.
    // \param max_streams Max number of streams that can be muxed/demuxed
    // \param recv_timeout_s This is a timeout that is used in the receiver thread
    //                       when fetching the next packet. Its specific effect
    //                       depends on the type of the underlying transport.
    //                       A longer timeout means the thread can block on
    //                       outstanding I/O for longer. A lower value will
    //                       increase CPU utilization.
    static sptr make(zero_copy_if::sptr base_xport,
        stream_classifier_fn classify_fn,
        size_t max_streams,
        const double recv_timeout_s = 0.0);
};

}} // namespace uhd::transport

#endif /* INCLUDED_LIBUHD_TRANSPORT_MUXED_ZERO_COPY_IF_HPP */
