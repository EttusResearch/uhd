//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "parser.hpp"
#include <uhd/utils/cast.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/assign.hpp>
#include <boost/make_shared.hpp>
#include <sstream>
#include <stack>

using namespace uhd::rfnoc::nocscript;
namespace lex = boost::spirit::lex;

class parser_impl : public parser
{
  public:
    /******************************************************************
     * Structors TODO make them protected
     *****************************************************************/
    parser_impl(
        function_table::sptr ftable,
        expression_variable::type_getter_type var_type_getter,
        expression_variable::value_getter_type var_value_getter
    ) : _ftable(ftable)
      , _var_type_getter(var_type_getter)
      , _var_value_getter(var_value_getter)
    {
        // nop
    }

    ~parser_impl() {};


    /******************************************************************
     * Parsing
     *****************************************************************/
    //! List of parser tokens
    enum token_ids
    {
        ID_WHITESPACE = lex::min_token_id + 42,
        ID_KEYWORD,
        ID_ARG_SEP,
        ID_PARENS_OPEN,
        ID_PARENS_CLOSE,
        ID_VARIABLE,
        ID_LITERAL_DOUBLE,
        ID_LITERAL_INT,
        ID_LITERAL_HEX,
        ID_LITERAL_STR,
        ID_LITERAL_VECTOR_INT
    };

    //! The Lexer object used for NocScript
    template <typename Lexer>
    struct ns_lexer : lex::lexer<Lexer>
    {
        ns_lexer()
        {
            this->self.add
                ("\\s+",                 ID_WHITESPACE)
                (",",                    ID_ARG_SEP)
                ("[A-Z][A-Z0-9_]*",      ID_KEYWORD)
                ("\\(",                  ID_PARENS_OPEN)
                ("\\)",                  ID_PARENS_CLOSE)
                ("\\$[a-z][a-z0-9_]*",   ID_VARIABLE)
                ("-?\\d+\\.\\d+",        ID_LITERAL_DOUBLE)
                ("-?\\d+",               ID_LITERAL_INT)
                ("0x[0-9A-F]+",          ID_LITERAL_HEX)
                ("\\\"[^\\\"]*\\\"",     ID_LITERAL_STR)
                ("'[^']*'",              ID_LITERAL_STR) // both work
                ("\\[[0-9]\\]",          ID_LITERAL_VECTOR_INT)
            ;
        }
    };

  private:
    struct grammar_props
    {
        function_table::sptr ftable;
        expression_variable::type_getter_type var_type_getter;
        expression_variable::value_getter_type var_value_getter;

        //! Store the last keyword
        std::string function_name;
        std::string error;
        std::stack<expression_container::sptr> expr_stack;

        grammar_props(
            function_table::sptr ftable_,
            expression_variable::type_getter_type var_type_getter_,
            expression_variable::value_getter_type var_value_getter_
        ) : ftable(ftable_), var_type_getter(var_type_getter_), var_value_getter(var_value_getter_),
            function_name("")
        {
            UHD_ASSERT_THROW(expr_stack.empty());
            // Push an empty container to the stack to hold the result
            expr_stack.push(expression_container::make());
        }

        expression::sptr get_result()
        {
            UHD_ASSERT_THROW(expr_stack.size() == 1);
            return expr_stack.top();
        }
    };

    //! This isn't strictly a grammar, as it also includes semantic
    //  actions etc. I'm not going to spend ages thinking of a better
    //  name at this point.
    struct grammar
    {
        // Implementation detail specific to boost::bind (see Boost::Spirit
        // examples)
        typedef bool result_type;

        static const int VALID_COMMA        = 0x1;
        static const int VALID_PARENS_OPEN  = 0x2;
        static const int VALID_PARENS_CLOSE = 0x4;
        static const int VALID_EXPRESSION   = 0x8 + 0x02;
        static const int VALID_OPERATOR      = 0x10;

        // !This function operator gets called for each of the matched tokens.
        template <typename Token>
        bool operator()(Token const& t, grammar_props &P, int &next_valid_state) const
        {
            //! This is totally not how Boost::Spirit is meant to be used,
            // as there's token types etc. But for now let's just convert
            // every token to a string, and then handle it as such.
            std::stringstream sstr;
            sstr << t.value();
            std::string val = sstr.str();
            //std::cout << "VAL: " << val << std::endl;
            //std::cout << "Next valid states:\n"
                    //<< boost::format("VALID_COMMA        [%s]\n") % ((next_valid_state & 0x1) ? "x" : " ")
                    //<< boost::format("VALID_PARENS_OPEN  [%s]\n") % ((next_valid_state & 0x2) ? "x" : " ")
                    //<< boost::format("VALID_PARENS_CLOSE [%s]\n") % ((next_valid_state & 0x4) ? "x" : " ")
                    //<< boost::format("VALID_EXPRESSION   [%s]\n") % ((next_valid_state & (0x8 + 0x02)) ? "x" : " ")
                    //<< boost::format("VALID_OPERATOR      [%s]\n") % ((next_valid_state & 0x10) ? "x" : " ")
                    //<< std::endl;

            switch (t.id()) {

            case ID_WHITESPACE:
                // Ignore
                break;

            case ID_KEYWORD:
                // Ambiguous, could be an operator (AND, OR) or a function name (ADD, MULT...).
                // So first, check which it is:
                if (val == "AND" or val == "OR") {
                    if (not (next_valid_state & VALID_OPERATOR)) {
                        P.error = str(boost::format("Unexpected operator: %s") % val);
                        return false;
                    }
                    next_valid_state = VALID_EXPRESSION;
                    try {
                        if (val == "AND") {
                            P.expr_stack.top()->set_combiner_safe(expression_container::COMBINE_AND);
                        } else if (val == "OR") {
                            P.expr_stack.top()->set_combiner_safe(expression_container::COMBINE_OR);
                        }
                    } catch (const uhd::syntax_error &e) {
                        P.error = str(boost::format("Operator %s is mixing operator types within this container.") % val);
                    }
                    // Right now, we can't have multiple operator types within a container.
                    // We might be able to change that, if there's enough demand. Either
                    // we keep track of multiple operators, or we open a new container.
                    // In the latter case, we'd need a way of keeping track of those containers,
                    // so it's a bit tricky.
                    break;
                }
                // If it's not a keyword, it has to be a function, so check the
                // function table:
                if (not (next_valid_state & VALID_EXPRESSION)) {
                    P.error = str(boost::format("Unexpected expression: %s") % val);
                    return false;
                }
                if (not P.ftable->function_exists(val)) {
                    P.error = str(boost::format("Unknown function: %s") % val);
                    return false;
                }
                P.function_name = val;
                next_valid_state = VALID_PARENS_OPEN;
                break;

            // Every () creates a new container, either a raw container or
            // a function.
            case ID_PARENS_OPEN:
                if (not (next_valid_state & VALID_PARENS_OPEN)) {
                    P.error = str(boost::format("Unexpected parentheses."));
                    return false;
                }
                if (not P.function_name.empty()) {
                    // We've already checked the function name exists
                    P.expr_stack.push(expression_function::make(P.function_name, P.ftable));
                    P.function_name.clear();
                } else {
                    P.expr_stack.push(expression_container::make());
                }
                // Push another empty container to hold the first element/argument
                // in this container:
                P.expr_stack.push(expression_container::make());
                next_valid_state = VALID_EXPRESSION | VALID_PARENS_CLOSE;
                break;

            case ID_PARENS_CLOSE:
            {
                if (not (next_valid_state & VALID_PARENS_CLOSE)) {
                    P.error = str(boost::format("Unexpected parentheses."));
                    return false;
                }
                if (P.expr_stack.size() < 2) {
                    P.error = str(boost::format("Unbalanced closing parentheses."));
                    return false;
                }
                // First pop the last expression inside the parentheses,
                // if it's not empty, add it to the top container (this also avoids
                // adding arguments to functions if none were provided):
                expression_container::sptr c = P.expr_stack.top();
                P.expr_stack.pop();
                if (not c->empty()) {
                    P.expr_stack.top()->add(c);
                }
                // At the end of (), either a function or container is complete,
                // so pop that and add it to its top container:
                expression_container::sptr c2 = P.expr_stack.top();
                P.expr_stack.pop();
                P.expr_stack.top()->add(c2);
                next_valid_state = VALID_OPERATOR | VALID_COMMA | VALID_PARENS_CLOSE;
            }
                break;

            case ID_ARG_SEP:
            {
                if (not (next_valid_state & VALID_COMMA)) {
                    P.error = str(boost::format("Unexpected comma."));
                    return false;
                }
                next_valid_state = VALID_EXPRESSION;
                // If stack size is 1, we're on the base container, which means we
                // simply string stuff.
                if (P.expr_stack.size() == 1) {
                    break;
                }
                // Otherwise, a ',' always means we add the previous expression to
                // the current container:
                expression_container::sptr c = P.expr_stack.top();
                P.expr_stack.pop();
                P.expr_stack.top()->add(c);
                // It also means another expression is following, so create another
                // empty container for that:
                P.expr_stack.push(expression_container::make());
            }
                break;

            // All the atomic expressions just get added to the current container:

            case ID_VARIABLE:
            {
                if (not (next_valid_state & VALID_EXPRESSION)) {
                    P.error = str(boost::format("Unexpected expression."));
                    return false;
                }
                expression_variable::sptr v = expression_variable::make(val, P.var_type_getter, P.var_value_getter);
                P.expr_stack.top()->add(v);
                next_valid_state = VALID_OPERATOR | VALID_COMMA | VALID_PARENS_CLOSE;
            }
                break;

            default:
            // If we get here, we assume it's a literal expression
            {
                if (not (next_valid_state & VALID_EXPRESSION)) {
                    P.error = str(boost::format("Unexpected expression."));
                    return false;
                }
                expression::type_t token_type;
                switch (t.id()) { // A map lookup would be more elegant, but we'd need a nicer C++ for that
                    case ID_LITERAL_DOUBLE: token_type = expression::TYPE_DOUBLE; break;
                    case ID_LITERAL_INT: token_type = expression::TYPE_INT; break;
                    case ID_LITERAL_HEX: token_type = expression::TYPE_INT; break;
                    case ID_LITERAL_STR: token_type = expression::TYPE_STRING; break;
                    case ID_LITERAL_VECTOR_INT: token_type = expression::TYPE_INT_VECTOR; break;
                    default: UHD_THROW_INVALID_CODE_PATH();
                }
                P.expr_stack.top()->add(boost::make_shared<expression_literal>(val, token_type));
                next_valid_state = VALID_OPERATOR | VALID_COMMA | VALID_PARENS_CLOSE;
                break;
            }

            } // end switch
            return true;
        }
    };

  public:
    expression::sptr create_expr_tree(const std::string &code)
    {
        // Create empty stack and keyword states
        grammar_props P(_ftable, _var_type_getter, _var_value_getter);
        int next_valid_state = grammar::VALID_EXPRESSION;

        // Create a lexer instance
        ns_lexer<lex::lexertl::lexer<> > lexer_functor;

        // Tokenize the string
        char const* first = code.c_str();
        char const* last = &first[code.size()];
        bool r = lex::tokenize(
            first, last, // Iterators
            lexer_functor, // Lexer
            boost::bind(grammar(), _1, boost::ref(P), boost::ref(next_valid_state)) // Function object
        );

        // Check the parsing worked:
        if (not r or P.expr_stack.size() != 1) {
            std::string rest(first, last);
            throw uhd::syntax_error(str(
                    boost::format("Parsing stopped at: %s\nError message: %s")
                    % rest % P.error
            ));
        }

        // Clear stack and return result
        return P.get_result();
    }

  private:

    function_table::sptr _ftable;
    expression_variable::type_getter_type _var_type_getter;
    expression_variable::value_getter_type _var_value_getter;
};

parser::sptr parser::make(
    function_table::sptr ftable,
    expression_variable::type_getter_type var_type_getter,
    expression_variable::value_getter_type var_value_getter
) {
    return sptr(new parser_impl(
        ftable,
        var_type_getter,
        var_value_getter
    ));
}

