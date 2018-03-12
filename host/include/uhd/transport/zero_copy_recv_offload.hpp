//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_ZERO_COPY_RECV_OFFLOAD_HPP
#define INCLUDED_UHD_ZERO_COPY_RECV_OFFLOAD_HPP

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd{ namespace transport{

/*!
 * A threaded transport offload that is meant to relieve the main thread of
 * the responsibility of making receive calls.
 */
class UHD_API zero_copy_recv_offload : public virtual zero_copy_if {
public:
    typedef boost::shared_ptr<zero_copy_recv_offload> sptr;

    /*!
     * This transport offload adds a receive thread in order to
     * communicate with the underlying transport. It is meant to be
     * used in cases where the main thread needs to be relieved of the burden
     * of the underlying transport receive calls.
     *
     * \param transport a shared pointer to the transport interface
     * \param timeout a general timeout for pushing and pulling on the bounded buffer
     */
    static sptr make(zero_copy_if::sptr transport,
                     const double timeout);
};

}} //namespace

#endif /* INCLUDED_ZERO_COPY_OFFLOAD_HPP */
