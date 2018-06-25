//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

//! RAII-style GIL release method
//
// To release the GIL using this method, simply instantiate this class in the
// scope that needs to release the GIL.
//
// Note that using this class assumes that threads have already been
// initialized. See also https://docs.python.org/3.5/c-api/init.html for more
// documentation on Python initialization and threads.
class scoped_gil_release
{
public:
    inline scoped_gil_release()
    {
        _thread_state = PyEval_SaveThread();
    }

    inline ~scoped_gil_release()
    {
        PyEval_RestoreThread(_thread_state);
        _thread_state = nullptr;
    }

private:
    PyThreadState* _thread_state;
};
