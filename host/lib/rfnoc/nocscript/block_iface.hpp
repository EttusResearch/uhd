//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "expression.hpp"
#include "parser.hpp"
#include <uhd/rfnoc/block_ctrl_base.hpp>
#include <boost/thread/mutex.hpp>

#ifndef INCLUDED_LIBUHD_NOCSCRIPT_BLOCK_IFACE_HPP
#define INCLUDED_LIBUHD_NOCSCRIPT_BLOCK_IFACE_HPP

namespace uhd { namespace rfnoc { namespace nocscript {

/*! NocScript / Block interface class.
 *
 * This class only exists as a member of an rfnoc::block_ctrl_base class.
 * It should never be instantiated anywhere else. It is used to execute
 * NocScript function calls that require access to the original block
 * controller class.
 */
class block_iface {

  public:
      typedef boost::shared_ptr<block_iface> sptr;

      static sptr make(uhd::rfnoc::block_ctrl_base* block_ptr);

      block_iface(uhd::rfnoc::block_ctrl_base* block_ptr);

    /*! Execute \p code and make sure it returns 'true'.
     *
     * \param code Must be a valid NocScript expression that returns a boolean value.
     *             If it returns false, this is interpreted as failure.
     * \param error_message If the expression fails, this error message is printed.
     * \throws uhd::runtime_error if the expression returns false.
     * \throws uhd::syntax_error if the expression is invalid.
     */
    void run_and_check(const std::string &code, const std::string &error_message="");

  private:
    //! For the local interpreter lock (lil)
    boost::mutex _lil_mutex;

    //! Wrapper for block_ctrl_base::sr_write, so we can call it from within NocScript
    expression_literal _nocscript__sr_write(expression_container::expr_list_type);

    //! Argument type getter that can be used within NocScript
    expression::type_t _nocscript__arg_get_type(const std::string &argname);

    //! Argument value getter that can be used within NocScript
    expression_literal _nocscript__arg_get_val(const std::string &argname);

    //! Argument value setters:
    expression_literal _nocscript__arg_set_int(const expression_container::expr_list_type &);
    expression_literal _nocscript__arg_set_string(const expression_container::expr_list_type &);
    expression_literal _nocscript__arg_set_double(const expression_container::expr_list_type &);
    expression_literal _nocscript__arg_set_intvec(const expression_container::expr_list_type &);

    //! Variable value getter
    expression_literal _nocscript__var_get(const expression_container::expr_list_type &);

    //! Variable value setter
    expression_literal _nocscript__var_set(const expression_container::expr_list_type &);

    //! Raw pointer to the block class. Note that since block_iface may
    // only live as a member of a block_ctrl_base, we don't really need
    // the reference counting.
    uhd::rfnoc::block_ctrl_base* _block_ptr;

    //! Pointer to the parser object
    parser::sptr _parser;

    //! Container for scoped variables
    std::map<std::string, expression_literal> _vars;
};

}}} /* namespace uhd::rfnoc::nocscript */

#endif /* INCLUDED_LIBUHD_NOCSCRIPT_BLOCK_IFACE_HPP */
// vim: sw=4 et:
