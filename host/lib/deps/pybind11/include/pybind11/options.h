/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "detail/common.h"

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)

class options {
public:

    options() : previous_state(global_state()) {}


    options(const options &) = delete;
    options &operator=(const options &) = delete;


    ~options() { global_state() = previous_state; }



    options &disable_user_defined_docstrings() & {
        global_state().show_user_defined_docstrings = false;
        return *this;
    }

    options &enable_user_defined_docstrings() & {
        global_state().show_user_defined_docstrings = true;
        return *this;
    }

    options &disable_function_signatures() & {
        global_state().show_function_signatures = false;
        return *this;
    }

    options &enable_function_signatures() & {
        global_state().show_function_signatures = true;
        return *this;
    }



    static bool show_user_defined_docstrings() {
        return global_state().show_user_defined_docstrings;
    }

    static bool show_function_signatures() { return global_state().show_function_signatures; }


    void *operator new(size_t) = delete;

private:
    struct state {
        bool show_user_defined_docstrings = true;
        bool show_function_signatures = true;

    };

    static state &global_state() {
        static state instance;
        return instance;
    }

    state previous_state;
};

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
