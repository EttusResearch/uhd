/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/




#pragma once

#include "pybind11.h"

#include <streambuf>
#include <ostream>
#include <string>
#include <memory>
#include <iostream>

NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
NAMESPACE_BEGIN(detail)


class pythonbuf : public std::streambuf {
private:
    using traits_type = std::streambuf::traits_type;

    char d_buffer[1024];
    object pywrite;
    object pyflush;

    int overflow(int c) {
        if (!traits_type::eq_int_type(c, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(c);
            pbump(1);
        }
        return sync() == 0 ? traits_type::not_eof(c) : traits_type::eof();
    }

    int sync() {
        if (pbase() != pptr()) {

            str line(pbase(), static_cast<size_t>(pptr() - pbase()));

            pywrite(line);
            pyflush();

            setp(pbase(), epptr());
        }
        return 0;
    }

public:
    pythonbuf(object pyostream)
        : pywrite(pyostream.attr("write")),
          pyflush(pyostream.attr("flush")) {
        setp(d_buffer, d_buffer + sizeof(d_buffer) - 1);
    }


    ~pythonbuf() {
        sync();
    }
};

NAMESPACE_END(detail)



class scoped_ostream_redirect {
protected:
    std::streambuf *old;
    std::ostream &costream;
    detail::pythonbuf buffer;

public:
    scoped_ostream_redirect(
            std::ostream &costream = std::cout,
            object pyostream = module::import("sys").attr("stdout"))
        : costream(costream), buffer(pyostream) {
        old = costream.rdbuf(&buffer);
    }

    ~scoped_ostream_redirect() {
        costream.rdbuf(old);
    }

    scoped_ostream_redirect(const scoped_ostream_redirect &) = delete;
    scoped_ostream_redirect(scoped_ostream_redirect &&other) = default;
    scoped_ostream_redirect &operator=(const scoped_ostream_redirect &) = delete;
    scoped_ostream_redirect &operator=(scoped_ostream_redirect &&) = delete;
};



class scoped_estream_redirect : public scoped_ostream_redirect {
public:
    scoped_estream_redirect(
            std::ostream &costream = std::cerr,
            object pyostream = module::import("sys").attr("stderr"))
        : scoped_ostream_redirect(costream,pyostream) {}
};


NAMESPACE_BEGIN(detail)


class OstreamRedirect {
    bool do_stdout_;
    bool do_stderr_;
    std::unique_ptr<scoped_ostream_redirect> redirect_stdout;
    std::unique_ptr<scoped_estream_redirect> redirect_stderr;

public:
    OstreamRedirect(bool do_stdout = true, bool do_stderr = true)
        : do_stdout_(do_stdout), do_stderr_(do_stderr) {}

    void enter() {
        if (do_stdout_)
            redirect_stdout.reset(new scoped_ostream_redirect());
        if (do_stderr_)
            redirect_stderr.reset(new scoped_estream_redirect());
    }

    void exit() {
        redirect_stdout.reset();
        redirect_stderr.reset();
    }
};

NAMESPACE_END(detail)


inline class_<detail::OstreamRedirect> add_ostream_redirect(module m, std::string name = "ostream_redirect") {
    return class_<detail::OstreamRedirect>(m, name.c_str(), module_local())
        .def(init<bool,bool>(), arg("stdout")=true, arg("stderr")=true)
        .def("__enter__", &detail::OstreamRedirect::enter)
        .def("__exit__", [](detail::OstreamRedirect &self_, args) { self_.exit(); });
}

NAMESPACE_END(PYBIND11_NAMESPACE)
