/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "class.h"

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)

template <>
class type_caster<value_and_holder> {
public:
    bool load(handle h, bool) {
        value = reinterpret_cast<value_and_holder *>(h.ptr());
        return true;
    }

    template <typename>
    using cast_op_type = value_and_holder &;
    explicit operator value_and_holder &() { return *value; }
    static constexpr auto name = const_name<value_and_holder>();

private:
    value_and_holder *value = nullptr;
};

PYBIND11_NAMESPACE_BEGIN(initimpl)

inline void no_nullptr(void *ptr) {
    if (!ptr) {
        throw type_error("pybind11::init(): factory function returned nullptr");
    }
}


template <typename Class>
using Cpp = typename Class::type;
template <typename Class>
using Alias = typename Class::type_alias;
template <typename Class>
using Holder = typename Class::holder_type;

template <typename Class>
using is_alias_constructible = std::is_constructible<Alias<Class>, Cpp<Class> &&>;


template <typename Class, enable_if_t<Class::has_alias, int> = 0>
bool is_alias(Cpp<Class> *ptr) {
    return dynamic_cast<Alias<Class> *>(ptr) != nullptr;
}

template <typename  >
constexpr bool is_alias(void *) {
    return false;
}






template <typename Class,
          typename... Args,
          detail::enable_if_t<std::is_constructible<Class, Args...>::value, int> = 0>
inline Class *construct_or_initialize(Args &&...args) {
    return new Class(std::forward<Args>(args)...);
}
template <typename Class,
          typename... Args,
          detail::enable_if_t<!std::is_constructible<Class, Args...>::value, int> = 0>
inline Class *construct_or_initialize(Args &&...args) {
    return new Class{std::forward<Args>(args)...};
}






template <typename Class>
void construct_alias_from_cpp(std::true_type  ,
                              value_and_holder &v_h,
                              Cpp<Class> &&base) {
    v_h.value_ptr() = new Alias<Class>(std::move(base));
}
template <typename Class>
[[noreturn]] void construct_alias_from_cpp(std::false_type  ,
                                           value_and_holder &,
                                           Cpp<Class> &&) {
    throw type_error("pybind11::init(): unable to convert returned instance to required "
                     "alias class: no `Alias<Class>(Class &&)` constructor available");
}



template <typename Class>
void construct(...) {
    static_assert(!std::is_same<Class, Class>::value  ,
                  "pybind11::init(): init function must return a compatible pointer, "
                  "holder, or value");
}





template <typename Class>
void construct(value_and_holder &v_h, Cpp<Class> *ptr, bool need_alias) {
    PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(need_alias);
    no_nullptr(ptr);
    if (PYBIND11_SILENCE_MSVC_C4127(Class::has_alias) && need_alias && !is_alias<Class>(ptr)) {








        v_h.value_ptr() = ptr;
        v_h.set_instance_registered(true);
        v_h.type->init_instance(v_h.inst, nullptr);
        Holder<Class> temp_holder(std::move(v_h.holder<Holder<Class>>()));
        v_h.type->dealloc(v_h);
        v_h.set_instance_registered(false);

        construct_alias_from_cpp<Class>(is_alias_constructible<Class>{}, v_h, std::move(*ptr));
    } else {

        v_h.value_ptr() = ptr;
    }
}



template <typename Class, enable_if_t<Class::has_alias, int> = 0>
void construct(value_and_holder &v_h, Alias<Class> *alias_ptr, bool) {
    no_nullptr(alias_ptr);
    v_h.value_ptr() = static_cast<Cpp<Class> *>(alias_ptr);
}





template <typename Class>
void construct(value_and_holder &v_h, Holder<Class> holder, bool need_alias) {
    PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(need_alias);
    auto *ptr = holder_helper<Holder<Class>>::get(holder);
    no_nullptr(ptr);

    if (PYBIND11_SILENCE_MSVC_C4127(Class::has_alias) && need_alias && !is_alias<Class>(ptr)) {
        throw type_error("pybind11::init(): construction failed: returned holder-wrapped instance "
                         "is not an alias instance");
    }

    v_h.value_ptr() = ptr;
    v_h.type->init_instance(v_h.inst, &holder);
}





template <typename Class>
void construct(value_and_holder &v_h, Cpp<Class> &&result, bool need_alias) {
    PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(need_alias);
    static_assert(std::is_move_constructible<Cpp<Class>>::value,
                  "pybind11::init() return-by-value factory function requires a movable class");
    if (PYBIND11_SILENCE_MSVC_C4127(Class::has_alias) && need_alias) {
        construct_alias_from_cpp<Class>(is_alias_constructible<Class>{}, v_h, std::move(result));
    } else {
        v_h.value_ptr() = new Cpp<Class>(std::move(result));
    }
}




template <typename Class>
void construct(value_and_holder &v_h, Alias<Class> &&result, bool) {
    static_assert(
        std::is_move_constructible<Alias<Class>>::value,
        "pybind11::init() return-by-alias-value factory function requires a movable alias class");
    v_h.value_ptr() = new Alias<Class>(std::move(result));
}


template <typename... Args>
struct constructor {
    template <typename Class, typename... Extra, enable_if_t<!Class::has_alias, int> = 0>
    static void execute(Class &cl, const Extra &...extra) {
        cl.def(
            "__init__",
            [](value_and_holder &v_h, Args... args) {
                v_h.value_ptr() = construct_or_initialize<Cpp<Class>>(std::forward<Args>(args)...);
            },
            is_new_style_constructor(),
            extra...);
    }

    template <typename Class,
              typename... Extra,
              enable_if_t<Class::has_alias && std::is_constructible<Cpp<Class>, Args...>::value,
                          int> = 0>
    static void execute(Class &cl, const Extra &...extra) {
        cl.def(
            "__init__",
            [](value_and_holder &v_h, Args... args) {
                if (Py_TYPE(v_h.inst) == v_h.type->type) {
                    v_h.value_ptr()
                        = construct_or_initialize<Cpp<Class>>(std::forward<Args>(args)...);
                } else {
                    v_h.value_ptr()
                        = construct_or_initialize<Alias<Class>>(std::forward<Args>(args)...);
                }
            },
            is_new_style_constructor(),
            extra...);
    }

    template <typename Class,
              typename... Extra,
              enable_if_t<Class::has_alias && !std::is_constructible<Cpp<Class>, Args...>::value,
                          int> = 0>
    static void execute(Class &cl, const Extra &...extra) {
        cl.def(
            "__init__",
            [](value_and_holder &v_h, Args... args) {
                v_h.value_ptr()
                    = construct_or_initialize<Alias<Class>>(std::forward<Args>(args)...);
            },
            is_new_style_constructor(),
            extra...);
    }
};


template <typename... Args>
struct alias_constructor {
    template <typename Class,
              typename... Extra,
              enable_if_t<Class::has_alias && std::is_constructible<Alias<Class>, Args...>::value,
                          int> = 0>
    static void execute(Class &cl, const Extra &...extra) {
        cl.def(
            "__init__",
            [](value_and_holder &v_h, Args... args) {
                v_h.value_ptr()
                    = construct_or_initialize<Alias<Class>>(std::forward<Args>(args)...);
            },
            is_new_style_constructor(),
            extra...);
    }
};


template <typename CFunc,
          typename AFunc = void_type (*)(),
          typename = function_signature_t<CFunc>,
          typename = function_signature_t<AFunc>>
struct factory;


template <typename Func, typename Return, typename... Args>
struct factory<Func, void_type (*)(), Return(Args...)> {
    remove_reference_t<Func> class_factory;


    factory(Func &&f) : class_factory(std::forward<Func>(f)) {}






    template <typename Class, typename... Extra>
    void execute(Class &cl, const Extra &...extra) && {
#if defined(PYBIND11_CPP14)
        cl.def(
            "__init__",
            [func = std::move(class_factory)]
#else
        auto &func = class_factory;
        cl.def(
            "__init__",
            [func]
#endif
            (value_and_holder &v_h, Args... args) {
                construct<Class>(
                    v_h, func(std::forward<Args>(args)...), Py_TYPE(v_h.inst) != v_h.type->type);
            },
            is_new_style_constructor(),
            extra...);
    }
};


template <typename CFunc,
          typename AFunc,
          typename CReturn,
          typename... CArgs,
          typename AReturn,
          typename... AArgs>
struct factory<CFunc, AFunc, CReturn(CArgs...), AReturn(AArgs...)> {
    static_assert(sizeof...(CArgs) == sizeof...(AArgs),
                  "pybind11::init(class_factory, alias_factory): class and alias factories "
                  "must have identical argument signatures");
    static_assert(all_of<std::is_same<CArgs, AArgs>...>::value,
                  "pybind11::init(class_factory, alias_factory): class and alias factories "
                  "must have identical argument signatures");

    remove_reference_t<CFunc> class_factory;
    remove_reference_t<AFunc> alias_factory;

    factory(CFunc &&c, AFunc &&a)
        : class_factory(std::forward<CFunc>(c)), alias_factory(std::forward<AFunc>(a)) {}



    template <typename Class, typename... Extra>
    void execute(Class &cl, const Extra &...extra) && {
        static_assert(Class::has_alias,
                      "The two-argument version of `py::init()` can "
                      "only be used if the class has an alias");
#if defined(PYBIND11_CPP14)
        cl.def(
            "__init__",
            [class_func = std::move(class_factory), alias_func = std::move(alias_factory)]
#else
        auto &class_func = class_factory;
        auto &alias_func = alias_factory;
        cl.def(
            "__init__",
            [class_func, alias_func]
#endif
            (value_and_holder &v_h, CArgs... args) {
                if (Py_TYPE(v_h.inst) == v_h.type->type) {


                    construct<Class>(v_h, class_func(std::forward<CArgs>(args)...), false);
                } else {
                    construct<Class>(v_h, alias_func(std::forward<CArgs>(args)...), true);
                }
            },
            is_new_style_constructor(),
            extra...);
    }
};


template <typename Class, typename T>
void setstate(value_and_holder &v_h, T &&result, bool need_alias) {
    construct<Class>(v_h, std::forward<T>(result), need_alias);
}


template <typename Class,
          typename T,
          typename O,
          enable_if_t<std::is_convertible<O, handle>::value, int> = 0>
void setstate(value_and_holder &v_h, std::pair<T, O> &&result, bool need_alias) {
    construct<Class>(v_h, std::move(result.first), need_alias);
    auto d = handle(result.second);
    if (PyDict_Check(d.ptr()) && PyDict_Size(d.ptr()) == 0) {


        return;
    }
    setattr((PyObject *) v_h.inst, "__dict__", d);
}


template <typename Get,
          typename Set,
          typename = function_signature_t<Get>,
          typename = function_signature_t<Set>>
struct pickle_factory;

template <typename Get,
          typename Set,
          typename RetState,
          typename Self,
          typename NewInstance,
          typename ArgState>
struct pickle_factory<Get, Set, RetState(Self), NewInstance(ArgState)> {
    static_assert(std::is_same<intrinsic_t<RetState>, intrinsic_t<ArgState>>::value,
                  "The type returned by `__getstate__` must be the same "
                  "as the argument accepted by `__setstate__`");

    remove_reference_t<Get> get;
    remove_reference_t<Set> set;

    pickle_factory(Get get, Set set) : get(std::forward<Get>(get)), set(std::forward<Set>(set)) {}

    template <typename Class, typename... Extra>
    void execute(Class &cl, const Extra &...extra) && {
        cl.def("__getstate__", std::move(get));

#if defined(PYBIND11_CPP14)
        cl.def(
            "__setstate__",
            [func = std::move(set)]
#else
        auto &func = set;
        cl.def(
            "__setstate__",
            [func]
#endif
            (value_and_holder &v_h, ArgState state) {
                setstate<Class>(
                    v_h, func(std::forward<ArgState>(state)), Py_TYPE(v_h.inst) != v_h.type->type);
            },
            is_new_style_constructor(),
            extra...);
    }
};

PYBIND11_NAMESPACE_END(initimpl)
PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
