//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "block_iface.hpp"
#include "function_table.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

#define UHD_NOCSCRIPT_LOG() UHD_LOGGER_TRACE("RFNOC")

using namespace uhd::rfnoc;
using namespace uhd::rfnoc::nocscript;

block_iface::block_iface(block_ctrl_base *block_ptr)
    : _block_ptr(block_ptr)
{
    function_table::sptr ft = function_table::make();

    // Add the SR_WRITE() function
    expression_function::argtype_list_type sr_write_args = boost::assign::list_of
        (expression::TYPE_STRING)
        (expression::TYPE_INT)
    ;
    ft->register_function(
        "SR_WRITE",
        boost::bind(&block_iface::_nocscript__sr_write, this, _1),
        expression::TYPE_BOOL,
        sr_write_args
    );

    // Add read access to arguments ($foo)
    expression_function::argtype_list_type arg_set_args_wo_port = boost::assign::list_of
        (expression::TYPE_STRING)
        (expression::TYPE_INT)
    ;
    expression_function::argtype_list_type arg_set_args_w_port = boost::assign::list_of
        (expression::TYPE_STRING)
        (expression::TYPE_INT)
        (expression::TYPE_INT)
    ;
#define REGISTER_ARG_SETTER(noctype, setter_func) \
    arg_set_args_wo_port[1] = expression::noctype; \
    arg_set_args_w_port[1] = expression::noctype; \
    ft->register_function( \
        "SET_ARG", \
        boost::bind(&block_iface::setter_func, this, _1), \
        expression::TYPE_BOOL, \
        arg_set_args_wo_port \
    ); \
    ft->register_function( \
        "SET_ARG", \
        boost::bind(&block_iface::setter_func, this, _1), \
        expression::TYPE_BOOL, \
        arg_set_args_w_port \
    );
    REGISTER_ARG_SETTER(TYPE_INT,        _nocscript__arg_set_int);
    REGISTER_ARG_SETTER(TYPE_STRING,     _nocscript__arg_set_string);
    REGISTER_ARG_SETTER(TYPE_DOUBLE,     _nocscript__arg_set_double);
    REGISTER_ARG_SETTER(TYPE_INT_VECTOR, _nocscript__arg_set_intvec);


    // Add read/write access to local variables
    expression_function::argtype_list_type set_var_args = boost::assign::list_of
        (expression::TYPE_STRING)
        (expression::TYPE_INT)
    ;
    const expression_function::argtype_list_type get_var_args = boost::assign::list_of
        (expression::TYPE_STRING)
    ;
#define REGISTER_VAR_ACCESS(noctype, typestr) \
    set_var_args[1] = expression::noctype; \
    ft->register_function( \
        "SET_VAR", \
        boost::bind(&block_iface::_nocscript__var_set, this, _1), \
        expression::TYPE_BOOL, \
        set_var_args \
    ); \
    ft->register_function( \
        "GET_"#typestr, \
        boost::bind(&block_iface::_nocscript__var_get, this, _1), \
        expression::noctype, \
        get_var_args \
    );
    REGISTER_VAR_ACCESS(TYPE_INT, INT);
    REGISTER_VAR_ACCESS(TYPE_STRING, STRING);
    REGISTER_VAR_ACCESS(TYPE_DOUBLE, DOUBLE);
    REGISTER_VAR_ACCESS(TYPE_INT_VECTOR, INT_VECTOR);

    // Create the parser
    _parser = parser::make(
        ft,
        boost::bind(&block_iface::_nocscript__arg_get_type, this, _1),
        boost::bind(&block_iface::_nocscript__arg_get_val,  this, _1)
    );
}


void block_iface::run_and_check(const std::string &code, const std::string &error_message)
{
    boost::mutex::scoped_lock local_interpreter_lock(_lil_mutex);

    UHD_NOCSCRIPT_LOG() << "[NocScript] Executing and asserting code: " << code ;
    expression::sptr e = _parser->create_expr_tree(code);
    expression_literal result = e->eval();
    if (not result.to_bool()) {
        if (error_message.empty()) {
            throw uhd::runtime_error(str(
                boost::format("[NocScript] Code returned false: %s")
                % code
            ));
        } else {
            throw uhd::runtime_error(str(
                boost::format("[NocScript] Error: %s")
                % error_message
            ));
        }
    }

    _vars.clear(); // We go out of scope, and so do NocScript variables
}


expression_literal block_iface::_nocscript__sr_write(expression_container::expr_list_type args)
{
    const std::string reg_name = args[0]->eval().get_string();
    const uint32_t reg_val = uint32_t(args[1]->eval().get_int());
    bool result = true;
    try {
        UHD_NOCSCRIPT_LOG() << "[NocScript] Executing SR_WRITE() " ;
        _block_ptr->sr_write(reg_name, reg_val);
    } catch (const uhd::exception &e) {
        UHD_LOGGER_ERROR("RFNOC") << boost::format("[NocScript] Error while executing SR_WRITE(%s, 0x%X):\n%s")
                          % reg_name % reg_val % e.what()
                       ;
        result = false;
    }

    return expression_literal(result);
}

expression::type_t block_iface::_nocscript__arg_get_type(const std::string &varname)
{
    const std::string var_type = _block_ptr->get_arg_type(varname);
    if (var_type == "int") {
        return expression::TYPE_INT;
    } else if (var_type == "string") {
        return expression::TYPE_STRING;
    } else if (var_type == "double") {
        return expression::TYPE_DOUBLE;
    } else if (var_type == "int_vector") {
        UHD_THROW_INVALID_CODE_PATH(); // TODO
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}

expression_literal block_iface::_nocscript__arg_get_val(const std::string &varname)
{
    const std::string var_type = _block_ptr->get_arg_type(varname);
    if (var_type == "int") {
        return expression_literal(_block_ptr->get_arg<int>(varname));
    } else if (var_type == "string") {
        return expression_literal(_block_ptr->get_arg<std::string>(varname));
    } else if (var_type == "double") {
        return expression_literal(_block_ptr->get_arg<double>(varname));
    } else if (var_type == "int_vector") {
        UHD_THROW_INVALID_CODE_PATH(); // TODO
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}

expression_literal block_iface::_nocscript__arg_set_int(const expression_container::expr_list_type &args)
{
    const std::string var_name = args[0]->eval().get_string();
    const int val              = args[1]->eval().get_int();
    size_t port = 0;
    if (args.size() == 3) {
        port = size_t(args[2]->eval().get_int());
    }
    UHD_NOCSCRIPT_LOG() << "[NocScript] Setting $" << var_name ;
    _block_ptr->set_arg<int>(var_name, val, port);
    return expression_literal(true);
}

expression_literal block_iface::_nocscript__arg_set_string(const expression_container::expr_list_type &args)
{
    const std::string var_name = args[0]->eval().get_string();
    const std::string val      = args[1]->eval().get_string();
    size_t port = 0;
    if (args.size() == 3) {
        port = size_t(args[2]->eval().get_int());
    }
    UHD_NOCSCRIPT_LOG() << "[NocScript] Setting $" << var_name ;
    _block_ptr->set_arg<std::string>(var_name, val, port);
    return expression_literal(true);
}

expression_literal block_iface::_nocscript__arg_set_double(const expression_container::expr_list_type &args)
{
    const std::string var_name = args[0]->eval().get_string();
    const double val              = args[1]->eval().get_double();
    size_t port = 0;
    if (args.size() == 3) {
        port = size_t(args[2]->eval().get_int());
    }
    UHD_NOCSCRIPT_LOG() << "[NocScript] Setting $" << var_name ;
    _block_ptr->set_arg<double>(var_name, val, port);
    return expression_literal(true);
}

expression_literal block_iface::_nocscript__arg_set_intvec(const expression_container::expr_list_type &)
{
    UHD_THROW_INVALID_CODE_PATH();
}

block_iface::sptr block_iface::make(uhd::rfnoc::block_ctrl_base* block_ptr)
{
    return sptr(new block_iface(block_ptr));
}

expression_literal block_iface::_nocscript__var_get(const expression_container::expr_list_type &args)
{
    expression_literal expr = _vars[args[0]->eval().get_string()];
    //std::cout << "[NocScript] Getting var " << args[0]->eval().get_string() << " == " << expr ;
    //std::cout << "[NocScript] Type " << expr.infer_type() ;
    //return _vars[args[0]->eval().get_string()];
    return expr;
}

expression_literal block_iface::_nocscript__var_set(const expression_container::expr_list_type &args)
{
    _vars[args[0]->eval().get_string()] = args[1]->eval();
    //std::cout << "[NocScript] Set var " << args[0]->eval().get_string() << " to " << _vars[args[0]->eval().get_string()] ;
    //std::cout << "[NocScript] Type " << _vars[args[0]->eval().get_string()].infer_type() ;
    return expression_literal(true);
}

