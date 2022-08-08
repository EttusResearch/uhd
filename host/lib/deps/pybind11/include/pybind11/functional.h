/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "pybind11.h"

#include <functional>

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)

template <typename Return, typename... Args>
struct type_caster<std::function<Return(Args...)>> {
    using type = std::function<Return(Args...)>;
    using retval_type = conditional_t<std::is_same<Return, void>::value, void_type, Return>;
    using function_type = Return (*)(Args...);

public:
    bool load(handle src, bool convert) {
        if (src.is_none()) {

            if (!convert) {
                return false;
            }
            return true;
        }

        if (!isinstance<function>(src)) {
            return false;
        }

        auto func = reinterpret_borrow<function>(src);


        if (auto cfunc = func.cpp_function()) {
            auto *cfunc_self = PyCFunction_GET_SELF(cfunc.ptr());
            if (isinstance<capsule>(cfunc_self)) {
                auto c = reinterpret_borrow<capsule>(cfunc_self);
                auto *rec = (function_record *) c;

                while (rec != nullptr) {
                    if (rec->is_stateless
                        && same_type(typeid(function_type),
                                     *reinterpret_cast<const std::type_info *>(rec->data[1]))) {
                        struct capture {
                            function_type f;
                        };
                        value = ((capture *) &rec->data)->f;
                        return true;
                    }
                    rec = rec->next;
                }
            }



        }


        struct func_handle {
            function f;
#if !(defined(_MSC_VER) && _MSC_VER == 1916 && defined(PYBIND11_CPP17))

            explicit
#endif
                func_handle(function &&f_) noexcept
                : f(std::move(f_)) {
            }
            func_handle(const func_handle &f_) { operator=(f_); }
            func_handle &operator=(const func_handle &f_) {
                gil_scoped_acquire acq;
                f = f_.f;
                return *this;
            }
            ~func_handle() {
                gil_scoped_acquire acq;
                function kill_f(std::move(f));
            }
        };


        struct func_wrapper {
            func_handle hfunc;
            explicit func_wrapper(func_handle &&hf) noexcept : hfunc(std::move(hf)) {}
            Return operator()(Args... args) const {
                gil_scoped_acquire acq;

                return hfunc.f(std::forward<Args>(args)...).template cast<Return>();
            }
        };

        value = func_wrapper(func_handle(std::move(func)));
        return true;
    }

    template <typename Func>
    static handle cast(Func &&f_, return_value_policy policy, handle  ) {
        if (!f_) {
            return none().inc_ref();
        }

        auto result = f_.template target<function_type>();
        if (result) {
            return cpp_function(*result, policy).release();
        }
        return cpp_function(std::forward<Func>(f_), policy).release();
    }

    PYBIND11_TYPE_CASTER(type,
                         const_name("Callable[[") + concat(make_caster<Args>::name...)
                             + const_name("], ") + make_caster<retval_type>::name
                             + const_name("]"));
};

PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
