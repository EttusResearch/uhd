//
// Copyright 2011-2012,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../common/mock_zero_copy.hpp"
#include "../lib/transport/super_recv_packet_handler.hpp"
#include <boost/shared_array.hpp>
#include <boost/test/unit_test.hpp>
#include <complex>
#include <functional>
#include <list>
#include <vector>

using namespace uhd::transport;

#define BOOST_CHECK_TS_CLOSE(a, b) \
    BOOST_CHECK_CLOSE((a).get_real_secs(), (b).get_real_secs(), 0.001)

/***********************************************************************
 * A dummy overflow handler for testing
 **********************************************************************/
struct overflow_handler_type
{
    overflow_handler_type(void)
    {
        num_overflow = 0;
    }
    void handle(void)
    {
        num_overflow++;
    }
    size_t num_overflow;
};

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_one_channel_normal)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "sc16_item32_be";
    id.num_inputs    = 1;
    id.output_format = "fc32";
    id.num_outputs   = 1;

    mock_zero_copy xport(vrt::if_packet_info_t::LINK_TYPE_VRLP);

    vrt::if_packet_info_t ifpi;
    ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count        = 0;
    ifpi.sob                 = true;
    ifpi.eob                 = false;
    ifpi.has_sid             = false;
    ifpi.has_cid             = false;
    ifpi.has_tsi             = true;
    ifpi.has_tsf             = true;
    ifpi.tsi                 = 0;
    ifpi.tsf                 = 0;
    ifpi.has_tlr             = false;

    static const double TICK_RATE        = 100e6;
    static const double SAMP_RATE        = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    // generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        ifpi.num_payload_words32 = 10 + i % 10;
        std::vector<uint32_t> data(ifpi.num_payload_words32, 0);
        xport.push_back_recv_packet(ifpi, data);
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32 * size_t(TICK_RATE / SAMP_RATE);
    }

    // create the super receive packet handler
    uhd::transport::sph::recv_packet_handler handler(1);
    handler.set_vrt_unpacker(&uhd::transport::vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(
        0, [&xport](double timeout) { return xport.get_recv_buff(timeout); });
    handler.set_converter(id);

    // check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float>> buff(20);
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret =
            handler.recv(&buff.front(), buff.size(), metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(
            metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10 + i % 10);
        num_accum_samps += num_samps_ret;
    }

    // subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++) {
        std::cout << "timeout check " << i << std::endl;
        handler.recv(&buff.front(), buff.size(), metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }

    // simulate the transport failing
    xport.set_simulate_io_error(true);
    BOOST_REQUIRE_THROW(
        handler.recv(&buff.front(), buff.size(), metadata, 1.0, true), uhd::io_error);
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_one_channel_sequence_error)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "sc16_item32_be";
    id.num_inputs    = 1;
    id.output_format = "fc32";
    id.num_outputs   = 1;

    mock_zero_copy xport(vrt::if_packet_info_t::LINK_TYPE_VRLP);

    vrt::if_packet_info_t ifpi;
    ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count        = 0;
    ifpi.sob                 = true;
    ifpi.eob                 = false;
    ifpi.has_sid             = false;
    ifpi.has_cid             = false;
    ifpi.has_tsi             = true;
    ifpi.has_tsf             = true;
    ifpi.tsi                 = 0;
    ifpi.tsf                 = 0;
    ifpi.has_tlr             = false;

    static const double TICK_RATE        = 100e6;
    static const double SAMP_RATE        = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    // generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        ifpi.num_payload_words32 = 10 + i % 10;
        if (i != NUM_PKTS_TO_TEST / 2) { // simulate a lost packet
            std::vector<uint32_t> data(ifpi.num_payload_words32, 0);
            xport.push_back_recv_packet(ifpi, data);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32 * size_t(TICK_RATE / SAMP_RATE);
    }

    // create the super receive packet handler
    sph::recv_packet_handler handler(1);
    handler.set_vrt_unpacker(&vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(
        0, [&xport](double timeout) { return xport.get_recv_buff(timeout); });
    handler.set_converter(id);

    // check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float>> buff(20);
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret =
            handler.recv(&buff.front(), buff.size(), metadata, 1.0, true);
        if (i == NUM_PKTS_TO_TEST / 2) {
            // must get the soft overflow here
            BOOST_REQUIRE(metadata.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
            BOOST_REQUIRE(metadata.out_of_sequence == true);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec,
                uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            num_accum_samps += 10 + i % 10;
        } else {
            BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
            BOOST_CHECK(not metadata.more_fragments);
            BOOST_CHECK(metadata.has_time_spec);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec,
                uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            BOOST_CHECK_EQUAL(num_samps_ret, 10 + i % 10);
            num_accum_samps += num_samps_ret;
        }
    }

    // subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++) {
        std::cout << "timeout check " << i << std::endl;
        handler.recv(&buff.front(), buff.size(), metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }

    // simulate the transport failing
    xport.set_simulate_io_error(true);
    BOOST_REQUIRE_THROW(
        handler.recv(&buff.front(), buff.size(), metadata, 1.0, true), uhd::io_error);
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_one_channel_inline_message)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "sc16_item32_be";
    id.num_inputs    = 1;
    id.output_format = "fc32";
    id.num_outputs   = 1;

    mock_zero_copy xport(vrt::if_packet_info_t::LINK_TYPE_VRLP);

    vrt::if_packet_info_t ifpi;
    ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count        = 0;
    ifpi.sob                 = true;
    ifpi.eob                 = false;
    ifpi.has_sid             = false;
    ifpi.has_cid             = false;
    ifpi.has_tsi             = true;
    ifpi.has_tsf             = true;
    ifpi.tsi                 = 0;
    ifpi.tsf                 = 0;
    ifpi.has_tlr             = false;

    static const double TICK_RATE        = 100e6;
    static const double SAMP_RATE        = 10e6;
    static const size_t NUM_PKTS_TO_TEST = 30;

    // generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
        ifpi.num_payload_words32 = 10 + i % 10;
        std::vector<uint32_t> data(ifpi.num_payload_words32, 0);
        xport.push_back_recv_packet(ifpi, data);
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32 * size_t(TICK_RATE / SAMP_RATE);

        // simulate overflow
        if (i == NUM_PKTS_TO_TEST / 2) {
            ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_CONTEXT;
            ifpi.num_payload_words32 = 1;

            xport.push_back_inline_message_packet(
                ifpi, uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
        }
    }

    // create the super receive packet handler
    sph::recv_packet_handler handler(1);
    handler.set_vrt_unpacker(&vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    handler.set_xport_chan_get_buff(
        0, [&xport](double timeout) { return xport.get_recv_buff(timeout); });
    handler.set_converter(id);

    // create an overflow handler
    overflow_handler_type overflow_handler;
    handler.set_overflow_handler(
        0, std::bind(&overflow_handler_type::handle, &overflow_handler));

    // check the received packets
    size_t num_accum_samps = 0;
    std::vector<std::complex<float>> buff(20);
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret =
            handler.recv(&buff.front(), buff.size(), metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(
            metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10 + i % 10);
        num_accum_samps += num_samps_ret;
        if (i == NUM_PKTS_TO_TEST / 2) {
            handler.recv(&buff.front(), buff.size(), metadata, 1.0, true);
            std::cout << "metadata.error_code " << metadata.error_code << std::endl;
            BOOST_REQUIRE(metadata.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec,
                uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            BOOST_CHECK_EQUAL(overflow_handler.num_overflow, size_t(1));
        }
    }

    // subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++) {
        std::cout << "timeout check " << i << std::endl;
        handler.recv(&buff.front(), buff.size(), metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }

    // simulate the transport failing
    xport.set_simulate_io_error(true);
    BOOST_REQUIRE_THROW(
        handler.recv(&buff.front(), buff.size(), metadata, 1.0, true), uhd::io_error);
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_multi_channel_normal)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "sc16_item32_be";
    id.num_inputs    = 1;
    id.output_format = "fc32";
    id.num_outputs   = 1;

    vrt::if_packet_info_t ifpi;
    ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count        = 0;
    ifpi.sob                 = true;
    ifpi.eob                 = false;
    ifpi.has_sid             = false;
    ifpi.has_cid             = false;
    ifpi.has_tsi             = true;
    ifpi.has_tsf             = true;
    ifpi.tsi                 = 0;
    ifpi.tsf                 = 0;
    ifpi.has_tlr             = false;

    static const double TICK_RATE          = 100e6;
    static const double SAMP_RATE          = 10e6;
    static const size_t NUM_PKTS_TO_TEST   = 30;
    static const size_t NUM_SAMPS_PER_BUFF = 20;
    static const size_t NCHANNELS          = 4;

    std::vector<mock_zero_copy::sptr> xports;
    for (size_t i = 0; i < NCHANNELS; i++) {
        xports.push_back(
            std::make_shared<mock_zero_copy>(vrt::if_packet_info_t::LINK_TYPE_VRLP));
    }

    // generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        ifpi.num_payload_words32 = 10 + i % 10;
        for (size_t ch = 0; ch < NCHANNELS; ch++) {
            std::vector<uint32_t> data(ifpi.num_payload_words32, 0);
            xports[ch]->push_back_recv_packet(ifpi, data);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32 * size_t(TICK_RATE / SAMP_RATE);
    }

    // create the super receive packet handler
    sph::recv_packet_handler handler(NCHANNELS);
    handler.set_vrt_unpacker(&vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        mock_zero_copy::sptr xport = xports[ch];
        handler.set_xport_chan_get_buff(
            ch, [xport](double timeout) { return xport->get_recv_buff(timeout); });
    }
    handler.set_converter(id);

    // check the received packets
    size_t num_accum_samps = 0;
    std::complex<float> mem[NUM_SAMPS_PER_BUFF * NCHANNELS];
    std::vector<std::complex<float>*> buffs(NCHANNELS);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        buffs[ch] = &mem[ch * NUM_SAMPS_PER_BUFF];
    }
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret =
            handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(
            metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10 + i % 10);
        num_accum_samps += num_samps_ret;
    }

    // subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++) {
        std::cout << "timeout check " << i << std::endl;
        handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }

    // simulate the transport failing
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        xports[ch]->set_simulate_io_error(true);
    }

    BOOST_REQUIRE_THROW(
        handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true), uhd::io_error);
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_multi_channel_sequence_error)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "sc16_item32_be";
    id.num_inputs    = 1;
    id.output_format = "fc32";
    id.num_outputs   = 1;

    vrt::if_packet_info_t ifpi;
    ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count        = 0;
    ifpi.sob                 = true;
    ifpi.eob                 = false;
    ifpi.has_sid             = false;
    ifpi.has_cid             = false;
    ifpi.has_tsi             = true;
    ifpi.has_tsf             = true;
    ifpi.tsi                 = 0;
    ifpi.tsf                 = 0;
    ifpi.has_tlr             = false;

    static const double TICK_RATE          = 100e6;
    static const double SAMP_RATE          = 10e6;
    static const size_t NUM_PKTS_TO_TEST   = 30;
    static const size_t NUM_SAMPS_PER_BUFF = 20;
    static const size_t NCHANNELS          = 4;

    std::vector<mock_zero_copy::sptr> xports;
    for (size_t i = 0; i < NCHANNELS; i++) {
        xports.push_back(
            std::make_shared<mock_zero_copy>(vrt::if_packet_info_t::LINK_TYPE_VRLP));
    }

    // generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        ifpi.num_payload_words32 = 10 + i % 10;
        for (size_t ch = 0; ch < NCHANNELS; ch++) {
            if (i == NUM_PKTS_TO_TEST / 2 and ch == 2) {
                continue; // simulates a lost packet
            }
            std::vector<uint32_t> data(ifpi.num_payload_words32, 0);
            xports[ch]->push_back_recv_packet(ifpi, data);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32 * size_t(TICK_RATE / SAMP_RATE);
    }

    // create the super receive packet handler
    sph::recv_packet_handler handler(NCHANNELS);
    handler.set_vrt_unpacker(&vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        mock_zero_copy::sptr xport = xports[ch];
        handler.set_xport_chan_get_buff(
            ch, [xport](double timeout) { return xport->get_recv_buff(timeout); });
    }
    handler.set_converter(id);

    // check the received packets
    size_t num_accum_samps = 0;
    std::complex<float> mem[NUM_SAMPS_PER_BUFF * NCHANNELS];
    std::vector<std::complex<float>*> buffs(NCHANNELS);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        buffs[ch] = &mem[ch * NUM_SAMPS_PER_BUFF];
    }
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret =
            handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true);
        if (i == NUM_PKTS_TO_TEST / 2) {
            // must get the soft overflow here
            BOOST_REQUIRE(metadata.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW);
            BOOST_REQUIRE(metadata.out_of_sequence == true);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec,
                uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            num_accum_samps += 10 + i % 10;
        } else {
            BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
            BOOST_CHECK(not metadata.more_fragments);
            BOOST_CHECK(metadata.has_time_spec);
            BOOST_CHECK_TS_CLOSE(metadata.time_spec,
                uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
            BOOST_CHECK_EQUAL(num_samps_ret, 10 + i % 10);
            num_accum_samps += num_samps_ret;
        }
    }

    // subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++) {
        std::cout << "timeout check " << i << std::endl;
        handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }

    // simulate the transport failing
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        xports[ch]->set_simulate_io_error(true);
    }

    BOOST_REQUIRE_THROW(
        handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true), uhd::io_error);
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_multi_channel_time_error)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "sc16_item32_be";
    id.num_inputs    = 1;
    id.output_format = "fc32";
    id.num_outputs   = 1;

    vrt::if_packet_info_t ifpi;
    ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count        = 0;
    ifpi.sob                 = true;
    ifpi.eob                 = false;
    ifpi.has_sid             = false;
    ifpi.has_cid             = false;
    ifpi.has_tsi             = true;
    ifpi.has_tsf             = true;
    ifpi.tsi                 = 0;
    ifpi.tsf                 = 0;
    ifpi.has_tlr             = false;

    static const double TICK_RATE          = 100e6;
    static const double SAMP_RATE          = 10e6;
    static const size_t NUM_PKTS_TO_TEST   = 30;
    static const size_t NUM_SAMPS_PER_BUFF = 20;
    static const size_t NCHANNELS          = 4;

    std::vector<mock_zero_copy::sptr> xports;
    for (size_t i = 0; i < NCHANNELS; i++) {
        xports.push_back(
            std::make_shared<mock_zero_copy>(vrt::if_packet_info_t::LINK_TYPE_VRLP));
    }

    // generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        ifpi.num_payload_words32 = 10 + i % 10;
        for (size_t ch = 0; ch < NCHANNELS; ch++) {
            std::vector<uint32_t> data(ifpi.num_payload_words32, 0);
            xports[ch]->push_back_recv_packet(ifpi, data);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32 * size_t(TICK_RATE / SAMP_RATE);
        if (i == NUM_PKTS_TO_TEST / 2) {
            ifpi.tsf = 0; // simulate the user changing the time
        }
    }

    // create the super receive packet handler
    sph::recv_packet_handler handler(NCHANNELS);
    handler.set_vrt_unpacker(&vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        mock_zero_copy::sptr xport = xports[ch];
        handler.set_xport_chan_get_buff(
            ch, [xport](double timeout) { return xport->get_recv_buff(timeout); });
    }
    handler.set_converter(id);

    // check the received packets
    size_t num_accum_samps = 0;
    std::complex<float> mem[NUM_SAMPS_PER_BUFF * NCHANNELS];
    std::vector<std::complex<float>*> buffs(NCHANNELS);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        buffs[ch] = &mem[ch * NUM_SAMPS_PER_BUFF];
    }
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret =
            handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(
            metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10 + i % 10);
        num_accum_samps += num_samps_ret;
        if (i == NUM_PKTS_TO_TEST / 2) {
            num_accum_samps = 0; // simulate the user changing the time
        }
    }

    // subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++) {
        std::cout << "timeout check " << i << std::endl;
        handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }

    // simulate the transport failing
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        xports[ch]->set_simulate_io_error(true);
    }

    BOOST_REQUIRE_THROW(
        handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true), uhd::io_error);
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_multi_channel_exception)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "sc16_item32_be";
    id.num_inputs    = 1;
    id.output_format = "fc32";
    id.num_outputs   = 1;

    vrt::if_packet_info_t ifpi;
    ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count        = 0;
    ifpi.sob                 = true;
    ifpi.eob                 = false;
    ifpi.has_sid             = false;
    ifpi.has_cid             = false;
    ifpi.has_tsi             = true;
    ifpi.has_tsf             = true;
    ifpi.tsi                 = 0;
    ifpi.tsf                 = 0;
    ifpi.has_tlr             = false;

    static const double TICK_RATE          = 100e6;
    static const double SAMP_RATE          = 10e6;
    static const size_t NUM_PKTS_TO_TEST   = 30;
    static const size_t NUM_SAMPS_PER_BUFF = 20;
    static const size_t NCHANNELS          = 4;

    std::vector<mock_zero_copy::sptr> xports;
    for (size_t i = 0; i < NCHANNELS; i++) {
        xports.push_back(
            std::make_shared<mock_zero_copy>(vrt::if_packet_info_t::LINK_TYPE_VRLP));
    }

    // generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        ifpi.num_payload_words32 = 10 + i % 10;
        for (size_t ch = 0; ch < NCHANNELS; ch++) {
            std::vector<uint32_t> data(ifpi.num_payload_words32, 0);
            xports[ch]->push_back_recv_packet(ifpi, data);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32 * size_t(TICK_RATE / SAMP_RATE);
        if (i == NUM_PKTS_TO_TEST / 2) {
            ifpi.tsf = 0; // simulate the user changing the time
        }
    }

    // create the super receive packet handler
    sph::recv_packet_handler handler(NCHANNELS);
    handler.set_vrt_unpacker(&vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        mock_zero_copy::sptr xport = xports[ch];
        handler.set_xport_chan_get_buff(
            ch, [xport](double timeout) { return xport->get_recv_buff(timeout); });
    }
    handler.set_converter(id);

    std::complex<float> mem[NUM_SAMPS_PER_BUFF * NCHANNELS];
    std::vector<std::complex<float>*> buffs(NCHANNELS);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        buffs[ch] = &mem[ch * NUM_SAMPS_PER_BUFF];
    }

    // simulate a failure on a channel (the last one)
    uhd::rx_metadata_t metadata;
    xports[NCHANNELS - 1]->set_simulate_io_error(true);

    std::cout << "exception check" << std::endl;

    BOOST_REQUIRE_THROW(
        handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true), uhd::io_error);
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_sph_recv_multi_channel_fragment)
{
    ////////////////////////////////////////////////////////////////////////
    uhd::convert::id_type id;
    id.input_format  = "sc16_item32_be";
    id.num_inputs    = 1;
    id.output_format = "fc32";
    id.num_outputs   = 1;

    vrt::if_packet_info_t ifpi;
    ifpi.packet_type         = vrt::if_packet_info_t::PACKET_TYPE_DATA;
    ifpi.num_payload_words32 = 0;
    ifpi.packet_count        = 0;
    ifpi.sob                 = true;
    ifpi.eob                 = false;
    ifpi.has_sid             = false;
    ifpi.has_cid             = false;
    ifpi.has_tsi             = true;
    ifpi.has_tsf             = true;
    ifpi.tsi                 = 0;
    ifpi.tsf                 = 0;
    ifpi.has_tlr             = false;

    static const double TICK_RATE          = 100e6;
    static const double SAMP_RATE          = 10e6;
    static const size_t NUM_PKTS_TO_TEST   = 30;
    static const size_t NUM_SAMPS_PER_BUFF = 10;
    static const size_t NCHANNELS          = 4;

    std::vector<mock_zero_copy::sptr> xports;
    for (size_t i = 0; i < NCHANNELS; i++) {
        xports.push_back(
            std::make_shared<mock_zero_copy>(vrt::if_packet_info_t::LINK_TYPE_VRLP));
    }

    // generate a bunch of packets
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        ifpi.num_payload_words32 = 10 + i % 10;
        for (size_t ch = 0; ch < NCHANNELS; ch++) {
            std::vector<uint32_t> data(ifpi.num_payload_words32, 0);
            xports[ch]->push_back_recv_packet(ifpi, data);
        }
        ifpi.packet_count++;
        ifpi.tsf += ifpi.num_payload_words32 * size_t(TICK_RATE / SAMP_RATE);
    }

    // create the super receive packet handler
    sph::recv_packet_handler handler(NCHANNELS);
    handler.set_vrt_unpacker(&vrt::if_hdr_unpack_be);
    handler.set_tick_rate(TICK_RATE);
    handler.set_samp_rate(SAMP_RATE);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        mock_zero_copy::sptr xport = xports[ch];
        handler.set_xport_chan_get_buff(
            ch, [xport](double timeout) { return xport->get_recv_buff(timeout); });
    }
    handler.set_converter(id);

    // check the received packets
    size_t num_accum_samps = 0;
    std::complex<float> mem[NUM_SAMPS_PER_BUFF * NCHANNELS];
    std::vector<std::complex<float>*> buffs(NCHANNELS);
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        buffs[ch] = &mem[ch * NUM_SAMPS_PER_BUFF];
    }
    uhd::rx_metadata_t metadata;
    for (size_t i = 0; i < NUM_PKTS_TO_TEST; i++) {
        std::cout << "data check " << i << std::endl;
        size_t num_samps_ret =
            handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(
            metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, 10UL);
        num_accum_samps += num_samps_ret;

        if (not metadata.more_fragments)
            continue;

        num_samps_ret = handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_NONE);
        BOOST_CHECK(not metadata.more_fragments);
        BOOST_CHECK_EQUAL(metadata.fragment_offset, 10UL);
        BOOST_CHECK(metadata.has_time_spec);
        BOOST_CHECK_TS_CLOSE(
            metadata.time_spec, uhd::time_spec_t::from_ticks(num_accum_samps, SAMP_RATE));
        BOOST_CHECK_EQUAL(num_samps_ret, i % 10);
        num_accum_samps += num_samps_ret;
    }

    // subsequent receives should be a timeout
    for (size_t i = 0; i < 3; i++) {
        std::cout << "timeout check " << i << std::endl;
        handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true);
        BOOST_CHECK_EQUAL(metadata.error_code, uhd::rx_metadata_t::ERROR_CODE_TIMEOUT);
    }

    // simulate the transport failing
    for (size_t ch = 0; ch < NCHANNELS; ch++) {
        xports[ch]->set_simulate_io_error(true);
    }

    BOOST_REQUIRE_THROW(
        handler.recv(buffs, NUM_SAMPS_PER_BUFF, metadata, 1.0, true), uhd::io_error);
}
