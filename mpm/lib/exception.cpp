//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <mpm/exception.hpp>
#include <boost/format.hpp>
#include <functional>

using namespace mpm;

exception::exception(const std::string &what):
    std::runtime_error(what){/* NOP */}

#define make_exception_impl(name, class, base) \
    class::class(const std::string &what): \
        base(str(boost::format("%s: %s") % name % what)){} \
    unsigned class::code(void) const{return std::hash<std::string>()(#class) & 0xfff;} \
    class *class::dynamic_clone(void) const{return new class(*this);} \
    void class::dynamic_throw(void) const{throw *this;}

make_exception_impl("AssertionError",        assertion_error,         exception)
make_exception_impl("LookupError",           lookup_error,            exception)
make_exception_impl("IndexError",            index_error,             lookup_error)
make_exception_impl("KeyError",              key_error,               lookup_error)
make_exception_impl("TypeError",             type_error,              exception)
make_exception_impl("ValueError",            value_error,             exception)
make_exception_impl("RuntimeError",          runtime_error,           exception)
make_exception_impl("NotImplementedError",   not_implemented_error,   runtime_error)
make_exception_impl("EnvironmentError",      environment_error,       exception)
make_exception_impl("IOError",               io_error,                environment_error)
make_exception_impl("OSError",               os_error,                environment_error)
make_exception_impl("SystemError",           system_error,            exception)

