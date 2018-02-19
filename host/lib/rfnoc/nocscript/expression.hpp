//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <vector>
#include <map>

#ifndef INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_EXPR_HPP
#define INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_EXPR_HPP

namespace uhd { namespace rfnoc { namespace nocscript {

// Forward declaration for expression::eval()
class expression_literal;

/*! Virtual base class for Noc-Script expressions.
 */
class expression
{
  public:
    typedef boost::shared_ptr<expression> sptr;

    //! All the possible return types for expressions within Noc-Script
    enum type_t {
        TYPE_INT,
        TYPE_DOUBLE,
        TYPE_STRING,
        TYPE_BOOL,
        TYPE_INT_VECTOR
    };

    // TODO make this a const and fix the [] usage
    static std::map<type_t, std::string> type_repr;

    //! Returns the type of this expression without evaluating it
    virtual type_t infer_type() const = 0;

    //! Evaluate current expression and return its return value
    virtual expression_literal eval() = 0;
};

/*! Literal (constant) expression class
 *
 * A literal is any value that is literally given in the NoC-Script
 * source code, such as '5', '"FOO"', or '2.3'.
 */
class expression_literal : public expression
{
  public:
    typedef boost::shared_ptr<expression_literal> sptr;

    template <typename expr_type>
    static sptr make(expr_type x) { return boost::make_shared<expression_literal>(x); };

    /*! Generate the literal expression from its token string representation.
     * This includes markup, e.g. a string would still have the quotes, and
     * a hex value would still have leading 0x.
     */
    expression_literal(
            const std::string token_val,
            expression::type_t type
    );

    //! Create a boolean literal expression from a C++ bool.
    expression_literal(bool b=false);
    //! Create an integer literal expression from a C++ int.
    expression_literal(int i);
    //! Create a double literal expression from a C++ double.
    expression_literal(double d);
    //! Create a string literal expression from a C++ string.
    expression_literal(const std::string &s);
    //! Create an int vector literal expression from a C++ vector<int>.
    expression_literal(std::vector<int> v);

    expression::type_t infer_type() const
    {
        return _type;
    }

    //! Literals aren't evaluated as such, so the evaluation
    //  simply returns a copy of itself.
    expression_literal eval()
    {
        return *this; // TODO make sure this is copy
    }

    /*! A 'type cast' to bool. Cast rules are similar to most
     * scripting languages:
     * - Integers and doubles are false if zero, true otherwise
     * - Strings are false if empty, true otherwise
     * - Vectors are false if empty, true otherwise
     */
    bool to_bool() const;

    /*! Convenience function to typecast to C++ int
     *
     * Note that the current type must be TYPE_INT.
     *
     * \return C++ int representation of current literal
     * \throws uhd::type_error if type didn't match
     */
    int get_int() const;

    /*! Convenience function to typecast to C++ double
     *
     * Note that the current type must be TYPE_DOUBLE.
     *
     * \return C++ double representation of current literal
     * \throws uhd::type_error if type didn't match
     */
    double get_double() const;

    /*! Convenience function to typecast to C++ std::string.
     *
     * Note that the current type must be TYPE_STRING.
     *
     * \return String representation of current literal.
     * \throws uhd::type_error if type didn't match.
     */
    std::string get_string() const;

    /*! Convenience function to typecast to C++ int vector.
     *
     * Note that the current type must be TYPE_INT_VECTOR.
     *
     * \return String representation of current literal.
     * \throws uhd::type_error if type didn't match.
     */
    std::vector<int> get_int_vector() const;

    /*! Convenience function to typecast to C++ bool.
     *
     * Note that the current type must be TYPE_BOOL.
     * See also expression_literal::to_bool() for a type-cast
     * style function.
     *
     * \return bool representation of current literal.
     * \throws uhd::type_error if type didn't match.
     */
    bool get_bool() const;

    //! String representation
    std::string repr() const;

    bool operator==(const expression_literal &rhs) const;

  private:
    //! For TYPE_BOOL
    bool _bool_val;

    //! For TYPE_INT
    int _int_val;

    //! For TYPE_DOUBLE
    double _double_val;

    //! For TYPE_INT_VECTOR
    std::vector<int> _int_vector_val;

    //! Store the token value
    std::string _val;

    //! Current expression type
    expression::type_t _type;
};

UHD_INLINE std::ostream& operator<< (std::ostream& out, const expression_literal &l)
{
    out << l.repr();
    return out;
}

UHD_INLINE std::ostream& operator<< (std::ostream& out, const expression_literal::sptr &l)
{
    out << l->repr();
    return out;
}

/*! Contains multiple (sub-)expressions.
 */
class expression_container : public expression
{
  public:
    typedef boost::shared_ptr<expression_container> sptr;
    typedef std::vector<expression::sptr> expr_list_type;

    //! Return an sptr to an empty container
    static sptr make();

    //! List of valid combination types (see expression_container::eval()).
    enum combiner_type {
        COMBINE_ALL,
        COMBINE_AND,
        COMBINE_OR,
        COMBINE_NOTSET
    };

    //! Create an empty container
    expression_container() : _combiner(COMBINE_NOTSET) {};
    virtual ~expression_container(){};

    /*! Type-deduction rules for containers are:
     * - If the combination type is COMBINE_ALL or COMBINE_AND,
     *   return value must be TYPE_BOOL
     * - In all other cases, we return the last expression return
     *   value, and hence its type is relevant
     */
    expression::type_t infer_type() const;

    /*! Add another expression container to this container.
     */
    virtual void add(expression::sptr new_expr);

    virtual bool empty() const;

    void set_combiner_safe(const combiner_type c);

    void set_combiner(const combiner_type c) { _combiner = c; };

    combiner_type get_combiner() const { return _combiner; };

    /*! Evaluate a container by evaluating its sub-expressions.
     *
     * If a container contains multiple sub-expressions, the rules
     * for evaluating them depend on the combiner_type:
     * - COMBINE_ALL: Run all the sub-expressions and return the last
     *   expression's return value
     * - COMBINE_AND: Run sub-expressions, in order, until one of them
     *   returns false. Following expressions are not evaluated (like
     *   most C++ compilers).
     * - COMBINE_OR: Run sub-expressions, in order, until one of them
     *   returns true. Following expressions are not evaluated.
     *
     * In the special case where no sub-expressions are contained, always
     * returns true.
     */
    virtual expression_literal eval();

  protected:
    //! Store all the sub-expressions, in order
    expr_list_type _sub_exprs;
    combiner_type _combiner;
};

// Forward declaration:
class function_table;
/*! A function call is a special type of container.
 *
 * All arguments are sub-expressions. The combiner type is
 * always COMBINE_ALL in this case (changing the combiner type
 * does not affect anything).
 *
 * The actual function maps to a C++ function available through
 * a uhd::rfnoc::nocscript::function_table object.
 *
 * The recommended to use this is:
 * 1. Create a function object giving its name (e.g. ADD)
 * 2. Use the add() method to add all the function arguments
 *    in the right order (left to right).
 * 3. Once step 2 is complete, the function object can be used.
 *    Call infer_type() to get the return value, if required.
 * 4. Calling eval() will call into the function table. The
 *    argument expressions are evaluated, if so required, inside
 *    the function (lazy evalulation). Functions do not need
 *    to evaluate arguments.
 */
class expression_function : public expression_container
{
  public:
    typedef boost::shared_ptr<expression_function> sptr;
    typedef std::vector<expression::type_t> argtype_list_type;

    //! Return an sptr to a function object without args
    static sptr make(
            const std::string &name,
            const boost::shared_ptr<function_table> func_table
    );

    static std::string to_string(const std::string &name, const argtype_list_type &types);

    expression_function(
            const std::string &name,
            const boost::shared_ptr<function_table> func_table
    );
    ~expression_function(){}

    //! Add an argument expression
    virtual void add(expression::sptr new_expr);

    /*! Looks up the function type in the function table.
     *
     * Note that this will only work after all arguments have been
     * added, as they are also used to look up a function's type in the
     * function table.
     */
    expression::type_t infer_type() const;

    /*! Evaluate all arguments, then the function itself.
     */
    expression_literal eval();

    //! String representation
    std::string repr() const;

  private:
    std::string _name;
    const boost::shared_ptr<function_table> _func_table;
    std::vector<expression::type_t> _arg_types;
};


/*! Variable expression
 *
 * Variables are like literals, only their type and value aren't known
 * at parse-time. Instead, we provide a function object to look up
 * variable's types and value.
 */
class expression_variable : public expression
{
  public:
    typedef boost::shared_ptr<expression_variable> sptr;
    typedef boost::function<expression::type_t(const std::string &)> type_getter_type;
    typedef boost::function<expression_literal(const std::string &)> value_getter_type;

    static sptr make(
            const std::string &token_val,
            type_getter_type type_getter,
            value_getter_type value_getter
    );

    /*! Create a variable object from its token value
     *  (e.g. '$spp', i.e. including the '$' symbol). The variable
     *  does not have to exist at this point.
     */
    expression_variable(
            const std::string &token_val,
            type_getter_type type_getter,
            value_getter_type value_getter
    );

    /*! Looks up the variable type in the variable table.
     *
     * \throws Depending on \p type_getter, this may throw when the variable does not exist.
     *         Recommended behaviour is to throw uhd::syntax_error.
     */
    expression::type_t infer_type() const;

    /*! Look up a variable's value in the variable table.
     *
     * \throws Depending on \p value_getter, this may throw when the variable does not exist.
     *         Recommended behaviour is to throw uhd::syntax_error.
     */
    expression_literal eval();

  private:
    std::string _varname;
    type_getter_type _type_getter;
    value_getter_type _value_getter;
};

}}} /* namespace uhd::rfnoc::nocscript */

#endif /* INCLUDED_LIBUHD_RFNOC_NOCSCRIPT_EXPR_HPP */
// vim: sw=4 et:
