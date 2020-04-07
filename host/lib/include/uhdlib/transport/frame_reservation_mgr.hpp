//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhdlib/transport/link_if.hpp>
#include <unordered_map>

namespace uhd { namespace transport {

/*!
 * Helper class to keep track of the number of frames reserved from a pair of links
 */
class frame_reservation_mgr
{
public:
    struct frame_reservation_t
    {
        recv_link_if::sptr recv_link;
        size_t num_recv_frames = 0;
        send_link_if::sptr send_link;
        size_t num_send_frames = 0;
    };

    void register_link(const recv_link_if::sptr& recv_link)
    {
        if (_recv_tbl[recv_link.get()] != 0) {
            throw uhd::runtime_error("Recv link already attached to I/O service");
        }
        _recv_tbl[recv_link.get()] = 0;
    }

    void register_link(const send_link_if::sptr& send_link)
    {
        if (_send_tbl[send_link.get()] != 0) {
            throw uhd::runtime_error("Send link already attached to I/O service");
        }
        _send_tbl[send_link.get()] = 0;
    }

    void unregister_link(const recv_link_if::sptr& recv_link)
    {
        auto link_ptr = recv_link.get();
        UHD_ASSERT_THROW(_recv_tbl.count(link_ptr) != 0);
        _recv_tbl.erase(link_ptr);
    }

    void unregister_link(const send_link_if::sptr& send_link)
    {
        auto link_ptr = send_link.get();
        UHD_ASSERT_THROW(_send_tbl.count(link_ptr) != 0);
        _send_tbl.erase(link_ptr);
    }

    void reserve_frames(const frame_reservation_t& reservation)
    {
        if (reservation.recv_link) {
            const size_t rsvd_frames = _recv_tbl.at(reservation.recv_link.get());
            const size_t capacity    = reservation.recv_link->get_num_recv_frames();
            if (rsvd_frames + reservation.num_recv_frames > capacity) {
                throw uhd::runtime_error(
                    "Number of frames requested exceeds link recv frame capacity");
            }

            recv_link_if* link_ptr = reservation.recv_link.get();
            _recv_tbl[link_ptr]    = rsvd_frames + reservation.num_recv_frames;
        }

        if (reservation.send_link) {
            const size_t rsvd_frames = _send_tbl.at(reservation.send_link.get());
            const size_t capacity    = reservation.send_link->get_num_send_frames();
            if (rsvd_frames + reservation.num_send_frames > capacity) {
                throw uhd::runtime_error(
                    "Number of frames requested exceeds link send frame capacity");
            }

            send_link_if* link_ptr = reservation.send_link.get();
            _send_tbl[link_ptr]    = rsvd_frames + reservation.num_send_frames;
        }
    }

    void unreserve_frames(const frame_reservation_t& reservation)
    {
        if (reservation.recv_link) {
            const size_t rsvd_frames = _recv_tbl.at(reservation.recv_link.get());
            recv_link_if* link_ptr   = reservation.recv_link.get();
            _recv_tbl[link_ptr]      = rsvd_frames - reservation.num_recv_frames;
        }

        if (reservation.send_link) {
            const size_t rsvd_frames = _send_tbl.at(reservation.send_link.get());
            send_link_if* link_ptr   = reservation.send_link.get();
            _send_tbl[link_ptr]      = rsvd_frames - reservation.num_send_frames;
        }
    }

private:
    std::unordered_map<recv_link_if*, size_t> _recv_tbl;
    std::unordered_map<send_link_if*, size_t> _send_tbl;
};

}} // namespace uhd::transport
