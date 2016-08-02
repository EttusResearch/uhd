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
