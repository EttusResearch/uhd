//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <stdexcept>
#include <string>

namespace mpm {

struct exception : std::runtime_error
{
    exception(const std::string& what);
    virtual unsigned code(void) const            = 0;
    virtual exception* dynamic_clone(void) const = 0;
    virtual void dynamic_throw(void) const       = 0;
};

struct assertion_error : exception
{
    assertion_error(const std::string& what);
    unsigned code(void) const override;
    assertion_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct lookup_error : exception
{
    lookup_error(const std::string& what);
    unsigned code(void) const override;
    lookup_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct index_error : lookup_error
{
    index_error(const std::string& what);
    unsigned code(void) const override;
    index_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct key_error : lookup_error
{
    key_error(const std::string& what);
    unsigned code(void) const override;
    key_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct type_error : exception
{
    type_error(const std::string& what);
    unsigned code(void) const override;
    type_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct value_error : exception
{
    value_error(const std::string& what);
    unsigned code(void) const override;
    value_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct runtime_error : exception
{
    runtime_error(const std::string& what);
    unsigned code(void) const override;
    runtime_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct not_implemented_error : runtime_error
{
    not_implemented_error(const std::string& what);
    unsigned code(void) const override;
    not_implemented_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct environment_error : exception
{
    environment_error(const std::string& what);
    unsigned code(void) const override;
    environment_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct io_error : environment_error
{
    io_error(const std::string& what);
    unsigned code(void) const override;
    io_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct os_error : environment_error
{
    os_error(const std::string& what);
    unsigned code(void) const override;
    os_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct system_error : exception
{
    system_error(const std::string& what);
    unsigned code(void) const override;
    system_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

struct syntax_error : exception
{
    syntax_error(const std::string& what);
    unsigned code(void) const override;
    syntax_error* dynamic_clone(void) const override;
    void dynamic_throw(void) const override;
};

/*!
 * Create a formatted string with throw-site information.
 * Fills in the function name, file name, and line number.
 * \param what the std::exception message
 * \return the formatted exception message
 */
#define MPM_THROW_SITE_INFO(what)                                                     \
    std::string(std::string(what) + "\n" + "  in " + std::string(__PRETTY_FUNCTION__) \
                + "\n" + "  at " + std::string(__FILE__) + ":"                        \
                + BOOST_STRINGIZE(__LINE__) + "\n")

/*!
 * Throws an invalid code path exception with throw-site information.
 * Use this macro in places that code execution is not supposed to go.
 */
#define MPM_THROW_INVALID_CODE_PATH() \
    throw mpm::system_error(MPM_THROW_SITE_INFO("invalid code path"))

/*!
 * Assert the result of the code evaluation.
 * If the code evaluates to false, throw an assertion error.
 * \param code the code that resolved to a boolean
 */
#define MPM_ASSERT_THROW(code)                                      \
    {                                                               \
        if (not(code))                                              \
            throw mpm::assertion_error(MPM_THROW_SITE_INFO(#code)); \
    }

} /* namespace mpm */
