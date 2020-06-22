//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/features/discoverable_feature_getter_iface.hpp>
#include <uhdlib/features/discoverable_feature_registry.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::features;

class test_feature0 : public discoverable_feature
{
public:
    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::RESERVED0;
    }

    std::string get_feature_name() const override
    {
        return "test_feature0";
    }
};

class test_feature1 : public discoverable_feature
{
public:
    static discoverable_feature::feature_id_t get_feature_id()
    {
        return discoverable_feature::RESERVED1;
    }

    std::string get_feature_name() const override
    {
        return "test_feature1";
    }
};

class test_feature_getter : public discoverable_feature_registry
{
public:
    test_feature_getter(bool feature0_enabled, bool feature1_enabled)
    {
        if (feature0_enabled)
            register_feature(std::make_shared<test_feature0>());
        if (feature1_enabled)
            register_feature(std::make_shared<test_feature1>());
    }
};

BOOST_AUTO_TEST_CASE(test_has_feature_works)
{
    test_feature_getter feature_getter(true, false);
    BOOST_CHECK_EQUAL(feature_getter.has_feature<test_feature0>(), true);
    BOOST_CHECK_EQUAL(feature_getter.has_feature<test_feature1>(), false);
}

BOOST_AUTO_TEST_CASE(test_enumerate_feature_works)
{
    test_feature_getter feature_getter(true, true);
    const auto features = feature_getter.enumerate_features();

    bool has_feature0 = false;
    bool has_feature1 = false;
    for (auto& feature : features) {
        if (feature == "test_feature0") {
            has_feature0 = true;
        }
        if (feature == "test_feature1") {
            has_feature1 = true;
        }
    }
    BOOST_CHECK_EQUAL(has_feature0, true);
    BOOST_CHECK_EQUAL(has_feature1, true);
    BOOST_CHECK_EQUAL(features.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_feature_works)
{
    test_feature_getter feature_getter(true, true);
    const auto f0 = feature_getter.get_feature<test_feature0>();
    const auto f1 = feature_getter.get_feature<test_feature1>();
    BOOST_CHECK_EQUAL(f0.get_feature_name(), "test_feature0");
    BOOST_CHECK_EQUAL(f1.get_feature_name(), "test_feature1");
}

BOOST_AUTO_TEST_CASE(test_get_feature_throws)
{
    test_feature_getter feature_getter(false, true);
    BOOST_CHECK_THROW(feature_getter.get_feature<test_feature0>(), std::exception);
}
