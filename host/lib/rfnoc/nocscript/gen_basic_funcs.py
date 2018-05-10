#!/usr/bin/env python
"""
Generate the function list for the basic NocScript functions
"""

import re
import os
import sys
from mako.template import Template

#############################################################################
# This is the interesting part: Add new functions in here
#
# Notes:
# - Lines starting with # are considered comments, and will be removed from
#   the output
# - C++ comments will be copied onto the generated file if inside functions
# - Docstrings start with //! and are required
# - Function signature is RETURN_TYPE NAME(ARG_TYPE1, ARG_TYPE2, ...)
# - Function body is valid C++
# - If your function requires special includes, put them in INCLUDE_LIST
# - End of functions is delimited by s/^}/, so take care with the indents!
# - Use these substitutions:
#   - ${RETURN}(...): Create a valid return value
#   - ${args[n]}: Access the n-th argument
#
INCLUDE_LIST = """
#include <boost/math/special_functions/round.hpp>
#include <chrono>
#include <thread>
"""
FUNCTION_LIST = """
CATEGORY: Math Functions
//! Returns x + y
INT ADD(INT, INT)
{
    ${RETURN}(${args[0]} + ${args[1]});
}

//! Returns x + y
DOUBLE ADD(DOUBLE, DOUBLE)
{
    ${RETURN}(${args[0]} + ${args[1]});
}

//! Returns x * y
DOUBLE MULT(DOUBLE, DOUBLE)
{
    ${RETURN}(${args[0]} * ${args[1]});
}

//! Returns x * y
INT MULT(INT, INT)
{
    ${RETURN}(${args[0]} * ${args[1]});
}

//! Returns x / y
DOUBLE DIV(DOUBLE, DOUBLE)
{
    ${RETURN}(${args[0]} / ${args[1]});
}

//! Returns true if x <= y (Less or Equal)
BOOL LE(INT, INT)
{
    ${RETURN}(bool(${args[0]} <= ${args[1]}));
}

//! Returns true if x <= y (Less or Equal)
BOOL LE(DOUBLE, DOUBLE)
{
    ${RETURN}(bool(${args[0]} <= ${args[1]}));
}

//! Returns true if x >= y (Greater or Equal)
BOOL GE(INT, INT)
{
    ${RETURN}(bool(${args[0]} >= ${args[1]}));
}

//! Returns true if x >= y (Greater or Equal)
BOOL GE(DOUBLE, DOUBLE)
{
    ${RETURN}(bool(${args[0]} >= ${args[1]}));
}

//! Returns true if x < y (Less Than)
BOOL LT(INT, INT)
{
    ${RETURN}(bool(${args[0]} < ${args[1]}));
}

//! Returns true if x > y (Greater Than)
BOOL GT(INT, INT)
{
    ${RETURN}(bool(${args[0]} > ${args[1]}));
}

//! Returns true if x < y (Less Than)
BOOL LT(DOUBLE, DOUBLE)
{
    ${RETURN}(bool(${args[0]} < ${args[1]}));
}

//! Returns true if x > y (Greater Than)
BOOL GT(DOUBLE, DOUBLE)
{
    ${RETURN}(bool(${args[0]} > ${args[1]}));
}

//! Round x and return it as an integer
INT IROUND(DOUBLE)
{
    ${RETURN}(int(boost::math::iround(${args[0]})));
}

//! Returns true if x is a power of 2
BOOL IS_PWR_OF_2(INT)
{
    if (${args[0]} < 0) return ${FALSE};
    int i = ${args[0]};
    while ( (i & 1) == 0 and (i > 1) ) {
        i >>= 1;
    }
    ${RETURN}(bool(i == 1));
}

//! Returns floor(log2(x)).
INT LOG2(INT)
{
    if (${args[0]} < 0) {
        throw uhd::runtime_error(str(
            boost::format("In NocScript function ${func_name}: Cannot calculate log2() of negative number.")
        ));
    }

    int power_value = ${args[0]};
    int log2_value = 0;
    while ( (power_value & 1) == 0 and (power_value > 1) ) {
        power_value >>= 1;
        log2_value++;
    }
    ${RETURN}(log2_value);
}

//! Returns x % y
INT MODULO(INT, INT)
{
    ${RETURN}(${args[0]} % ${args[1]});
}

//! Returns true if x == y
BOOL EQUAL(INT, INT)
{
    ${RETURN}(bool(${args[0]} == ${args[1]}));
}

//! Returns true if x == y
BOOL EQUAL(DOUBLE, DOUBLE)
{
    ${RETURN}(bool(${args[0]} == ${args[1]}));
}

//! Returns true if x == y
BOOL EQUAL(STRING, STRING)
{
    ${RETURN}(bool(${args[0]} == ${args[1]}));
}

CATEGORY: Bitwise Operations
//! Returns x >> y
INT SHIFT_RIGHT(INT, INT)
{
    ${RETURN}(${args[0]} >> ${args[1]});
}

//! Returns x << y
INT SHIFT_LEFT(INT, INT)
{
    ${RETURN}(${args[0]} << ${args[1]});
}

//! Returns x & y
INT BITWISE_AND(INT, INT)
{
    ${RETURN}(${args[0]} & ${args[1]});
}

//! Returns x | y
INT BITWISE_OR(INT, INT)
{
    ${RETURN}(${args[0]} | ${args[1]});
}

//! Returns x ^ y
INT BITWISE_XOR(INT, INT)
{
    ${RETURN}(${args[0]} ^ ${args[1]});
}

CATEGORY: Boolean Logic
//! Returns x xor y.
BOOL XOR(BOOL, BOOL)
{
    ${RETURN}(${args[0]} xor ${args[1]});
}

//! Returns !x
BOOL NOT(BOOL)
{
    ${RETURN}(not ${args[0]});
}

//! Always returns true
BOOL TRUE()
{
    return ${TRUE};
}

//! Always returns false
BOOL FALSE()
{
    return ${FALSE};
}

CATEGORY: Conditional Execution
//! Executes x, if true, execute y. Returns true if x is true.
BOOL IF(BOOL, BOOL)
{
    if (${args[0]}) {
        ${args[1]};
        ${RETURN}(true);
    }
    ${RETURN}(false);
}

//! Executes x, if true, execute y, otherwise, execute z. Returns true if x is true.
BOOL IF_ELSE(BOOL, BOOL, BOOL)
{
    if (${args[0]}) {
        ${args[1]};
        ${RETURN}(true);
    } else {
        ${args[2]};
    }
    ${RETURN}(false);
}

CATEGORY: Execution Control
//! Sleep for x seconds. Fractions are allowed. Millisecond accuracy.
BOOL SLEEP(DOUBLE)
{
    int ms = ${args[0]} / 1000;
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    ${RETURN}(true);
}
"""
# End of interesting part. The rest will take this and turn into a C++
# header file.
#############################################################################

HEADER = """<% import time %>//
///////////////////////////////////////////////////////////////////////
// This file was generated by ${file} on ${time.strftime("%c")}
///////////////////////////////////////////////////////////////////////
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/****************************************************************************
 * This file is autogenerated! Any manual changes in here will be
 * overwritten by calling nocscript_gen_basic_funcs.py!
 ***************************************************************************/

#include "expression.hpp"
#include "function_table.hpp"
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
${INCLUDE_LIST}

#ifndef INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_BASICFUNCS_HPP
#define INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_BASICFUNCS_HPP

namespace uhd { namespace rfnoc { namespace nocscript {
"""

# Not a Mako template:
FOOTER="""
}}} /* namespace uhd::rfnoc::nocscript */

#endif /* INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_BASICFUNCS_HPP */
"""

# Not a Mako template:
FUNC_TEMPLATE = """
expression_literal {NAME}(expression_container::expr_list_type &{ARGS})
{BODY}
"""

REGISTER_MACRO_TEMPLATE = """#define _REGISTER_ALL_FUNCS()${registry}
"""

REGISTER_COMMANDS_TEMPLATE = """
    % if len(arglist):
    expression_function::argtype_list_type ${func_name}_args = boost::assign::list_of
    % for this_type in arglist:
        (expression::TYPE_${this_type})
    % endfor
    ;
    % else:
    expression_function::argtype_list_type ${func_name}_args;
    % endif
    register_function(
            "${name}",
            boost::bind(&${func_name}, _1),
            expression::TYPE_${retval},
            ${func_name}_args
    );"""

DOXY_TEMPLATE = """/*! \page page_nocscript_funcs NocScript Function Reference
% for cat, func_by_name in func_list_tree.items():
- ${cat}
%   for func_name, func_info_list in func_by_name.items():
  - ${func_name}: ${func_info_list[0]['docstring']}
%     for func_info in func_info_list:
    - ${func_info['arglist']} -> ${func_info['retval']}
%     endfor
%   endfor
% endfor

*/
"""

def parse_tmpl(_tmpl_text, **kwargs):
    return Template(_tmpl_text).render(**kwargs)

def make_cxx_func_name(func_dict):
    """
    Creates a unique C++ function name from a function description
    """
    return "{name}__{retval}__{arglist}".format(
        name=func_dict['name'],
        retval=func_dict['retval'],
        arglist="_".join(func_dict['arglist'])
    )

def make_cxx_func_body(func_dict):
    """
    Formats the function body properly
    """
    type_lookup_methods = {
            'INT': 'get_int',
            'DOUBLE': 'get_double',
            'BOOL': 'get_bool',
            'STRING': 'get_string',
    }
    args_lookup = []
    for idx, arg_type in enumerate(func_dict['arglist']):
        args_lookup.append("args[{idx}]->eval().{getter}()".format(idx=idx, getter=type_lookup_methods[arg_type]))
    return parse_tmpl(
        func_dict['body'],
        args=args_lookup,
        FALSE='expression_literal(false)',
        TRUE='expression_literal(true)',
        RETURN='return expression_literal',
        **func_dict
    )

def prep_function_list():
    """
    - Remove all comments
    - Split the function list into individual functions
    - Split the functions into return value, name, argument list and body
    """
    comment_remove_re = re.compile(r'^\s*#.*$', flags=re.MULTILINE)
    func_list_wo_comments = comment_remove_re.sub('', FUNCTION_LIST)
    func_splitter_re = re.compile(r'(?<=^})\s*$', flags=re.MULTILINE)
    func_list_split = func_splitter_re.split(func_list_wo_comments)
    func_list_split = [x.strip() for x in func_list_split if len(x.strip())]
    func_list = []
    last_category = ''
    for func in func_list_split:
        split_regex = r'(^CATEGORY: (?P<cat>[^\n]*)\s*)?' \
                      r'//!(?P<docstring>[^\n]*)\s*' + \
                      r'(?P<retval>[A-Z][A-Z0-9_]*)\s+' + \
                      r'(?P<funcname>[A-Z][A-Z0-9_]*)\s*\((?P<arglist>[^\)]*)\)\s*' + \
                      r'(?P<funcbody>^{.*)'
        split_re = re.compile(split_regex, flags=re.MULTILINE|re.DOTALL)
        mo = split_re.match(func)
        if mo.group('cat'):
            last_category = mo.group('cat').strip()
        func_dict = {
            'docstring': mo.group('docstring').strip(),
            'name': mo.group('funcname'),
            'retval': mo.group('retval'),
            'arglist': [x.strip() for x in mo.group('arglist').split(',') if len(x.strip())],
            'body': mo.group('funcbody'),
            'category': last_category,
        }
        func_dict['func_name'] = make_cxx_func_name(func_dict)
        func_list.append(func_dict)
    return func_list

def write_function_header(output_filename):
    """
    Create the .hpp file that defines all the NocScript functions in C++.
    """
    func_list = prep_function_list()
    # Step 1: Write the prototypes
    func_prototypes = ''
    registry_commands = ''
    for func in func_list:
        func_prototypes += FUNC_TEMPLATE.format(
            NAME=func['func_name'],
            BODY=make_cxx_func_body(func),
            ARGS="args" if len(func['arglist']) else ""
        )
        registry_commands += parse_tmpl(
                REGISTER_COMMANDS_TEMPLATE,
                **func
        )
    # Step 2: Write the registry process
    register_func = parse_tmpl(REGISTER_MACRO_TEMPLATE, registry=registry_commands)
    register_func = register_func.replace('\n', ' \\\n')

    # Final step: Join parts and write to file
    full_file = "\n".join((
            parse_tmpl(HEADER, file = os.path.basename(__file__), INCLUDE_LIST=INCLUDE_LIST),
            func_prototypes,
            register_func,
            FOOTER,
    ))
    open(output_filename, 'w').write(full_file)

def write_manual_file(output_filename):
    """
    Write the Doxygen file for the NocScript functions.
    """
    func_list = prep_function_list()
    func_list_tree = {}
    for func in func_list:
        if func['category'] not in func_list_tree:
            func_list_tree[func['category']] = {}
        if func['name'] not in func_list_tree[func['category']]:
            func_list_tree[func['category']][func['name']] = []
        func_list_tree[func['category']][func['name']].append(func)
    open(output_filename, 'w').write(parse_tmpl(DOXY_TEMPLATE, func_list_tree=func_list_tree))


def main():
    if len(sys.argv) < 2:
        print("No output file specified!")
        exit(1)
    outfile = sys.argv[1]
    if os.path.splitext(outfile)[1] == '.dox':
        write_manual_file(outfile)
    else:
        write_function_header(outfile)

if __name__ == "__main__":
    main()
