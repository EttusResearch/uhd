//
// Copyright 2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

