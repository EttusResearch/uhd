//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/lc_block_control.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

/*
 * This class extends mock_reg_iface_t by adding a constructor that initializes some of
 * the read memory to carry a valid compat number.
 */
class lc_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    lc_mock_reg_iface_t()
    {
        read_memory[lc_block_control::REG_COMPAT_NUM] =
            (lc_block_control::MINOR_COMPAT
                | (static_cast<uint32_t>(lc_block_control::MAJOR_COMPAT) << 16));
        read_memory[lc_block_control::REG_FEATURE_LIST_RB] = 0xC0DEF00D;
    }
};

/* lc_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container and lc_block_control
 * object, all of which are accessible to the test case. The instance of the
 * object is destroyed at the end of each test case.
 */
struct lc_block_fixture
{
    lc_block_fixture()
        : reg_iface(std::make_shared<lc_mock_reg_iface_t>())
        , block_container(
              get_mock_block(LICCHECK_BLOCK, 0, 0, "", 0, ANY_DEVICE, reg_iface))
        , test_lc(block_container.get_block<lc_block_control>())
    {
    }

    std::shared_ptr<lc_mock_reg_iface_t> reg_iface;
    mock_block_container block_container;
    std::shared_ptr<lc_block_control> test_lc;
    node_accessor_t node_accessor{};
};

BOOST_FIXTURE_TEST_CASE(lc_test_fid_list, lc_block_fixture)
{
    const auto fids = test_lc->get_feature_ids();
    BOOST_REQUIRE_EQUAL(fids.size(), 1);
    BOOST_CHECK_EQUAL(fids[0], 0xC0DEF00D);
}

BOOST_FIXTURE_TEST_CASE(lc_test_good_write, lc_block_fixture)
{
    reg_iface->read_memory[lc_block_control::REG_FEATURE_ENABLE_RB] = (1 << 31);

    BOOST_REQUIRE(test_lc->load_key(
        "AHAN5-4ANAA-ISEM2-EKVTH-PCEZV-K54ZX-PO74A-ACAQD-AQCQM-BYIBE-FAWDA-NBYHQ"));

    BOOST_CHECK_MESSAGE(
        reg_iface->write_memory[lc_block_control::REG_FEATURE_ID] == 0xC0DEF00D,
        "FID is different from 0xC0DEF00D: 0x"
            << std::hex << reg_iface->write_memory[lc_block_control::REG_FEATURE_ID]);
    BOOST_CHECK_MESSAGE(
        reg_iface->write_memory[lc_block_control::REG_USER_KEY], 0x0C0D0E0F);
    BOOST_CHECK_MESSAGE(
        reg_iface->write_memory[lc_block_control::REG_USER_KEY] == 0x0C0D0E0F,
        "KEY is different from 0x0C0D0E0F: 0x"
            << std::hex << reg_iface->write_memory[lc_block_control::REG_USER_KEY]);
}

BOOST_FIXTURE_TEST_CASE(lc_test_keyload_fail, lc_block_fixture)
{
    // Enable success flag
    reg_iface->read_memory[lc_block_control::REG_FEATURE_ENABLE_RB] = 1;

    // Invalid key (too short, wrong format)
    BOOST_CHECK(
        !test_lc->load_key("AHAN5-4ANAPCEZV-K54ZX-PO74A-ACAQD-AQCQM-BYIBE-FAWDA-NBYHQ"));
    // Definitely invalid key
    BOOST_CHECK(!test_lc->load_key("!"));
    // Disable success flag
    reg_iface->read_memory[lc_block_control::REG_FEATURE_ENABLE_RB] = 0;
    // This key is valid, but the success flag is not set
    BOOST_CHECK(!test_lc->load_key(
        "AHAN5-4ANAA-ISEM2-EKVTH-PCEZV-K54ZX-PO74A-ACAQD-AQCQM-BYIBE-FAWDA-NBYHQ"));
}
