//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "expression.hpp"
#include "function_table.hpp"
#include <uhd/utils/cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

using namespace uhd::rfnoc::nocscript;

std::map<expression::type_t, std::string> expression::type_repr{
    {TYPE_INT, "INT"},
    {TYPE_DOUBLE, "DOUBLE"},
    {TYPE_STRING, "STRING"},
    {TYPE_BOOL, "BOOL"},
    {TYPE_INT_VECTOR, "INT_VECTOR"}
};

/********************************************************************
 * Literal expressions (constants)
 *******************************************************************/
expression_literal::expression_literal(
        const std::string token_val,
        expression::type_t type
) : _bool_val(false)
  , _int_val(0)
  , _double_val(0.0)
  , _val(token_val)
  , _type(type)
{
    switch (_type) {
    case expression::TYPE_STRING:
        // Remove the leading and trailing quotes:
        _val = _val.substr(1, _val.size()-2);
        break;

    case expression::TYPE_INT:
        if (_val.substr(0, 2) == "0x") {
            _int_val = uhd::cast::hexstr_cast<int>(_val);
        } else {
            _int_val = std::stoi(_val);
        }
        break;

    case expression::TYPE_DOUBLE:
        _double_val = std::stod(_val);
        break;

    case expression::TYPE_BOOL:
        if (boost::to_upper_copy(_val) == "TRUE") {
            _bool_val = true;
        } else {
            // lexical cast to bool is too picky
            _bool_val = bool(std::stoi(_val));
        }
        break;

    case expression::TYPE_INT_VECTOR:
        {
        std::string str_vec = _val.substr(1, _val.size()-2);
        std::vector<std::string> subtoken_list;
        boost::split(subtoken_list, str_vec, boost::is_any_of(", "), boost::token_compress_on);
        for(const std::string &t:  subtoken_list) {
            _int_vector_val.push_back(std::stoi(t));
        }
        break;
        }

    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

expression_literal::expression_literal(bool b)
  : _bool_val(b)
  , _int_val(0)
  , _double_val(0.0)
  , _val("")
  , _type(expression::TYPE_BOOL)
{
    // nop
}

expression_literal::expression_literal(int i)
  : _bool_val(false)
  , _int_val(i)
  , _double_val(0.0)
  , _val("")
  , _type(expression::TYPE_INT)
{
    // nop
}

expression_literal::expression_literal(double d)
  : _bool_val(false)
  , _int_val(0)
  , _double_val(d)
  , _val("")
  , _type(expression::TYPE_DOUBLE)
{
    // nop
}

expression_literal::expression_literal(const std::string &s)
  : _bool_val(false)
  , _int_val(0)
  , _double_val(0.0)
  , _val(s)
  , _type(expression::TYPE_STRING)
{
    // nop
}

expression_literal::expression_literal(const std::vector<int> v)
  : _bool_val(false)
  , _int_val(0)
  , _double_val(0.0)
  , _int_vector_val(v)
  , _val("")
  , _type(expression::TYPE_INT_VECTOR)
{
    // nop
}

bool expression_literal::to_bool() const
{
    switch (_type) {
        case TYPE_INT:
            return bool(std::stoi(_val));
        case TYPE_STRING:
            return not _val.empty();
        case TYPE_DOUBLE:
            return bool(std::stod(_val));
        case TYPE_BOOL:
            return _bool_val;
        case TYPE_INT_VECTOR:
            return not _int_vector_val.empty();
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

int expression_literal::get_int() const
{
    if (_type != TYPE_INT) {
        throw uhd::type_error("Cannot call get_int() on non-int value.");
    }

    return _int_val;
}

double expression_literal::get_double() const
{
    if (_type != TYPE_DOUBLE) {
        throw uhd::type_error("Cannot call get_double() on non-double value.");
    }

    return _double_val;
}

std::string expression_literal::get_string() const
{
    if (_type != TYPE_STRING) {
        throw uhd::type_error("Cannot call get_string() on non-string value.");
    }

    return _val;
}

bool expression_literal::get_bool() const
{
    if (_type != TYPE_BOOL) {
        throw uhd::type_error("Cannot call get_bool() on non-boolean value.");
    }

    return _bool_val;
}

std::vector<int> expression_literal::get_int_vector() const
{
    if (_type != TYPE_INT_VECTOR) {
        throw uhd::type_error("Cannot call get_bool() on non-boolean value.");
    }

    return _int_vector_val;
}

std::string expression_literal::repr() const
{
    switch (_type) {
        case TYPE_INT:
            return std::to_string(_int_val);
        case TYPE_STRING:
            return _val;
        case TYPE_DOUBLE:
            return std::to_string(_double_val);
        case TYPE_BOOL:
            return _bool_val ? "TRUE" : "FALSE";
        case TYPE_INT_VECTOR:
            {
            std::stringstream sstr;
            sstr << "[";
            for (size_t i = 0; i < _int_vector_val.size(); i++) {
                if (i > 0) {
                    sstr << ", ";
                }
                sstr << _int_vector_val[i];
            }
            sstr << "]";
            return sstr.str();
            }
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

bool expression_literal::operator==(const expression_literal &rhs) const
{
    if (rhs.infer_type() != _type) {
        return false;
    }

    switch (_type) {
        case TYPE_INT:
            return get_int() == rhs.get_int();
        case TYPE_STRING:
            return get_string() == rhs.get_string();
        case TYPE_DOUBLE:
            return get_double() == rhs.get_double();
        case TYPE_BOOL:
            return get_bool() == rhs.get_bool();
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

/********************************************************************
 * Containers
 *******************************************************************/
expression_container::sptr expression_container::make()
{
    return sptr(new expression_container);
}

expression::type_t expression_container::infer_type() const
{
    if (_combiner == COMBINE_OR or _combiner == COMBINE_AND) {
        return TYPE_BOOL;
    }

    if (_sub_exprs.empty()) {
        return TYPE_BOOL;
    }

    return _sub_exprs.back()->infer_type();
}

void expression_container::add(expression::sptr new_expr)
{
    _sub_exprs.push_back(new_expr);
}

bool expression_container::empty() const
{
    return _sub_exprs.empty();
}

void expression_container::set_combiner_safe(const combiner_type c)
{
    if (_combiner == COMBINE_NOTSET) {
        _combiner = c;
        return;
    }

    throw uhd::syntax_error("Attempting to override combiner type");
}

expression_literal expression_container::eval()
{
    if (_sub_exprs.empty()) {
        return expression_literal(true);
    }

    expression_literal ret_val;
    for(const expression::sptr &sub_expr:  _sub_exprs) {
        ret_val = sub_expr->eval();
        if (_combiner == COMBINE_AND and ret_val.to_bool() == false) {
            return ret_val;
        }
        if (_combiner == COMBINE_OR and ret_val.to_bool() == true) {
            return ret_val;
        }
        // For ALL, we return the last one, so just overwrite it
    }
    return ret_val;
}

/********************************************************************
 * Functions
 *******************************************************************/
std::string expression_function::to_string(const std::string &name, const argtype_list_type &types)
{
    std::string s = name;
    int arg_count = 0;
    for(const expression::type_t type:  types) {
        if (arg_count == 0) {
            s += "(";
        } else {
            s += ", ";
        }
        s += type_repr[type];
        arg_count++;
    }
    s += ")";

    return s;
}

expression_function::expression_function(
    const std::string &name,
    const function_table::sptr func_table
) : _name(name)
  , _func_table(func_table)
{
    _combiner = COMBINE_ALL;
    if (not _func_table->function_exists(_name)) {
        throw uhd::syntax_error(str(
                boost::format("Unknown function: %s")
                % _name
        ));
    }
}

void expression_function::add(expression::sptr new_expr)
{
    expression_container::add(new_expr);
    _arg_types.push_back(new_expr->infer_type());
}

expression::type_t expression_function::infer_type() const
{
    return _func_table->get_type(_name, _arg_types);
}

expression_literal expression_function::eval()
{
    return _func_table->eval(_name, _arg_types, _sub_exprs);
}


std::string expression_function::repr() const
{
    return to_string(_name, _arg_types);
}

expression_function::sptr expression_function::make(
    const std::string &name,
    const function_table::sptr func_table
) {
    return sptr(new expression_function(name, func_table));
}

/********************************************************************
 * Variables
 *******************************************************************/
expression_variable::expression_variable(
    const std::string &token_val,
    type_getter_type type_getter,
    value_getter_type value_getter
) : _type_getter(type_getter)
  , _value_getter(value_getter)
{
    // We can assume this is true because otherwise, it's not a valid token:
    UHD_ASSERT_THROW(not token_val.empty() and token_val[0] == '$');

    _varname = token_val.substr(1);
}

expression::type_t expression_variable::infer_type() const
{
    return _type_getter(_varname);
}

expression_literal expression_variable::eval()
{
    return _value_getter(_varname);
}

expression_variable::sptr expression_variable::make(
        const std::string &token_val,
        type_getter_type type_getter,
        value_getter_type value_getter
) {
    return sptr(new expression_variable(token_val, type_getter, value_getter));
}

