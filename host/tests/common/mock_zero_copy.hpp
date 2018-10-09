//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MOCK_XPORT_HPP
#define INCLUDED_MOCK_XPORT_HPP

#include <uhdlib/rfnoc/xports.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <list>
#include <vector>
/***********************************************************************
 * Transport mockups
 **********************************************************************/
/*! A single transport class that implements send() and recv()
 *
 * Tx and Rx are separate. We can access the other end of the FIFOs from
 * this class.
 */
static constexpr size_t SEND_BUFF_SIZE = 1024;
static constexpr size_t RECV_BUFF_SIZE = 1024;

/***********************************************************************
 * Dummy managed buffers for testing
 **********************************************************************/
class mock_msb : public uhd::transport::managed_send_buffer {
    public:
    void release(void) { /* nop */
    }

    sptr get_new(boost::shared_array<uint8_t> mem, size_t* len) {
        _mem = mem;
        return make(this, mem.get(), *len);
    }

    private:
    boost::shared_array<uint8_t> _mem;
};


class mock_mrb : public uhd::transport::managed_recv_buffer {
    public:
    void release(void) { /* nop */
    }

    sptr get_new(boost::shared_array<uint8_t> mem, size_t len) {
        _mem = mem;
        return make(this, _mem.get(), len);
    }

    private:
    boost::shared_array<uint8_t> _mem;
};

class mock_zero_copy : public uhd::transport::zero_copy_if {
    public:
    typedef boost::shared_ptr<mock_zero_copy> sptr;
    uhd::transport::managed_recv_buffer::sptr get_recv_buff(double);

    uhd::transport::managed_send_buffer::sptr get_send_buff(double);

    size_t get_num_recv_frames(void) const { return 1; }
    size_t get_num_send_frames(void) const { return 1; }
    size_t get_recv_frame_size(void) const { return RECV_BUFF_SIZE; }
    size_t get_send_frame_size(void) const { return SEND_BUFF_SIZE; }

    private:
    std::list<boost::shared_array<uint8_t>> _tx_mems;
    std::list<size_t> _tx_lens;

    std::list<boost::shared_array<uint8_t>> _rx_mems;
    std::list<size_t> _rx_lens;

    std::vector<boost::shared_ptr<mock_msb>> _msbs;
    std::vector<boost::shared_ptr<mock_mrb>> _mrbs;
};

#endif /*INCLUDED_MOCK_XPORT_HPP*/