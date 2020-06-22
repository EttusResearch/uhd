//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/features/discoverable_feature_getter_iface.hpp>
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

class test_feature_getter : public discoverable_feature_getter_iface
{
public:
    test_feature_getter(bool feature0_enabled, bool feature1_enabled)
    {
        if (feature0_enabled)
            _test_feature0.reset(new test_feature0());
        if (feature1_enabled)
            _test_feature1.reset(new test_feature1);
    }

    std::vector<std::string> enumerate_features() override
    {
        std::vector<std::string> features;
        if (_test_feature0)
            features.push_back(_test_feature0->get_feature_name());
        if (_test_feature1)
            features.push_back(_test_feature1->get_feature_name());
        return features;
    }

private:
    discoverable_feature::sptr get_feature_ptr(
        discoverable_feature::feature_id_t feature_id) override
    {
        switch (feature_id) {
            case discoverable_feature::RESERVED0:
                return _test_feature0;
            case discoverable_feature::RESERVED1:
                return _test_feature1;
            default:
                return discoverable_feature::sptr();
        };
    }

    discoverable_feature::sptr _test_feature0;
    discoverable_feature::sptr _test_feature1;
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

    // Note that ordering isn't strictly a requirement of the interface,
    // we just leverage that here to demonstrate API usage concisely.
    BOOST_CHECK_EQUAL(features[0], "test_feature0");
    BOOST_CHECK_EQUAL(features[1], "test_feature1");
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
