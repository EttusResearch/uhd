//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../lib/rfnoc/nocscript/expression.hpp"
#include <boost/assign/list_of.hpp>

using namespace uhd::rfnoc::nocscript;

// Some global defs to make tests easier to write
expression_function::argtype_list_type one_int_arg  = boost::assign::list_of(expression::TYPE_INT);
expression_function::argtype_list_type two_int_args = boost::assign::list_of(expression::TYPE_INT)(expression::TYPE_INT);
expression_function::argtype_list_type one_double_arg  = boost::assign::list_of(expression::TYPE_DOUBLE);
expression_function::argtype_list_type two_double_args = boost::assign::list_of(expression::TYPE_DOUBLE)(expression::TYPE_DOUBLE);
expression_function::argtype_list_type one_bool_arg  = boost::assign::list_of(expression::TYPE_BOOL);
expression_function::argtype_list_type two_bool_args = boost::assign::list_of(expression::TYPE_BOOL)(expression::TYPE_BOOL);
expression_function::argtype_list_type no_args;

expression_container::expr_list_type empty_arg_list;

#define E(x) expression_literal::make(x)

