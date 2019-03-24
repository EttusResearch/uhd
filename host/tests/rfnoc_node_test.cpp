//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/node.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

class test_node_t : public node_t
{
public:
    test_node_t(size_t num_inputs, size_t num_outputs)
        : _num_input_ports(num_inputs), _num_output_ports(num_outputs)
    {
        register_property(&_double_prop_user);
        register_property(&_double_prop_in);
        register_property(&_double_prop_out);

        // A property with a simple 1:1 dependency
        add_property_resolver(
            {&_double_prop_user}, {&_double_prop_out}, []() { std::cout << "foo" << std::endl; });
    }

    //! Register a property for the second time, with the goal of triggering an
    // exception
    void double_register()
    {
        register_property(&_double_prop_user);
    }

    //! Register an identical property for the first time, with the goal of
    //triggering an exception
    void double_register_input()
    {
        property_t<double> double_prop_in{"double_prop", 0.0, {res_source_info::INPUT_EDGE, 0}};
        register_property(&double_prop_in);
    }

    //! This should throw an error because the property in the output isn't
    // registered
    void add_unregistered_resolver_in() {
        property_t<double> temp{"temp", 0.0, {res_source_info::INPUT_EDGE, 5}};
        add_property_resolver(
            {&temp}, {}, []() { std::cout << "foo" << std::endl; });
    }

    //! This should throw an error because the property in the output isn't
    // registered
    void add_unregistered_resolver_out() {
        property_t<double> temp{"temp", 0.0, {res_source_info::INPUT_EDGE, 5}};
        add_property_resolver(
            {&_double_prop_user}, {&temp}, []() { std::cout << "foo" << std::endl; });
    }

    size_t get_num_input_ports() const { return _num_input_ports; }
    size_t get_num_output_ports() const { return _num_output_ports; }

private:
    property_t<double> _double_prop_user{"double_prop", 0.0, {res_source_info::USER}};
    property_t<double> _double_prop_in{"double_prop", 0.0, {res_source_info::INPUT_EDGE, 0}};
    property_t<double> _double_prop_out{
        "double_prop", 0.0, {res_source_info::OUTPUT_EDGE, 1}};

    const size_t _num_input_ports;
    const size_t _num_output_ports;
};

BOOST_AUTO_TEST_CASE(test_node_prop_access)
{
    test_node_t TN1(2, 3);
    test_node_t TN2(1, 1);

    BOOST_REQUIRE_THROW(TN2.double_register(), uhd::runtime_error);
    BOOST_REQUIRE_THROW(TN2.double_register_input(), uhd::runtime_error);
    BOOST_REQUIRE_THROW(TN2.add_unregistered_resolver_in(), uhd::runtime_error);
    BOOST_REQUIRE_THROW(TN2.add_unregistered_resolver_out(), uhd::runtime_error);

    BOOST_CHECK_EQUAL(TN1.get_num_input_ports(), 2);
    BOOST_CHECK_EQUAL(TN1.get_num_output_ports(), 3);

    std::cout << TN1.get_unique_id() << std::endl;
    BOOST_CHECK(TN1.get_unique_id() != TN2.get_unique_id());

    auto user_prop_ids = TN1.get_property_ids();
    BOOST_REQUIRE_EQUAL(user_prop_ids.size(), 1);
    BOOST_CHECK_EQUAL(user_prop_ids[0], "double_prop");

    BOOST_REQUIRE_THROW(TN1.get_property<int>("nonexistant_prop"), uhd::lookup_error);
    // If this next test fails, RTTI is not available. There might be cases when
    // that's expected, and when we encounter those we'll reconsider the test.
    BOOST_REQUIRE_THROW(TN1.get_property<int>("double_prop"), uhd::type_error);
    BOOST_REQUIRE_THROW(TN1.get_property<double>("double_prop", 5), uhd::lookup_error);

    BOOST_CHECK_EQUAL(TN1.get_property<double>("double_prop"), 0.0);

    BOOST_REQUIRE_THROW(TN1.set_property<int>("nonexistant_prop", 5), uhd::lookup_error);
    // If this next test fails, RTTI is not available. There might be cases when
    // that's expected, and when we encounter those we'll reconsider the test.
    BOOST_REQUIRE_THROW(TN1.set_property<int>("double_prop", 5), uhd::type_error);

    TN1.set_property<double>("double_prop", 4.2);
    BOOST_CHECK_EQUAL(TN1.get_property<double>("double_prop"), 4.2);
}

