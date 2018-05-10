//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../lib/rfnoc/nocscript/function_table.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <algorithm>
#include <iostream>

#include "nocscript_common.hpp"

BOOST_AUTO_TEST_CASE(test_basic_funcs)
{
    function_table::sptr ft = function_table::make();
    BOOST_CHECK(ft->function_exists("ADD"));
    BOOST_CHECK(ft->function_exists("ADD", two_int_args));
    BOOST_CHECK(ft->function_exists("LE", two_int_args));
    BOOST_CHECK(ft->function_exists("GE"));
    BOOST_CHECK(ft->function_exists("GE", two_int_args));
    BOOST_CHECK(ft->function_exists("TRUE"));
    BOOST_CHECK(ft->function_exists("FALSE"));

    // Math
    expression_container::expr_list_type two_int_values{E(2), E(3)};
    expression_container::expr_list_type two_int_values2{E(3), E(2)};
    expression_container::expr_list_type two_double_values{E(2.0), E(3.0)};

    BOOST_REQUIRE_EQUAL(ft->get_type("ADD", two_int_args), expression::TYPE_INT);
    expression_literal e_add = ft->eval("ADD", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(e_add.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e_add.get_int(), 5);

    BOOST_REQUIRE_EQUAL(ft->get_type("MULT", two_int_args), expression::TYPE_INT);
    expression_literal e_mult_i = ft->eval("MULT", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(e_mult_i.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e_mult_i.get_int(), 6);

    BOOST_REQUIRE_EQUAL(ft->get_type("MULT", two_double_args), expression::TYPE_DOUBLE);
    expression_literal e_mult_d = ft->eval("MULT", two_double_args, two_double_values);
    BOOST_REQUIRE_EQUAL(e_mult_d.infer_type(), expression::TYPE_DOUBLE);
    BOOST_CHECK_CLOSE(e_mult_d.get_double(), 6.0, 0.01);

    BOOST_REQUIRE_EQUAL(ft->get_type("DIV", two_double_args), expression::TYPE_DOUBLE);
    expression_literal e_div_d = ft->eval("DIV", two_double_args, two_double_values);
    BOOST_REQUIRE_EQUAL(e_div_d.infer_type(), expression::TYPE_DOUBLE);
    BOOST_CHECK_CLOSE(e_div_d.get_double(), 2.0/3.0, 0.01);

    BOOST_REQUIRE_EQUAL(ft->get_type("MODULO", two_int_args), expression::TYPE_INT);
    expression_literal e_modulo_i = ft->eval("MODULO", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(e_modulo_i.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e_modulo_i.get_int(), 2 % 3);

    BOOST_REQUIRE_EQUAL(ft->get_type("LE", two_int_args), expression::TYPE_BOOL);
    expression_literal e_le = ft->eval("LE", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(e_le.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK_EQUAL(e_le.get_bool(), true);
    BOOST_CHECK_EQUAL(ft->eval("LE", two_int_args, two_int_values2).get_bool(), false);
    expression_literal e_ge = ft->eval("GE", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(ft->get_type("GE", two_int_args), expression::TYPE_BOOL);
    BOOST_REQUIRE_EQUAL(e_ge.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK_EQUAL(e_ge.get_bool(), false);

    expression_container::expr_list_type sixty_four{E(64)};
    expression_literal e_pwr2 = ft->eval("IS_PWR_OF_2", one_int_arg, sixty_four);
    BOOST_REQUIRE_EQUAL(e_pwr2.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK_EQUAL(e_pwr2.get_bool(), true);
    expression_literal e_log2 = ft->eval("LOG2", one_int_arg, sixty_four);
    BOOST_REQUIRE_EQUAL(e_log2.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e_log2.get_int(), 6);

    // Boolean Logic
    expression_container::expr_list_type e_true{E(true)};
    expression_container::expr_list_type e_false{E(false)};
    BOOST_CHECK(ft->eval("TRUE", no_args, empty_arg_list).to_bool());
    BOOST_CHECK(ft->eval("TRUE", no_args, empty_arg_list).get_bool());
    BOOST_CHECK_EQUAL(ft->eval("FALSE", no_args, empty_arg_list).to_bool(), false);
    BOOST_CHECK_EQUAL(ft->eval("FALSE", no_args, empty_arg_list).get_bool(), false);
    BOOST_CHECK(ft->eval("NOT", one_bool_arg, e_false).to_bool());

    // Control
    std::cout << "Checking ~1s sleep until... ";
    expression_container::expr_list_type e_sleeptime{E(.999)};
    BOOST_CHECK(ft->eval("SLEEP", one_double_arg, e_sleeptime).get_bool());
    std::cout << "Now." << std::endl;
}

// Some bogus function to test the registry
expression_literal add_plus2_int(expression_container::expr_list_type args)
{
    return expression_literal(args[0]->eval().get_int() + args[1]->eval().get_int() + 2);
}

BOOST_AUTO_TEST_CASE(test_add_funcs)
{
    function_table::sptr ft = function_table::make();

    BOOST_CHECK(not ft->function_exists("ADD_PLUS_2"));

    expression_function::argtype_list_type add_int_args{
        expression::TYPE_INT,
        expression::TYPE_INT
    };
    ft->register_function(
            "ADD_PLUS_2",
            boost::bind(&add_plus2_int, _1),
            expression::TYPE_INT,
            add_int_args
    );

    BOOST_CHECK(ft->function_exists("ADD_PLUS_2"));
    BOOST_CHECK(ft->function_exists("ADD_PLUS_2", add_int_args));

    expression_container::expr_list_type add_int_values{E(2), E(3)};
    expression_literal e = ft->eval("ADD_PLUS_2", two_int_args, add_int_values);
    BOOST_REQUIRE_EQUAL(e.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e.get_int(), 7);
}

int dummy_true_counter = 0;
// Some bogus function to test the registry
expression_literal dummy_true(expression_container::expr_list_type)
{
    dummy_true_counter++;
    std::cout << "Running dummy/true statement." << std::endl;
    return expression_literal(true);
}

int dummy_false_counter = 0;
// Some bogus function to test the registry
expression_literal dummy_false(expression_container::expr_list_type)
{
    dummy_false_counter++;
    std::cout << "Running dummy/false statement." << std::endl;
    return expression_literal(false);
}

BOOST_AUTO_TEST_CASE(test_conditionals)
{
    function_table::sptr ft = function_table::make();
    ft->register_function(
            "DUMMY",
            boost::bind(&dummy_true, _1),
            expression::TYPE_BOOL,
            no_args
    );
    ft->register_function(
            "DUMMY_F",
            boost::bind(&dummy_false, _1),
            expression::TYPE_BOOL,
            no_args
    );
    BOOST_REQUIRE(ft->function_exists("DUMMY", no_args));
    BOOST_REQUIRE(ft->function_exists("DUMMY_F", no_args));

    expression_function::sptr dummy_statement = boost::make_shared<expression_function>("DUMMY", ft);
    expression_function::sptr if_statement = boost::make_shared<expression_function>("IF", ft);
    if_statement->add(E(true));
    if_statement->add(dummy_statement);

    std::cout << "Dummy statement should run once until END:" << std::endl;
    dummy_true_counter = 0;
    BOOST_CHECK(if_statement->eval().get_bool());
    BOOST_CHECK_EQUAL(dummy_true_counter, 1);
    std::cout << "END." << std::endl;

    std::cout << "Dummy statement should not run until END:" << std::endl;
    expression_function::sptr if_statement2 = boost::make_shared<expression_function>("IF", ft);
    if_statement2->add(E(false));
    if_statement2->add(dummy_statement);
    dummy_true_counter = 0;
    BOOST_CHECK(not if_statement2->eval().get_bool());
    BOOST_CHECK_EQUAL(dummy_true_counter, 0);
    std::cout << "END." << std::endl;

    expression_function::sptr if_else_statement = boost::make_shared<expression_function>("IF_ELSE", ft);
    expression_function::sptr dummy_statement_f = boost::make_shared<expression_function>("DUMMY_F", ft);
    if_else_statement->add(E(true));
    if_else_statement->add(dummy_statement);
    if_else_statement->add(dummy_statement_f);
    dummy_true_counter = 0;
    dummy_false_counter = 0;
    std::cout << "Should execute dummy/true statement before END:" << std::endl;
    BOOST_CHECK(if_else_statement->eval().get_bool());
    BOOST_CHECK_EQUAL(dummy_true_counter, 1);
    BOOST_CHECK_EQUAL(dummy_false_counter, 0);
    std::cout << "END." << std::endl;

    expression_function::sptr if_else_statement2 = boost::make_shared<expression_function>("IF_ELSE", ft);
    if_else_statement2->add(E(false));
    if_else_statement2->add(dummy_statement);
    if_else_statement2->add(dummy_statement_f);
    dummy_true_counter = 0;
    dummy_false_counter = 0;
    std::cout << "Should execute dummy/false statement before END:" << std::endl;
    BOOST_CHECK(not if_else_statement2->eval().get_bool());
    BOOST_CHECK_EQUAL(dummy_true_counter, 0);
    BOOST_CHECK_EQUAL(dummy_false_counter, 1);
    std::cout << "END." << std::endl;
}

BOOST_AUTO_TEST_CASE(test_bitwise_funcs)
{
    function_table::sptr ft = function_table::make();
    BOOST_CHECK(ft->function_exists("SHIFT_RIGHT"));
    BOOST_CHECK(ft->function_exists("SHIFT_RIGHT", two_int_args));
    BOOST_CHECK(ft->function_exists("SHIFT_LEFT"));
    BOOST_CHECK(ft->function_exists("SHIFT_LEFT", two_int_args));
    BOOST_CHECK(ft->function_exists("BITWISE_AND"));
    BOOST_CHECK(ft->function_exists("BITWISE_AND", two_int_args));
    BOOST_CHECK(ft->function_exists("BITWISE_OR"));
    BOOST_CHECK(ft->function_exists("BITWISE_OR", two_int_args));
    BOOST_CHECK(ft->function_exists("BITWISE_XOR"));
    BOOST_CHECK(ft->function_exists("BITWISE_XOR", two_int_args));

    // Bitwise Math
    int int_value1 = 0x2;
    int int_value2 = 0x3;
    expression_container::expr_list_type two_int_values{
        E(int_value1),
        E(int_value2)
    };

    BOOST_REQUIRE_EQUAL(ft->get_type("SHIFT_RIGHT", two_int_args), expression::TYPE_INT);
    expression_literal e_shift_right = ft->eval("SHIFT_RIGHT", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(e_shift_right.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e_shift_right.get_int(), int_value1 >> int_value2);

    BOOST_REQUIRE_EQUAL(ft->get_type("SHIFT_LEFT", two_int_args), expression::TYPE_INT);
    expression_literal e_shift_left = ft->eval("SHIFT_LEFT", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(e_shift_left.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e_shift_left.get_int(), int_value1 << int_value2);

    BOOST_REQUIRE_EQUAL(ft->get_type("BITWISE_AND", two_int_args), expression::TYPE_INT);
    expression_literal e_bitwise_and = ft->eval("BITWISE_AND", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(e_bitwise_and.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e_bitwise_and.get_int(), int_value1 & int_value2);

    BOOST_REQUIRE_EQUAL(ft->get_type("BITWISE_OR", two_int_args), expression::TYPE_INT);
    expression_literal e_bitwise_or = ft->eval("BITWISE_OR", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(e_bitwise_or.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e_bitwise_or.get_int(), int_value1 | int_value2);

    BOOST_REQUIRE_EQUAL(ft->get_type("BITWISE_XOR", two_int_args), expression::TYPE_INT);
    expression_literal e_bitwise_xor = ft->eval("BITWISE_XOR", two_int_args, two_int_values);
    BOOST_REQUIRE_EQUAL(e_bitwise_xor.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(e_bitwise_xor.get_int(), int_value1 ^ int_value2);
}
