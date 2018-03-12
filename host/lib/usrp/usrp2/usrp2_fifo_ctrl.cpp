//
// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "usrp2_regs.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include "usrp2_fifo_ctrl.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp> //htonl
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::transport;

static const size_t POKE32_CMD = (1 << 8);
static const size_t PEEK32_CMD = 0;
static const double ACK_TIMEOUT = 0.5;
static const double MASSIVE_TIMEOUT = 10.0; //for when we wait on a timed command
static const uint32_t MAX_SEQS_OUT = 63;

#define SPI_DIV SR_SPI_CORE + 0
#define SPI_CTRL SR_SPI_CORE + 1
#define SPI_DATA SR_SPI_CORE + 2
#define SPI_READBACK 0
// spi clock rate = master_clock/(div+1)/2 (10MHz in this case)
#define SPI_DIVIDER 4

class usrp2_fifo_ctrl_impl : public usrp2_fifo_ctrl{
public:

    usrp2_fifo_ctrl_impl(zero_copy_if::sptr xport):
        _xport(xport),
        _seq_out(0),
        _seq_ack(0),
        _timeout(ACK_TIMEOUT)
    {
        while (_xport->get_recv_buff(0.0)){} //flush
        this->set_time(uhd::time_spec_t(0.0));
        this->set_tick_rate(1.0); //something possible but bogus
        this->init_spi();
    }

    ~usrp2_fifo_ctrl_impl(void){
        _timeout = ACK_TIMEOUT; //reset timeout to something small
        UHD_SAFE_CALL(
            this->peek32(0); //dummy peek with the purpose of ack'ing all packets
        )
    }

    /*******************************************************************
     * Peek and poke 32 bit implementation
     ******************************************************************/
    void poke32(const wb_addr_type addr, const uint32_t data){
        boost::mutex::scoped_lock lock(_mutex);

        this->send_pkt((addr - SETTING_REGS_BASE)/4, data, POKE32_CMD);

        this->wait_for_ack(_seq_out-MAX_SEQS_OUT);
    }

    uint32_t peek32(const wb_addr_type addr){
        boost::mutex::scoped_lock lock(_mutex);

        this->send_pkt((addr - READBACK_BASE)/4, 0, PEEK32_CMD);

        return this->wait_for_ack(_seq_out);
    }

    /*******************************************************************
     * Peek and poke 16 bit not implemented
     ******************************************************************/
    void poke16(const wb_addr_type, const uint16_t){
        throw uhd::not_implemented_error("poke16 not implemented in fifo ctrl module");
    }

    uint16_t peek16(const wb_addr_type){
        throw uhd::not_implemented_error("peek16 not implemented in fifo ctrl module");
    }

    /*******************************************************************
     * FIFO controlled SPI implementation
     ******************************************************************/
    void init_spi(void){
        boost::mutex::scoped_lock lock(_mutex);

        this->send_pkt(SPI_DIV, SPI_DIVIDER, POKE32_CMD);
        this->wait_for_ack(_seq_out-MAX_SEQS_OUT);

        _ctrl_word_cache = 0; // force update first time around
    }

    uint32_t transact_spi(
        int which_slave,
        const spi_config_t &config,
        uint32_t data,
        size_t num_bits,
        bool readback
    ){
        boost::mutex::scoped_lock lock(_mutex);

        //load control word
        uint32_t ctrl_word = 0;
        ctrl_word |= ((which_slave & 0xffffff) << 0);
        ctrl_word |= ((num_bits & 0x3ff) << 24);
        if (config.mosi_edge == spi_config_t::EDGE_FALL) ctrl_word |= (1 << 31);
        if (config.miso_edge == spi_config_t::EDGE_RISE) ctrl_word |= (1 << 30);

        //load data word (must be in upper bits)
        const uint32_t data_out = data << (32 - num_bits);

        //conditionally send control word
        if (_ctrl_word_cache != ctrl_word){
            this->send_pkt(SPI_CTRL, ctrl_word, POKE32_CMD);
            this->wait_for_ack(_seq_out-MAX_SEQS_OUT);
            _ctrl_word_cache = ctrl_word;
        }

        //send data word
        this->send_pkt(SPI_DATA, data_out, POKE32_CMD);
        this->wait_for_ack(_seq_out-MAX_SEQS_OUT);

        //conditional readback
        if (readback){
            this->send_pkt(SPI_READBACK, 0, PEEK32_CMD);
            return this->wait_for_ack(_seq_out);
        }

        return 0;
    }

    /*******************************************************************
     * Update methods for time
     ******************************************************************/
    void set_time(const uhd::time_spec_t &time){
        boost::mutex::scoped_lock lock(_mutex);
        _time = time;
        _use_time = _time != uhd::time_spec_t(0.0);
        if (_use_time) _timeout = MASSIVE_TIMEOUT; //permanently sets larger timeout
    }

    uhd::time_spec_t get_time()
    {
        boost::mutex::scoped_lock lock(_mutex);
        return _time;
    }

    void set_tick_rate(const double rate){
        boost::mutex::scoped_lock lock(_mutex);
        _tick_rate = rate;
    }

private:

    /*******************************************************************
     * Primary control and interaction private methods
     ******************************************************************/
    UHD_INLINE void send_pkt(wb_addr_type addr, uint32_t data, int cmd){
        managed_send_buffer::sptr buff = _xport->get_send_buff(0.0);
        if (not buff){
            throw uhd::runtime_error("fifo ctrl timed out getting a send buffer");
        }
        uint32_t *trans = buff->cast<uint32_t *>();
        trans[0] = htonl(++_seq_out);
        uint32_t *pkt = trans + 1;

        //load packet info
        vrt::if_packet_info_t packet_info;
        packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_CONTEXT;
        packet_info.num_payload_words32 = 2;
        packet_info.num_payload_bytes = packet_info.num_payload_words32*sizeof(uint32_t);
        packet_info.packet_count = _seq_out;
        packet_info.tsf = _time.to_ticks(_tick_rate);
        packet_info.sob = false;
        packet_info.eob = false;
        packet_info.has_sid = false;
        packet_info.has_cid = false;
        packet_info.has_tsi = false;
        packet_info.has_tsf = _use_time;
        packet_info.has_tlr = false;

        //load header
        vrt::if_hdr_pack_be(pkt, packet_info);

        //load payload
        const uint32_t ctrl_word = (addr & 0xff) | cmd | (_seq_out << 16);
        pkt[packet_info.num_header_words32+0] = htonl(ctrl_word);
        pkt[packet_info.num_header_words32+1] = htonl(data);

        //send the buffer over the interface
        buff->commit(sizeof(uint32_t)*(packet_info.num_packet_words32+1));
    }

    UHD_INLINE bool wraparound_lt16(const int16_t i0, const int16_t i1){
        if (((i0 ^ i1) & 0x8000) == 0) //same sign bits
            return uint16_t(i0) < uint16_t(i1);
        return int16_t(i1 - i0) > 0;
    }

    UHD_INLINE uint32_t wait_for_ack(const uint16_t seq_to_ack){

        while (wraparound_lt16(_seq_ack, seq_to_ack)){
            managed_recv_buffer::sptr buff = _xport->get_recv_buff(_timeout);
            if (not buff){
                throw uhd::runtime_error("fifo ctrl timed out looking for acks");
            }
            const uint32_t *pkt = buff->cast<const uint32_t *>();
            vrt::if_packet_info_t packet_info;
            packet_info.num_packet_words32 = buff->size()/sizeof(uint32_t);
            vrt::if_hdr_unpack_be(pkt, packet_info);
            _seq_ack = ntohl(pkt[packet_info.num_header_words32+0]) >> 16;
            if (_seq_ack == seq_to_ack){
                return ntohl(pkt[packet_info.num_header_words32+1]);
            }
        }

        return 0;
    }

    zero_copy_if::sptr _xport;
    boost::mutex _mutex;
    uint16_t _seq_out;
    uint16_t _seq_ack;
    uhd::time_spec_t _time;
    bool _use_time;
    double _tick_rate;
    double _timeout;
    uint32_t _ctrl_word_cache;
};


usrp2_fifo_ctrl::sptr usrp2_fifo_ctrl::make(zero_copy_if::sptr xport){
    return sptr(new usrp2_fifo_ctrl_impl(xport));
}
