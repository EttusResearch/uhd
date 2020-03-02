//
// Copyright 2011-2012,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../common/mock_zero_copy.hpp"
#include "../lib/transport/super_send_packet_handler.hpp"
#include <boost/shared_array.hpp>
#include <boost/test/unit_test.hpp>
#include <complex>
#include <functional>
#include <list>
#include <vector>

using namespace uhd::transport;

#define BOOST_CHECK_TS_CLOSE(a, b) \
    BOOST_CHECK_CLOSE((a).get_real_secs(), (b).get_real_secs(), 0.001)

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_send_one_channel_one_packet_mode)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    mock_zero_copy xport(vrt::if_packet_info_t::LINK_TYPE_VRLP);

    static const double TICK_RATE        = 100e6;
    static const double SAMP_RATE        = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    // create the super send packet handler
    sph::send_packet_handler handler(1);
    handler.set_vrt_packer(&vrt::if_hdr_pack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(
        0, [&xport](double timeout) { return xport.get_send_buff(timeout); });
    handler.set_converter(id);
    handler.set_max_samples_per_packet(20);

    // allocate metadata and buffer
    std::vector<std::complex<float>> buff(20);
    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec     = uhd::time_spec_t(0.0);

    // generate the test data
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        metadata.start_of_burst = (i == 0);
        metadata.end_of_burst   = (i == NUM_PKTS_TO_TEST - 1);
        const size_t num_sent   = handler.send(&buff.front(), 10 + i % 10, metadata, 1.0);
        BOOST_CHECK_EQUAL(num_sent, 10 + i % 10);
        metadata.time_spec += uhd::time_spec_t(0, num_sent, SAMP_RATE);
    }

    // check the sent packets
    size_t num_accum_samps = 0;
    vrt::if_packet_info_t ifpi;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "data check " << i << std::endl;
        xport.pop_send_packet(ifpi);
        BOOST_CHECK_EQUAL(ifpi.num_payload_words32, 10 + i % 10);
        BOOST_CHECK(ifpi.has_tsf);
        BOOST_CHECK_EQUAL(ifpi.tsf, num_accum_samps * TICK_RATE / SAMP_RATE);
        BOOST_CHECK_EQUAL(ifpi.sob, i == 0);
        BOOST_CHECK_EQUAL(ifpi.eob, i == NUM_PKTS_TO_TEST - 1);
        num_accum_samps += ifpi.num_payload_words32;
    }
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_send_one_channel_full_buffer_mode)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    mock_zero_copy xport(vrt::if_packet_info_t::LINK_TYPE_VRLP);

    static const double TICK_RATE        = 100e6;
    static const double SAMP_RATE        = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    // create the super send packet handler
    sph::send_packet_handler handler(1);
    handler.set_vrt_packer(&vrt::if_hdr_pack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(
        0, [&xport](double timeout) { return xport.get_send_buff(timeout); });
    handler.set_converter(id);
    handler.set_max_samples_per_packet(20);

    // allocate metadata and buffer
    std::vector<std::complex<float>> buff(20 * NUM_PKTS_TO_TEST);
    uhd::tx_metadata_t metadata;
    metadata.start_of_burst = true;
    metadata.end_of_burst   = true;
    metadata.has_time_spec  = true;
    metadata.time_spec      = uhd::time_spec_t(0.0);

    // generate the test data
    const size_t num_sent = handler.send(&buff.front(), buff.size(), metadata, 1.0);
    BOOST_CHECK_EQUAL(num_sent, buff.size());

    // check the sent packets
    size_t num_accum_samps = 0;
    vrt::if_packet_info_t ifpi;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "data check " << i << std::endl;
        xport.pop_send_packet(ifpi);
        BOOST_CHECK_EQUAL(ifpi.num_payload_words32, 20UL);
        BOOST_CHECK(ifpi.has_tsf);
        BOOST_CHECK_EQUAL(ifpi.tsf, num_accum_samps * TICK_RATE / SAMP_RATE);
        BOOST_CHECK_EQUAL(ifpi.sob, i == 0);
        BOOST_CHECK_EQUAL(ifpi.eob, i == NUM_PKTS_TO_TEST - 1);
        num_accum_samps += ifpi.num_payload_words32;
    }
}
