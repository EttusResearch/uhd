//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "expression.hpp"
#include "function_table.hpp"
#include <boost/shared_ptr.hpp>

#ifndef INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_PARSER_HPP
#define INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_PARSER_HPP

namespace uhd { namespace rfnoc { namespace nocscript {

class parser
{
  public:
    typedef boost::shared_ptr<parser> sptr;

    static sptr make(
        function_table::sptr ftable,
        expression_variable::type_getter_type var_type_getter,
        expression_variable::value_getter_type var_value_getter
    );

    /*! The main parsing call: Turn a string of code into an expression tree.
     *
     * Evaluating the returned object will execute the code.
     *
     * \throws uhd::syntax_error if \p code contains syntax errors
     */
    virtual expression::sptr create_expr_tree(const std::string &code) = 0;
};

}}} /* namespace uhd::rfnoc::nocscript */

#endif /* INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_PARSER_HPP */
// vim: sw=4 et:
