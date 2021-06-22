/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "pybind11.h"
#include "eval.h"

#if defined(PYPY_VERSION)
#  error Embedding the interpreter is not supported with PyPy
#endif

#if PY_MAJOR_VERSION >= 3
#  define PYBIND11_EMBEDDED_MODULE_IMPL(name)            \
      extern "C" PyObject *pybind11_init_impl_##name();  \
      extern "C" PyObject *pybind11_init_impl_##name() { \
          return pybind11_init_wrapper_##name();         \
      }
#else
#  define PYBIND11_EMBEDDED_MODULE_IMPL(name)            \
      extern "C" void pybind11_init_impl_##name();       \
      extern "C" void pybind11_init_impl_##name() {      \
          pybind11_init_wrapper_##name();                \
      }
#endif


#define PYBIND11_EMBEDDED_MODULE(name, variable)                                \
    static ::pybind11::module_::module_def                                      \
        PYBIND11_CONCAT(pybind11_module_def_, name);                            \
    static void PYBIND11_CONCAT(pybind11_init_, name)(::pybind11::module_ &);   \
    static PyObject PYBIND11_CONCAT(*pybind11_init_wrapper_, name)() {          \
        auto m = ::pybind11::module_::create_extension_module(                  \
            PYBIND11_TOSTRING(name), nullptr,                                   \
            &PYBIND11_CONCAT(pybind11_module_def_, name));                      \
        try {                                                                   \
            PYBIND11_CONCAT(pybind11_init_, name)(m);                           \
            return m.ptr();                                                     \
        } PYBIND11_CATCH_INIT_EXCEPTIONS                                        \
    }                                                                           \
    PYBIND11_EMBEDDED_MODULE_IMPL(name)                                         \
    ::pybind11::detail::embedded_module PYBIND11_CONCAT(pybind11_module_, name) \
                              (PYBIND11_TOSTRING(name),                         \
                               PYBIND11_CONCAT(pybind11_init_impl_, name));     \
    void PYBIND11_CONCAT(pybind11_init_, name)(::pybind11::module_ &variable)


PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)


struct embedded_module {
#if PY_MAJOR_VERSION >= 3
    using init_t = PyObject *(*)();
#else
    using init_t = void (*)();
#endif
    embedded_module(const char *name, init_t init) {
        if (Py_IsInitialized())
            pybind11_fail("Can't add new modules after the interpreter has been initialized");

        auto result = PyImport_AppendInittab(name, init);
        if (result == -1)
            pybind11_fail("Insufficient memory to add a new module");
    }
};

PYBIND11_NAMESPACE_END(detail)


inline void initialize_interpreter(bool init_signal_handlers = true) {
    if (Py_IsInitialized())
        pybind11_fail("The interpreter is already running");

    Py_InitializeEx(init_signal_handlers ? 1 : 0);


    module_::import("sys").attr("path").cast<list>().append(".");
}


inline void finalize_interpreter() {
    handle builtins(PyEval_GetBuiltins());
    const char *id = PYBIND11_INTERNALS_ID;




    detail::internals **internals_ptr_ptr = detail::get_internals_pp();

    if (builtins.contains(id) && isinstance<capsule>(builtins[id]))
        internals_ptr_ptr = capsule(builtins[id]);

    Py_Finalize();

    if (internals_ptr_ptr) {
        delete *internals_ptr_ptr;
        *internals_ptr_ptr = nullptr;
    }
}


class scoped_interpreter {
public:
    scoped_interpreter(bool init_signal_handlers = true) {
        initialize_interpreter(init_signal_handlers);
    }

    scoped_interpreter(const scoped_interpreter &) = delete;
    scoped_interpreter(scoped_interpreter &&other) noexcept { other.is_valid = false; }
    scoped_interpreter &operator=(const scoped_interpreter &) = delete;
    scoped_interpreter &operator=(scoped_interpreter &&) = delete;

    ~scoped_interpreter() {
        if (is_valid)
            finalize_interpreter();
    }

private:
    bool is_valid = true;
};

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
