/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "pybind11.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <streambuf>
#include <string>
#include <utility>

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)


class pythonbuf : public std::streambuf {
private:
    using traits_type = std::streambuf::traits_type;

    const size_t buf_size;
    std::unique_ptr<char[]> d_buffer;
    object pywrite;
    object pyflush;

    int overflow(int c) override {
        if (!traits_type::eq_int_type(c, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(c);
            pbump(1);
        }
        return sync() == 0 ? traits_type::not_eof(c) : traits_type::eof();
    }




    size_t utf8_remainder() const {
        const auto rbase = std::reverse_iterator<char *>(pbase());
        const auto rpptr = std::reverse_iterator<char *>(pptr());
        auto is_ascii = [](char c) { return (static_cast<unsigned char>(c) & 0x80) == 0x00; };
        auto is_leading = [](char c) { return (static_cast<unsigned char>(c) & 0xC0) == 0xC0; };
        auto is_leading_2b = [](char c) { return static_cast<unsigned char>(c) <= 0xDF; };
        auto is_leading_3b = [](char c) { return static_cast<unsigned char>(c) <= 0xEF; };

        if (is_ascii(*rpptr)) {
            return 0;
        }


        const auto rpend = rbase - rpptr >= 3 ? rpptr + 3 : rbase;
        const auto leading = std::find_if(rpptr, rpend, is_leading);
        if (leading == rbase) {
            return 0;
        }
        const auto dist = static_cast<size_t>(leading - rpptr);
        size_t remainder = 0;

        if (dist == 0) {
            remainder = 1;
        } else if (dist == 1) {
            remainder = is_leading_2b(*leading) ? 0 : dist + 1;
        } else if (dist == 2) {
            remainder = is_leading_3b(*leading) ? 0 : dist + 1;
        }




        return remainder;
    }


    int _sync() {
        if (pbase() != pptr()) {
            gil_scoped_acquire tmp;

            auto size = static_cast<size_t>(pptr() - pbase());
            size_t remainder = utf8_remainder();

            if (size > remainder) {
                str line(pbase(), size - remainder);
                pywrite(std::move(line));
                pyflush();
            }


            if (remainder > 0) {
                std::memmove(pbase(), pptr() - remainder, remainder);
            }
            setp(pbase(), epptr());
            pbump(static_cast<int>(remainder));
        }
        return 0;
    }

    int sync() override { return _sync(); }

public:
    explicit pythonbuf(const object &pyostream, size_t buffer_size = 1024)
        : buf_size(buffer_size), d_buffer(new char[buf_size]), pywrite(pyostream.attr("write")),
          pyflush(pyostream.attr("flush")) {
        setp(d_buffer.get(), d_buffer.get() + buf_size - 1);
    }

    pythonbuf(pythonbuf &&) = default;


    ~pythonbuf() override { _sync(); }
};

PYBIND11_NAMESPACE_END(detail)


class scoped_ostream_redirect {
protected:
    std::streambuf *old;
    std::ostream &costream;
    detail::pythonbuf buffer;

public:
    explicit scoped_ostream_redirect(std::ostream &costream = std::cout,
                                     const object &pyostream
                                     = module_::import("sys").attr("stdout"))
        : costream(costream), buffer(pyostream) {
        old = costream.rdbuf(&buffer);
    }

    ~scoped_ostream_redirect() { costream.rdbuf(old); }

    scoped_ostream_redirect(const scoped_ostream_redirect &) = delete;
    scoped_ostream_redirect(scoped_ostream_redirect &&other) = default;
    scoped_ostream_redirect &operator=(const scoped_ostream_redirect &) = delete;
    scoped_ostream_redirect &operator=(scoped_ostream_redirect &&) = delete;
};


class scoped_estream_redirect : public scoped_ostream_redirect {
public:
    explicit scoped_estream_redirect(std::ostream &costream = std::cerr,
                                     const object &pyostream
                                     = module_::import("sys").attr("stderr"))
        : scoped_ostream_redirect(costream, pyostream) {}
};

PYBIND11_NAMESPACE_BEGIN(detail)


class OstreamRedirect {
    bool do_stdout_;
    bool do_stderr_;
    std::unique_ptr<scoped_ostream_redirect> redirect_stdout;
    std::unique_ptr<scoped_estream_redirect> redirect_stderr;

public:
    explicit OstreamRedirect(bool do_stdout = true, bool do_stderr = true)
        : do_stdout_(do_stdout), do_stderr_(do_stderr) {}

    void enter() {
        if (do_stdout_) {
            redirect_stdout.reset(new scoped_ostream_redirect());
        }
        if (do_stderr_) {
            redirect_stderr.reset(new scoped_estream_redirect());
        }
    }

    void exit() {
        redirect_stdout.reset();
        redirect_stderr.reset();
    }
};

PYBIND11_NAMESPACE_END(detail)


inline class_<detail::OstreamRedirect>
add_ostream_redirect(module_ m, const std::string &name = "ostream_redirect") {
    return class_<detail::OstreamRedirect>(std::move(m), name.c_str(), module_local())
        .def(init<bool, bool>(), arg("stdout") = true, arg("stderr") = true)
        .def("__enter__", &detail::OstreamRedirect::enter)
        .def("__exit__", [](detail::OstreamRedirect &self_, const args &) { self_.exit(); });
}

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
