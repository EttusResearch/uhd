/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "detail/common.h"
#include "cast.h"

#include <functional>

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)





struct is_method {
    handle class_;
    explicit is_method(const handle &c) : class_(c) {}
};


struct is_operator {};


struct is_final {};


struct scope {
    handle value;
    explicit scope(const handle &s) : value(s) {}
};


struct doc {
    const char *value;
    explicit doc(const char *value) : value(value) {}
};


struct name {
    const char *value;
    explicit name(const char *value) : value(value) {}
};


struct sibling {
    handle value;
    explicit sibling(const handle &value) : value(value.ptr()) {}
};


template <typename T>
struct base {

    PYBIND11_DEPRECATED(
        "base<T>() was deprecated in favor of specifying 'T' as a template argument to class_")
    base() = default;
};


template <size_t Nurse, size_t Patient>
struct keep_alive {};


struct multiple_inheritance {};


struct dynamic_attr {};


struct buffer_protocol {};


struct metaclass {
    handle value;

    PYBIND11_DEPRECATED("py::metaclass() is no longer required. It's turned on by default now.")
    metaclass() = default;


    explicit metaclass(handle value) : value(value) {}
};










struct custom_type_setup {
    using callback = std::function<void(PyHeapTypeObject *heap_type)>;

    explicit custom_type_setup(callback value) : value(std::move(value)) {}

    callback value;
};


struct module_local {
    const bool value;
    constexpr explicit module_local(bool v = true) : value(v) {}
};


struct arithmetic {};


struct prepend {};


template <typename... Ts>
struct call_guard;

template <>
struct call_guard<> {
    using type = detail::void_type;
};

template <typename T>
struct call_guard<T> {
    static_assert(std::is_default_constructible<T>::value,
                  "The guard type must be default constructible");

    using type = T;
};

template <typename T, typename... Ts>
struct call_guard<T, Ts...> {
    struct type {
        T guard{};
        typename call_guard<Ts...>::type next{};
    };
};



PYBIND11_NAMESPACE_BEGIN(detail)

enum op_id : int;
enum op_type : int;
struct undefined_t;
template <op_id id, op_type ot, typename L = undefined_t, typename R = undefined_t>
struct op_;
void keep_alive_impl(size_t Nurse, size_t Patient, function_call &call, handle ret);


struct argument_record {
    const char *name;
    const char *descr;
    handle value;
    bool convert : 1;
    bool none : 1;

    argument_record(const char *name, const char *descr, handle value, bool convert, bool none)
        : name(name), descr(descr), value(value), convert(convert), none(none) {}
};



struct function_record {
    function_record()
        : is_constructor(false), is_new_style_constructor(false), is_stateless(false),
          is_operator(false), is_method(false), has_args(false), has_kwargs(false),
          prepend(false) {}


    char *name = nullptr;


    char *doc = nullptr;


    char *signature = nullptr;


    std::vector<argument_record> args;


    handle (*impl)(function_call &) = nullptr;


    void *data[3] = {};


    void (*free_data)(function_record *ptr) = nullptr;


    return_value_policy policy = return_value_policy::automatic;


    bool is_constructor : 1;


    bool is_new_style_constructor : 1;


    bool is_stateless : 1;


    bool is_operator : 1;


    bool is_method : 1;


    bool has_args : 1;


    bool has_kwargs : 1;


    bool prepend : 1;


    std::uint16_t nargs;



    std::uint16_t nargs_pos = 0;


    std::uint16_t nargs_pos_only = 0;


    PyMethodDef *def = nullptr;


    handle scope;


    handle sibling;


    function_record *next = nullptr;
};


struct type_record {
    PYBIND11_NOINLINE type_record()
        : multiple_inheritance(false), dynamic_attr(false), buffer_protocol(false),
          default_holder(true), module_local(false), is_final(false) {}


    handle scope;


    const char *name = nullptr;


    const std::type_info *type = nullptr;


    size_t type_size = 0;


    size_t type_align = 0;


    size_t holder_size = 0;


    void *(*operator_new)(size_t) = nullptr;


    void (*init_instance)(instance *, const void *) = nullptr;


    void (*dealloc)(detail::value_and_holder &) = nullptr;


    list bases;


    const char *doc = nullptr;


    handle metaclass;


    custom_type_setup::callback custom_type_setup_callback;


    bool multiple_inheritance : 1;


    bool dynamic_attr : 1;


    bool buffer_protocol : 1;


    bool default_holder : 1;


    bool module_local : 1;


    bool is_final : 1;

    PYBIND11_NOINLINE void add_base(const std::type_info &base, void *(*caster)(void *) ) {
        auto *base_info = detail::get_type_info(base, false);
        if (!base_info) {
            std::string tname(base.name());
            detail::clean_type_id(tname);
            pybind11_fail("generic_type: type \"" + std::string(name)
                          + "\" referenced unknown base type \"" + tname + "\"");
        }

        if (default_holder != base_info->default_holder) {
            std::string tname(base.name());
            detail::clean_type_id(tname);
            pybind11_fail("generic_type: type \"" + std::string(name) + "\" "
                          + (default_holder ? "does not have" : "has")
                          + " a non-default holder type while its base \"" + tname + "\" "
                          + (base_info->default_holder ? "does not" : "does"));
        }

        bases.append((PyObject *) base_info->type);

#if PY_VERSION_HEX < 0x030B0000
        dynamic_attr |= base_info->type->tp_dictoffset != 0;
#else
        dynamic_attr |= (base_info->type->tp_flags & Py_TPFLAGS_MANAGED_DICT) != 0;
#endif

        if (caster) {
            base_info->implicit_casts.emplace_back(type, caster);
        }
    }
};

inline function_call::function_call(const function_record &f, handle p) : func(f), parent(p) {
    args.reserve(f.nargs);
    args_convert.reserve(f.nargs);
}


struct is_new_style_constructor {};


template <typename T, typename SFINAE = void>
struct process_attribute;

template <typename T>
struct process_attribute_default {

    static void init(const T &, function_record *) {}
    static void init(const T &, type_record *) {}
    static void precall(function_call &) {}
    static void postcall(function_call &, handle) {}
};


template <>
struct process_attribute<name> : process_attribute_default<name> {
    static void init(const name &n, function_record *r) { r->name = const_cast<char *>(n.value); }
};


template <>
struct process_attribute<doc> : process_attribute_default<doc> {
    static void init(const doc &n, function_record *r) { r->doc = const_cast<char *>(n.value); }
};


template <>
struct process_attribute<const char *> : process_attribute_default<const char *> {
    static void init(const char *d, function_record *r) { r->doc = const_cast<char *>(d); }
    static void init(const char *d, type_record *r) { r->doc = const_cast<char *>(d); }
};
template <>
struct process_attribute<char *> : process_attribute<const char *> {};


template <>
struct process_attribute<return_value_policy> : process_attribute_default<return_value_policy> {
    static void init(const return_value_policy &p, function_record *r) { r->policy = p; }
};



template <>
struct process_attribute<sibling> : process_attribute_default<sibling> {
    static void init(const sibling &s, function_record *r) { r->sibling = s.value; }
};


template <>
struct process_attribute<is_method> : process_attribute_default<is_method> {
    static void init(const is_method &s, function_record *r) {
        r->is_method = true;
        r->scope = s.class_;
    }
};


template <>
struct process_attribute<scope> : process_attribute_default<scope> {
    static void init(const scope &s, function_record *r) { r->scope = s.value; }
};


template <>
struct process_attribute<is_operator> : process_attribute_default<is_operator> {
    static void init(const is_operator &, function_record *r) { r->is_operator = true; }
};

template <>
struct process_attribute<is_new_style_constructor>
    : process_attribute_default<is_new_style_constructor> {
    static void init(const is_new_style_constructor &, function_record *r) {
        r->is_new_style_constructor = true;
    }
};

inline void check_kw_only_arg(const arg &a, function_record *r) {
    if (r->args.size() > r->nargs_pos && (!a.name || a.name[0] == '\0')) {
        pybind11_fail("arg(): cannot specify an unnamed argument after a kw_only() annotation or "
                      "args() argument");
    }
}

inline void append_self_arg_if_needed(function_record *r) {
    if (r->is_method && r->args.empty()) {
        r->args.emplace_back("self", nullptr, handle(),  true,  false);
    }
}


template <>
struct process_attribute<arg> : process_attribute_default<arg> {
    static void init(const arg &a, function_record *r) {
        append_self_arg_if_needed(r);
        r->args.emplace_back(a.name, nullptr, handle(), !a.flag_noconvert, a.flag_none);

        check_kw_only_arg(a, r);
    }
};


template <>
struct process_attribute<arg_v> : process_attribute_default<arg_v> {
    static void init(const arg_v &a, function_record *r) {
        if (r->is_method && r->args.empty()) {
            r->args.emplace_back(
                "self",  nullptr,  handle(),  true,  false);
        }

        if (!a.value) {
#if defined(PYBIND11_DETAILED_ERROR_MESSAGES)
            std::string descr("'");
            if (a.name) {
                descr += std::string(a.name) + ": ";
            }
            descr += a.type + "'";
            if (r->is_method) {
                if (r->name) {
                    descr += " in method '" + (std::string) str(r->scope) + "."
                             + (std::string) r->name + "'";
                } else {
                    descr += " in method of '" + (std::string) str(r->scope) + "'";
                }
            } else if (r->name) {
                descr += " in function '" + (std::string) r->name + "'";
            }
            pybind11_fail("arg(): could not convert default argument " + descr
                          + " into a Python object (type not registered yet?)");
#else
            pybind11_fail("arg(): could not convert default argument "
                          "into a Python object (type not registered yet?). "
                          "#define PYBIND11_DETAILED_ERROR_MESSAGES or compile in debug mode for "
                          "more information.");
#endif
        }
        r->args.emplace_back(a.name, a.descr, a.value.inc_ref(), !a.flag_noconvert, a.flag_none);

        check_kw_only_arg(a, r);
    }
};


template <>
struct process_attribute<kw_only> : process_attribute_default<kw_only> {
    static void init(const kw_only &, function_record *r) {
        append_self_arg_if_needed(r);
        if (r->has_args && r->nargs_pos != static_cast<std::uint16_t>(r->args.size())) {
            pybind11_fail("Mismatched args() and kw_only(): they must occur at the same relative "
                          "argument location (or omit kw_only() entirely)");
        }
        r->nargs_pos = static_cast<std::uint16_t>(r->args.size());
    }
};


template <>
struct process_attribute<pos_only> : process_attribute_default<pos_only> {
    static void init(const pos_only &, function_record *r) {
        append_self_arg_if_needed(r);
        r->nargs_pos_only = static_cast<std::uint16_t>(r->args.size());
        if (r->nargs_pos_only > r->nargs_pos) {
            pybind11_fail("pos_only(): cannot follow a py::args() argument");
        }

    }
};



template <typename T>
struct process_attribute<T, enable_if_t<is_pyobject<T>::value>>
    : process_attribute_default<handle> {
    static void init(const handle &h, type_record *r) { r->bases.append(h); }
};


template <typename T>
struct process_attribute<base<T>> : process_attribute_default<base<T>> {
    static void init(const base<T> &, type_record *r) { r->add_base(typeid(T), nullptr); }
};


template <>
struct process_attribute<multiple_inheritance> : process_attribute_default<multiple_inheritance> {
    static void init(const multiple_inheritance &, type_record *r) {
        r->multiple_inheritance = true;
    }
};

template <>
struct process_attribute<dynamic_attr> : process_attribute_default<dynamic_attr> {
    static void init(const dynamic_attr &, type_record *r) { r->dynamic_attr = true; }
};

template <>
struct process_attribute<custom_type_setup> {
    static void init(const custom_type_setup &value, type_record *r) {
        r->custom_type_setup_callback = value.value;
    }
};

template <>
struct process_attribute<is_final> : process_attribute_default<is_final> {
    static void init(const is_final &, type_record *r) { r->is_final = true; }
};

template <>
struct process_attribute<buffer_protocol> : process_attribute_default<buffer_protocol> {
    static void init(const buffer_protocol &, type_record *r) { r->buffer_protocol = true; }
};

template <>
struct process_attribute<metaclass> : process_attribute_default<metaclass> {
    static void init(const metaclass &m, type_record *r) { r->metaclass = m.value; }
};

template <>
struct process_attribute<module_local> : process_attribute_default<module_local> {
    static void init(const module_local &l, type_record *r) { r->module_local = l.value; }
};


template <>
struct process_attribute<prepend> : process_attribute_default<prepend> {
    static void init(const prepend &, function_record *r) { r->prepend = true; }
};


template <>
struct process_attribute<arithmetic> : process_attribute_default<arithmetic> {};

template <typename... Ts>
struct process_attribute<call_guard<Ts...>> : process_attribute_default<call_guard<Ts...>> {};


template <size_t Nurse, size_t Patient>
struct process_attribute<keep_alive<Nurse, Patient>>
    : public process_attribute_default<keep_alive<Nurse, Patient>> {
    template <size_t N = Nurse, size_t P = Patient, enable_if_t<N != 0 && P != 0, int> = 0>
    static void precall(function_call &call) {
        keep_alive_impl(Nurse, Patient, call, handle());
    }
    template <size_t N = Nurse, size_t P = Patient, enable_if_t<N != 0 && P != 0, int> = 0>
    static void postcall(function_call &, handle) {}
    template <size_t N = Nurse, size_t P = Patient, enable_if_t<N == 0 || P == 0, int> = 0>
    static void precall(function_call &) {}
    template <size_t N = Nurse, size_t P = Patient, enable_if_t<N == 0 || P == 0, int> = 0>
    static void postcall(function_call &call, handle ret) {
        keep_alive_impl(Nurse, Patient, call, ret);
    }
};


template <typename... Args>
struct process_attributes {
    static void init(const Args &...args, function_record *r) {
        PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(r);
        PYBIND11_WORKAROUND_INCORRECT_GCC_UNUSED_BUT_SET_PARAMETER(r);
        using expander = int[];
        (void) expander{
            0, ((void) process_attribute<typename std::decay<Args>::type>::init(args, r), 0)...};
    }
    static void init(const Args &...args, type_record *r) {
        PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(r);
        PYBIND11_WORKAROUND_INCORRECT_GCC_UNUSED_BUT_SET_PARAMETER(r);
        using expander = int[];
        (void) expander{0,
                        (process_attribute<typename std::decay<Args>::type>::init(args, r), 0)...};
    }
    static void precall(function_call &call) {
        PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(call);
        using expander = int[];
        (void) expander{0,
                        (process_attribute<typename std::decay<Args>::type>::precall(call), 0)...};
    }
    static void postcall(function_call &call, handle fn_ret) {
        PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(call, fn_ret);
        PYBIND11_WORKAROUND_INCORRECT_GCC_UNUSED_BUT_SET_PARAMETER(fn_ret);
        using expander = int[];
        (void) expander{
            0, (process_attribute<typename std::decay<Args>::type>::postcall(call, fn_ret), 0)...};
    }
};

template <typename T>
using is_call_guard = is_instantiation<call_guard, T>;


template <typename... Extra>
using extract_guard_t = typename exactly_one_t<is_call_guard, call_guard<>, Extra...>::type;


template <typename... Extra,
          size_t named = constexpr_sum(std::is_base_of<arg, Extra>::value...),
          size_t self = constexpr_sum(std::is_same<is_method, Extra>::value...)>
constexpr bool expected_num_args(size_t nargs, bool has_args, bool has_kwargs) {
    PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(nargs, has_args, has_kwargs);
    return named == 0 || (self + named + size_t(has_args) + size_t(has_kwargs)) == nargs;
}

PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
