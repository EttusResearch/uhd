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
#include "../lib/transport/super_send_packet_handler.hpp"
#include <boost/shared_array.hpp>
#include <boost/bind.hpp>
#include <complex>
#include <vector>
#include <list>

#define BOOST_CHECK_TS_CLOSE(a, b) \
    BOOST_CHECK_CLOSE((a).get_real_secs(), (b).get_real_secs(), 0.001)

/***********************************************************************
 * A dummy managed send buffer for testing
 **********************************************************************/
class dummy_msb : public uhd::transport::managed_send_buffer{
public:
    void release(void){
        //NOP
    }

    sptr get_new(boost::shared_array<char> mem, size_t *len){
        _mem = mem;
        return make(this, mem.get(), *len);
    }

private:
    boost::shared_array<char> _mem;
};

/***********************************************************************
 * A dummy transport class to fill with fake data
 **********************************************************************/
class dummy_send_xport_class{
public:
    dummy_send_xport_class(const std::string &end){
        _end = end;
    }

    void pop_front_packet(
        uhd::transport::vrt::if_packet_info_t &ifpi
    ){
        ifpi.num_packet_words32 = _lens.front()/sizeof(boost::uint32_t);
        if (_end == "big"){
            uhd::transport::vrt::if_hdr_unpack_be(reinterpret_cast<boost::uint32_t *>(_mems.front().get()), ifpi);
        }
        if (_end == "little"){
            uhd::transport::vrt::if_hdr_unpack_le(reinterpret_cast<boost::uint32_t *>(_mems.front().get()), ifpi);
        }
        _mems.pop_front();
        _lens.pop_front();
    }

    uhd::transport::managed_send_buffer::sptr get_send_buff(double){
        _msbs.push_back(boost::shared_ptr<dummy_msb>(new dummy_msb()));
        _mems.push_back(boost::shared_array<char>(new char[1000]));
        _lens.push_back(1000);
        uhd::transport::managed_send_buffer::sptr mrb = _msbs.back()->get_new(_mems.back(), &_lens.back());
        return mrb;
    }

private:
    std::list<boost::shared_array<char> > _mems;
    std::list<size_t> _lens;
    std::vector<boost::shared_ptr<dummy_msb> > _msbs;
    std::string _end;
};

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_send_one_channel_one_packet_mode){
////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs = 1;

    dummy_send_xport_class dummy_send_xport("big");

    static const double TICK_RATE = 100e6;
    static const double SAMP_RATE = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    //create the super send packet handler
    uhd::transport::sph::send_packet_handler handler(1);
    handler.set_vrt_packer(&uhd::transport::vrt::if_hdr_pack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(0, boost::bind(&dummy_send_xport_class::get_send_buff, &dummy_send_xport, _1));
    handler.set_converter(id);
    handler.set_max_samples_per_packet(20);

    //allocate metadata and buffer
    std::vector<std::complex<float> > buff(20);
    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec = uhd::time_spec_t(0.0);

    //generate the test data
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        metadata.start_of_burst = (i == 0);
        metadata.end_of_burst = (i == NUM_PKTS_TO_TEST-1);
        const size_t num_sent = handler.send(
            &buff.front(), 10 + i%10, metadata, 1.0
        );
        BOOST_CHECK_EQUAL(num_sent, 10 + i%10);
        metadata.time_spec += uhd::time_spec_t(0, num_sent, SAMP_RATE);
    }

    //check the sent packets
    size_t num_accum_samps = 0;
    uhd::transport::vrt::if_packet_info_t ifpi;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        std::cout << "data check " << i << std::endl;
        dummy_send_xport.pop_front_packet(ifpi);
        BOOST_CHECK_EQUAL(ifpi.num_payload_words32, 10+i%10);
        BOOST_CHECK(ifpi.has_tsf);
        BOOST_CHECK_EQUAL(ifpi.tsf, num_accum_samps*TICK_RATE/SAMP_RATE);
        BOOST_CHECK_EQUAL(ifpi.sob, i == 0);
        BOOST_CHECK_EQUAL(ifpi.eob, i == NUM_PKTS_TO_TEST-1);
        num_accum_samps += ifpi.num_payload_words32;
    }
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_send_one_channel_full_buffer_mode){
////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs = 1;

    dummy_send_xport_class dummy_send_xport("big");

    static const double TICK_RATE = 100e6;
    static const double SAMP_RATE = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    //create the super send packet handler
    uhd::transport::sph::send_packet_handler handler(1);
    handler.set_vrt_packer(&uhd::transport::vrt::if_hdr_pack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(0, boost::bind(&dummy_send_xport_class::get_send_buff, &dummy_send_xport, _1));
    handler.set_converter(id);
    handler.set_max_samples_per_packet(20);

    //allocate metadata and buffer
    std::vector<std::complex<float> > buff(20*NUM_PKTS_TO_TEST);
    uhd::tx_metadata_t metadata;
    metadata.start_of_burst = true;
    metadata.end_of_burst = true;
    metadata.has_time_spec = true;
    metadata.time_spec = uhd::time_spec_t(0.0);

    //generate the test data
    const size_t num_sent = handler.send(
        &buff.front(), buff.size(), metadata, 1.0
    );
    BOOST_CHECK_EQUAL(num_sent, buff.size());

    //check the sent packets
    size_t num_accum_samps = 0;
    uhd::transport::vrt::if_packet_info_t ifpi;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++){
        std::cout << "data check " << i << std::endl;
        dummy_send_xport.pop_front_packet(ifpi);
        BOOST_CHECK_EQUAL(ifpi.num_payload_words32, 20);
        BOOST_CHECK(ifpi.has_tsf);
        BOOST_CHECK_EQUAL(ifpi.tsf, num_accum_samps*TICK_RATE/SAMP_RATE);
        BOOST_CHECK_EQUAL(ifpi.sob, i == 0);
        BOOST_CHECK_EQUAL(ifpi.eob, i == NUM_PKTS_TO_TEST-1);
        num_accum_samps += ifpi.num_payload_words32;
    }
}
