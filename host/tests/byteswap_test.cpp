//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/endianness.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/predef/other/endian.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_endianness_macro)
{
#if BOOST_ENDIAN_BIG_BYTE

#    ifdef UHD_BIG_ENDIAN
    BOOST_CHECK(true);
#    else
    BOOST_CHECK(false);
#    endif

#elif BOOST_ENDIAN_LITTLE_BYTE

#    ifdef UHD_LITTLE_ENDIAN
    BOOST_CHECK(true);
#    else
    BOOST_CHECK(false);
#    endif

#else
    // We should never get here
    BOOST_CHECK(false);
#endif
}

BOOST_AUTO_TEST_CASE(test_byteswap16)
{
    uint16_t x = 0x0123;
    uint16_t y = 0x2301;
    BOOST_CHECK_EQUAL(uhd::byteswap(x), y);
}

BOOST_AUTO_TEST_CASE(test_byteswap32)
{
    uint32_t x = 0x01234567;
    uint32_t y = 0x67452301;
    BOOST_CHECK_EQUAL(uhd::byteswap(x), y);
}

BOOST_AUTO_TEST_CASE(test_byteswap64)
{
    // split up 64 bit constants to avoid long-long compiler warnings
    uint64_t x = 0x01234567 | (uint64_t(0x89abcdef) << 32);
    uint64_t y = 0xefcdab89 | (uint64_t(0x67452301) << 32);
    BOOST_CHECK_EQUAL(uhd::byteswap(x), y);
}

BOOST_AUTO_TEST_CASE(test_htolx)
{
    // Test host to link conversion with big endian
    constexpr uint16_t x16 = 0x0123;
    constexpr uint32_t x32 = 0x01234567;
    constexpr uint64_t x64 = 0x0123456789abcdefULL;

#ifdef UHD_BIG_ENDIAN
    // On big endian host, converting to big endian should be identity
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_BIG>(x16), x16);
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_BIG>(x32), x32);
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_BIG>(x64), x64);

    // On big endian host, converting to little endian should byteswap
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_LITTLE>(x16), uhd::byteswap(x16));
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_LITTLE>(x32), uhd::byteswap(x32));
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_LITTLE>(x64), uhd::byteswap(x64));
#else
    // On little endian host, converting to little endian should be identity
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_LITTLE>(x16), x16);
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_LITTLE>(x32), x32);
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_LITTLE>(x64), x64);

    // On little endian host, converting to big endian should byteswap
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_BIG>(x16), uhd::byteswap(x16));
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_BIG>(x32), uhd::byteswap(x32));
    BOOST_CHECK_EQUAL(uhd::htolx<uhd::ENDIANNESS_BIG>(x64), uhd::byteswap(x64));
#endif
}

BOOST_AUTO_TEST_CASE(test_ltohx)
{
    // Test link to host conversion with big endian
    constexpr uint16_t x16 = 0x0123;
    constexpr uint32_t x32 = 0x01234567;
    constexpr uint64_t x64 = 0x0123456789abcdefULL;

#ifdef UHD_BIG_ENDIAN
    // On big endian host, converting from big endian should be identity
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_BIG>(x16), x16);
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_BIG>(x32), x32);
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_BIG>(x64), x64);

    // On big endian host, converting from little endian should byteswap
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_LITTLE>(x16), uhd::byteswap(x16));
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_LITTLE>(x32), uhd::byteswap(x32));
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_LITTLE>(x64), uhd::byteswap(x64));
#else
    // On little endian host, converting from little endian should be identity
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_LITTLE>(x16), x16);
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_LITTLE>(x32), x32);
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_LITTLE>(x64), x64);

    // On little endian host, converting from big endian should byteswap
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_BIG>(x16), uhd::byteswap(x16));
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_BIG>(x32), uhd::byteswap(x32));
    BOOST_CHECK_EQUAL(uhd::ltohx<uhd::ENDIANNESS_BIG>(x64), uhd::byteswap(x64));
#endif
}

BOOST_AUTO_TEST_CASE(test_ltohx_htolx_roundtrip)
{
    // Test that ltohx and htolx are inverse operations
    constexpr uint16_t x16 = 0x0123;
    constexpr uint32_t x32 = 0x01234567;
    constexpr uint64_t x64 = 0x0123456789abcdefULL;

    // Test roundtrip with big endian
    BOOST_CHECK_EQUAL(
        uhd::ltohx<uhd::ENDIANNESS_BIG>(uhd::htolx<uhd::ENDIANNESS_BIG>(x16)), x16);
    BOOST_CHECK_EQUAL(
        uhd::ltohx<uhd::ENDIANNESS_BIG>(uhd::htolx<uhd::ENDIANNESS_BIG>(x32)), x32);
    BOOST_CHECK_EQUAL(
        uhd::ltohx<uhd::ENDIANNESS_BIG>(uhd::htolx<uhd::ENDIANNESS_BIG>(x64)), x64);

    // Test roundtrip with little endian
    BOOST_CHECK_EQUAL(
        uhd::ltohx<uhd::ENDIANNESS_LITTLE>(uhd::htolx<uhd::ENDIANNESS_LITTLE>(x16)), x16);
    BOOST_CHECK_EQUAL(
        uhd::ltohx<uhd::ENDIANNESS_LITTLE>(uhd::htolx<uhd::ENDIANNESS_LITTLE>(x32)), x32);
    BOOST_CHECK_EQUAL(
        uhd::ltohx<uhd::ENDIANNESS_LITTLE>(uhd::htolx<uhd::ENDIANNESS_LITTLE>(x64)), x64);

    // Test reverse roundtrip with big endian
    BOOST_CHECK_EQUAL(
        uhd::htolx<uhd::ENDIANNESS_BIG>(uhd::ltohx<uhd::ENDIANNESS_BIG>(x16)), x16);
    BOOST_CHECK_EQUAL(
        uhd::htolx<uhd::ENDIANNESS_BIG>(uhd::ltohx<uhd::ENDIANNESS_BIG>(x32)), x32);
    BOOST_CHECK_EQUAL(
        uhd::htolx<uhd::ENDIANNESS_BIG>(uhd::ltohx<uhd::ENDIANNESS_BIG>(x64)), x64);

    // Test reverse roundtrip with little endian
    BOOST_CHECK_EQUAL(
        uhd::htolx<uhd::ENDIANNESS_LITTLE>(uhd::ltohx<uhd::ENDIANNESS_LITTLE>(x16)), x16);
    BOOST_CHECK_EQUAL(
        uhd::htolx<uhd::ENDIANNESS_LITTLE>(uhd::ltohx<uhd::ENDIANNESS_LITTLE>(x32)), x32);
    BOOST_CHECK_EQUAL(
        uhd::htolx<uhd::ENDIANNESS_LITTLE>(uhd::ltohx<uhd::ENDIANNESS_LITTLE>(x64)), x64);
}
