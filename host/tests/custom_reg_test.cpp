//
// Copyright 2024 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/rfnoc/clock_iface.hpp>
#include <uhdlib/rfnoc/ctrlport_endpoint.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>

using namespace uhd::rfnoc;

BOOST_AUTO_TEST_CASE(test_custom_register_space)
{
    ctrlport_endpoint::send_fn_t mock_send_fn = [](const chdr::ctrl_payload& /*payload*/,
                                                    double /*timeout*/) {
        // noop
    };
    std::shared_ptr<clock_iface> mock_clock = std::make_shared<clock_iface>("mock_clock");

    // Create ctrlport_endpoint instance, this unit test is designed to test the custom
    // register spaces, so all other arguments don't matter since they will not be used
    // for their intended functionality
    std::shared_ptr<ctrlport_endpoint> endpoint = ctrlport_endpoint::make(mock_send_fn,
        0, // my_epid
        0, // local_port
        1024, // buff_capacity
        10, // max_outstanding_async_msgs
        *mock_clock, // client_clk
        *mock_clock // timebase_clk
    );

    // Custom register spaces that do not overlap, so should not error when defined
    uint32_t valid_start_addr1 = 0x1000;
    uint32_t valid_start_addr2 = 0x2000;
    uint32_t valid_start_addr3 = 0x3000;
    uint32_t valid_length      = 4096;
    // Custom register space that overlaps starting address with the existing space, so
    // should error
    uint32_t overlap_start_addr1 = 0x1FFF;
    uint32_t overlap_length1     = 1;
    // Custom register space that overlaps end address with the existing space, so should
    // error
    uint32_t overlap_start_addr2 = 0xFFF;
    uint32_t overlap_length2     = 2;

    // With this mock implementation, if the custom peek/poke is not called a system error
    // will be thrown since the clocks are not running, so use this error as a way to test
    // that the default peek/poke is being called.
    BOOST_CHECK_THROW(endpoint->poke32(0x1050, 2), uhd::system_error);
    BOOST_CHECK_THROW(endpoint->peek32(0x1050), uhd::system_error);

    uint32_t register_value1 = 0;
    uint32_t register_value2 = 30;
    uint32_t register_value3 = 10;

    // Define custom register space
    endpoint->define_custom_register_space(
        valid_start_addr3,
        valid_length,
        [&register_value3](uint32_t /*addr*/, uint32_t data) { register_value3 -= data; },
        [&register_value3](uint32_t /*addr*/) -> uint32_t { return register_value3; });

    BOOST_CHECK_NO_THROW(endpoint->define_custom_register_space(
        valid_start_addr1,
        valid_length,
        [&register_value1](uint32_t /*addr*/, uint32_t data) { register_value1 = data; },
        [&register_value1](uint32_t /*addr*/) -> uint32_t { return register_value1; }));

    BOOST_CHECK_NO_THROW(endpoint->define_custom_register_space(
        valid_start_addr2,
        valid_length,
        [&register_value2](uint32_t /*addr*/, uint32_t data) { register_value2 += data; },
        [&register_value2](uint32_t /*addr*/) -> uint32_t { return register_value2; }));

    // Now same peek calls should not throw since that space is used by custom
    // register space, poke will still throw since the default poke is also called,
    // that error is caught in test_poke32 function
    BOOST_CHECK_EQUAL(endpoint->peek32(0x1050), 0);
    endpoint->poke32(0x1050, 2);
    BOOST_CHECK_EQUAL(endpoint->peek32(0x1050), 2);
    BOOST_CHECK_EQUAL(endpoint->peek32(0x2050), 30);
    endpoint->poke32(0x2050, 2);
    BOOST_CHECK_EQUAL(endpoint->peek32(0x2050), 32);
    BOOST_CHECK_EQUAL(endpoint->peek32(0x3050), 10);
    endpoint->poke32(0x3050, 2);
    BOOST_CHECK_EQUAL(endpoint->peek32(0x3050), 8);

    BOOST_CHECK_THROW(endpoint->define_custom_register_space(
                          overlap_start_addr1,
                          overlap_length1,
                          [](uint32_t, uint32_t) {},
                          [](uint32_t) -> uint32_t { return 0; }),
        uhd::rfnoc_error);

    BOOST_CHECK_THROW(endpoint->define_custom_register_space(
                          overlap_start_addr2,
                          overlap_length2,
                          [](uint32_t, uint32_t) {},
                          [](uint32_t) -> uint32_t { return 0; }),
        uhd::rfnoc_error);

    // Invalid custom register space, length of space is 0
    BOOST_CHECK_THROW(
        endpoint->define_custom_register_space(
            0x4000, 0, [](uint32_t, uint32_t) {}, [](uint32_t) -> uint32_t { return 0; }),
        uhd::value_error);

    // Invalid custom register space, length causes end_addr to exceed limit for uint32_t
    BOOST_CHECK_THROW(endpoint->define_custom_register_space(
                          0xFFFFFFF0,
                          17,
                          [](uint32_t, uint32_t) {},
                          [](uint32_t) -> uint32_t { return 0; }),
        uhd::value_error);

    // Check that addresses outside of the custom spaces still throw system error
    // signalling that default peek/poke is being called
    BOOST_CHECK_THROW(endpoint->peek32(0x50), uhd::system_error);
    BOOST_CHECK_THROW(endpoint->peek32(0x4050), uhd::system_error);
    BOOST_CHECK_THROW(endpoint->poke32(0x50, 0), uhd::system_error);
    BOOST_CHECK_THROW(endpoint->poke32(0x4050, 0), uhd::system_error);
}
