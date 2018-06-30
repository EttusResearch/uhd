//
// Copyright 2012-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhdlib/rfnoc/ctrl_iface.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <queue>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::transport;

static const double ACK_TIMEOUT = 2.0; //supposed to be worst case practical timeout
static const double MASSIVE_TIMEOUT = 10.0; //for when we wait on a timed command

template <uhd::endianness_t _endianness>
class ctrl_iface_impl: public ctrl_iface
{
public:

    ctrl_iface_impl(
            const both_xports_t &xports,
            const std::string &name
    ) : _xports(xports),
        _name(name),
        _seq_out(0),
        _max_outstanding_acks(
            std::min(
                uhd::rfnoc::CMD_FIFO_SIZE / 3, // Max command packet size is 3 lines
                _xports.recv->get_num_recv_frames()
            )
        )
    {
        UHD_ASSERT_THROW(bool(_xports.send));
        UHD_ASSERT_THROW(bool(_xports.recv));
        // Flush the response transport in case we have something over:
        while (_xports.recv->get_recv_buff(0.0)) {}
    }

    virtual ~ctrl_iface_impl(void)
    {
        UHD_SAFE_CALL(
            // dummy peek with the purpose of ack'ing all packets
            this->send_cmd_pkt(0, 0, true);
        )
    }

    /*******************************************************************
     * Get and set register implementation
     ******************************************************************/
    uint64_t send_cmd_pkt(
            const size_t addr,
            const size_t data,
            const bool readback,
            const uint64_t timestamp=0
    ) {
        boost::mutex::scoped_lock lock(_mutex);
        this->send_pkt(addr, data, timestamp);
        return this->wait_for_ack(
                readback,
                bool(timestamp) ? MASSIVE_TIMEOUT : ACK_TIMEOUT
        );
    }

private:
    // This is the buffer type for response messages
    struct resp_buff_type
    {
        uint32_t data[8];
    };

    /*******************************************************************
     * Primary control and interaction private methods
     ******************************************************************/
    inline void send_pkt(
            const uint32_t addr,
            const uint32_t data,
            const uint64_t timestamp
    ) {
        managed_send_buffer::sptr buff = _xports.send->get_send_buff(0.0);
        if (not buff) {
            throw uhd::runtime_error("fifo ctrl timed out getting a send buffer");
        }
        uint32_t *pkt = buff->cast<uint32_t *>();

        //load packet info
        vrt::if_packet_info_t packet_info;
        packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
        packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_CMD;
        packet_info.num_payload_words32 = 2;
        packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(uint32_t);
        packet_info.packet_count = _seq_out;
        packet_info.tsf = timestamp;
        packet_info.sob = false;
        packet_info.eob = false;
        packet_info.sid = _xports.send_sid;
        packet_info.has_sid = true;
        packet_info.has_cid = false;
        packet_info.has_tsi = false;
        packet_info.has_tsf = bool(timestamp);
        packet_info.has_tlr = false;

        // Unpack header and load payload
        if (_endianness == uhd::ENDIANNESS_BIG) { // This if statement gets compiled out
            vrt::if_hdr_pack_be(pkt, packet_info);
            pkt[packet_info.num_header_words32+0] = uhd::htonx(addr);
            pkt[packet_info.num_header_words32+1] = uhd::htonx(data);
        } else {
            vrt::if_hdr_pack_le(pkt, packet_info);
            pkt[packet_info.num_header_words32+0] = uhd::htowx(addr);
            pkt[packet_info.num_header_words32+1] = uhd::htowx(data);
        }

        //UHD_LOGGER_TRACE("RFNOC") << boost::format("0x%08x, 0x%08x\n") % addr % data;
        //send the buffer over the interface
        _outstanding_seqs.push(_seq_out);
        buff->commit(sizeof(uint32_t)*(packet_info.num_packet_words32));

        _seq_out++;//inc seq for next call
    }

    inline uint64_t wait_for_ack(const bool readback, const double timeout)
    {
        while (readback or (_outstanding_seqs.size() >= _max_outstanding_acks))
        {
            //get seq to ack from outstanding packets list
            UHD_ASSERT_THROW(not _outstanding_seqs.empty());
            const size_t seq_to_ack = _outstanding_seqs.front();

            //parse the packet
            vrt::if_packet_info_t packet_info;
            resp_buff_type resp_buff;
            memset(&resp_buff, 0x00, sizeof(resp_buff));
            uint32_t const *pkt = NULL;
            managed_recv_buffer::sptr buff;

            buff = _xports.recv->get_recv_buff(timeout);
            try {
                UHD_ASSERT_THROW(bool(buff));
                UHD_ASSERT_THROW(buff->size() > 0);
                _outstanding_seqs.pop();
            }
            catch(const std::exception &ex) {
                throw uhd::io_error(str(
                    boost::format("Block ctrl (%s) no response packet - %s")
                    % _name
                    % ex.what()
                ));
            }
            pkt = buff->cast<const uint32_t *>();
            packet_info.num_packet_words32 = buff->size()/sizeof(uint32_t);

            //parse the buffer
            try {
                if (_endianness == uhd::ENDIANNESS_BIG) {
                    vrt::chdr::if_hdr_unpack_be(pkt, packet_info);
                } else {
                    vrt::chdr::if_hdr_unpack_le(pkt, packet_info);
                }
            }
            catch(const std::exception &ex)
            {
                UHD_LOGGER_ERROR("RFNOC") << "[" << _name << "] Block ctrl bad VITA packet: " << ex.what() ;
                if (buff){
                    UHD_LOGGER_INFO("RFNOC") << boost::format("%08X") % pkt[0] ;
                    UHD_LOGGER_INFO("RFNOC") << boost::format("%08X") % pkt[1] ;
                    UHD_LOGGER_INFO("RFNOC") << boost::format("%08X") % pkt[2] ;
                    UHD_LOGGER_INFO("RFNOC") << boost::format("%08X") % pkt[3] ;
                }
                else{
                    UHD_LOGGER_INFO("RFNOC") << "buff is NULL" ;
                }
            }

            //check the buffer
            try {
                UHD_ASSERT_THROW(packet_info.has_sid);
                if (packet_info.sid != _xports.recv_sid.get()) {
                    throw uhd::io_error(
                        str(
                            boost::format("Expected SID: %s  Received SID: %s")
                            % _xports.recv_sid.to_pp_string_hex()
                            % uhd::sid_t(packet_info.sid).to_pp_string_hex()
                        )
                    );
                }

                if (packet_info.packet_count != (seq_to_ack & 0xfff)) {
                    throw uhd::io_error(
                        str(
                            boost::format("Expected packet index: %d " \
                                          "Received index: %d")
                            % (seq_to_ack & 0xfff)
                            % packet_info.packet_count
                        )
                    );
                }

                UHD_ASSERT_THROW(packet_info.num_payload_words32 == 2);
            }
            catch (const std::exception &ex) {
                throw uhd::io_error(str(
                    boost::format("Block ctrl (%s) packet parse error - %s")
                    % _name
                    % ex.what()
                ));
            }

            //return the readback value
            if (readback and _outstanding_seqs.empty()) {
                const uint64_t hi = (_endianness == uhd::ENDIANNESS_BIG) ?
                    uhd::ntohx(pkt[packet_info.num_header_words32+0])
                    : uhd::wtohx(pkt[packet_info.num_header_words32+0]);
                const uint64_t lo = (_endianness == uhd::ENDIANNESS_BIG) ?
                    uhd::ntohx(pkt[packet_info.num_header_words32+1])
                    : uhd::wtohx(pkt[packet_info.num_header_words32+1]);
                return ((hi << 32) | lo);
            }
        }

        return 0;
    }


    const uhd::both_xports_t _xports;
    const std::string _name;
    size_t _seq_out;
    std::queue<size_t> _outstanding_seqs;
    const size_t _max_outstanding_acks;

    boost::mutex _mutex;
};

ctrl_iface::sptr ctrl_iface::make(
        const both_xports_t &xports,
        const std::string &name
) {
    if (xports.endianness == uhd::ENDIANNESS_BIG) {
        return boost::make_shared<ctrl_iface_impl<uhd::ENDIANNESS_BIG>>(
            xports, name
        );
    } else {
        return boost::make_shared<ctrl_iface_impl<uhd::ENDIANNESS_LITTLE>>(
            xports, name
        );
    }
}
