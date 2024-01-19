//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_xport.hpp>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::chdr;
using namespace uhd::transport;

chdr_ctrl_xport::chdr_ctrl_xport(io_service::sptr io_srv,
    send_link_if::sptr send_link,
    recv_link_if::sptr recv_link,
    const chdr::chdr_packet_factory& pkt_factory,
    sep_id_t my_epid,
    size_t num_send_frames,
    size_t num_recv_frames,
    disconnect_callback_t disconnect)
    : _my_epid(my_epid), _recv_packet(pkt_factory.make_generic()), _disconnect(disconnect)
{
    /* Make dumb send pipe */
    send_io_if::send_callback_t send_cb = [](frame_buff::uptr buff, send_link_if* link) {
        link->release_send_buff(std::move(buff));
    };
    _send_if = io_srv->make_send_client(
        send_link, num_send_frames, send_cb, recv_link_if::sptr(), 0, nullptr, nullptr);

    /* Make dumb recv pipe that matches management and control packets */
    uhd::transport::recv_callback_t ctrl_recv_cb =
        [this](frame_buff::uptr& buff,
            recv_link_if* /*recv_link*/,
            send_link_if* /*send_link*/) -> bool { return this->_ctrl_recv_cb(buff); };

    recv_io_if::fc_callback_t release_cb =
        [this](frame_buff::uptr buff, recv_link_if* link, send_link_if* /*send_link*/) {
            this->_release_cb(std::move(buff), link);
        };

    _ctrl_recv_if = io_srv->make_recv_client(
        recv_link, num_recv_frames, ctrl_recv_cb, send_link_if::sptr(), 0, release_cb);

    uhd::transport::recv_callback_t mgmt_recv_cb =
        [this](frame_buff::uptr& buff,
            recv_link_if* /*recv_link*/,
            send_link_if* /*send_link*/) -> bool { return this->_mgmt_recv_cb(buff); };

    _mgmt_recv_if = io_srv->make_recv_client(
        recv_link, 1, mgmt_recv_cb, send_link_if::sptr(), 0, release_cb);
}

bool chdr_ctrl_xport::_ctrl_recv_cb(uhd::transport::frame_buff::uptr& buff)
{
    _recv_packet->refresh(buff->data());
    auto hdr      = _recv_packet->get_chdr_header();
    auto pkt_type = hdr.get_pkt_type();
    auto dst_epid = hdr.get_dst_epid();

    /* Check type and destination EPID */
    if ((pkt_type == PKT_TYPE_CTRL) && (dst_epid == _my_epid)) {
        return true;
    } else {
        return false;
    }
    return false;
}

bool chdr_ctrl_xport::_mgmt_recv_cb(uhd::transport::frame_buff::uptr& buff)
{
    _recv_packet->refresh(buff->data());
    auto hdr      = _recv_packet->get_chdr_header();
    auto pkt_type = hdr.get_pkt_type();
    auto dst_epid = hdr.get_dst_epid();

    /* Check type and destination EPID */
    if ((pkt_type == PKT_TYPE_MGMT) && (dst_epid == _my_epid)) {
        return true;
    } else {
        return false;
    }
    return false;
}

void chdr_ctrl_xport::_release_cb(
    uhd::transport::frame_buff::uptr buff, uhd::transport::recv_link_if* recv_link)
{
    recv_link->release_recv_buff(std::move(buff));
}


chdr_ctrl_xport::~chdr_ctrl_xport()
{
    // Release I/O service clients before allowing members needed by callbacks
    // be destroyed
    _send_if.reset();
    _ctrl_recv_if.reset();
    _mgmt_recv_if.reset();

    // Disconnect the transport
    _disconnect();
}

/*!
 * Get an empty frame buffer in which to write packet contents.
 *
 * \param timeout_ms a positive timeout value specifies the maximum number
                     of ms to wait, a negative value specifies to block
                     until successful, and a value of 0 specifies no wait.
 * \return a frame buffer, or null uptr if timeout occurs
 */
frame_buff::uptr chdr_ctrl_xport::get_send_buff(int32_t timeout_ms)
{
    // FIXME: Remove mutex when have threaded_io_service
    std::lock_guard<std::mutex> lock(_mutex);
    frame_buff::uptr buff = _send_if->get_send_buff(timeout_ms);
    if (!buff) {
        return frame_buff::uptr();
    }
    return frame_buff::uptr(std::move(buff));
}

/*!
 * Release a frame buffer, allowing the driver to reuse it.
 *
 * \param buffer frame buffer to release for reuse by the link
 */
void chdr_ctrl_xport::release_send_buff(frame_buff::uptr buff)
{
    // FIXME: Remove mutex when have threaded_io_service
    std::lock_guard<std::mutex> lock(_mutex);
    _send_if->release_send_buff(std::move(buff));
}

/*!
 * Attempt to get a frame buffer with data from the recv link.
 *
 * \param timeout_ms a positive timeout value specifies the maximum number
                     of ms to wait, a negative value specifies to block
                     until successful, and a value of 0 specifies no wait.
 * \return a frame buffer, or null uptr if timeout occurs
 */
frame_buff::uptr chdr_ctrl_xport::get_recv_buff(int32_t timeout_ms)
{
    // FIXME: Remove mutex when have threaded_io_service
    std::lock_guard<std::mutex> lock(_mutex);
    return _ctrl_recv_if->get_recv_buff(timeout_ms);
}

frame_buff::uptr chdr_ctrl_xport::get_mgmt_buff(int32_t timeout_ms)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _mgmt_recv_if->get_recv_buff(timeout_ms);
}

/*!
 * Release a frame buffer, allowing the recv link driver to reuse it.
 *
 * \param buffer frame buffer to release for reuse by the link
 */
void chdr_ctrl_xport::release_recv_buff(frame_buff::uptr buff)
{
    // FIXME: Remove mutex when have threaded_io_service
    std::lock_guard<std::mutex> lock(_mutex);
    _ctrl_recv_if->release_recv_buff(std::move(buff));
}

void chdr_ctrl_xport::release_mgmt_buff(frame_buff::uptr buff)
{
    // FIXME: Remove mutex when have threaded_io_service
    std::lock_guard<std::mutex> lock(_mutex);
    _mgmt_recv_if->release_recv_buff(std::move(buff));
}

/*!
 * Get this xport's EPID
 */
sep_id_t chdr_ctrl_xport::get_epid() const
{
    return _my_epid;
}
