/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "pybind11.h"

#include <utility>

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)

inline void ensure_builtins_in_globals(object &global) {
#if defined(PYPY_VERSION) || PY_VERSION_HEX < 0x03080000




    if (!global.contains("__builtins__"))
        global["__builtins__"] = module_::import(PYBIND11_BUILTINS_MODULE);
#else
    (void) global;
#endif
}

PYBIND11_NAMESPACE_END(detail)

enum eval_mode {

    eval_expr,


    eval_single_statement,


    eval_statements
};

template <eval_mode mode = eval_expr>
object eval(const str &expr, object global = globals(), object local = object()) {
    if (!local) {
        local = global;
    }

    detail::ensure_builtins_in_globals(global);


    std::string buffer = "# -*- coding: utf-8 -*-\n" + (std::string) expr;

    int start = 0;
    switch (mode) {
        case eval_expr:
            start = Py_eval_input;
            break;
        case eval_single_statement:
            start = Py_single_input;
            break;
        case eval_statements:
            start = Py_file_input;
            break;
        default:
            pybind11_fail("invalid evaluation mode");
    }

    PyObject *result = PyRun_String(buffer.c_str(), start, global.ptr(), local.ptr());
    if (!result) {
        throw error_already_set();
    }
    return reinterpret_steal<object>(result);
}

template <eval_mode mode = eval_expr, size_t N>
object eval(const char (&s)[N], object global = globals(), object local = object()) {

    auto expr = (s[0] == '\n') ? str(module_::import("textwrap").attr("dedent")(s)) : str(s);
    return eval<mode>(expr, std::move(global), std::move(local));
}

inline void exec(const str &expr, object global = globals(), object local = object()) {
    eval<eval_statements>(expr, std::move(global), std::move(local));
}

template <size_t N>
void exec(const char (&s)[N], object global = globals(), object local = object()) {
    eval<eval_statements>(s, std::move(global), std::move(local));
}

#if defined(PYPY_VERSION)
template <eval_mode mode = eval_statements>
object eval_file(str, object, object) {
    pybind11_fail("eval_file not supported in PyPy3. Use eval");
}
template <eval_mode mode = eval_statements>
object eval_file(str, object) {
    pybind11_fail("eval_file not supported in PyPy3. Use eval");
}
template <eval_mode mode = eval_statements>
object eval_file(str) {
    pybind11_fail("eval_file not supported in PyPy3. Use eval");
}
#else
template <eval_mode mode = eval_statements>
object eval_file(str fname, object global = globals(), object local = object()) {
    if (!local) {
        local = global;
    }

    detail::ensure_builtins_in_globals(global);

    int start = 0;
    switch (mode) {
        case eval_expr:
            start = Py_eval_input;
            break;
        case eval_single_statement:
            start = Py_single_input;
            break;
        case eval_statements:
            start = Py_file_input;
            break;
        default:
            pybind11_fail("invalid evaluation mode");
    }

    int closeFile = 1;
    std::string fname_str = (std::string) fname;
    FILE *f = _Py_fopen_obj(fname.ptr(), "r");
    if (!f) {
        PyErr_Clear();
        pybind11_fail("File \"" + fname_str + "\" could not be opened!");
    }

    if (!global.contains("__file__")) {
        global["__file__"] = std::move(fname);
    }

    PyObject *result
        = PyRun_FileEx(f, fname_str.c_str(), start, global.ptr(), local.ptr(), closeFile);

    if (!result) {
        throw error_already_set();
    }
    return reinterpret_steal<object>(result);
}
#endif

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
