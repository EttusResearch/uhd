//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "expression.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

#ifndef INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_FUNCTABLE_HPP
#define INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_FUNCTABLE_HPP

namespace uhd { namespace rfnoc { namespace nocscript {

class function_table
{
  public:
    typedef boost::shared_ptr<function_table> sptr;
    typedef boost::function<expression_literal(expression_container::expr_list_type&)> function_ptr;

    static sptr make();
    virtual ~function_table() {};

    /*! Check if any function with a given name exists
     *
     * \returns True, if any function with name \p name is registered.
     */
    virtual bool function_exists(const std::string &name) const = 0;

    /*! Check if a function with a given name and list of argument types exists
     *
     * \returns True, if such a function is registered.
     */
    virtual bool function_exists(
            const std::string &name,
            const expression_function::argtype_list_type &arg_types
    ) const = 0;

    /*! Get the return type of a function with given name and argument type list
     *
     * \returns The function's return type
     * \throws uhd::syntax_error if no such function is registered
     */
    virtual expression::type_t get_type(
            const std::string &name,
            const expression_function::argtype_list_type &arg_types
    ) const = 0;

    /*! Calls the function \p name with the argument list \p arguments
     *
     * \param arg_types A list of types for each argument
     * \param arguments An expression list of the arguments
     * \returns The return value of the called function
     * \throws uhd::syntax_error if no such function is found
     */
    virtual expression_literal eval(
            const std::string &name,
            const expression_function::argtype_list_type &arg_types,
            expression_container::expr_list_type &arguments
    ) = 0;

    /*! Register a new function
     *
     * \param name Name of the function (e.g. 'ADD')
     * \param ptr Function object
     * \param return_type The function's return value
     * \param sig The function signature (list of argument types)
     */
    virtual void register_function(
            const std::string &name,
            const function_ptr &ptr,
            const expression::type_t return_type,
            const expression_function::argtype_list_type &sig
    ) = 0;
};

}}} /* namespace uhd::rfnoc::nocscript */

#endif /* INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_FUNCTABLE_HPP */
// vim: sw=4 et:
