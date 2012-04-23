//
// Copyright 2011-2012 Ettus Research LLC
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

#include <boost/test/unit_test.hpp>
#include "../lib/transport/super_recv_packet_handler.hpp"
#include <boost/shared_array.hpp>
#include <boost/bind.hpp>
#include <complex>
#include <vector>
#include <list>

#define BOOST_CHECK_TS_CLOSE(a, b) \
    BOOST_CHECK_CLOSE((a).get_real_secs(), (b).get_real_secs(), 0.001)

/***********************************************************************
 * A dummy overflow handler for testing
 **********************************************************************/
struct overflow_handler_type{
    overflow_handler_type(void){
        num_overflow = 0;
    }
    void handle(void){
        num_overflow++;
    }
    size_t num_overflow;
};

/***********************************************************************
 * A dummy managed receive buffer for testing
 **********************************************************************/
class dummy_mrb : public uhd::transport::managed_recv_buffer{
public:
    void release(void){
        //NOP
    }

    sptr get_new(boost::shared_array<char> mem, size_t len){
        _mem = mem;
        return make(this, _mem.get(), len);
    }

private:
    boost::shared_array<char> _mem;
};

/***********************************************************************
 * A dummy transport class to fill with fake data
 **********************************************************************/
class dummy_recv_xport_class{
public:
    dummy_recv_xport_class(const std::string &end){
        _end = end;
    }

    void push_back_packet(
        uhd::transport::vrt::if_packet_info_t &ifpi,
        const boost::uint32_t optional_msg_word = 0
    ){
        const size_t max_pkt_len = (ifpi.num_payload_words32 + uhd::transport::vrt::max_if_hdr_words32 + 1/*tlr*/)*sizeof(boost::uint32_t);
        _mems.push_back(boost::shared_array<char>(new char[max_pkt_len]));
        if (_end == "big"){
            uhd::transport::vrt::if_hdr_pack_be(reinterpret_cast<boost::uint32_t *>(_mems.back().get()), ifpi);
        }
        if (_end == "little"){
            uhd::transport::vrt::if_hdr_pack_le(reinterpret_cast<boost::uint32_t *>(_mems.back().get()), ifpi);
        }
        (reinterpret_cast<boost::uint32_t *>(_mems.back().get()) + ifpi.num_header_words32)[0] = optional_msg_word | uhd::byteswap(optional_msg_word);
        _lens.push_back(ifpi.num_packet_words32*sizeof(boost::uint32_t));
    }

    uhd::transport::managed_recv_buffer::sptr get_recv_buff(double){
        if (_mems.empty()) return uhd::transport::managed_recv_buffer::sptr(); //timeout
        _mrbs.push_back(boost::shared_ptr<dummy_mrb>(new dummy_mrb()));
        uhd::transport::managed_recv_buffer::sptr mrb = _mrbs.back()->get_new(_mems.front(), _lens.front());
        _mems.pop_front();
        _lens.pop_front();
        return mrb;
    }

private:
    std::list<boost::shared_array<char> > _mems;
    std::list<size_t> _lens;
    std::vector<boost::shared_ptr<dummy_mrb> > _mrbs;
    std::string _end;
};

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_one_channel_normal){
////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format = "sc16_item32_be";
    id.num_inputs = 1;
    id.output_format = "fc32";
    id.num_outputs = 1;

    dummy_recv_xport_class dummy_recv_xport("big");
    uhd::transport::vrt::if_packet_info_t ifpi;
    ifpi.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count = 0;
    ifpi.sob = true;
    ifpi.eob = false;
    ifpi.has_sid = false;
    ifpi.has_cid = false;
    ifpi.has_tsi = true;
    ifpi.has_tsf = true;
    ifpi.tsi = 0;
    ifpi.tsf = 0;
    ifpi.has_tlr = false;

    static const double TICK_RATE = 100e6;
    static const double SAMP_RATE = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    //generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        ifpi.num_payload_words32 = 10 + i%10;
        dummy_recv_xport.push_back_packet(ifpi);
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32*size_t(TICK_RATE/SAMP_RATE);
    }

    //create the super receive packet handler
    uhd::transport::sph::recv_packet_handler handler(1);
    handler.set_vrt_unpacker(&uhd::transport::vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(0, boost::bind(&dummy_recv_xport_class::get_recv_buff, &dummy_recv_xport, _1));
    handler.set_converter(id);

    //check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float> > buff(20);
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret = handler.recv(
            &buff.front(), buff.size(), metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10 + i%10);
        num_accum_samps += num_samps_ret;
    }

    //subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++){
        std::cout << "timeout check " << i << std::endl;
        handler.recv(
            &buff.front(), buff.size(), metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_one_channel_sequence_error){
////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format = "sc16_item32_be";
    id.num_inputs = 1;
    id.output_format = "fc32";
    id.num_outputs = 1;

    dummy_recv_xport_class dummy_recv_xport("big");
    uhd::transport::vrt::if_packet_info_t ifpi;
    ifpi.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count = 0;
    ifpi.sob = true;
    ifpi.eob = false;
    ifpi.has_sid = false;
    ifpi.has_cid = false;
    ifpi.has_tsi = true;
    ifpi.has_tsf = true;
    ifpi.tsi = 0;
    ifpi.tsf = 0;
    ifpi.has_tlr = false;

    static const double TICK_RATE = 100e6;
    static const double SAMP_RATE = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    //generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        ifpi.num_payload_words32 = 10 + i%10;
        if (i != NUM_PKTS_TO_TEST/2){ //simulate a lost packet
            dummy_recv_xport.push_back_packet(ifpi);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32*size_t(TICK_RATE/SAMP_RATE);
    }

    //create the super receive packet handler
    uhd::transport::sph::recv_packet_handler handler(1);
    handler.set_vrt_unpacker(&uhd::transport::vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(0, boost::bind(&dummy_recv_xport_class::get_recv_buff, &dummy_recv_xport, _1));
    handler.set_converter(id);

    //check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float> > buff(20);
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret = handler.recv(
            &buff.front(), buff.size(), metadata, 1.0, true
        );
        if (i == NUM_PKTS_TO_TEST/2){
            //must get the soft overflow here
            BOOST_REQUIRE(metadata.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            num_accum_samps += 10 + i%10;
        }
        else{
            BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
            BOOST_CHECK(not metadata.more_fragments);
            BOOST_CHECK(metadata.has_time_spec);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            BOOST_CHECK_EQUAL(num_samps_ret, 10 + i%10);
            num_accum_samps += num_samps_ret;
        }
    }

    //subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++){
        std::cout << "timeout check " << i << std::endl;
        handler.recv(
            &buff.front(), buff.size(), metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_one_channel_inline_message){
////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format = "sc16_item32_be";
    id.num_inputs = 1;
    id.output_format = "fc32";
    id.num_outputs = 1;

    dummy_recv_xport_class dummy_recv_xport("big");
    uhd::transport::vrt::if_packet_info_t ifpi;
    ifpi.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count = 0;
    ifpi.sob = true;
    ifpi.eob = false;
    ifpi.has_sid = false;
    ifpi.has_cid = false;
    ifpi.has_tsi = true;
    ifpi.has_tsf = true;
    ifpi.tsi = 0;
    ifpi.tsf = 0;
    ifpi.has_tlr = false;

    static const double TICK_RATE = 100e6;
    static const double SAMP_RATE = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    //generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        ifpi.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_DATA;
        ifpi.num_payload_words32 = 10 + i%10;
        dummy_recv_xport.push_back_packet(ifpi);
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32*size_t(TICK_RATE/SAMP_RATE);

        //simulate overflow
        if (i == NUM_PKTS_TO_TEST/2){
            ifpi.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_EXTENSION;
            ifpi.num_payload_words32 = 1;
            dummy_recv_xport.push_back_packet(ifpi, uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
        }
    }

    //create the super receive packet handler
    uhd::transport::sph::recv_packet_handler handler(1);
    handler.set_vrt_unpacker(&uhd::transport::vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(0, boost::bind(&dummy_recv_xport_class::get_recv_buff, &dummy_recv_xport, _1));
    handler.set_converter(id);

    //create an overflow handler
    overflow_handler_type overflow_handler;
    handler.set_overflow_handler(0, boost::bind(&overflow_handler_type::handle, &overflow_handler));

    //check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float> > buff(20);
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret = handler.recv(
            &buff.front(), buff.size(), metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10 + i%10);
        num_accum_samps += num_samps_ret;
        if (i == NUM_PKTS_TO_TEST/2){
            handler.recv(
                &buff.front(), buff.size(), metadata, 1.0, true
            );
            std::cout << "metadata.error_code " << metadata.error_code << std::endl;
            BOOST_REQUIRE(metadata.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            BOOST_CHECK_EQUAL(overflow_handler.num_overflow, size_t(1));
        }
    }

    //subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++){
        std::cout << "timeout check " << i << std::endl;
        handler.recv(
            &buff.front(), buff.size(), metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_multi_channel_normal){
////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format = "sc16_item32_be";
    id.num_inputs = 1;
    id.output_format = "fc32";
    id.num_outputs = 1;

    uhd::transport::vrt::if_packet_info_t ifpi;
    ifpi.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count = 0;
    ifpi.sob = true;
    ifpi.eob = false;
    ifpi.has_sid = false;
    ifpi.has_cid = false;
    ifpi.has_tsi = true;
    ifpi.has_tsf = true;
    ifpi.tsi = 0;
    ifpi.tsf = 0;
    ifpi.has_tlr = false;

    static const double TICK_RATE = 100e6;
    static const double SAMP_RATE = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;
    static const size_t NUM_SAMPS_PER_BUFF = 20;
    static const size_t NCHANNELS = 4;

    std::vector<dummy_recv_xport_class> dummy_recv_xports(NCHANNELS, dummy_recv_xport_class("big"));

    //generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        ifpi.num_payload_words32 = 10 + i%10;
        for (size_t ch = 0; ch < NCHANNELS; ch++){
            dummy_recv_xports[ch].push_back_packet(ifpi);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32*size_t(TICK_RATE/SAMP_RATE);
    }

    //create the super receive packet handler
    uhd::transport::sph::recv_packet_handler handler(NCHANNELS);
    handler.set_vrt_unpacker(&uhd::transport::vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    for (size_t ch = 0; ch < NCHANNELS; ch++){
        handler.set_xport_chan_get_buff(ch, boost::bind(&dummy_recv_xport_class::get_recv_buff, &dummy_recv_xports[ch], _1));
    }
    handler.set_converter(id);

    //check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float> > mem(NUM_SAMPS_PER_BUFF*NCHANNELS);
    std::vector<std::complex<float> *> buffs(NCHANNELS);
    for (size_t ch = 0; ch < NCHANNELS; ch++){
        buffs[ch] = &mem[ch*NUM_SAMPS_PER_BUFF];
    }
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret = handler.recv(
            buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10 + i%10);
        num_accum_samps += num_samps_ret;
    }

    //subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++){
        std::cout << "timeout check " << i << std::endl;
        handler.recv(
            buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }

}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_multi_channel_sequence_error){
////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format = "sc16_item32_be";
    id.num_inputs = 1;
    id.output_format = "fc32";
    id.num_outputs = 1;

    uhd::transport::vrt::if_packet_info_t ifpi;
    ifpi.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count = 0;
    ifpi.sob = true;
    ifpi.eob = false;
    ifpi.has_sid = false;
    ifpi.has_cid = false;
    ifpi.has_tsi = true;
    ifpi.has_tsf = true;
    ifpi.tsi = 0;
    ifpi.tsf = 0;
    ifpi.has_tlr = false;

    static const double TICK_RATE = 100e6;
    static const double SAMP_RATE = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;
    static const size_t NUM_SAMPS_PER_BUFF = 20;
    static const size_t NCHANNELS = 4;

    std::vector<dummy_recv_xport_class> dummy_recv_xports(NCHANNELS, dummy_recv_xport_class("big"));

    //generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        ifpi.num_payload_words32 = 10 + i%10;
        for (size_t ch = 0; ch < NCHANNELS; ch++){
            if (i == NUM_PKTS_TO_TEST/2 and ch == 2){
                continue; //simulates a lost packet
            }
            dummy_recv_xports[ch].push_back_packet(ifpi);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32*size_t(TICK_RATE/SAMP_RATE);
    }

    //create the super receive packet handler
    uhd::transport::sph::recv_packet_handler handler(NCHANNELS);
    handler.set_vrt_unpacker(&uhd::transport::vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    for (size_t ch = 0; ch < NCHANNELS; ch++){
        handler.set_xport_chan_get_buff(ch, boost::bind(&dummy_recv_xport_class::get_recv_buff, &dummy_recv_xports[ch], _1));
    }
    handler.set_converter(id);

    //check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float> > mem(NUM_SAMPS_PER_BUFF*NCHANNELS);
    std::vector<std::complex<float> *> buffs(NCHANNELS);
    for (size_t ch = 0; ch < NCHANNELS; ch++){
        buffs[ch] = &mem[ch*NUM_SAMPS_PER_BUFF];
    }
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret = handler.recv(
            buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true
        );
        if (i == NUM_PKTS_TO_TEST/2){
            //must get the soft overflow here
            BOOST_REQUIRE(metadata.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            num_accum_samps += 10 + i%10;
        }
        else{
            BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
            BOOST_CHECK(not metadata.more_fragments);
            BOOST_CHECK(metadata.has_time_spec);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            BOOST_CHECK_EQUAL(num_samps_ret, 10 + i%10);
            num_accum_samps += num_samps_ret;
        }
    }

    //subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++){
        std::cout << "timeout check " << i << std::endl;
        handler.recv(
            buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_multi_channel_time_error){
////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format = "sc16_item32_be";
    id.num_inputs = 1;
    id.output_format = "fc32";
    id.num_outputs = 1;

    uhd::transport::vrt::if_packet_info_t ifpi;
    ifpi.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count = 0;
    ifpi.sob = true;
    ifpi.eob = false;
    ifpi.has_sid = false;
    ifpi.has_cid = false;
    ifpi.has_tsi = true;
    ifpi.has_tsf = true;
    ifpi.tsi = 0;
    ifpi.tsf = 0;
    ifpi.has_tlr = false;

    static const double TICK_RATE = 100e6;
    static const double SAMP_RATE = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;
    static const size_t NUM_SAMPS_PER_BUFF = 20;
    static const size_t NCHANNELS = 4;

    std::vector<dummy_recv_xport_class> dummy_recv_xports(NCHANNELS, dummy_recv_xport_class("big"));

    //generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        ifpi.num_payload_words32 = 10 + i%10;
        for (size_t ch = 0; ch < NCHANNELS; ch++){
            dummy_recv_xports[ch].push_back_packet(ifpi);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32*size_t(TICK_RATE/SAMP_RATE);
        if (i == NUM_PKTS_TO_TEST/2){
            ifpi.tsf = 0; //simulate the user changing the time
        }
    }

    //create the super receive packet handler
    uhd::transport::sph::recv_packet_handler handler(NCHANNELS);
    handler.set_vrt_unpacker(&uhd::transport::vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    for (size_t ch = 0; ch < NCHANNELS; ch++){
        handler.set_xport_chan_get_buff(ch, boost::bind(&dummy_recv_xport_class::get_recv_buff, &dummy_recv_xports[ch], _1));
    }
    handler.set_converter(id);

    //check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float> > mem(NUM_SAMPS_PER_BUFF*NCHANNELS);
    std::vector<std::complex<float> *> buffs(NCHANNELS);
    for (size_t ch = 0; ch < NCHANNELS; ch++){
        buffs[ch] = &mem[ch*NUM_SAMPS_PER_BUFF];
    }
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret = handler.recv(
            buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10 + i%10);
        num_accum_samps += num_samps_ret;
        if (i == NUM_PKTS_TO_TEST/2){
            num_accum_samps = 0; //simulate the user changing the time
        }
    }

    //subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++){
        std::cout << "timeout check " << i << std::endl;
        handler.recv(
            buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_multi_channel_fragment){
////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format = "sc16_item32_be";
    id.num_inputs = 1;
    id.output_format = "fc32";
    id.num_outputs = 1;

    uhd::transport::vrt::if_packet_info_t ifpi;
    ifpi.packet_type = uhd::transport::vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count = 0;
    ifpi.sob = true;
    ifpi.eob = false;
    ifpi.has_sid = false;
    ifpi.has_cid = false;
    ifpi.has_tsi = true;
    ifpi.has_tsf = true;
    ifpi.tsi = 0;
    ifpi.tsf = 0;
    ifpi.has_tlr = false;

    static const double TICK_RATE = 100e6;
    static const double SAMP_RATE = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;
    static const size_t NUM_SAMPS_PER_BUFF = 10;
    static const size_t NCHANNELS = 4;

    std::vector<dummy_recv_xport_class> dummy_recv_xports(NCHANNELS, dummy_recv_xport_class("big"));

    //generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        ifpi.num_payload_words32 = 10 + i%10;
        for (size_t ch = 0; ch < NCHANNELS; ch++){
            dummy_recv_xports[ch].push_back_packet(ifpi);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32*size_t(TICK_RATE/SAMP_RATE);
    }

    //create the super receive packet handler
    uhd::transport::sph::recv_packet_handler handler(NCHANNELS);
    handler.set_vrt_unpacker(&uhd::transport::vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    for (size_t ch = 0; ch < NCHANNELS; ch++){
        handler.set_xport_chan_get_buff(ch, boost::bind(&dummy_recv_xport_class::get_recv_buff, &dummy_recv_xports[ch], _1));
    }
    handler.set_converter(id);

    //check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float> > mem(NUM_SAMPS_PER_BUFF*NCHANNELS);
    std::vector<std::complex<float> *> buffs(NCHANNELS);
    for (size_t ch = 0; ch < NCHANNELS; ch++){
        buffs[ch] = &mem[ch*NUM_SAMPS_PER_BUFF];
    }
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret = handler.recv(
            buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10);
        num_accum_samps += num_samps_ret;

        if (not metadata.more_fragments) continue;

        num_samps_ret = handler.recv(
            buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK_EQUAL(metadata.fragment_offset, 10);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, i%10);
        num_accum_samps += num_samps_ret;
    }

    //subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++){
        std::cout << "timeout check " << i << std::endl;
        handler.recv(
            buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true
        );
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }

}
