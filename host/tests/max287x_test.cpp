//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/max287x.hpp>
#include <boost/test/unit_test.hpp>

// Datasheet for max2871:
// https://www.analog.com/media/en/technical-documentation/data-sheets/MAX2871.pdf
// Used to confirm the proper register addresses and values

// Based on values set in gen_max2871_regs.py and the constructor of max2871
std::map<uint8_t, uint32_t> expected_init_values = {
    {0, 0x003e8000},
    {1, 0x2000fff9},
    {2, 0x78006e42},
    {3, 0x0100000b},
    {4, 0x6080023c},
    {5, 0x61400005},
};

// Using macro instead of function so that failures print the line associated with the
// call to UHD_CHECK_REGMAP rather than pointing out the line in UHD_CHECK_REGMAP itself
#define UHD_CHECK_REGMAP(lo, expected)                                            \
    {                                                                             \
        std::map<uint8_t, uint32_t> actual;                                       \
        for (int i = 0; i < 6; i++) {                                             \
            actual.insert(std::pair<uint8_t, uint32_t>(i, lo->get_register(i)));  \
        }                                                                         \
        actual.insert(std::pair<uint8_t, uint32_t>(0, 0));                        \
        for (const auto& expected_r : expected) {                                 \
            std::stringstream exp_ss, act_ss;                                     \
            exp_ss << std::hex << expected_r.second;                              \
            act_ss << std::hex << actual.at(expected_r.first);                    \
            const std::string exp_str =                                           \
                "R" + std::to_string(expected_r.first) + " == 0x" + exp_ss.str(); \
            const std::string act_str =                                           \
                "R" + std::to_string(expected_r.first) + " == 0x" + act_ss.str(); \
            BOOST_CHECK_EQUAL(exp_str, act_str);                                  \
        }                                                                         \
    }

// Used to set the field of a register to use for an expected value
void set_field(uint32_t& reg, uint32_t mask, uint32_t shift, uint32_t value)
{
    reg &= ~(mask << shift);
    reg |= (value & mask) << shift;
}

// Test that the set_register and get_register functions work as expected for all
// writeable registers (non-address) and also that set_register works when masking
// for specific registers
BOOST_AUTO_TEST_CASE(max2871_set_get_register_test)
{
    auto test_lo =
        max287x_iface::make<max2871>([this](const std::vector<uint32_t>&) { /* noop */ });

    test_lo->set_register(0, 0xFFFFFFF8, 0x12345678);
    test_lo->set_register(1, 0xFFFFFFF8, 0x12345678);
    test_lo->set_register(2, 0xFFFFFFF8, 0x12345678);
    test_lo->set_register(3, 0xFFFFFFF8, 0x12345678);
    test_lo->set_register(4, 0xFFFFFFF8, 0x12345678);
    test_lo->set_register(5, 0xFFFFFFF8, 0x12345678);
    // Registers should differ only by the address bits of each register
    std::map<uint8_t, uint32_t> expected_values = {
        {0, 0x12345678},
        {1, 0x12345679},
        {2, 0x1234567a},
        {3, 0x1234567b},
        {4, 0x1234567c},
        {5, 0x1234567d},
    };
    UHD_CHECK_REGMAP(test_lo, expected_values);

    test_lo->set_register(3, 0x0F000000, 0xFFFFFFFF);
    test_lo->set_register(5, 0x000FF000, 0xFFFFFFFF);
    test_lo->set_register(1, 0xF0000000, 0xFFFFFFFF);

    set_field(expected_values[3], 0xF, 24, 0XF);
    set_field(expected_values[5], 0xFF, 12, 0XFF);
    set_field(expected_values[1], 0xF, 28, 0XF);
    UHD_CHECK_REGMAP(test_lo, expected_values);
}

// Test that registers are at the expected values after initialization
BOOST_AUTO_TEST_CASE(max2871_init_test)
{
    auto test_lo =
        max287x_iface::make<max2871>([this](const std::vector<uint32_t>&) { /* noop */ });

    UHD_CHECK_REGMAP(test_lo, expected_init_values);
}

// Test that functions that set output power function correctly
BOOST_AUTO_TEST_CASE(max2871_set_output_power_test)
{
    auto test_lo =
        max287x_iface::make<max2871>([this](const std::vector<uint32_t>&) { /* noop */ });

    // set_output_power should only affect RFOUTA Output Power (bits 4:3 of reg 4)
    test_lo->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
    auto expected_values = expected_init_values;
    set_field(expected_values[4], 0x3, 3, max287x_iface::OUTPUT_POWER_5DBM);
    UHD_CHECK_REGMAP(test_lo, expected_values);

    test_lo->set_output_power(max287x_iface::OUTPUT_POWER_M1DBM);
    set_field(expected_values[4], 0x3, 3, max287x_iface::OUTPUT_POWER_M1DBM);
    UHD_CHECK_REGMAP(test_lo, expected_values);

    // set_aux_output_power should only affect RFOUTB Output Power (bits 7:6 of reg 4)
    test_lo->set_aux_output_power(max287x_iface::AUX_OUTPUT_POWER_5DBM);
    set_field(expected_values[4], 0x3, 6, max287x_iface::AUX_OUTPUT_POWER_5DBM);
    UHD_CHECK_REGMAP(test_lo, expected_values);

    test_lo->set_aux_output_power(max287x_iface::AUX_OUTPUT_POWER_M1DBM);
    set_field(expected_values[4], 0x3, 6, max287x_iface::AUX_OUTPUT_POWER_M1DBM);
    UHD_CHECK_REGMAP(test_lo, expected_values);

    // set_aux_output_power_enable should only affect RFOUTB Output Mode (bit 8 of reg 4)
    test_lo->set_aux_output_power_enable(true);
    set_field(expected_values[4], 0x1, 8, 1);
    UHD_CHECK_REGMAP(test_lo, expected_values);
    test_lo->set_aux_output_power_enable(false);
    set_field(expected_values[4], 0x1, 8, 0);
    UHD_CHECK_REGMAP(test_lo, expected_values);
}

// Test that registers that need to be set for phase synchronization are set correctly
// Also, test that phase synchronization only advertises as possible when in frac_n mode
BOOST_AUTO_TEST_CASE(max2871_phase_sync_test)
{
    auto test_lo =
        max287x_iface::make<max2871>([this](const std::vector<uint32_t>&) { /* noop */ });

    test_lo->set_frequency(2e9, 50e6, 50e6, true);
    BOOST_CHECK_EQUAL(max2871_regs_t::SHUTDOWN_VAS_ENABLED,
        static_cast<max2871_regs_t::shutdown_vas_t>(
            (test_lo->get_register(3) >> 25) & 0x1));
    BOOST_CHECK_EQUAL(max2871_regs_t::LOW_NOISE_AND_SPUR_LOW_SPUR_2,
        static_cast<max2871_regs_t::low_noise_and_spur_t>(
            (test_lo->get_register(2) >> 29) & 0x3));
    BOOST_CHECK_EQUAL(max2871_regs_t::F01_AUTO,
        static_cast<max2871_regs_t::f01_t>((test_lo->get_register(5) >> 24) & 0x1));
    BOOST_CHECK_EQUAL(max2871_regs_t::AUX_OUTPUT_SELECT_FUNDAMENTAL,
        static_cast<max2871_regs_t::aux_output_select_t>(
            (test_lo->get_register(4) >> 9) & 0x1));

    // For phase synchronization, ensure the following registers are set to the following
    test_lo->config_for_sync(true);
    test_lo->set_frequency(2e9, 50e6, 50e6, true);
    BOOST_CHECK_EQUAL(max2871_regs_t::SHUTDOWN_VAS_DISABLED,
        static_cast<max2871_regs_t::shutdown_vas_t>(
            (test_lo->get_register(3) >> 25) & 0x1));
    BOOST_CHECK_EQUAL(max2871_regs_t::LOW_NOISE_AND_SPUR_LOW_NOISE,
        static_cast<max2871_regs_t::low_noise_and_spur_t>(
            (test_lo->get_register(2) >> 29) & 0x3));
    BOOST_CHECK_EQUAL(max2871_regs_t::F01_FRAC_N,
        static_cast<max2871_regs_t::f01_t>((test_lo->get_register(5) >> 24) & 0x1));
    BOOST_CHECK_EQUAL(max2871_regs_t::AUX_OUTPUT_SELECT_DIVIDED,
        static_cast<max2871_regs_t::aux_output_select_t>(
            (test_lo->get_register(4) >> 9) & 0x1));

    test_lo->commit();
    // Have configured for sync, but cannot sync in int_n mode
    BOOST_CHECK(!test_lo->can_sync());
    test_lo->set_frequency(2e9, 50e6, 50e6, false);
    test_lo->commit();
    // Now should be possible to sync in frac_n mode
    BOOST_CHECK(test_lo->can_sync());
}

// Test that the registers that need to be set for int_n vs frac_n mode within
// the set_frequency function are set correctly
BOOST_AUTO_TEST_CASE(max2871_set_frequency_int_n_test)
{
    auto test_lo =
        max287x_iface::make<max2871>([this](const std::vector<uint32_t>&) { /* noop */ });

    // set_frequency with int_n = true, ensure that all registers that need to be set for
    // int_n are set correctly (other arguments set arbitrarily for this test)
    test_lo->set_frequency(2e9, 50e6, 50e6, true);
    BOOST_CHECK_EQUAL(max2871_regs_t::FEEDBACK_SELECT_DIVIDED,
        static_cast<max2871_regs_t::feedback_select_t>(
            (test_lo->get_register(4) >> 23) & 0x1));
    BOOST_CHECK_EQUAL(max2871_regs_t::CPL_DISABLED,
        static_cast<max2871_regs_t::cpl_t>((test_lo->get_register(1) >> 29) & 0x3));
    BOOST_CHECK_EQUAL(max2871_regs_t::LDF_INT_N,
        static_cast<max2871_regs_t::ldf_t>((test_lo->get_register(2) >> 8) & 0x1));
    BOOST_CHECK_EQUAL(max2871_regs_t::INT_N_MODE_INT_N,
        static_cast<max2871_regs_t::int_n_mode_t>(
            (test_lo->get_register(0) >> 31) & 0x1));

    // set_frequency with int_n = false, ensure that all registers that need to be set for
    // frac_n are set correctly (other arguments set arbitrarily for this test)
    test_lo->set_frequency(2e9, 50e6, 50e6, false);
    BOOST_CHECK_EQUAL(max2871_regs_t::FEEDBACK_SELECT_DIVIDED,
        static_cast<max2871_regs_t::feedback_select_t>(
            (test_lo->get_register(4) >> 23) & 0x1));
    BOOST_CHECK_EQUAL(max2871_regs_t::CPL_ENABLED,
        static_cast<max2871_regs_t::cpl_t>((test_lo->get_register(1) >> 29) & 0x3));
    BOOST_CHECK_EQUAL(max2871_regs_t::LDF_FRAC_N,
        static_cast<max2871_regs_t::ldf_t>((test_lo->get_register(2) >> 8) & 0x1));
    BOOST_CHECK_EQUAL(max2871_regs_t::INT_N_MODE_FRAC_N,
        static_cast<max2871_regs_t::int_n_mode_t>(
            (test_lo->get_register(0) >> 31) & 0x1));
}
