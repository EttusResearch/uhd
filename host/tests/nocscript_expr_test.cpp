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
#include <boost/format.hpp>
#include <algorithm>
#include <iostream>

#include "nocscript_common.hpp"

// We need this global variable for one of the later tests
int and_counter = 0;

BOOST_AUTO_TEST_CASE(test_literals)
{
    expression_literal literal_int("5", expression::TYPE_INT);
    BOOST_CHECK_EQUAL(literal_int.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(literal_int.get_int(), 5);
    BOOST_CHECK_EQUAL(literal_int.to_bool(), true);
    BOOST_REQUIRE_THROW(literal_int.get_string(), uhd::type_error);
    BOOST_REQUIRE_THROW(literal_int.get_bool(), uhd::type_error);

    expression_literal literal_int0("0", expression::TYPE_INT);
    BOOST_CHECK_EQUAL(literal_int0.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(literal_int0.to_bool(), false);

    expression_literal literal_double("2.3", expression::TYPE_DOUBLE);
    BOOST_CHECK_EQUAL(literal_double.infer_type(), expression::TYPE_DOUBLE);
    BOOST_CHECK_CLOSE(literal_double.get_double(), 2.3, 0.01);
    BOOST_CHECK_EQUAL(literal_double.to_bool(), true);
    BOOST_REQUIRE_THROW(literal_double.get_string(), uhd::type_error);
    BOOST_REQUIRE_THROW(literal_double.get_bool(), uhd::type_error);

    expression_literal literal_bool(true);
    BOOST_CHECK_EQUAL(literal_bool.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK_EQUAL(literal_bool.get_bool(), true);
    BOOST_CHECK_EQUAL(literal_bool.to_bool(), true);
    BOOST_CHECK_EQUAL(literal_bool.eval().get_bool(), true);
    BOOST_REQUIRE_THROW(literal_bool.get_string(), uhd::type_error);
    BOOST_REQUIRE_THROW(literal_bool.get_int(), uhd::type_error);

    expression_literal literal_bool_false(false);
    BOOST_CHECK_EQUAL(literal_bool_false.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK_EQUAL(literal_bool_false.get_bool(), false);
    BOOST_CHECK_EQUAL(literal_bool_false.to_bool(), false);
    BOOST_REQUIRE_EQUAL(literal_bool_false.eval().get_bool(), false);
    BOOST_REQUIRE_THROW(literal_bool_false.get_string(), uhd::type_error);
    BOOST_REQUIRE_THROW(literal_bool_false.get_int(), uhd::type_error);

    expression_literal literal_string("'foo bar'", expression::TYPE_STRING);
    BOOST_CHECK_EQUAL(literal_string.infer_type(), expression::TYPE_STRING);
    BOOST_CHECK_EQUAL(literal_string.get_string(), "foo bar");
    BOOST_REQUIRE_THROW(literal_string.get_bool(), uhd::type_error);
    BOOST_REQUIRE_THROW(literal_string.get_int(), uhd::type_error);

    expression_literal literal_int_vec("[1, 2, 3]", expression::TYPE_INT_VECTOR);
    BOOST_CHECK_EQUAL(literal_int_vec.infer_type(), expression::TYPE_INT_VECTOR);
    std::vector<int> test_data{1, 2, 3};
    std::vector<int> result = literal_int_vec.get_int_vector();
    BOOST_CHECK_EQUAL_COLLECTIONS(test_data.begin(), test_data.end(),
                                  result.begin(), result.end());
    BOOST_REQUIRE_THROW(literal_int_vec.get_bool(), uhd::type_error);
    BOOST_REQUIRE_THROW(literal_int_vec.get_int(), uhd::type_error);
}


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
        return expression_literal(5);
    }
    if (var_name == "is_true") {
        std::cout << "Returning value for $is_true..." << std::endl;
        return expression_literal(true);
    }

    throw uhd::syntax_error("Cannot read value (unknown variable)");
}

BOOST_AUTO_TEST_CASE(test_variables)
{
    BOOST_REQUIRE_THROW(
        expression_variable v_fail(
                "foo", // Invalid token
                boost::bind(&variable_get_type, _1), boost::bind(&variable_get_value, _1)
        ),
        uhd::assertion_error
    );

    expression_variable v(
            "$spp", // The token
            boost::bind(&variable_get_type, _1), // type-getter
            boost::bind(&variable_get_value, _1) // value-getter
    );
    BOOST_CHECK_EQUAL(v.infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(v.eval().get_int(), 5);
}

BOOST_AUTO_TEST_CASE(test_container)
{
    // Create some sub-expressions:
    expression_literal::sptr l_true = E(true);
    expression_literal::sptr l_false = E(false);
    expression_literal::sptr l_int = E(5);
    BOOST_REQUIRE_EQUAL(l_false->get_bool(), false);
    BOOST_REQUIRE_EQUAL(l_false->to_bool(), false);
    expression_variable::sptr l_boolvar = boost::make_shared<expression_variable>(
        "$is_true",
        boost::bind(&variable_get_type, _1),
        boost::bind(&variable_get_value, _1)
    );

    // This will throw anytime it's evaluated:
    expression_variable::sptr l_failvar = boost::make_shared<expression_variable>(
        "$does_not_exist",
        boost::bind(&variable_get_type, _1),
        boost::bind(&variable_get_value, _1)
    );

    expression_container c;
    std::cout << "One true, OR: " << std::endl;
    c.add(l_true);
    c.set_combiner_safe(expression_container::COMBINE_OR);
    expression_literal ret_val_1 = c.eval();
    BOOST_CHECK_EQUAL(ret_val_1.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK_EQUAL(ret_val_1.eval().get_bool(), true);

    std::cout << std::endl << std::endl << "Two true, one false, OR: " << std::endl;
    c.add(l_true);
    c.add(l_false);
    expression_literal ret_val_2 = c.eval();
    BOOST_CHECK_EQUAL(ret_val_2.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK_EQUAL(ret_val_2.eval().get_bool(), true);

    expression_container c2;
    c2.add(l_false);
    c2.add(l_false);
    c2.set_combiner(expression_container::COMBINE_AND);
    std::cout << std::endl << std::endl << "Two false, AND: " << std::endl;
    expression_literal ret_val_3 = c2.eval();
    BOOST_CHECK_EQUAL(ret_val_3.infer_type(), expression::TYPE_BOOL);
    BOOST_REQUIRE_EQUAL(ret_val_3.eval().get_bool(), false);

    c2.add(l_failvar);
    // Will not fail, because l_failvar never gets eval'd:
    expression_literal ret_val_4 = c2.eval();
    BOOST_CHECK_EQUAL(ret_val_4.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK_EQUAL(ret_val_4.eval().get_bool(), false);

    // Same here:
    c.add(l_failvar);
    expression_literal ret_val_5 = c.eval();
    BOOST_CHECK_EQUAL(ret_val_5.infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK_EQUAL(ret_val_5.eval().get_bool(), true);

    // Now it'll throw:
    c.set_combiner(expression_container::COMBINE_ALL);
    BOOST_REQUIRE_THROW(c.eval(), uhd::syntax_error);

    std::cout << "Checking type inference on ',' sequences: " << std::endl;
    // Check types match
    BOOST_CHECK_EQUAL(c2.infer_type(), expression::TYPE_BOOL);
    expression_container c3;
    c3.set_combiner(expression_container::COMBINE_ALL);
    c3.add(l_false);
    c3.add(l_int);
    BOOST_CHECK_EQUAL(c3.infer_type(), expression::TYPE_INT);
}


// We'll define two functions here: ADD and XOR. The former shall
// be defined for INT and DOUBLE
class functable_mockup_impl : public function_table
{
  public:
    functable_mockup_impl(void) {};

    bool function_exists(const std::string &name) const {
        return name == "ADD" or name == "XOR" or name == "AND";
    }

    bool function_exists(
            const std::string &name,
            const expression_function::argtype_list_type &arg_types
    ) const {
        if (name == "ADD") {
            if (arg_types.size() == 2
                and arg_types[0] == expression::TYPE_DOUBLE
                and arg_types[1] == expression::TYPE_DOUBLE
            ) {
                return true;
            }
            if (arg_types.size() == 2
                and arg_types[0] == expression::TYPE_INT
                and arg_types[1] == expression::TYPE_INT
            ) {
                return true;
            }
            return false;
        }

        if (name == "XOR" or name == "AND") {
            if (arg_types.size() == 2
                and arg_types[0] == expression::TYPE_BOOL
                and arg_types[1] == expression::TYPE_BOOL
            ) {
                return true;
            }
            return false;
        }

        return false;
    }

    expression::type_t get_type(
            const std::string &name,
            const expression_function::argtype_list_type &arg_types
    ) const {
        if (not function_exists(name, arg_types)) {
            throw uhd::syntax_error(str(
                boost::format("[EXPR_TEXT] get_type(): Unknown function: %s, %d arguments")
                % name % arg_types.size()
            ));
        }

        if (name == "XOR" or name == "AND") {
            return expression::TYPE_BOOL;
        }
        if (name == "ADD") {
            return arg_types[0];
        }
        UHD_THROW_INVALID_CODE_PATH();
    }

    expression_literal eval(
            const std::string &name,
            const expression_function::argtype_list_type &arg_types,
            expression_container::expr_list_type &args
    ) {
        if (name == "XOR") {
            if (arg_types.size() != 2
                or args.size() != 2
                or arg_types[0] != expression::TYPE_BOOL
                or arg_types[1] != expression::TYPE_BOOL
                or args[0]->infer_type() != expression::TYPE_BOOL
                or args[1]->infer_type() != expression::TYPE_BOOL
            ) {
                throw uhd::syntax_error("eval(): XOR type mismatch");
            }
            return expression_literal(bool(
                args[0]->eval().get_bool() xor args[1]->eval().get_bool()
            ));
        }

        if (name == "AND") {
            if (arg_types.size() != 2
                or args.size() != 2
                or arg_types[0] != expression::TYPE_BOOL
                or arg_types[1] != expression::TYPE_BOOL
                or args[0]->infer_type() != expression::TYPE_BOOL
                or args[1]->infer_type() != expression::TYPE_BOOL
            ) {
                throw uhd::syntax_error("eval(): AND type mismatch");
            }
            std::cout << "Calling AND" << std::endl;
            and_counter++;
            return expression_literal(bool(
                args[0]->eval().get_bool() and args[1]->eval().get_bool()
            ));
        }

        if (name == "ADD") {
            if (args.size() != 2) {
                throw uhd::syntax_error("eval(): ADD type mismatch");
            }
            if ((args[0]->infer_type() == expression::TYPE_INT) and
                (args[1]->infer_type() == expression::TYPE_INT)) {
                return expression_literal(int(
                    args[0]->eval().get_int() + args[1]->eval().get_int()
                ));
            }
            else if ((args[0]->infer_type() == expression::TYPE_DOUBLE) and
                (args[1]->infer_type() == expression::TYPE_DOUBLE)) {
                return expression_literal(double(
                    args[0]->eval().get_double() + args[1]->eval().get_double()
                ));
            }
            throw uhd::syntax_error("eval(): ADD type mismatch");
        }
        throw uhd::syntax_error("eval(): unknown function");
    }

    // We don't actually need this
    void register_function(
            const std::string &,
            const function_table::function_ptr &,
            const expression::type_t,
            const expression_function::argtype_list_type &
    ) {};

};


// The annoying part: Testing the test fixtures
BOOST_AUTO_TEST_CASE(test_functable_mockup)
{
    functable_mockup_impl functable;

    BOOST_CHECK(functable.function_exists("ADD"));
    BOOST_CHECK(functable.function_exists("XOR"));
    BOOST_CHECK(not functable.function_exists("FOOBAR"));

    BOOST_CHECK(functable.function_exists("ADD", two_int_args));
    BOOST_CHECK(functable.function_exists("ADD", two_double_args));
    BOOST_CHECK(functable.function_exists("XOR", two_bool_args));
    BOOST_CHECK(not functable.function_exists("ADD", two_bool_args));
    BOOST_CHECK(not functable.function_exists("ADD", no_args));
    BOOST_CHECK(not functable.function_exists("XOR", no_args));

    BOOST_CHECK_EQUAL(functable.get_type("ADD", two_int_args), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(functable.get_type("ADD", two_double_args), expression::TYPE_DOUBLE);
    BOOST_CHECK_EQUAL(functable.get_type("XOR", two_bool_args), expression::TYPE_BOOL);

    expression_container::expr_list_type add_args_int{E(2), E(3)};
    expression_container::expr_list_type add_args_dbl{E(2.25), E(5.0)};
    expression_container::expr_list_type xor_args_bool{E(true), E(false)};

    BOOST_CHECK_EQUAL(functable.eval("ADD", two_int_args, add_args_int), expression_literal(5));
    BOOST_CHECK_EQUAL(functable.eval("ADD", two_double_args, add_args_dbl), expression_literal(7.25));
    BOOST_CHECK_EQUAL(functable.eval("XOR", two_bool_args, xor_args_bool), expression_literal(true));
}

BOOST_AUTO_TEST_CASE(test_function_expression)
{
    function_table::sptr ft = boost::make_shared<functable_mockup_impl>();

    // Very simple function: ADD(2, 3)
    expression_function func1("ADD", ft);
    func1.add(E(2));
    func1.add(E(3));

    BOOST_CHECK_EQUAL(func1.eval(), expression_literal(5));

    // More elaborate: ADD(ADD(2, 3), ADD(ADD(4, 5), 6)) ?= 20
    //                 f4  f1         f3  f2
    expression_function f1("ADD", ft);
    f1.add(E(2));
    f1.add(E(3));
    expression_function f2("ADD", ft);
    f2.add(E(4));
    f2.add(E(5));
    expression_function f3("ADD", ft);
    f3.add(boost::make_shared<expression_function>(f2));
    f3.add(E(6));
    expression_function f4("ADD", ft);
    f4.add(boost::make_shared<expression_function>(f1));
    f4.add(boost::make_shared<expression_function>(f3));

    BOOST_CHECK_EQUAL(f4.eval().get_int(), 20);
}

BOOST_AUTO_TEST_CASE(test_function_expression_laziness)
{
    function_table::sptr ft = boost::make_shared<functable_mockup_impl>();

    // We run AND(AND(false, false), AND(false, false)).
    //        f1  f2                 f3
    // That makes three ANDs
    // in total. However, we will only see AND being evaluated twice, because
    // the outcome is clear after running the first AND in the argument list.
    expression_function::sptr f2 = boost::make_shared<expression_function>("AND", ft);
    f2->add(E(false));
    f2->add(E(false));
    BOOST_CHECK(not f2->eval().get_bool());

    expression_function::sptr f3 = boost::make_shared<expression_function>("AND", ft);
    f3->add(E(false));
    f3->add(E(false));
    BOOST_CHECK(not f3->eval().get_bool());

    and_counter = 0;
    expression_function::sptr f1 = boost::make_shared<expression_function>("AND", ft);
    f1->add(f2);
    f1->add(f3);

    BOOST_CHECK(not f1->eval().get_bool());
    BOOST_CHECK_EQUAL(and_counter, 2);
}

BOOST_AUTO_TEST_CASE(test_sptrs)
{
    expression_container::sptr c = expression_container::make();
    BOOST_CHECK_EQUAL(c->infer_type(), expression::TYPE_BOOL);
    BOOST_CHECK(c->eval().get_bool());

    expression_variable::sptr v = expression_variable::make(
            "$spp",
            boost::bind(&variable_get_type, _1), // type-getter
            boost::bind(&variable_get_value, _1) // value-getter
    );

    c->add(v);
    BOOST_REQUIRE_EQUAL(c->infer_type(), expression::TYPE_INT);
    BOOST_CHECK_EQUAL(c->eval().get_int(), 5);
}

