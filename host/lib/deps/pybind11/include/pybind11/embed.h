/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "pybind11.h"
#include "eval.h"

#include <memory>
#include <vector>

#if defined(PYPY_VERSION)
#    error Embedding the interpreter is not supported with PyPy
#endif

#define PYBIND11_EMBEDDED_MODULE_IMPL(name)                                                       \
    extern "C" PyObject *pybind11_init_impl_##name();                                             \
    extern "C" PyObject *pybind11_init_impl_##name() { return pybind11_init_wrapper_##name(); }


#define PYBIND11_EMBEDDED_MODULE(name, variable)                                                  \
    static ::pybind11::module_::module_def PYBIND11_CONCAT(pybind11_module_def_, name);           \
    static void PYBIND11_CONCAT(pybind11_init_, name)(::pybind11::module_ &);                     \
    static PyObject PYBIND11_CONCAT(*pybind11_init_wrapper_, name)() {                            \
        auto m = ::pybind11::module_::create_extension_module(                                    \
            PYBIND11_TOSTRING(name), nullptr, &PYBIND11_CONCAT(pybind11_module_def_, name));      \
        try {                                                                                     \
            PYBIND11_CONCAT(pybind11_init_, name)(m);                                             \
            return m.ptr();                                                                       \
        }                                                                                         \
        PYBIND11_CATCH_INIT_EXCEPTIONS                                                            \
    }                                                                                             \
    PYBIND11_EMBEDDED_MODULE_IMPL(name)                                                           \
    ::pybind11::detail::embedded_module PYBIND11_CONCAT(pybind11_module_, name)(                  \
        PYBIND11_TOSTRING(name), PYBIND11_CONCAT(pybind11_init_impl_, name));                     \
    void PYBIND11_CONCAT(pybind11_init_, name)(::pybind11::module_                                \
                                               & variable)

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)


struct embedded_module {
    using init_t = PyObject *(*) ();
    embedded_module(const char *name, init_t init) {
        if (Py_IsInitialized() != 0) {
            pybind11_fail("Can't add new modules after the interpreter has been initialized");
        }

        auto result = PyImport_AppendInittab(name, init);
        if (result == -1) {
            pybind11_fail("Insufficient memory to add a new module");
        }
    }
};

struct wide_char_arg_deleter {
    void operator()(wchar_t *ptr) const {

        PyMem_RawFree(ptr);
    }
};

inline wchar_t *widen_chars(const char *safe_arg) {
    wchar_t *widened_arg = Py_DecodeLocale(safe_arg, nullptr);
    return widened_arg;
}

PYBIND11_NAMESPACE_END(detail)


inline void initialize_interpreter(bool init_signal_handlers = true,
                                   int argc = 0,
                                   const char *const *argv = nullptr,
                                   bool add_program_dir_to_path = true) {
    if (Py_IsInitialized() != 0) {
        pybind11_fail("The interpreter is already running");
    }

#if PY_VERSION_HEX < 0x030B0000

    Py_InitializeEx(init_signal_handlers ? 1 : 0);



    bool special_case = (argv == nullptr || argc <= 0);

    const char *const empty_argv[]{"\0"};
    const char *const *safe_argv = special_case ? empty_argv : argv;
    if (special_case) {
        argc = 1;
    }

    auto argv_size = static_cast<size_t>(argc);

    std::unique_ptr<wchar_t *[]> widened_argv(new wchar_t *[argv_size]);
    std::vector<std::unique_ptr<wchar_t[], detail::wide_char_arg_deleter>> widened_argv_entries;
    widened_argv_entries.reserve(argv_size);
    for (size_t ii = 0; ii < argv_size; ++ii) {
        widened_argv_entries.emplace_back(detail::widen_chars(safe_argv[ii]));
        if (!widened_argv_entries.back()) {


            return;
        }
        widened_argv[ii] = widened_argv_entries.back().get();
    }

    auto *pysys_argv = widened_argv.get();

    PySys_SetArgvEx(argc, pysys_argv, static_cast<int>(add_program_dir_to_path));
#else
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    config.install_signal_handlers = init_signal_handlers ? 1 : 0;

    PyStatus status = PyConfig_SetBytesArgv(&config, argc, const_cast<char *const *>(argv));
    if (PyStatus_Exception(status)) {


        PyConfig_Clear(&config);
        throw std::runtime_error(PyStatus_IsError(status) ? status.err_msg
                                                          : "Failed to prepare CPython");
    }
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        throw std::runtime_error(PyStatus_IsError(status) ? status.err_msg
                                                          : "Failed to init CPython");
    }
    if (add_program_dir_to_path) {
        PyRun_SimpleString("import sys, os.path; "
                           "sys.path.insert(0, "
                           "os.path.abspath(os.path.dirname(sys.argv[0])) "
                           "if sys.argv and os.path.exists(sys.argv[0]) else '')");
    }
#endif
}


inline void finalize_interpreter() {
    handle builtins(PyEval_GetBuiltins());
    const char *id = PYBIND11_INTERNALS_ID;




    detail::internals **internals_ptr_ptr = detail::get_internals_pp();

    if (builtins.contains(id) && isinstance<capsule>(builtins[id])) {
        internals_ptr_ptr = capsule(builtins[id]);
    }


    detail::get_local_internals().registered_types_cpp.clear();
    detail::get_local_internals().registered_exception_translators.clear();

    Py_Finalize();

    if (internals_ptr_ptr) {
        delete *internals_ptr_ptr;
        *internals_ptr_ptr = nullptr;
    }
}


class scoped_interpreter {
public:
    explicit scoped_interpreter(bool init_signal_handlers = true,
                                int argc = 0,
                                const char *const *argv = nullptr,
                                bool add_program_dir_to_path = true) {
        initialize_interpreter(init_signal_handlers, argc, argv, add_program_dir_to_path);
    }

    scoped_interpreter(const scoped_interpreter &) = delete;
    scoped_interpreter(scoped_interpreter &&other) noexcept { other.is_valid = false; }
    scoped_interpreter &operator=(const scoped_interpreter &) = delete;
    scoped_interpreter &operator=(scoped_interpreter &&) = delete;

    ~scoped_interpreter() {
        if (is_valid) {
            finalize_interpreter();
        }
    }

private:
    bool is_valid = true;
};

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
