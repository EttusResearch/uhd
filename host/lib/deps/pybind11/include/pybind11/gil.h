/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "detail/common.h"
#include "detail/internals.h"

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)

PYBIND11_NAMESPACE_BEGIN(detail)


PyThreadState *get_thread_state_unchecked();

PYBIND11_NAMESPACE_END(detail)

#if defined(WITH_THREAD) && !defined(PYPY_VERSION)



class gil_scoped_acquire {
public:
    PYBIND11_NOINLINE gil_scoped_acquire() {
        auto &internals = detail::get_internals();
        tstate = (PyThreadState *) PYBIND11_TLS_GET_VALUE(internals.tstate);

        if (!tstate) {

            tstate = PyGILState_GetThisThreadState();
        }

        if (!tstate) {
            tstate = PyThreadState_New(internals.istate);
#    if defined(PYBIND11_DETAILED_ERROR_MESSAGES)
            if (!tstate) {
                pybind11_fail("scoped_acquire: could not create thread state!");
            }
#    endif
            tstate->gilstate_counter = 0;
            PYBIND11_TLS_REPLACE_VALUE(internals.tstate, tstate);
        } else {
            release = detail::get_thread_state_unchecked() != tstate;
        }

        if (release) {
            PyEval_AcquireThread(tstate);
        }

        inc_ref();
    }

    void inc_ref() { ++tstate->gilstate_counter; }

    PYBIND11_NOINLINE void dec_ref() {
        --tstate->gilstate_counter;
#    if defined(PYBIND11_DETAILED_ERROR_MESSAGES)
        if (detail::get_thread_state_unchecked() != tstate) {
            pybind11_fail("scoped_acquire::dec_ref(): thread state must be current!");
        }
        if (tstate->gilstate_counter < 0) {
            pybind11_fail("scoped_acquire::dec_ref(): reference count underflow!");
        }
#    endif
        if (tstate->gilstate_counter == 0) {
#    if defined(PYBIND11_DETAILED_ERROR_MESSAGES)
            if (!release) {
                pybind11_fail("scoped_acquire::dec_ref(): internal error!");
            }
#    endif
            PyThreadState_Clear(tstate);
            if (active) {
                PyThreadState_DeleteCurrent();
            }
            PYBIND11_TLS_DELETE_VALUE(detail::get_internals().tstate);
            release = false;
        }
    }






    PYBIND11_NOINLINE void disarm() { active = false; }

    PYBIND11_NOINLINE ~gil_scoped_acquire() {
        dec_ref();
        if (release) {
            PyEval_SaveThread();
        }
    }

private:
    PyThreadState *tstate = nullptr;
    bool release = true;
    bool active = true;
};

class gil_scoped_release {
public:
    explicit gil_scoped_release(bool disassoc = false) : disassoc(disassoc) {



        auto &internals = detail::get_internals();

        tstate = PyEval_SaveThread();
        if (disassoc) {


            auto key = internals.tstate;
            PYBIND11_TLS_DELETE_VALUE(key);
        }
    }






    PYBIND11_NOINLINE void disarm() { active = false; }

    ~gil_scoped_release() {
        if (!tstate) {
            return;
        }

        if (active) {
            PyEval_RestoreThread(tstate);
        }
        if (disassoc) {


            auto key = detail::get_internals().tstate;
            PYBIND11_TLS_REPLACE_VALUE(key, tstate);
        }
    }

private:
    PyThreadState *tstate;
    bool disassoc;
    bool active = true;
};
#elif defined(PYPY_VERSION)
class gil_scoped_acquire {
    PyGILState_STATE state;

public:
    gil_scoped_acquire() { state = PyGILState_Ensure(); }
    ~gil_scoped_acquire() { PyGILState_Release(state); }
    void disarm() {}
};

class gil_scoped_release {
    PyThreadState *state;

public:
    gil_scoped_release() { state = PyEval_SaveThread(); }
    ~gil_scoped_release() { PyEval_RestoreThread(state); }
    void disarm() {}
};
#else
class gil_scoped_acquire {
    void disarm() {}
};
class gil_scoped_release {
    void disarm() {}
};
#endif

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
