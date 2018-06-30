//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../lib/rfnoc/nocscript/function_table.hpp"
#include "../lib/rfnoc/nocscript/parser.hpp"
#include <uhd/exception.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <algorithm>
#include <iostream>

#include "nocscript_common.hpp"

const int SPP_VALUE = 64;

// Need those for the variable testing:
expression::type_t variable_get_type(const std::string &var_name)
{
    if (var_name == "spp") {
        std::cout << "Returning type for $spp..." << std::endl;
        return expression::TYPE_INT;
    }
    if (var_name == "is_true") {
        std::cout << "Returning type for $is_true..." << std::endl;
        return expression::TYPE_BOOL;
    }

    throw uhd::syntax_error("Cannot infer type (unknown variable)");
}

expression_literal variable_get_value(const std::string &var_name)
{
    if (var_name == "spp") {
        std::cout << "Returning value for $spp..." << std::endl;
        return expression_literal(SPP_VALUE);
    }
    if (var_name == "is_true") {
        std::cout << "Returning value for $is_true..." << std::endl;
        return expression_literal(true);
    }

    throw uhd::syntax_error("Cannot read value (unknown variable)");
}

#define SETUP_FT_AND_PARSER() \
    function_table::sptr ft = function_table::make(); \
    parser::sptr p = parser::make( \
            ft, \
            boost::bind(&variable_get_type, _1), \
            boost::bind(&variable_get_value, _1) \
    );

BOOST_AUTO_TEST_CASE(test_fail)
{
    SETUP_FT_AND_PARSER();

    // Missing closing parens:
    BOOST_REQUIRE_THROW(p->create_expr_tree("ADD1(1, "), uhd::syntax_error);
    // Double comma:
    BOOST_REQUIRE_THROW(p->create_expr_tree("ADD(1,, 2)"), uhd::syntax_error);
    // No comma:
    BOOST_REQUIRE_THROW(p->create_expr_tree("ADD(1 2)"), uhd::syntax_error);
    // Double closing parens:
    BOOST_REQUIRE_THROW(p->create_expr_tree("ADD(1, 2))"), uhd::syntax_error);
    // Unknown function:
    BOOST_REQUIRE_THROW(p->create_expr_tree("GLORPGORP(1, 2)"), uhd::syntax_error);
}

BOOST_AUTO_TEST_CASE(test_adds_no_vars)
{
    SETUP_FT_AND_PARSER();
    BOOST_REQUIRE(ft->function_exists("ADD"));

    const std::string line("ADD(1, ADD(2, ADD(3, 4)))");
    expression::sptr e = p->create_expr_tree(line);
    expression_literal result = e->eval();

    BOOST_REQUIRE_EQUAL(result.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(result.get_int(), 1+2+3+4);
}

BOOST_AUTO_TEST_CASE(test_adds_with_vars)
{
    SETUP_FT_AND_PARSER();

    const std::string line("ADD(1, ADD(2, $spp))");
    expression::sptr e = p->create_expr_tree(line);
    expression_literal result = e->eval();

    BOOST_REQUIRE_EQUAL(result.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(result.get_int(), 1+2+SPP_VALUE);
}

BOOST_AUTO_TEST_CASE(test_fft_check)
{
    SETUP_FT_AND_PARSER();

    const std::string line("GE($spp, 16) AND LE($spp, 4096) AND IS_PWR_OF_2($spp)");
    expression::sptr e = p->create_expr_tree(line);
    expression_literal result = e->eval();

    BOOST_REQUIRE_EQUAL(result.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK(result.get_bool());
}

BOOST_AUTO_TEST_CASE(test_pure_string)
{
    SETUP_FT_AND_PARSER();

    // Eval all, return last expression
    const std::string line("'foo foo', \"bar\"");
    expression_literal result = p->create_expr_tree(line)->eval();

    BOOST_REQUIRE_EQUAL(result.infer_type(), expression::TYPE_STRING);
    BOOST_CHECK_EQUAL(result.get_string(), "bar");
}

int dummy_false_counter = 0;
expression_literal dummy_false(expression_container::expr_list_type)
{
    dummy_false_counter++;
    std::cout << "Running dummy/false statement." << std::endl;
    return expression_literal(false);
}

BOOST_AUTO_TEST_CASE(test_multi_commmand)
{
    SETUP_FT_AND_PARSER();

    ft->register_function(
            "DUMMY",
            boost::bind(&dummy_false, _1),
            expression::TYPE_BOOL,
            no_args
    );

    dummy_false_counter = 0;
    p->create_expr_tree("DUMMY(), DUMMY(), DUMMY()")->eval();
    BOOST_CHECK_EQUAL(dummy_false_counter, 3);

    dummy_false_counter = 0;
    p->create_expr_tree("DUMMY() AND DUMMY() AND DUMMY()")->eval();
    BOOST_CHECK_EQUAL(dummy_false_counter, 1);

    dummy_false_counter = 0;
    p->create_expr_tree("DUMMY() OR DUMMY() OR DUMMY()")->eval();
    BOOST_CHECK_EQUAL(dummy_false_counter, 3);
}

