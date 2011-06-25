//
// Copyright 2010-2011 Ettus Research LLC
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

#ifndef INCLUDED_UHD_EXCEPTION_HPP
#define INCLUDED_UHD_EXCEPTION_HPP

#include <uhd/config.hpp>
#include <boost/current_function.hpp>
#include <stdexcept>
#include <string>

/*!
 * Define common exceptions used throughout the code:
 *
 * - The python built-in exceptions were used as inspiration.
 * - Exceptions inherit from std::exception to provide what().
 * - Exceptions inherit from uhd::exception to provide code().
 *
 * The code() provides an error code which allows the application
 * the option of printing a cryptic error message from the 1990s.
 *
 * The dynamic_clone() and dynamic_throw() methods allow us to:
 * catch an exception by dynamic type (i.e. derived class), save it,
 * and later rethrow it, knowing only the static type (i.e. base class),
 * and then finally to catch it again using the derived type.
 *
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n2106.html
 */
namespace uhd{

    struct UHD_API exception : std::runtime_error{
        exception(const std::string &what);
        virtual unsigned code(void) const = 0;
        virtual exception *dynamic_clone(void) const = 0;
        virtual void dynamic_throw(void) const = 0;
    };

    struct UHD_API assertion_error : exception{
        assertion_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual assertion_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API lookup_error : exception{
        lookup_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual lookup_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API index_error : lookup_error{
        index_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual index_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API key_error : lookup_error{
        key_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual key_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API type_error : exception{
        type_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual type_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API value_error : exception{
        value_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual value_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API runtime_error : exception{
        runtime_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual runtime_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API not_implemented_error : runtime_error{
        not_implemented_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual not_implemented_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API environment_error : exception{
        environment_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual environment_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API io_error : environment_error{
        io_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual io_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API os_error : environment_error{
        os_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual os_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    struct UHD_API system_error : exception{
        system_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual system_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };

    /*!
     * Create a formated string with throw-site information.
     * Fills in the function name, file name, and line number.
     * \param what the std::exeption message
     * \return the formatted exception message
     */
    #define UHD_THROW_SITE_INFO(what) std::string( \
        std::string(what) + "\n" + \
        "  in " + std::string(BOOST_CURRENT_FUNCTION) + "\n" + \
        "  at " + std::string(__FILE__) + ":" + BOOST_STRINGIZE(__LINE__) + "\n" \
    )

    /*!
     * Throws an invalid code path exception with throw-site information.
     * Use this macro in places that code execution is not supposed to go.
     */
    #define UHD_THROW_INVALID_CODE_PATH() \
        throw uhd::system_error(UHD_THROW_SITE_INFO("invalid code path"))

    /*!
     * Assert the result of the code evaluation.
     * If the code evaluates to false, throw an assertion error.
     * \param code the code that resolved to a boolean
     */
    #define UHD_ASSERT_THROW(code) if (not (code)) \
        throw uhd::assertion_error(UHD_THROW_SITE_INFO(#code)); \
    else void(0)

} //namespace uhd

#endif /* INCLUDED_UHD_EXCEPTION_HPP */
