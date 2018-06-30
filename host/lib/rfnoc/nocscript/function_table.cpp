//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "function_table.hpp"
#include "basic_functions.hpp"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <map>

using namespace uhd::rfnoc::nocscript;

class function_table_impl : public function_table
{
  public:
    struct function_info {
        expression::type_t return_type;
        function_ptr function;

        function_info(): return_type(expression::TYPE_INT)  {};
        function_info(const expression::type_t return_type_, const function_ptr &function_)
            : return_type(return_type_), function(function_)
        {};
    };
    // Should be an unordered_map... sigh, we'll get to C++11 someday.
    typedef std::map<std::string, std::map<expression_function::argtype_list_type, function_info> > table_type;

    /************************************************************************
     * Structors
     ***********************************************************************/
    function_table_impl()
    {
        _REGISTER_ALL_FUNCS();
    }

    ~function_table_impl() {};


    /************************************************************************
     * Interface implementation
     ***********************************************************************/
    bool function_exists(const std::string &name) const {
        return bool(_table.count(name));
    }

    bool function_exists(
            const std::string &name,
            const expression_function::argtype_list_type &arg_types
    ) const {
        table_type::const_iterator it = _table.find(name);
        return (it != _table.end()) and bool(it->second.count(arg_types));
    }

    expression::type_t get_type(
            const std::string &name,
            const expression_function::argtype_list_type &arg_types
    ) const {
        table_type::const_iterator it = _table.find(name);
        if (it == _table.end() or (it->second.find(arg_types) == it->second.end())) {
            throw uhd::syntax_error(str(
                    boost::format("Unable to retrieve return value for function %s")
                    % expression_function::to_string(name, arg_types)
            ));
        }
        return it->second.find(arg_types)->second.return_type;
    }

    expression_literal eval(
            const std::string &name,
            const expression_function::argtype_list_type &arg_types,
            expression_container::expr_list_type &arguments
    ) {
        if (not function_exists(name, arg_types)) {
            throw uhd::syntax_error(str(
                        boost::format("Cannot eval() function %s, not a known signature")
                        % expression_function::to_string(name, arg_types)
            ));
        }

        return _table[name][arg_types].function(arguments);
    }

    void register_function(
            const std::string &name,
            const function_table::function_ptr &ptr,
            const expression::type_t return_type,
            const expression_function::argtype_list_type &sig
    ) {
        _table[name][sig] = function_info(return_type, ptr);
    }

  private:
    table_type _table;
};

function_table::sptr function_table::make()
{
    return sptr(new function_table_impl());
}
