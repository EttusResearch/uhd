//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/dirtifier.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhdlib/rfnoc/prop_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

BOOST_AUTO_TEST_CASE(test_res_source_info)
{
    res_source_info S1(res_source_info::USER);

    BOOST_CHECK_EQUAL(S1.type, res_source_info::USER);
    BOOST_CHECK_EQUAL(S1.instance, 0);
    BOOST_CHECK_EQUAL(S1.to_string(), "USER:0");

    res_source_info S2{res_source_info::USER, 5};
    BOOST_CHECK_EQUAL(S2.type, res_source_info::USER);
    BOOST_CHECK_EQUAL(S2.instance, 5);

    // Check initializer
    auto src_info_printer = [](res_source_info rsi) {
        std::cout << static_cast<int>(rsi.type) << "::" << rsi.instance << std::endl;
    };
    src_info_printer({res_source_info::OUTPUT_EDGE, 5});
    src_info_printer({res_source_info::OUTPUT_EDGE});

    res_source_info S3{res_source_info::USER, 5};
    BOOST_CHECK(S2 == S3);
}

BOOST_AUTO_TEST_CASE(test_get_set)
{
    constexpr int prop_i_val = 5;
    // This is a legitimate way to initialize a property:
    property_t<int> prop_i{"int_prop", prop_i_val, {res_source_info::USER, 2}};
    BOOST_CHECK_EQUAL(prop_i.get_src_info().instance, 2);
    BOOST_CHECK_EQUAL(prop_i.get_src_info().type, res_source_info::USER);
    prop_accessor_t prop_accessor;

    prop_accessor.mark_clean(prop_i);
    auto access_lock = prop_accessor.get_scoped_prop_access(prop_i, property_base_t::RW);

    BOOST_CHECK(!prop_i.is_dirty());
    BOOST_CHECK_EQUAL(prop_i.get_id(), "int_prop");
    BOOST_CHECK_EQUAL(prop_i.get_src_info().type, res_source_info::USER);

    BOOST_CHECK_EQUAL(prop_i.get(), prop_i_val);
    BOOST_CHECK_EQUAL(int(prop_i), prop_i_val);
    BOOST_CHECK(prop_i == 5);

    prop_i.set(42);
    BOOST_CHECK_EQUAL(prop_i.get(), 42);
    BOOST_CHECK(prop_i.is_dirty());
    prop_accessor.mark_clean(prop_i);
    BOOST_CHECK(!prop_i.is_dirty());
    prop_i = 23;
    BOOST_CHECK_EQUAL(prop_i.get(), 23);
    BOOST_CHECK(prop_i.is_dirty());
}

BOOST_AUTO_TEST_CASE(test_valid_names)
{
    bool value_error_caught = false;
    try {
        property_t<int> prop_i{"int_prop:0", 10, {res_source_info::USER, 0}};
    } catch (const uhd::value_error& e) {
        value_error_caught = true;
    } catch (...) {
    }

    BOOST_CHECK(value_error_caught);
}

BOOST_AUTO_TEST_CASE(test_lock)
{
    prop_accessor_t prop_accessor;
    constexpr int prop_i_val = 5;
    property_t<int> prop_i{"int_prop", prop_i_val, {res_source_info::USER}};
    prop_accessor.mark_clean(prop_i);
    prop_accessor.set_access(prop_i, property_base_t::RWLOCKED);
    prop_i.set(5);
    BOOST_REQUIRE_THROW(prop_i.set(42), uhd::resolve_error);
}

BOOST_AUTO_TEST_CASE(test_access)
{
    constexpr int prop_i_val = 5;
    property_t<int> prop_i{"int_prop", prop_i_val, {res_source_info::USER}};
    prop_accessor_t prop_accessor;

    prop_accessor.mark_clean(prop_i);
    BOOST_REQUIRE_THROW(prop_i.set(23), uhd::access_error);
    prop_accessor.set_access(prop_i, property_base_t::RO);
    BOOST_CHECK_EQUAL(prop_i.get(), prop_i_val);
    BOOST_REQUIRE_THROW(prop_i.set(23), uhd::access_error);
    prop_accessor.set_access(prop_i, property_base_t::RW);
    prop_i.set(23);
    BOOST_CHECK_EQUAL(prop_i.get(), 23);
    prop_accessor.set_access(prop_i, property_base_t::NONE);

    // Now test the scoped access mode
    {
        auto access_lock = prop_accessor.get_scoped_prop_access(
            prop_i, property_base_t::RW, property_base_t::NONE);
        prop_i.set(42);
        BOOST_CHECK_EQUAL(prop_i.get(), 42);
    }
    BOOST_REQUIRE_THROW(prop_i.get(), uhd::access_error);
    BOOST_REQUIRE_THROW(prop_i.set(23), uhd::access_error);

    // Now test a different default access mode
    {
        auto access_lock = prop_accessor.get_scoped_prop_access(
            prop_i, property_base_t::RW, property_base_t::RO);
        prop_i.set(42);
    }
    BOOST_CHECK_EQUAL(prop_i.get(), 42);
    // The prop is still in RO mode now!
    BOOST_CHECK_EQUAL(prop_i.get_access_mode(), property_base_t::RO);

    res_source_info new_src_info{res_source_info::INPUT_EDGE, 1};
    auto cloned_prop = prop_i.clone(new_src_info);
    BOOST_CHECK(cloned_prop->get_src_info() == new_src_info);
    BOOST_CHECK(prop_i.equal(cloned_prop.get()));
}

BOOST_AUTO_TEST_CASE(test_forward)
{
    prop_accessor_t prop_accessor;
    property_t<int> prop_i1{"int_prop", 5, {res_source_info::USER}};
    property_t<int> prop_i2{"int_prop", 0, {res_source_info::USER}};
    property_t<double> prop_d{"double_prop", 0.0, {res_source_info::USER}};
    prop_accessor.mark_clean(prop_i1);
    prop_accessor.mark_clean(prop_i2);

    BOOST_CHECK(prop_accessor.are_compatible(&prop_i1, &prop_i2));
    prop_accessor.forward<false>(&prop_i1, &prop_i2);
    prop_accessor.set_access(prop_i1, property_base_t::RO);
    prop_accessor.set_access(prop_i2, property_base_t::RW);
    BOOST_CHECK_EQUAL(prop_i2.get(), prop_i1.get());
    BOOST_CHECK(!prop_i1.is_dirty());
    BOOST_CHECK(prop_i2.is_dirty());
    BOOST_CHECK(!prop_accessor.are_compatible(&prop_i1, &prop_d));
    BOOST_REQUIRE_THROW(prop_accessor.forward<false>(&prop_i1, &prop_d), uhd::type_error);
}

BOOST_AUTO_TEST_CASE(test_dirtifier)
{
    prop_accessor_t prop_accessor{};
    dirtifier_t dirtifier;
    BOOST_CHECK(dirtifier.is_dirty());
    prop_accessor.mark_clean(dirtifier);
    BOOST_CHECK(dirtifier.is_dirty());
    property_t<int> prop_i{"int_prop", 5, {res_source_info::USER}};
    BOOST_REQUIRE_THROW(
        prop_accessor.forward<false>(&dirtifier, &prop_i), uhd::type_error);
    BOOST_REQUIRE_THROW(
        prop_accessor.forward<false>(&prop_i, &dirtifier), uhd::type_error);
    BOOST_CHECK(!prop_accessor.are_compatible(&prop_i, &dirtifier));
    BOOST_CHECK(!prop_accessor.are_compatible(&dirtifier, &prop_i));
}

BOOST_AUTO_TEST_CASE(test_from_str)
{
    prop_accessor_t prop_accessor{};
    property_t<double> prop_d{"double_prop", 0.0, {res_source_info::USER}};
    property_t<int> prop_i{"int_prop", 0, {res_source_info::USER}};
    property_t<std::string> prop_s{"str_prop", "0", {res_source_info::USER}};
    prop_accessor.set_access(prop_d, property_base_t::RW);
    prop_accessor.set_access(prop_i, property_base_t::RW);
    prop_accessor.set_access(prop_s, property_base_t::RW);

    property_base_t* prop_base_ptr_d = static_cast<property_base_t*>(&prop_d);
    property_base_t* prop_base_ptr_i = static_cast<property_base_t*>(&prop_i);
    property_base_t* prop_base_ptr_s = static_cast<property_base_t*>(&prop_s);

    prop_base_ptr_d->set_from_str(".25");
    BOOST_CHECK_EQUAL(prop_d.get(), 0.25);

    prop_base_ptr_i->set_from_str("23");
    BOOST_CHECK_EQUAL(prop_i.get(), 23);

    prop_base_ptr_s->set_from_str("foo");
    BOOST_CHECK_EQUAL(prop_s.get(), "foo");

    BOOST_REQUIRE_THROW(prop_base_ptr_d->set_from_str("banana"), uhd::runtime_error);
    BOOST_REQUIRE_THROW(prop_base_ptr_i->set_from_str("potato"), uhd::runtime_error);
}
