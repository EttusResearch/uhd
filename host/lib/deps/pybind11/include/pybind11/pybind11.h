/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "detail/class.h"
#include "detail/init.h"
#include "attr.h"
#include "gil.h"
#include "options.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#if defined(__cpp_lib_launder) && !(defined(_MSC_VER) && (_MSC_VER < 1914))
#    define PYBIND11_STD_LAUNDER std::launder
#    define PYBIND11_HAS_STD_LAUNDER 1
#else
#    define PYBIND11_STD_LAUNDER
#    define PYBIND11_HAS_STD_LAUNDER 0
#endif
#if defined(__GNUG__) && !defined(__clang__)
#    include <cxxabi.h>
#endif


#if defined(__GNUC__) && __GNUC__ == 7
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wnoexcept-type"
#endif

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)

PYBIND11_NAMESPACE_BEGIN(detail)





inline bool apply_exception_translators(std::forward_list<ExceptionTranslator> &translators) {
    auto last_exception = std::current_exception();

    for (auto &translator : translators) {
        try {
            translator(last_exception);
            return true;
        } catch (...) {
            last_exception = std::current_exception();
        }
    }
    return false;
}

#if defined(_MSC_VER)
#    define PYBIND11_COMPAT_STRDUP _strdup
#else
#    define PYBIND11_COMPAT_STRDUP strdup
#endif

PYBIND11_NAMESPACE_END(detail)


class cpp_function : public function {
public:
    cpp_function() = default;

    cpp_function(std::nullptr_t) {}


    template <typename Return, typename... Args, typename... Extra>

    cpp_function(Return (*f)(Args...), const Extra &...extra) {
        initialize(f, f, extra...);
    }


    template <typename Func,
              typename... Extra,
              typename = detail::enable_if_t<detail::is_lambda<Func>::value>>

    cpp_function(Func &&f, const Extra &...extra) {
        initialize(
            std::forward<Func>(f), (detail::function_signature_t<Func> *) nullptr, extra...);
    }


    template <typename Return, typename Class, typename... Arg, typename... Extra>

    cpp_function(Return (Class::*f)(Arg...), const Extra &...extra) {
        initialize(
            [f](Class *c, Arg... args) -> Return { return (c->*f)(std::forward<Arg>(args)...); },
            (Return(*)(Class *, Arg...)) nullptr,
            extra...);
    }




    template <typename Return, typename Class, typename... Arg, typename... Extra>

    cpp_function(Return (Class::*f)(Arg...) &, const Extra &...extra) {
        initialize(
            [f](Class *c, Arg... args) -> Return { return (c->*f)(std::forward<Arg>(args)...); },
            (Return(*)(Class *, Arg...)) nullptr,
            extra...);
    }


    template <typename Return, typename Class, typename... Arg, typename... Extra>

    cpp_function(Return (Class::*f)(Arg...) const, const Extra &...extra) {
        initialize([f](const Class *c,
                       Arg... args) -> Return { return (c->*f)(std::forward<Arg>(args)...); },
                   (Return(*)(const Class *, Arg...)) nullptr,
                   extra...);
    }




    template <typename Return, typename Class, typename... Arg, typename... Extra>

    cpp_function(Return (Class::*f)(Arg...) const &, const Extra &...extra) {
        initialize([f](const Class *c,
                       Arg... args) -> Return { return (c->*f)(std::forward<Arg>(args)...); },
                   (Return(*)(const Class *, Arg...)) nullptr,
                   extra...);
    }


    object name() const { return attr("__name__"); }

protected:
    struct InitializingFunctionRecordDeleter {


        void operator()(detail::function_record *rec) { destruct(rec, false); }
    };
    using unique_function_record
        = std::unique_ptr<detail::function_record, InitializingFunctionRecordDeleter>;


    PYBIND11_NOINLINE unique_function_record make_function_record() {
        return unique_function_record(new detail::function_record());
    }


    template <typename Func, typename Return, typename... Args, typename... Extra>
    void initialize(Func &&f, Return (*)(Args...), const Extra &...extra) {
        using namespace detail;
        struct capture {
            remove_reference_t<Func> f;
        };



        auto unique_rec = make_function_record();
        auto *rec = unique_rec.get();


        if (PYBIND11_SILENCE_MSVC_C4127(sizeof(capture) <= sizeof(rec->data))) {

#if defined(__GNUG__) && __GNUC__ >= 6 && !defined(__clang__) && !defined(__INTEL_COMPILER)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wplacement-new"
#endif
            new ((capture *) &rec->data) capture{std::forward<Func>(f)};
#if defined(__GNUG__) && __GNUC__ >= 6 && !defined(__clang__) && !defined(__INTEL_COMPILER)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUG__) && !PYBIND11_HAS_STD_LAUNDER && !defined(__INTEL_COMPILER)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif


            if (!std::is_trivially_destructible<capture>::value) {
                rec->free_data = [](function_record *r) {
                    auto data = PYBIND11_STD_LAUNDER((capture *) &r->data);
                    (void) data;
                    data->~capture();
                };
            }
#if defined(__GNUG__) && !PYBIND11_HAS_STD_LAUNDER && !defined(__INTEL_COMPILER)
#    pragma GCC diagnostic pop
#endif
        } else {
            rec->data[0] = new capture{std::forward<Func>(f)};
            rec->free_data = [](function_record *r) { delete ((capture *) r->data[0]); };
        }


        using cast_in = argument_loader<Args...>;
        using cast_out
            = make_caster<conditional_t<std::is_void<Return>::value, void_type, Return>>;

        static_assert(
            expected_num_args<Extra...>(
                sizeof...(Args), cast_in::args_pos >= 0, cast_in::has_kwargs),
            "The number of argument annotations does not match the number of function arguments");


        rec->impl = [](function_call &call) -> handle {
            cast_in args_converter;


            if (!args_converter.load_args(call)) {
                return PYBIND11_TRY_NEXT_OVERLOAD;
            }


            process_attributes<Extra...>::precall(call);


            const auto *data = (sizeof(capture) <= sizeof(call.func.data) ? &call.func.data
                                                                          : call.func.data[0]);
            auto *cap = const_cast<capture *>(reinterpret_cast<const capture *>(data));


            return_value_policy policy
                = return_value_policy_override<Return>::policy(call.func.policy);


            using Guard = extract_guard_t<Extra...>;


            handle result
                = cast_out::cast(std::move(args_converter).template call<Return, Guard>(cap->f),
                                 policy,
                                 call.parent);


            process_attributes<Extra...>::postcall(call, result);

            return result;
        };

        rec->nargs_pos = cast_in::args_pos >= 0
                             ? static_cast<std::uint16_t>(cast_in::args_pos)
                             : sizeof...(Args) - cast_in::has_kwargs;

        rec->has_args = cast_in::args_pos >= 0;
        rec->has_kwargs = cast_in::has_kwargs;


        process_attributes<Extra...>::init(extra..., rec);

        {
            constexpr bool has_kw_only_args = any_of<std::is_same<kw_only, Extra>...>::value,
                           has_pos_only_args = any_of<std::is_same<pos_only, Extra>...>::value,
                           has_arg_annotations = any_of<is_keyword<Extra>...>::value;
            static_assert(has_arg_annotations || !has_kw_only_args,
                          "py::kw_only requires the use of argument annotations");
            static_assert(has_arg_annotations || !has_pos_only_args,
                          "py::pos_only requires the use of argument annotations (for docstrings "
                          "and aligning the annotations to the argument)");

            static_assert(constexpr_sum(is_kw_only<Extra>::value...) <= 1,
                          "py::kw_only may be specified only once");
            static_assert(constexpr_sum(is_pos_only<Extra>::value...) <= 1,
                          "py::pos_only may be specified only once");
            constexpr auto kw_only_pos = constexpr_first<is_kw_only, Extra...>();
            constexpr auto pos_only_pos = constexpr_first<is_pos_only, Extra...>();
            static_assert(!(has_kw_only_args && has_pos_only_args) || pos_only_pos < kw_only_pos,
                          "py::pos_only must come before py::kw_only");
        }


        static constexpr auto signature
            = const_name("(") + cast_in::arg_names + const_name(") -> ") + cast_out::name;
        PYBIND11_DESCR_CONSTEXPR auto types = decltype(signature)::types();



        initialize_generic(std::move(unique_rec), signature.text, types.data(), sizeof...(Args));


        using FunctionType = Return (*)(Args...);
        constexpr bool is_function_ptr
            = std::is_convertible<Func, FunctionType>::value && sizeof(capture) == sizeof(void *);
        if (is_function_ptr) {
            rec->is_stateless = true;
            rec->data[1]
                = const_cast<void *>(reinterpret_cast<const void *>(&typeid(FunctionType)));
        }
    }




    class strdup_guard {
    public:
        strdup_guard() = default;
        strdup_guard(const strdup_guard &) = delete;
        strdup_guard &operator=(const strdup_guard &) = delete;

        ~strdup_guard() {
            for (auto *s : strings) {
                std::free(s);
            }
        }
        char *operator()(const char *s) {
            auto *t = PYBIND11_COMPAT_STRDUP(s);
            strings.push_back(t);
            return t;
        }
        void release() { strings.clear(); }

    private:
        std::vector<char *> strings;
    };


    void initialize_generic(unique_function_record &&unique_rec,
                            const char *text,
                            const std::type_info *const *types,
                            size_t args) {




        auto *rec = unique_rec.get();







        strdup_guard guarded_strdup;


        rec->name = guarded_strdup(rec->name ? rec->name : "");
        if (rec->doc) {
            rec->doc = guarded_strdup(rec->doc);
        }
        for (auto &a : rec->args) {
            if (a.name) {
                a.name = guarded_strdup(a.name);
            }
            if (a.descr) {
                a.descr = guarded_strdup(a.descr);
            } else if (a.value) {
                a.descr = guarded_strdup(repr(a.value).cast<std::string>().c_str());
            }
        }

        rec->is_constructor = (std::strcmp(rec->name, "__init__") == 0)
                              || (std::strcmp(rec->name, "__setstate__") == 0);

#if defined(PYBIND11_DETAILED_ERROR_MESSAGES) && !defined(PYBIND11_DISABLE_NEW_STYLE_INIT_WARNING)
        if (rec->is_constructor && !rec->is_new_style_constructor) {
            const auto class_name
                = detail::get_fully_qualified_tp_name((PyTypeObject *) rec->scope.ptr());
            const auto func_name = std::string(rec->name);
            PyErr_WarnEx(PyExc_FutureWarning,
                         ("pybind11-bound class '" + class_name
                          + "' is using an old-style "
                            "placement-new '"
                          + func_name
                          + "' which has been deprecated. See "
                            "the upgrade guide in pybind11's docs. This message is only visible "
                            "when compiled in debug mode.")
                             .c_str(),
                         0);
        }
#endif


        std::string signature;
        size_t type_index = 0, arg_index = 0;
        bool is_starred = false;
        for (const auto *pc = text; *pc != '\0'; ++pc) {
            const auto c = *pc;

            if (c == '{') {

                is_starred = *(pc + 1) == '*';
                if (is_starred) {
                    continue;
                }


                if (!rec->has_args && arg_index == rec->nargs_pos) {
                    signature += "*, ";
                }
                if (arg_index < rec->args.size() && rec->args[arg_index].name) {
                    signature += rec->args[arg_index].name;
                } else if (arg_index == 0 && rec->is_method) {
                    signature += "self";
                } else {
                    signature += "arg" + std::to_string(arg_index - (rec->is_method ? 1 : 0));
                }
                signature += ": ";
            } else if (c == '}') {

                if (!is_starred && arg_index < rec->args.size() && rec->args[arg_index].descr) {
                    signature += " = ";
                    signature += rec->args[arg_index].descr;
                }


                if (rec->nargs_pos_only > 0 && (arg_index + 1) == rec->nargs_pos_only) {
                    signature += ", /";
                }
                if (!is_starred) {
                    arg_index++;
                }
            } else if (c == '%') {
                const std::type_info *t = types[type_index++];
                if (!t) {
                    pybind11_fail("Internal error while parsing type signature (1)");
                }
                if (auto *tinfo = detail::get_type_info(*t)) {
                    handle th((PyObject *) tinfo->type);
                    signature += th.attr("__module__").cast<std::string>() + "."
                                 + th.attr("__qualname__").cast<std::string>();
                } else if (rec->is_new_style_constructor && arg_index == 0) {


                    signature += rec->scope.attr("__module__").cast<std::string>() + "."
                                 + rec->scope.attr("__qualname__").cast<std::string>();
                } else {
                    std::string tname(t->name());
                    detail::clean_type_id(tname);
                    signature += tname;
                }
            } else {
                signature += c;
            }
        }

        if (arg_index != args - rec->has_args - rec->has_kwargs || types[type_index] != nullptr) {
            pybind11_fail("Internal error while parsing type signature (2)");
        }

        rec->signature = guarded_strdup(signature.c_str());
        rec->args.shrink_to_fit();
        rec->nargs = (std::uint16_t) args;

        if (rec->sibling && PYBIND11_INSTANCE_METHOD_CHECK(rec->sibling.ptr())) {
            rec->sibling = PYBIND11_INSTANCE_METHOD_GET_FUNCTION(rec->sibling.ptr());
        }

        detail::function_record *chain = nullptr, *chain_start = rec;
        if (rec->sibling) {
            if (PyCFunction_Check(rec->sibling.ptr())) {
                auto *self = PyCFunction_GET_SELF(rec->sibling.ptr());
                capsule rec_capsule = isinstance<capsule>(self) ? reinterpret_borrow<capsule>(self)
                                                                : capsule(self);
                chain = (detail::function_record *) rec_capsule;

                if (!chain->scope.is(rec->scope)) {
                    chain = nullptr;
                }
            }


            else if (!rec->sibling.is_none() && rec->name[0] != '_') {
                pybind11_fail("Cannot overload existing non-function object \""
                              + std::string(rec->name) + "\" with a function of the same name");
            }
        }

        if (!chain) {

            rec->def = new PyMethodDef();
            std::memset(rec->def, 0, sizeof(PyMethodDef));
            rec->def->ml_name = rec->name;
            rec->def->ml_meth
                = reinterpret_cast<PyCFunction>(reinterpret_cast<void (*)()>(dispatcher));
            rec->def->ml_flags = METH_VARARGS | METH_KEYWORDS;

            capsule rec_capsule(unique_rec.release(),
                                [](void *ptr) { destruct((detail::function_record *) ptr); });
            guarded_strdup.release();

            object scope_module;
            if (rec->scope) {
                if (hasattr(rec->scope, "__module__")) {
                    scope_module = rec->scope.attr("__module__");
                } else if (hasattr(rec->scope, "__name__")) {
                    scope_module = rec->scope.attr("__name__");
                }
            }

            m_ptr = PyCFunction_NewEx(rec->def, rec_capsule.ptr(), scope_module.ptr());
            if (!m_ptr) {
                pybind11_fail("cpp_function::cpp_function(): Could not allocate function object");
            }
        } else {

            m_ptr = rec->sibling.ptr();
            inc_ref();
            if (chain->is_method != rec->is_method) {
                pybind11_fail(
                    "overloading a method with both static and instance methods is not supported; "
#if !defined(PYBIND11_DETAILED_ERROR_MESSAGES)
                    "#define PYBIND11_DETAILED_ERROR_MESSAGES or compile in debug mode for more "
                    "details"
#else
                    "error while attempting to bind "
                    + std::string(rec->is_method ? "instance" : "static") + " method "
                    + std::string(pybind11::str(rec->scope.attr("__name__"))) + "."
                    + std::string(rec->name) + signature
#endif
                );
            }

            if (rec->prepend) {



                chain_start = rec;
                rec->next = chain;
                auto rec_capsule
                    = reinterpret_borrow<capsule>(((PyCFunctionObject *) m_ptr)->m_self);
                rec_capsule.set_pointer(unique_rec.release());
                guarded_strdup.release();
            } else {

                chain_start = chain;
                while (chain->next) {
                    chain = chain->next;
                }
                chain->next = unique_rec.release();
                guarded_strdup.release();
            }
        }

        std::string signatures;
        int index = 0;

        if (chain && options::show_function_signatures()) {

            signatures += rec->name;
            signatures += "(*args, **kwargs)\n";
            signatures += "Overloaded function.\n\n";
        }

        bool first_user_def = true;
        for (auto *it = chain_start; it != nullptr; it = it->next) {
            if (options::show_function_signatures()) {
                if (index > 0) {
                    signatures += '\n';
                }
                if (chain) {
                    signatures += std::to_string(++index) + ". ";
                }
                signatures += rec->name;
                signatures += it->signature;
                signatures += '\n';
            }
            if (it->doc && it->doc[0] != '\0' && options::show_user_defined_docstrings()) {


                if (!options::show_function_signatures()) {
                    if (first_user_def) {
                        first_user_def = false;
                    } else {
                        signatures += '\n';
                    }
                }
                if (options::show_function_signatures()) {
                    signatures += '\n';
                }
                signatures += it->doc;
                if (options::show_function_signatures()) {
                    signatures += '\n';
                }
            }
        }


        auto *func = (PyCFunctionObject *) m_ptr;
        std::free(const_cast<char *>(func->m_ml->ml_doc));

        func->m_ml->ml_doc
            = signatures.empty() ? nullptr : PYBIND11_COMPAT_STRDUP(signatures.c_str());

        if (rec->is_method) {
            m_ptr = PYBIND11_INSTANCE_METHOD_NEW(m_ptr, rec->scope.ptr());
            if (!m_ptr) {
                pybind11_fail(
                    "cpp_function::cpp_function(): Could not allocate instance method object");
            }
            Py_DECREF(func);
        }
    }


    static void destruct(detail::function_record *rec, bool free_strings = true) {


#if !defined(PYPY_VERSION) && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION == 9
        static bool is_zero = Py_GetVersion()[4] == '0';
#endif

        while (rec) {
            detail::function_record *next = rec->next;
            if (rec->free_data) {
                rec->free_data(rec);
            }



            if (free_strings) {
                std::free((char *) rec->name);
                std::free((char *) rec->doc);
                std::free((char *) rec->signature);
                for (auto &arg : rec->args) {
                    std::free(const_cast<char *>(arg.name));
                    std::free(const_cast<char *>(arg.descr));
                }
            }
            for (auto &arg : rec->args) {
                arg.value.dec_ref();
            }
            if (rec->def) {
                std::free(const_cast<char *>(rec->def->ml_doc));



#if !defined(PYPY_VERSION) && PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION == 9
                if (!is_zero) {
                    delete rec->def;
                }
#else
                delete rec->def;
#endif
            }
            delete rec;
            rec = next;
        }
    }


    static PyObject *dispatcher(PyObject *self, PyObject *args_in, PyObject *kwargs_in) {
        using namespace detail;


        const function_record *overloads = (function_record *) PyCapsule_GetPointer(self, nullptr),
                              *it = overloads;


        const auto n_args_in = (size_t) PyTuple_GET_SIZE(args_in);

        handle parent = n_args_in > 0 ? PyTuple_GET_ITEM(args_in, 0) : nullptr,
               result = PYBIND11_TRY_NEXT_OVERLOAD;

        auto self_value_and_holder = value_and_holder();
        if (overloads->is_constructor) {
            if (!parent
                || !PyObject_TypeCheck(parent.ptr(), (PyTypeObject *) overloads->scope.ptr())) {
                PyErr_SetString(
                    PyExc_TypeError,
                    "__init__(self, ...) called with invalid or missing `self` argument");
                return nullptr;
            }

            auto *const tinfo = get_type_info((PyTypeObject *) overloads->scope.ptr());
            auto *const pi = reinterpret_cast<instance *>(parent.ptr());
            self_value_and_holder = pi->get_value_and_holder(tinfo, true);



            if (self_value_and_holder.instance_registered()) {
                return none().release().ptr();
            }
        }

        try {




            std::vector<function_call> second_pass;


            const bool overloaded = it != nullptr && it->next != nullptr;

            for (; it != nullptr; it = it->next) {



                const function_record &func = *it;
                size_t num_args = func.nargs;
                if (func.has_args) {
                    --num_args;
                }
                if (func.has_kwargs) {
                    --num_args;
                }
                size_t pos_args = func.nargs_pos;

                if (!func.has_args && n_args_in > pos_args) {
                    continue;
                }

                if (n_args_in < pos_args && func.args.size() < pos_args) {
                    continue;

                }

                function_call call(func, parent);


                size_t args_to_copy = (std::min)(pos_args, n_args_in);
                size_t args_copied = 0;


                if (func.is_new_style_constructor) {


                    if (self_value_and_holder) {
                        self_value_and_holder.type->dealloc(self_value_and_holder);
                    }

                    call.init_self = PyTuple_GET_ITEM(args_in, 0);
                    call.args.emplace_back(reinterpret_cast<PyObject *>(&self_value_and_holder));
                    call.args_convert.push_back(false);
                    ++args_copied;
                }


                bool bad_arg = false;
                for (; args_copied < args_to_copy; ++args_copied) {
                    const argument_record *arg_rec
                        = args_copied < func.args.size() ? &func.args[args_copied] : nullptr;
                    if (kwargs_in && arg_rec && arg_rec->name
                        && dict_getitemstring(kwargs_in, arg_rec->name)) {
                        bad_arg = true;
                        break;
                    }

                    handle arg(PyTuple_GET_ITEM(args_in, args_copied));
                    if (arg_rec && !arg_rec->none && arg.is_none()) {
                        bad_arg = true;
                        break;
                    }
                    call.args.push_back(arg);
                    call.args_convert.push_back(arg_rec ? arg_rec->convert : true);
                }
                if (bad_arg) {
                    continue;
                }



                size_t positional_args_copied = args_copied;


                dict kwargs = reinterpret_borrow<dict>(kwargs_in);


                if (args_copied < func.nargs_pos_only) {
                    for (; args_copied < func.nargs_pos_only; ++args_copied) {
                        const auto &arg_rec = func.args[args_copied];
                        handle value;

                        if (arg_rec.value) {
                            value = arg_rec.value;
                        }
                        if (value) {
                            call.args.push_back(value);
                            call.args_convert.push_back(arg_rec.convert);
                        } else {
                            break;
                        }
                    }

                    if (args_copied < func.nargs_pos_only) {
                        continue;
                    }
                }


                if (args_copied < num_args) {
                    bool copied_kwargs = false;

                    for (; args_copied < num_args; ++args_copied) {
                        const auto &arg_rec = func.args[args_copied];

                        handle value;
                        if (kwargs_in && arg_rec.name) {
                            value = dict_getitemstring(kwargs.ptr(), arg_rec.name);
                        }

                        if (value) {

                            if (!copied_kwargs) {
                                kwargs = reinterpret_steal<dict>(PyDict_Copy(kwargs.ptr()));
                                copied_kwargs = true;
                            }
                            if (PyDict_DelItemString(kwargs.ptr(), arg_rec.name) == -1) {
                                throw error_already_set();
                            }
                        } else if (arg_rec.value) {
                            value = arg_rec.value;
                        }

                        if (!arg_rec.none && value.is_none()) {
                            break;
                        }

                        if (value) {


                            if (func.has_args && call.args.size() == func.nargs_pos) {
                                call.args.push_back(none());
                            }

                            call.args.push_back(value);
                            call.args_convert.push_back(arg_rec.convert);
                        } else {
                            break;
                        }
                    }

                    if (args_copied < num_args) {
                        continue;

                    }
                }


                if (kwargs && !kwargs.empty() && !func.has_kwargs) {
                    continue;
                }


                if (func.has_args) {
                    tuple extra_args;
                    if (args_to_copy == 0) {


                        extra_args = reinterpret_borrow<tuple>(args_in);
                    } else if (positional_args_copied >= n_args_in) {
                        extra_args = tuple(0);
                    } else {
                        size_t args_size = n_args_in - positional_args_copied;
                        extra_args = tuple(args_size);
                        for (size_t i = 0; i < args_size; ++i) {
                            extra_args[i] = PyTuple_GET_ITEM(args_in, positional_args_copied + i);
                        }
                    }
                    if (call.args.size() <= func.nargs_pos) {
                        call.args.push_back(extra_args);
                    } else {
                        call.args[func.nargs_pos] = extra_args;
                    }
                    call.args_convert.push_back(false);
                    call.args_ref = std::move(extra_args);
                }


                if (func.has_kwargs) {
                    if (!kwargs.ptr()) {
                        kwargs = dict();
                    }
                    call.args.push_back(kwargs);
                    call.args_convert.push_back(false);
                    call.kwargs_ref = std::move(kwargs);
                }



#if defined(PYBIND11_DETAILED_ERROR_MESSAGES)
                if (call.args.size() != func.nargs || call.args_convert.size() != func.nargs) {
                    pybind11_fail("Internal error: function call dispatcher inserted wrong number "
                                  "of arguments!");
                }
#endif

                std::vector<bool> second_pass_convert;
                if (overloaded) {



                    second_pass_convert.resize(func.nargs, false);
                    call.args_convert.swap(second_pass_convert);
                }


                try {
                    loader_life_support guard{};
                    result = func.impl(call);
                } catch (reference_cast_error &) {
                    result = PYBIND11_TRY_NEXT_OVERLOAD;
                }

                if (result.ptr() != PYBIND11_TRY_NEXT_OVERLOAD) {
                    break;
                }

                if (overloaded) {



                    for (size_t i = func.is_method ? 1 : 0; i < pos_args; i++) {
                        if (second_pass_convert[i]) {


                            call.args_convert.swap(second_pass_convert);
                            second_pass.push_back(std::move(call));
                            break;
                        }
                    }
                }
            }

            if (overloaded && !second_pass.empty() && result.ptr() == PYBIND11_TRY_NEXT_OVERLOAD) {


                for (auto &call : second_pass) {
                    try {
                        loader_life_support guard{};
                        result = call.func.impl(call);
                    } catch (reference_cast_error &) {
                        result = PYBIND11_TRY_NEXT_OVERLOAD;
                    }

                    if (result.ptr() != PYBIND11_TRY_NEXT_OVERLOAD) {


                        if (!result) {
                            it = &call.func;
                        }
                        break;
                    }
                }
            }
        } catch (error_already_set &e) {
            e.restore();
            return nullptr;
#ifdef __GLIBCXX__
        } catch (abi::__forced_unwind &) {
            throw;
#endif
        } catch (...) {


            auto &local_exception_translators
                = get_local_internals().registered_exception_translators;
            if (detail::apply_exception_translators(local_exception_translators)) {
                return nullptr;
            }
            auto &exception_translators = get_internals().registered_exception_translators;
            if (detail::apply_exception_translators(exception_translators)) {
                return nullptr;
            }

            PyErr_SetString(PyExc_SystemError,
                            "Exception escaped from default exception translator!");
            return nullptr;
        }

        auto append_note_if_missing_header_is_suspected = [](std::string &msg) {
            if (msg.find("std::") != std::string::npos) {
                msg += "\n\n"
                       "Did you forget to `#include <pybind11/stl.h>`? Or <pybind11/complex.h>,\n"
                       "<pybind11/functional.h>, <pybind11/chrono.h>, etc. Some automatic\n"
                       "conversions are optional and require extra headers to be included\n"
                       "when compiling your pybind11 module.";
            }
        };

        if (result.ptr() == PYBIND11_TRY_NEXT_OVERLOAD) {
            if (overloads->is_operator) {
                return handle(Py_NotImplemented).inc_ref().ptr();
            }

            std::string msg = std::string(overloads->name) + "(): incompatible "
                              + std::string(overloads->is_constructor ? "constructor" : "function")
                              + " arguments. The following argument types are supported:\n";

            int ctr = 0;
            for (const function_record *it2 = overloads; it2 != nullptr; it2 = it2->next) {
                msg += "    " + std::to_string(++ctr) + ". ";

                bool wrote_sig = false;
                if (overloads->is_constructor) {


                    std::string sig = it2->signature;
                    size_t start = sig.find('(') + 7;
                    if (start < sig.size()) {

                        size_t end = sig.find(", "), next = end + 2;
                        size_t ret = sig.rfind(" -> ");

                        if (end >= sig.size()) {
                            next = end = sig.find(')');
                        }
                        if (start < end && next < sig.size()) {
                            msg.append(sig, start, end - start);
                            msg += '(';
                            msg.append(sig, next, ret - next);
                            wrote_sig = true;
                        }
                    }
                }
                if (!wrote_sig) {
                    msg += it2->signature;
                }

                msg += '\n';
            }
            msg += "\nInvoked with: ";
            auto args_ = reinterpret_borrow<tuple>(args_in);
            bool some_args = false;
            for (size_t ti = overloads->is_constructor ? 1 : 0; ti < args_.size(); ++ti) {
                if (!some_args) {
                    some_args = true;
                } else {
                    msg += ", ";
                }
                try {
                    msg += pybind11::repr(args_[ti]);
                } catch (const error_already_set &) {
                    msg += "<repr raised Error>";
                }
            }
            if (kwargs_in) {
                auto kwargs = reinterpret_borrow<dict>(kwargs_in);
                if (!kwargs.empty()) {
                    if (some_args) {
                        msg += "; ";
                    }
                    msg += "kwargs: ";
                    bool first = true;
                    for (auto kwarg : kwargs) {
                        if (first) {
                            first = false;
                        } else {
                            msg += ", ";
                        }
                        msg += pybind11::str("{}=").format(kwarg.first);
                        try {
                            msg += pybind11::repr(kwarg.second);
                        } catch (const error_already_set &) {
                            msg += "<repr raised Error>";
                        }
                    }
                }
            }

            append_note_if_missing_header_is_suspected(msg);

            if (PyErr_Occurred()) {

                raise_from(PyExc_TypeError, msg.c_str());
                return nullptr;
            }
            PyErr_SetString(PyExc_TypeError, msg.c_str());
            return nullptr;
        }
        if (!result) {
            std::string msg = "Unable to convert function return value to a "
                              "Python type! The signature was\n\t";
            msg += it->signature;
            append_note_if_missing_header_is_suspected(msg);

            if (PyErr_Occurred()) {
                raise_from(PyExc_TypeError, msg.c_str());
                return nullptr;
            }
            PyErr_SetString(PyExc_TypeError, msg.c_str());
            return nullptr;
        }
        if (overloads->is_constructor && !self_value_and_holder.holder_constructed()) {
            auto *pi = reinterpret_cast<instance *>(parent.ptr());
            self_value_and_holder.type->init_instance(pi, nullptr);
        }
        return result.ptr();
    }
};


class module_ : public object {
public:
    PYBIND11_OBJECT_DEFAULT(module_, object, PyModule_Check)


    PYBIND11_DEPRECATED("Use PYBIND11_MODULE or module_::create_extension_module instead")
    explicit module_(const char *name, const char *doc = nullptr) {
        *this = create_extension_module(name, doc, new PyModuleDef());
    }


    template <typename Func, typename... Extra>
    module_ &def(const char *name_, Func &&f, const Extra &...extra) {
        cpp_function func(std::forward<Func>(f),
                          name(name_),
                          scope(*this),
                          sibling(getattr(*this, name_, none())),
                          extra...);



        add_object(name_, func, true  );
        return *this;
    }


    module_ def_submodule(const char *name, const char *doc = nullptr) {
        const char *this_name = PyModule_GetName(m_ptr);
        if (this_name == nullptr) {
            throw error_already_set();
        }
        std::string full_name = std::string(this_name) + '.' + name;
        handle submodule = PyImport_AddModule(full_name.c_str());
        if (!submodule) {
            throw error_already_set();
        }
        auto result = reinterpret_borrow<module_>(submodule);
        if (doc && options::show_user_defined_docstrings()) {
            result.attr("__doc__") = pybind11::str(doc);
        }
        attr(name) = result;
        return result;
    }


    static module_ import(const char *name) {
        PyObject *obj = PyImport_ImportModule(name);
        if (!obj) {
            throw error_already_set();
        }
        return reinterpret_steal<module_>(obj);
    }


    void reload() {
        PyObject *obj = PyImport_ReloadModule(ptr());
        if (!obj) {
            throw error_already_set();
        }
        *this = reinterpret_steal<module_>(obj);
    }


    PYBIND11_NOINLINE void add_object(const char *name, handle obj, bool overwrite = false) {
        if (!overwrite && hasattr(*this, name)) {
            pybind11_fail(
                "Error during initialization: multiple incompatible definitions with name \""
                + std::string(name) + "\"");
        }

        PyModule_AddObject(ptr(), name, obj.inc_ref().ptr()  );
    }

    using module_def = PyModuleDef;


    static module_ create_extension_module(const char *name, const char *doc, module_def *def) {


        def = new (def)
            PyModuleDef{  PyModuleDef_HEAD_INIT,
                          name,
                          options::show_user_defined_docstrings() ? doc : nullptr,
                          -1,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr};
        auto *m = PyModule_Create(def);
        if (m == nullptr) {
            if (PyErr_Occurred()) {
                throw error_already_set();
            }
            pybind11_fail("Internal error in module_::create_extension_module()");
        }



        return reinterpret_borrow<module_>(m);
    }
};




using module = module_;




inline dict globals() {
    PyObject *p = PyEval_GetGlobals();
    return reinterpret_borrow<dict>(p ? p : module_::import("__main__").attr("__dict__").ptr());
}

template <typename... Args, typename = detail::enable_if_t<args_are_all_keyword_or_ds<Args...>()>>
PYBIND11_DEPRECATED("make_simple_namespace should be replaced with "
                    "py::module_::import(\"types\").attr(\"SimpleNamespace\") ")
object make_simple_namespace(Args &&...args_) {
    return module_::import("types").attr("SimpleNamespace")(std::forward<Args>(args_)...);
}

PYBIND11_NAMESPACE_BEGIN(detail)

class generic_type : public object {
public:
    PYBIND11_OBJECT_DEFAULT(generic_type, object, PyType_Check)
protected:
    void initialize(const type_record &rec) {
        if (rec.scope && hasattr(rec.scope, "__dict__")
            && rec.scope.attr("__dict__").contains(rec.name)) {
            pybind11_fail("generic_type: cannot initialize type \"" + std::string(rec.name)
                          + "\": an object with that name is already defined");
        }

        if ((rec.module_local ? get_local_type_info(*rec.type) : get_global_type_info(*rec.type))
            != nullptr) {
            pybind11_fail("generic_type: type \"" + std::string(rec.name)
                          + "\" is already registered!");
        }

        m_ptr = make_new_python_type(rec);


        auto *tinfo = new detail::type_info();
        tinfo->type = (PyTypeObject *) m_ptr;
        tinfo->cpptype = rec.type;
        tinfo->type_size = rec.type_size;
        tinfo->type_align = rec.type_align;
        tinfo->operator_new = rec.operator_new;
        tinfo->holder_size_in_ptrs = size_in_ptrs(rec.holder_size);
        tinfo->init_instance = rec.init_instance;
        tinfo->dealloc = rec.dealloc;
        tinfo->simple_type = true;
        tinfo->simple_ancestors = true;
        tinfo->default_holder = rec.default_holder;
        tinfo->module_local = rec.module_local;

        auto &internals = get_internals();
        auto tindex = std::type_index(*rec.type);
        tinfo->direct_conversions = &internals.direct_conversions[tindex];
        if (rec.module_local) {
            get_local_internals().registered_types_cpp[tindex] = tinfo;
        } else {
            internals.registered_types_cpp[tindex] = tinfo;
        }
        internals.registered_types_py[(PyTypeObject *) m_ptr] = {tinfo};

        if (rec.bases.size() > 1 || rec.multiple_inheritance) {
            mark_parents_nonsimple(tinfo->type);
            tinfo->simple_ancestors = false;
        } else if (rec.bases.size() == 1) {
            auto *parent_tinfo = get_type_info((PyTypeObject *) rec.bases[0].ptr());
            assert(parent_tinfo != nullptr);
            bool parent_simple_ancestors = parent_tinfo->simple_ancestors;
            tinfo->simple_ancestors = parent_simple_ancestors;

            parent_tinfo->simple_type = parent_tinfo->simple_type && parent_simple_ancestors;
        }

        if (rec.module_local) {

            tinfo->module_local_load = &type_caster_generic::local_load;
            setattr(m_ptr, PYBIND11_MODULE_LOCAL_ID, capsule(tinfo));
        }
    }


    void mark_parents_nonsimple(PyTypeObject *value) {
        auto t = reinterpret_borrow<tuple>(value->tp_bases);
        for (handle h : t) {
            auto *tinfo2 = get_type_info((PyTypeObject *) h.ptr());
            if (tinfo2) {
                tinfo2->simple_type = false;
            }
            mark_parents_nonsimple((PyTypeObject *) h.ptr());
        }
    }

    void install_buffer_funcs(buffer_info *(*get_buffer)(PyObject *, void *),
                              void *get_buffer_data) {
        auto *type = (PyHeapTypeObject *) m_ptr;
        auto *tinfo = detail::get_type_info(&type->ht_type);

        if (!type->ht_type.tp_as_buffer) {
            pybind11_fail("To be able to register buffer protocol support for the type '"
                          + get_fully_qualified_tp_name(tinfo->type)
                          + "' the associated class<>(..) invocation must "
                            "include the pybind11::buffer_protocol() annotation!");
        }

        tinfo->get_buffer = get_buffer;
        tinfo->get_buffer_data = get_buffer_data;
    }


    void def_property_static_impl(const char *name,
                                  handle fget,
                                  handle fset,
                                  detail::function_record *rec_func) {
        const auto is_static = (rec_func != nullptr) && !(rec_func->is_method && rec_func->scope);
        const auto has_doc = (rec_func != nullptr) && (rec_func->doc != nullptr)
                             && pybind11::options::show_user_defined_docstrings();
        auto property = handle(
            (PyObject *) (is_static ? get_internals().static_property_type : &PyProperty_Type));
        attr(name) = property(fget.ptr() ? fget : none(),
                              fset.ptr() ? fset : none(),
                                none(),
                              pybind11::str(has_doc ? rec_func->doc : ""));
    }
};


template <typename T,
          typename = void_t<decltype(static_cast<void *(*) (size_t)>(T::operator new))>>
void set_operator_new(type_record *r) {
    r->operator_new = &T::operator new;
}

template <typename>
void set_operator_new(...) {}

template <typename T, typename SFINAE = void>
struct has_operator_delete : std::false_type {};
template <typename T>
struct has_operator_delete<T, void_t<decltype(static_cast<void (*)(void *)>(T::operator delete))>>
    : std::true_type {};
template <typename T, typename SFINAE = void>
struct has_operator_delete_size : std::false_type {};
template <typename T>
struct has_operator_delete_size<
    T,
    void_t<decltype(static_cast<void (*)(void *, size_t)>(T::operator delete))>> : std::true_type {
};

template <typename T, enable_if_t<has_operator_delete<T>::value, int> = 0>
void call_operator_delete(T *p, size_t, size_t) {
    T::operator delete(p);
}
template <
    typename T,
    enable_if_t<!has_operator_delete<T>::value && has_operator_delete_size<T>::value, int> = 0>
void call_operator_delete(T *p, size_t s, size_t) {
    T::operator delete(p, s);
}

inline void call_operator_delete(void *p, size_t s, size_t a) {
    (void) s;
    (void) a;
#if defined(__cpp_aligned_new) && (!defined(_MSC_VER) || _MSC_VER >= 1912)
    if (a > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
#    ifdef __cpp_sized_deallocation
        ::operator delete(p, s, std::align_val_t(a));
#    else
        ::operator delete(p, std::align_val_t(a));
#    endif
        return;
    }
#endif
#ifdef __cpp_sized_deallocation
    ::operator delete(p, s);
#else
    ::operator delete(p);
#endif
}

inline void add_class_method(object &cls, const char *name_, const cpp_function &cf) {
    cls.attr(cf.name()) = cf;
    if (std::strcmp(name_, "__eq__") == 0 && !cls.attr("__dict__").contains("__hash__")) {
        cls.attr("__hash__") = none();
    }
}

PYBIND11_NAMESPACE_END(detail)



template <typename  , typename F>
auto method_adaptor(F &&f) -> decltype(std::forward<F>(f)) {
    return std::forward<F>(f);
}

template <typename Derived, typename Return, typename Class, typename... Args>
auto method_adaptor(Return (Class::*pmf)(Args...)) -> Return (Derived::*)(Args...) {
    static_assert(
        detail::is_accessible_base_of<Class, Derived>::value,
        "Cannot bind an inaccessible base class method; use a lambda definition instead");
    return pmf;
}

template <typename Derived, typename Return, typename Class, typename... Args>
auto method_adaptor(Return (Class::*pmf)(Args...) const) -> Return (Derived::*)(Args...) const {
    static_assert(
        detail::is_accessible_base_of<Class, Derived>::value,
        "Cannot bind an inaccessible base class method; use a lambda definition instead");
    return pmf;
}

template <typename type_, typename... options>
class class_ : public detail::generic_type {
    template <typename T>
    using is_holder = detail::is_holder_type<type_, T>;
    template <typename T>
    using is_subtype = detail::is_strict_base_of<type_, T>;
    template <typename T>
    using is_base = detail::is_strict_base_of<T, type_>;

    template <typename T>
    struct is_valid_class_option : detail::any_of<is_holder<T>, is_subtype<T>, is_base<T>> {};

public:
    using type = type_;
    using type_alias = detail::exactly_one_t<is_subtype, void, options...>;
    constexpr static bool has_alias = !std::is_void<type_alias>::value;
    using holder_type = detail::exactly_one_t<is_holder, std::unique_ptr<type>, options...>;

    static_assert(detail::all_of<is_valid_class_option<options>...>::value,
                  "Unknown/invalid class_ template parameters provided");

    static_assert(!has_alias || std::is_polymorphic<type>::value,
                  "Cannot use an alias class with a non-polymorphic type");

    PYBIND11_OBJECT(class_, generic_type, PyType_Check)

    template <typename... Extra>
    class_(handle scope, const char *name, const Extra &...extra) {
        using namespace detail;


        static_assert(
            none_of<is_pyobject<Extra>...>::value ||
                (constexpr_sum(is_pyobject<Extra>::value...) == 1 &&
                 constexpr_sum(is_base<options>::value...) == 0 &&

                 none_of<std::is_same<multiple_inheritance, Extra>...>::value),
            "Error: multiple inheritance bases must be specified via class_ template options");

        type_record record;
        record.scope = scope;
        record.name = name;
        record.type = &typeid(type);
        record.type_size = sizeof(conditional_t<has_alias, type_alias, type>);
        record.type_align = alignof(conditional_t<has_alias, type_alias, type> &);
        record.holder_size = sizeof(holder_type);
        record.init_instance = init_instance;
        record.dealloc = dealloc;
        record.default_holder = detail::is_instantiation<std::unique_ptr, holder_type>::value;

        set_operator_new<type>(&record);


        PYBIND11_EXPAND_SIDE_EFFECTS(add_base<options>(record));


        process_attributes<Extra...>::init(extra..., &record);

        generic_type::initialize(record);

        if (has_alias) {
            auto &instances = record.module_local ? get_local_internals().registered_types_cpp
                                                  : get_internals().registered_types_cpp;
            instances[std::type_index(typeid(type_alias))]
                = instances[std::type_index(typeid(type))];
        }
    }

    template <typename Base, detail::enable_if_t<is_base<Base>::value, int> = 0>
    static void add_base(detail::type_record &rec) {
        rec.add_base(typeid(Base), [](void *src) -> void * {
            return static_cast<Base *>(reinterpret_cast<type *>(src));
        });
    }

    template <typename Base, detail::enable_if_t<!is_base<Base>::value, int> = 0>
    static void add_base(detail::type_record &) {}

    template <typename Func, typename... Extra>
    class_ &def(const char *name_, Func &&f, const Extra &...extra) {
        cpp_function cf(method_adaptor<type>(std::forward<Func>(f)),
                        name(name_),
                        is_method(*this),
                        sibling(getattr(*this, name_, none())),
                        extra...);
        add_class_method(*this, name_, cf);
        return *this;
    }

    template <typename Func, typename... Extra>
    class_ &def_static(const char *name_, Func &&f, const Extra &...extra) {
        static_assert(!std::is_member_function_pointer<Func>::value,
                      "def_static(...) called with a non-static member function pointer");
        cpp_function cf(std::forward<Func>(f),
                        name(name_),
                        scope(*this),
                        sibling(getattr(*this, name_, none())),
                        extra...);
        auto cf_name = cf.name();
        attr(std::move(cf_name)) = staticmethod(std::move(cf));
        return *this;
    }

    template <detail::op_id id, detail::op_type ot, typename L, typename R, typename... Extra>
    class_ &def(const detail::op_<id, ot, L, R> &op, const Extra &...extra) {
        op.execute(*this, extra...);
        return *this;
    }

    template <detail::op_id id, detail::op_type ot, typename L, typename R, typename... Extra>
    class_ &def_cast(const detail::op_<id, ot, L, R> &op, const Extra &...extra) {
        op.execute_cast(*this, extra...);
        return *this;
    }

    template <typename... Args, typename... Extra>
    class_ &def(const detail::initimpl::constructor<Args...> &init, const Extra &...extra) {
        PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(init);
        init.execute(*this, extra...);
        return *this;
    }

    template <typename... Args, typename... Extra>
    class_ &def(const detail::initimpl::alias_constructor<Args...> &init, const Extra &...extra) {
        PYBIND11_WORKAROUND_INCORRECT_MSVC_C4100(init);
        init.execute(*this, extra...);
        return *this;
    }

    template <typename... Args, typename... Extra>
    class_ &def(detail::initimpl::factory<Args...> &&init, const Extra &...extra) {
        std::move(init).execute(*this, extra...);
        return *this;
    }

    template <typename... Args, typename... Extra>
    class_ &def(detail::initimpl::pickle_factory<Args...> &&pf, const Extra &...extra) {
        std::move(pf).execute(*this, extra...);
        return *this;
    }

    template <typename Func>
    class_ &def_buffer(Func &&func) {
        struct capture {
            Func func;
        };
        auto *ptr = new capture{std::forward<Func>(func)};
        install_buffer_funcs(
            [](PyObject *obj, void *ptr) -> buffer_info * {
                detail::make_caster<type> caster;
                if (!caster.load(obj, false)) {
                    return nullptr;
                }
                return new buffer_info(((capture *) ptr)->func(std::move(caster)));
            },
            ptr);
        weakref(m_ptr, cpp_function([ptr](handle wr) {
                    delete ptr;
                    wr.dec_ref();
                }))
            .release();
        return *this;
    }

    template <typename Return, typename Class, typename... Args>
    class_ &def_buffer(Return (Class::*func)(Args...)) {
        return def_buffer([func](type &obj) { return (obj.*func)(); });
    }

    template <typename Return, typename Class, typename... Args>
    class_ &def_buffer(Return (Class::*func)(Args...) const) {
        return def_buffer([func](const type &obj) { return (obj.*func)(); });
    }

    template <typename C, typename D, typename... Extra>
    class_ &def_readwrite(const char *name, D C::*pm, const Extra &...extra) {
        static_assert(std::is_same<C, type>::value || std::is_base_of<C, type>::value,
                      "def_readwrite() requires a class member (or base class member)");
        cpp_function fget([pm](const type &c) -> const D & { return c.*pm; }, is_method(*this)),
            fset([pm](type &c, const D &value) { c.*pm = value; }, is_method(*this));
        def_property(name, fget, fset, return_value_policy::reference_internal, extra...);
        return *this;
    }

    template <typename C, typename D, typename... Extra>
    class_ &def_readonly(const char *name, const D C::*pm, const Extra &...extra) {
        static_assert(std::is_same<C, type>::value || std::is_base_of<C, type>::value,
                      "def_readonly() requires a class member (or base class member)");
        cpp_function fget([pm](const type &c) -> const D & { return c.*pm; }, is_method(*this));
        def_property_readonly(name, fget, return_value_policy::reference_internal, extra...);
        return *this;
    }

    template <typename D, typename... Extra>
    class_ &def_readwrite_static(const char *name, D *pm, const Extra &...extra) {
        cpp_function fget([pm](const object &) -> const D & { return *pm; }, scope(*this)),
            fset([pm](const object &, const D &value) { *pm = value; }, scope(*this));
        def_property_static(name, fget, fset, return_value_policy::reference, extra...);
        return *this;
    }

    template <typename D, typename... Extra>
    class_ &def_readonly_static(const char *name, const D *pm, const Extra &...extra) {
        cpp_function fget([pm](const object &) -> const D & { return *pm; }, scope(*this));
        def_property_readonly_static(name, fget, return_value_policy::reference, extra...);
        return *this;
    }


    template <typename Getter, typename... Extra>
    class_ &def_property_readonly(const char *name, const Getter &fget, const Extra &...extra) {
        return def_property_readonly(name,
                                     cpp_function(method_adaptor<type>(fget)),
                                     return_value_policy::reference_internal,
                                     extra...);
    }


    template <typename... Extra>
    class_ &
    def_property_readonly(const char *name, const cpp_function &fget, const Extra &...extra) {
        return def_property(name, fget, nullptr, extra...);
    }


    template <typename Getter, typename... Extra>
    class_ &
    def_property_readonly_static(const char *name, const Getter &fget, const Extra &...extra) {
        return def_property_readonly_static(
            name, cpp_function(fget), return_value_policy::reference, extra...);
    }


    template <typename... Extra>
    class_ &def_property_readonly_static(const char *name,
                                         const cpp_function &fget,
                                         const Extra &...extra) {
        return def_property_static(name, fget, nullptr, extra...);
    }


    template <typename Getter, typename Setter, typename... Extra>
    class_ &
    def_property(const char *name, const Getter &fget, const Setter &fset, const Extra &...extra) {
        return def_property(name, fget, cpp_function(method_adaptor<type>(fset)), extra...);
    }
    template <typename Getter, typename... Extra>
    class_ &def_property(const char *name,
                         const Getter &fget,
                         const cpp_function &fset,
                         const Extra &...extra) {
        return def_property(name,
                            cpp_function(method_adaptor<type>(fget)),
                            fset,
                            return_value_policy::reference_internal,
                            extra...);
    }


    template <typename... Extra>
    class_ &def_property(const char *name,
                         const cpp_function &fget,
                         const cpp_function &fset,
                         const Extra &...extra) {
        return def_property_static(name, fget, fset, is_method(*this), extra...);
    }


    template <typename Getter, typename... Extra>
    class_ &def_property_static(const char *name,
                                const Getter &fget,
                                const cpp_function &fset,
                                const Extra &...extra) {
        return def_property_static(
            name, cpp_function(fget), fset, return_value_policy::reference, extra...);
    }


    template <typename... Extra>
    class_ &def_property_static(const char *name,
                                const cpp_function &fget,
                                const cpp_function &fset,
                                const Extra &...extra) {
        static_assert(0 == detail::constexpr_sum(std::is_base_of<arg, Extra>::value...),
                      "Argument annotations are not allowed for properties");
        auto rec_fget = get_function_record(fget), rec_fset = get_function_record(fset);
        auto *rec_active = rec_fget;
        if (rec_fget) {
            char *doc_prev = rec_fget->doc;
            detail::process_attributes<Extra...>::init(extra..., rec_fget);
            if (rec_fget->doc && rec_fget->doc != doc_prev) {
                std::free(doc_prev);
                rec_fget->doc = PYBIND11_COMPAT_STRDUP(rec_fget->doc);
            }
        }
        if (rec_fset) {
            char *doc_prev = rec_fset->doc;
            detail::process_attributes<Extra...>::init(extra..., rec_fset);
            if (rec_fset->doc && rec_fset->doc != doc_prev) {
                std::free(doc_prev);
                rec_fset->doc = PYBIND11_COMPAT_STRDUP(rec_fset->doc);
            }
            if (!rec_active) {
                rec_active = rec_fset;
            }
        }
        def_property_static_impl(name, fget, fset, rec_active);
        return *this;
    }

private:

    template <typename T>
    static void init_holder(detail::instance *inst,
                            detail::value_and_holder &v_h,
                            const holder_type *  ,
                            const std::enable_shared_from_this<T> *  ) {

        auto sh = std::dynamic_pointer_cast<typename holder_type::element_type>(
            detail::try_get_shared_from_this(v_h.value_ptr<type>()));
        if (sh) {
            new (std::addressof(v_h.holder<holder_type>())) holder_type(std::move(sh));
            v_h.set_holder_constructed();
        }

        if (!v_h.holder_constructed() && inst->owned) {
            new (std::addressof(v_h.holder<holder_type>())) holder_type(v_h.value_ptr<type>());
            v_h.set_holder_constructed();
        }
    }

    static void init_holder_from_existing(const detail::value_and_holder &v_h,
                                          const holder_type *holder_ptr,
                                          std::true_type  ) {
        new (std::addressof(v_h.holder<holder_type>()))
            holder_type(*reinterpret_cast<const holder_type *>(holder_ptr));
    }

    static void init_holder_from_existing(const detail::value_and_holder &v_h,
                                          const holder_type *holder_ptr,
                                          std::false_type  ) {
        new (std::addressof(v_h.holder<holder_type>()))
            holder_type(std::move(*const_cast<holder_type *>(holder_ptr)));
    }



    static void init_holder(detail::instance *inst,
                            detail::value_and_holder &v_h,
                            const holder_type *holder_ptr,
                            const void *  ) {
        if (holder_ptr) {
            init_holder_from_existing(v_h, holder_ptr, std::is_copy_constructible<holder_type>());
            v_h.set_holder_constructed();
        } else if (PYBIND11_SILENCE_MSVC_C4127(detail::always_construct_holder<holder_type>::value)
                   || inst->owned) {
            new (std::addressof(v_h.holder<holder_type>())) holder_type(v_h.value_ptr<type>());
            v_h.set_holder_constructed();
        }
    }





    static void init_instance(detail::instance *inst, const void *holder_ptr) {
        auto v_h = inst->get_value_and_holder(detail::get_type_info(typeid(type)));
        if (!v_h.instance_registered()) {
            register_instance(inst, v_h.value_ptr(), v_h.type);
            v_h.set_instance_registered();
        }
        init_holder(inst, v_h, (const holder_type *) holder_ptr, v_h.value_ptr<type>());
    }


    static void dealloc(detail::value_and_holder &v_h) {






        error_scope scope;
        if (v_h.holder_constructed()) {
            v_h.holder<holder_type>().~holder_type();
            v_h.set_holder_constructed(false);
        } else {
            detail::call_operator_delete(
                v_h.value_ptr<type>(), v_h.type->type_size, v_h.type->type_align);
        }
        v_h.value_ptr() = nullptr;
    }

    static detail::function_record *get_function_record(handle h) {
        h = detail::get_function(h);
        return h ? (detail::function_record *) reinterpret_borrow<capsule>(
                   PyCFunction_GET_SELF(h.ptr()))
                 : nullptr;
    }
};


template <typename... Args>
detail::initimpl::constructor<Args...> init() {
    return {};
}


template <typename... Args>
detail::initimpl::alias_constructor<Args...> init_alias() {
    return {};
}


template <typename Func, typename Ret = detail::initimpl::factory<Func>>
Ret init(Func &&f) {
    return {std::forward<Func>(f)};
}




template <typename CFunc, typename AFunc, typename Ret = detail::initimpl::factory<CFunc, AFunc>>
Ret init(CFunc &&c, AFunc &&a) {
    return {std::forward<CFunc>(c), std::forward<AFunc>(a)};
}



template <typename GetState, typename SetState>
detail::initimpl::pickle_factory<GetState, SetState> pickle(GetState &&g, SetState &&s) {
    return {std::forward<GetState>(g), std::forward<SetState>(s)};
}

PYBIND11_NAMESPACE_BEGIN(detail)

inline str enum_name(handle arg) {
    dict entries = arg.get_type().attr("__entries");
    for (auto kv : entries) {
        if (handle(kv.second[int_(0)]).equal(arg)) {
            return pybind11::str(kv.first);
        }
    }
    return "???";
}

struct enum_base {
    enum_base(const handle &base, const handle &parent) : m_base(base), m_parent(parent) {}

    PYBIND11_NOINLINE void init(bool is_arithmetic, bool is_convertible) {
        m_base.attr("__entries") = dict();
        auto property = handle((PyObject *) &PyProperty_Type);
        auto static_property = handle((PyObject *) get_internals().static_property_type);

        m_base.attr("__repr__") = cpp_function(
            [](const object &arg) -> str {
                handle type = type::handle_of(arg);
                object type_name = type.attr("__name__");
                return pybind11::str("<{}.{}: {}>")
                    .format(std::move(type_name), enum_name(arg), int_(arg));
            },
            name("__repr__"),
            is_method(m_base));

        m_base.attr("name") = property(cpp_function(&enum_name, name("name"), is_method(m_base)));

        m_base.attr("__str__") = cpp_function(
            [](handle arg) -> str {
                object type_name = type::handle_of(arg).attr("__name__");
                return pybind11::str("{}.{}").format(std::move(type_name), enum_name(arg));
            },
            name("name"),
            is_method(m_base));

        m_base.attr("__doc__") = static_property(
            cpp_function(
                [](handle arg) -> std::string {
                    std::string docstring;
                    dict entries = arg.attr("__entries");
                    if (((PyTypeObject *) arg.ptr())->tp_doc) {
                        docstring += std::string(((PyTypeObject *) arg.ptr())->tp_doc) + "\n\n";
                    }
                    docstring += "Members:";
                    for (auto kv : entries) {
                        auto key = std::string(pybind11::str(kv.first));
                        auto comment = kv.second[int_(1)];
                        docstring += "\n\n  " + key;
                        if (!comment.is_none()) {
                            docstring += " : " + (std::string) pybind11::str(comment);
                        }
                    }
                    return docstring;
                },
                name("__doc__")),
            none(),
            none(),
            "");

        m_base.attr("__members__") = static_property(cpp_function(
                                                         [](handle arg) -> dict {
                                                             dict entries = arg.attr("__entries"),
                                                                  m;
                                                             for (auto kv : entries) {
                                                                 m[kv.first] = kv.second[int_(0)];
                                                             }
                                                             return m;
                                                         },
                                                         name("__members__")),
                                                     none(),
                                                     none(),
                                                     "");

#define PYBIND11_ENUM_OP_STRICT(op, expr, strict_behavior)                                        \
    m_base.attr(op) = cpp_function(                                                               \
        [](const object &a, const object &b) {                                                    \
            if (!type::handle_of(a).is(type::handle_of(b)))                                       \
                strict_behavior;                           \
            return expr;                                                                          \
        },                                                                                        \
        name(op),                                                                                 \
        is_method(m_base),                                                                        \
        arg("other"))

#define PYBIND11_ENUM_OP_CONV(op, expr)                                                           \
    m_base.attr(op) = cpp_function(                                                               \
        [](const object &a_, const object &b_) {                                                  \
            int_ a(a_), b(b_);                                                                    \
            return expr;                                                                          \
        },                                                                                        \
        name(op),                                                                                 \
        is_method(m_base),                                                                        \
        arg("other"))

#define PYBIND11_ENUM_OP_CONV_LHS(op, expr)                                                       \
    m_base.attr(op) = cpp_function(                                                               \
        [](const object &a_, const object &b) {                                                   \
            int_ a(a_);                                                                           \
            return expr;                                                                          \
        },                                                                                        \
        name(op),                                                                                 \
        is_method(m_base),                                                                        \
        arg("other"))

        if (is_convertible) {
            PYBIND11_ENUM_OP_CONV_LHS("__eq__", !b.is_none() && a.equal(b));
            PYBIND11_ENUM_OP_CONV_LHS("__ne__", b.is_none() || !a.equal(b));

            if (is_arithmetic) {
                PYBIND11_ENUM_OP_CONV("__lt__", a < b);
                PYBIND11_ENUM_OP_CONV("__gt__", a > b);
                PYBIND11_ENUM_OP_CONV("__le__", a <= b);
                PYBIND11_ENUM_OP_CONV("__ge__", a >= b);
                PYBIND11_ENUM_OP_CONV("__and__", a & b);
                PYBIND11_ENUM_OP_CONV("__rand__", a & b);
                PYBIND11_ENUM_OP_CONV("__or__", a | b);
                PYBIND11_ENUM_OP_CONV("__ror__", a | b);
                PYBIND11_ENUM_OP_CONV("__xor__", a ^ b);
                PYBIND11_ENUM_OP_CONV("__rxor__", a ^ b);
                m_base.attr("__invert__")
                    = cpp_function([](const object &arg) { return ~(int_(arg)); },
                                   name("__invert__"),
                                   is_method(m_base));
            }
        } else {
            PYBIND11_ENUM_OP_STRICT("__eq__", int_(a).equal(int_(b)), return false);
            PYBIND11_ENUM_OP_STRICT("__ne__", !int_(a).equal(int_(b)), return true);

            if (is_arithmetic) {
#define PYBIND11_THROW throw type_error("Expected an enumeration of matching type!");
                PYBIND11_ENUM_OP_STRICT("__lt__", int_(a) < int_(b), PYBIND11_THROW);
                PYBIND11_ENUM_OP_STRICT("__gt__", int_(a) > int_(b), PYBIND11_THROW);
                PYBIND11_ENUM_OP_STRICT("__le__", int_(a) <= int_(b), PYBIND11_THROW);
                PYBIND11_ENUM_OP_STRICT("__ge__", int_(a) >= int_(b), PYBIND11_THROW);
#undef PYBIND11_THROW
            }
        }

#undef PYBIND11_ENUM_OP_CONV_LHS
#undef PYBIND11_ENUM_OP_CONV
#undef PYBIND11_ENUM_OP_STRICT

        m_base.attr("__getstate__") = cpp_function(
            [](const object &arg) { return int_(arg); }, name("__getstate__"), is_method(m_base));

        m_base.attr("__hash__") = cpp_function(
            [](const object &arg) { return int_(arg); }, name("__hash__"), is_method(m_base));
    }

    PYBIND11_NOINLINE void value(char const *name_, object value, const char *doc = nullptr) {
        dict entries = m_base.attr("__entries");
        str name(name_);
        if (entries.contains(name)) {
            std::string type_name = (std::string) str(m_base.attr("__name__"));
            throw value_error(std::move(type_name) + ": element \"" + std::string(name_)
                              + "\" already exists!");
        }

        entries[name] = std::make_pair(value, doc);
        m_base.attr(std::move(name)) = std::move(value);
    }

    PYBIND11_NOINLINE void export_values() {
        dict entries = m_base.attr("__entries");
        for (auto kv : entries) {
            m_parent.attr(kv.first) = kv.second[int_(0)];
        }
    }

    handle m_base;
    handle m_parent;
};

template <bool is_signed, size_t length>
struct equivalent_integer {};
template <>
struct equivalent_integer<true, 1> {
    using type = int8_t;
};
template <>
struct equivalent_integer<false, 1> {
    using type = uint8_t;
};
template <>
struct equivalent_integer<true, 2> {
    using type = int16_t;
};
template <>
struct equivalent_integer<false, 2> {
    using type = uint16_t;
};
template <>
struct equivalent_integer<true, 4> {
    using type = int32_t;
};
template <>
struct equivalent_integer<false, 4> {
    using type = uint32_t;
};
template <>
struct equivalent_integer<true, 8> {
    using type = int64_t;
};
template <>
struct equivalent_integer<false, 8> {
    using type = uint64_t;
};

template <typename IntLike>
using equivalent_integer_t =
    typename equivalent_integer<std::is_signed<IntLike>::value, sizeof(IntLike)>::type;

PYBIND11_NAMESPACE_END(detail)


template <typename Type>
class enum_ : public class_<Type> {
public:
    using Base = class_<Type>;
    using Base::attr;
    using Base::def;
    using Base::def_property_readonly;
    using Base::def_property_readonly_static;
    using Underlying = typename std::underlying_type<Type>::type;

    using Scalar = detail::conditional_t<detail::any_of<detail::is_std_char_type<Underlying>,
                                                        std::is_same<Underlying, bool>>::value,
                                         detail::equivalent_integer_t<Underlying>,
                                         Underlying>;

    template <typename... Extra>
    enum_(const handle &scope, const char *name, const Extra &...extra)
        : class_<Type>(scope, name, extra...), m_base(*this, scope) {
        constexpr bool is_arithmetic = detail::any_of<std::is_same<arithmetic, Extra>...>::value;
        constexpr bool is_convertible = std::is_convertible<Type, Underlying>::value;
        m_base.init(is_arithmetic, is_convertible);

        def(init([](Scalar i) { return static_cast<Type>(i); }), arg("value"));
        def_property_readonly("value", [](Type value) { return (Scalar) value; });
        def("__int__", [](Type value) { return (Scalar) value; });
        def("__index__", [](Type value) { return (Scalar) value; });
        attr("__setstate__") = cpp_function(
            [](detail::value_and_holder &v_h, Scalar arg) {
                detail::initimpl::setstate<Base>(
                    v_h, static_cast<Type>(arg), Py_TYPE(v_h.inst) != v_h.type->type);
            },
            detail::is_new_style_constructor(),
            pybind11::name("__setstate__"),
            is_method(*this),
            arg("state"));
    }


    enum_ &export_values() {
        m_base.export_values();
        return *this;
    }


    enum_ &value(char const *name, Type value, const char *doc = nullptr) {
        m_base.value(name, pybind11::cast(value, return_value_policy::copy), doc);
        return *this;
    }

private:
    detail::enum_base m_base;
};

PYBIND11_NAMESPACE_BEGIN(detail)

PYBIND11_NOINLINE void keep_alive_impl(handle nurse, handle patient) {
    if (!nurse || !patient) {
        pybind11_fail("Could not activate keep_alive!");
    }

    if (patient.is_none() || nurse.is_none()) {
        return;
    }

    auto tinfo = all_type_info(Py_TYPE(nurse.ptr()));
    if (!tinfo.empty()) {

        add_patient(nurse.ptr(), patient.ptr());
    } else {

        cpp_function disable_lifesupport([patient](handle weakref) {
            patient.dec_ref();
            weakref.dec_ref();
        });

        weakref wr(nurse, disable_lifesupport);

        patient.inc_ref();
        (void) wr.release();
    }
}

PYBIND11_NOINLINE void
keep_alive_impl(size_t Nurse, size_t Patient, function_call &call, handle ret) {
    auto get_arg = [&](size_t n) {
        if (n == 0) {
            return ret;
        }
        if (n == 1 && call.init_self) {
            return call.init_self;
        }
        if (n <= call.args.size()) {
            return call.args[n - 1];
        }
        return handle();
    };

    keep_alive_impl(get_arg(Nurse), get_arg(Patient));
}

inline std::pair<decltype(internals::registered_types_py)::iterator, bool>
all_type_info_get_cache(PyTypeObject *type) {
    auto res = get_internals()
                   .registered_types_py
#ifdef __cpp_lib_unordered_map_try_emplace
                   .try_emplace(type);
#else
                   .emplace(type, std::vector<detail::type_info *>());
#endif
    if (res.second) {


        weakref((PyObject *) type, cpp_function([type](handle wr) {
                    get_internals().registered_types_py.erase(type);


                    auto &cache = get_internals().inactive_override_cache;
                    for (auto it = cache.begin(), last = cache.end(); it != last;) {
                        if (it->first == reinterpret_cast<PyObject *>(type)) {
                            it = cache.erase(it);
                        } else {
                            ++it;
                        }
                    }

                    wr.dec_ref();
                }))
            .release();
    }

    return res;
}


template <typename Access,
          return_value_policy Policy,
          typename Iterator,
          typename Sentinel,
          typename ValueType,
          typename... Extra>
struct iterator_state {
    Iterator it;
    Sentinel end;
    bool first_or_done;
};





template <typename Iterator, typename SFINAE = decltype(*std::declval<Iterator &>())>
struct iterator_access {
    using result_type = decltype(*std::declval<Iterator &>());

    result_type operator()(Iterator &it) const { return *it; }
};

template <typename Iterator, typename SFINAE = decltype((*std::declval<Iterator &>()).first)>
class iterator_key_access {
private:
    using pair_type = decltype(*std::declval<Iterator &>());

public:

    using result_type
        = conditional_t<std::is_reference<decltype(*std::declval<Iterator &>())>::value,
                        decltype(((*std::declval<Iterator &>()).first)),
                        decltype(std::declval<pair_type>().first)>;
    result_type operator()(Iterator &it) const { return (*it).first; }
};

template <typename Iterator, typename SFINAE = decltype((*std::declval<Iterator &>()).second)>
class iterator_value_access {
private:
    using pair_type = decltype(*std::declval<Iterator &>());

public:
    using result_type
        = conditional_t<std::is_reference<decltype(*std::declval<Iterator &>())>::value,
                        decltype(((*std::declval<Iterator &>()).second)),
                        decltype(std::declval<pair_type>().second)>;
    result_type operator()(Iterator &it) const { return (*it).second; }
};

template <typename Access,
          return_value_policy Policy,
          typename Iterator,
          typename Sentinel,
          typename ValueType,
          typename... Extra>
iterator make_iterator_impl(Iterator &&first, Sentinel &&last, Extra &&...extra) {
    using state = detail::iterator_state<Access, Policy, Iterator, Sentinel, ValueType, Extra...>;


    if (!detail::get_type_info(typeid(state), false)) {
        class_<state>(handle(), "iterator", pybind11::module_local())
            .def("__iter__", [](state &s) -> state & { return s; })
            .def(
                "__next__",
                [](state &s) -> ValueType {
                    if (!s.first_or_done) {
                        ++s.it;
                    } else {
                        s.first_or_done = false;
                    }
                    if (s.it == s.end) {
                        s.first_or_done = true;
                        throw stop_iteration();
                    }
                    return Access()(s.it);

                },
                std::forward<Extra>(extra)...,
                Policy);
    }

    return cast(state{std::forward<Iterator>(first), std::forward<Sentinel>(last), true});
}

PYBIND11_NAMESPACE_END(detail)


template <return_value_policy Policy = return_value_policy::reference_internal,
          typename Iterator,
          typename Sentinel,
          typename ValueType = typename detail::iterator_access<Iterator>::result_type,
          typename... Extra>
iterator make_iterator(Iterator &&first, Sentinel &&last, Extra &&...extra) {
    return detail::make_iterator_impl<detail::iterator_access<Iterator>,
                                      Policy,
                                      Iterator,
                                      Sentinel,
                                      ValueType,
                                      Extra...>(std::forward<Iterator>(first),
                                                std::forward<Sentinel>(last),
                                                std::forward<Extra>(extra)...);
}



template <return_value_policy Policy = return_value_policy::reference_internal,
          typename Iterator,
          typename Sentinel,
          typename KeyType = typename detail::iterator_key_access<Iterator>::result_type,
          typename... Extra>
iterator make_key_iterator(Iterator &&first, Sentinel &&last, Extra &&...extra) {
    return detail::make_iterator_impl<detail::iterator_key_access<Iterator>,
                                      Policy,
                                      Iterator,
                                      Sentinel,
                                      KeyType,
                                      Extra...>(std::forward<Iterator>(first),
                                                std::forward<Sentinel>(last),
                                                std::forward<Extra>(extra)...);
}



template <return_value_policy Policy = return_value_policy::reference_internal,
          typename Iterator,
          typename Sentinel,
          typename ValueType = typename detail::iterator_value_access<Iterator>::result_type,
          typename... Extra>
iterator make_value_iterator(Iterator &&first, Sentinel &&last, Extra &&...extra) {
    return detail::make_iterator_impl<detail::iterator_value_access<Iterator>,
                                      Policy,
                                      Iterator,
                                      Sentinel,
                                      ValueType,
                                      Extra...>(std::forward<Iterator>(first),
                                                std::forward<Sentinel>(last),
                                                std::forward<Extra>(extra)...);
}



template <return_value_policy Policy = return_value_policy::reference_internal,
          typename Type,
          typename... Extra>
iterator make_iterator(Type &value, Extra &&...extra) {
    return make_iterator<Policy>(
        std::begin(value), std::end(value), std::forward<Extra>(extra)...);
}



template <return_value_policy Policy = return_value_policy::reference_internal,
          typename Type,
          typename... Extra>
iterator make_key_iterator(Type &value, Extra &&...extra) {
    return make_key_iterator<Policy>(
        std::begin(value), std::end(value), std::forward<Extra>(extra)...);
}



template <return_value_policy Policy = return_value_policy::reference_internal,
          typename Type,
          typename... Extra>
iterator make_value_iterator(Type &value, Extra &&...extra) {
    return make_value_iterator<Policy>(
        std::begin(value), std::end(value), std::forward<Extra>(extra)...);
}

template <typename InputType, typename OutputType>
void implicitly_convertible() {
    struct set_flag {
        bool &flag;
        explicit set_flag(bool &flag_) : flag(flag_) { flag_ = true; }
        ~set_flag() { flag = false; }
    };
    auto implicit_caster = [](PyObject *obj, PyTypeObject *type) -> PyObject * {
        static bool currently_used = false;
        if (currently_used) {
            return nullptr;
        }
        set_flag flag_helper(currently_used);
        if (!detail::make_caster<InputType>().load(obj, false)) {
            return nullptr;
        }
        tuple args(1);
        args[0] = obj;
        PyObject *result = PyObject_Call((PyObject *) type, args.ptr(), nullptr);
        if (result == nullptr) {
            PyErr_Clear();
        }
        return result;
    };

    if (auto *tinfo = detail::get_type_info(typeid(OutputType))) {
        tinfo->implicit_conversions.emplace_back(std::move(implicit_caster));
    } else {
        pybind11_fail("implicitly_convertible: Unable to find type " + type_id<OutputType>());
    }
}

inline void register_exception_translator(ExceptionTranslator &&translator) {
    detail::get_internals().registered_exception_translators.push_front(
        std::forward<ExceptionTranslator>(translator));
}


inline void register_local_exception_translator(ExceptionTranslator &&translator) {
    detail::get_local_internals().registered_exception_translators.push_front(
        std::forward<ExceptionTranslator>(translator));
}


template <typename type>
class exception : public object {
public:
    exception() = default;
    exception(handle scope, const char *name, handle base = PyExc_Exception) {
        std::string full_name
            = scope.attr("__name__").cast<std::string>() + std::string(".") + name;
        m_ptr = PyErr_NewException(const_cast<char *>(full_name.c_str()), base.ptr(), nullptr);
        if (hasattr(scope, "__dict__") && scope.attr("__dict__").contains(name)) {
            pybind11_fail("Error during initialization: multiple incompatible "
                          "definitions with name \""
                          + std::string(name) + "\"");
        }
        scope.attr(name) = *this;
    }


    void operator()(const char *message) { PyErr_SetString(m_ptr, message); }
};

PYBIND11_NAMESPACE_BEGIN(detail)



template <typename CppException>
exception<CppException> &get_exception_object() {
    static exception<CppException> ex;
    return ex;
}


template <typename CppException>
exception<CppException> &
register_exception_impl(handle scope, const char *name, handle base, bool isLocal) {
    auto &ex = detail::get_exception_object<CppException>();
    if (!ex) {
        ex = exception<CppException>(scope, name, base);
    }

    auto register_func
        = isLocal ? &register_local_exception_translator : &register_exception_translator;

    register_func([](std::exception_ptr p) {
        if (!p) {
            return;
        }
        try {
            std::rethrow_exception(p);
        } catch (const CppException &e) {
            detail::get_exception_object<CppException>()(e.what());
        }
    });
    return ex;
}

PYBIND11_NAMESPACE_END(detail)


template <typename CppException>
exception<CppException> &
register_exception(handle scope, const char *name, handle base = PyExc_Exception) {
    return detail::register_exception_impl<CppException>(scope, name, base, false  );
}


template <typename CppException>
exception<CppException> &
register_local_exception(handle scope, const char *name, handle base = PyExc_Exception) {
    return detail::register_exception_impl<CppException>(scope, name, base, true  );
}

PYBIND11_NAMESPACE_BEGIN(detail)
PYBIND11_NOINLINE void print(const tuple &args, const dict &kwargs) {
    auto strings = tuple(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        strings[i] = str(args[i]);
    }
    auto sep = kwargs.contains("sep") ? kwargs["sep"] : str(" ");
    auto line = sep.attr("join")(std::move(strings));

    object file;
    if (kwargs.contains("file")) {
        file = kwargs["file"].cast<object>();
    } else {
        try {
            file = module_::import("sys").attr("stdout");
        } catch (const error_already_set &) {

            return;
        }
    }

    auto write = file.attr("write");
    write(std::move(line));
    write(kwargs.contains("end") ? kwargs["end"] : str("\n"));

    if (kwargs.contains("flush") && kwargs["flush"].cast<bool>()) {
        file.attr("flush")();
    }
}
PYBIND11_NAMESPACE_END(detail)

template <return_value_policy policy = return_value_policy::automatic_reference, typename... Args>
void print(Args &&...args) {
    auto c = detail::collect_arguments<policy>(std::forward<Args>(args)...);
    detail::print(c.args(), c.kwargs());
}

inline void
error_already_set::m_fetched_error_deleter(detail::error_fetch_and_normalize *raw_ptr) {
    gil_scoped_acquire gil;
    error_scope scope;
    delete raw_ptr;
}

inline const char *error_already_set::what() const noexcept {
    gil_scoped_acquire gil;
    error_scope scope;
    return m_fetched_error->error_string().c_str();
}

PYBIND11_NAMESPACE_BEGIN(detail)

inline function
get_type_override(const void *this_ptr, const type_info *this_type, const char *name) {
    handle self = get_object_handle(this_ptr, this_type);
    if (!self) {
        return function();
    }
    handle type = type::handle_of(self);
    auto key = std::make_pair(type.ptr(), name);


    auto &cache = get_internals().inactive_override_cache;
    if (cache.find(key) != cache.end()) {
        return function();
    }

    function override = getattr(self, name, function());
    if (override.is_cpp_function()) {
        cache.insert(std::move(key));
        return function();
    }


#if !defined(PYPY_VERSION)
#    if PY_VERSION_HEX >= 0x03090000
    PyFrameObject *frame = PyThreadState_GetFrame(PyThreadState_Get());
    if (frame != nullptr) {
        PyCodeObject *f_code = PyFrame_GetCode(frame);

        if ((std::string) str(f_code->co_name) == name && f_code->co_argcount > 0) {
            PyObject *locals = PyEval_GetLocals();
            if (locals != nullptr) {
                PyObject *co_varnames = PyObject_GetAttrString((PyObject *) f_code, "co_varnames");
                PyObject *self_arg = PyTuple_GET_ITEM(co_varnames, 0);
                Py_DECREF(co_varnames);
                PyObject *self_caller = dict_getitem(locals, self_arg);
                if (self_caller == self.ptr()) {
                    Py_DECREF(f_code);
                    Py_DECREF(frame);
                    return function();
                }
            }
        }
        Py_DECREF(f_code);
        Py_DECREF(frame);
    }
#    else
    PyFrameObject *frame = PyThreadState_Get()->frame;
    if (frame != nullptr && (std::string) str(frame->f_code->co_name) == name
        && frame->f_code->co_argcount > 0) {
        PyFrame_FastToLocals(frame);
        PyObject *self_caller
            = dict_getitem(frame->f_locals, PyTuple_GET_ITEM(frame->f_code->co_varnames, 0));
        if (self_caller == self.ptr()) {
            return function();
        }
    }
#    endif

#else

    dict d;
    d["self"] = self;
    d["name"] = pybind11::str(name);
    PyObject *result
        = PyRun_String("import inspect\n"
                       "frame = inspect.currentframe()\n"
                       "if frame is not None:\n"
                       "    frame = frame.f_back\n"
                       "    if frame is not None and str(frame.f_code.co_name) == name and "
                       "frame.f_code.co_argcount > 0:\n"
                       "        self_caller = frame.f_locals[frame.f_code.co_varnames[0]]\n"
                       "        if self_caller == self:\n"
                       "            self = None\n",
                       Py_file_input,
                       d.ptr(),
                       d.ptr());
    if (result == nullptr)
        throw error_already_set();
    Py_DECREF(result);
    if (d["self"].is_none())
        return function();
#endif

    return override;
}
PYBIND11_NAMESPACE_END(detail)


template <class T>
function get_override(const T *this_ptr, const char *name) {
    auto *tinfo = detail::get_type_info(typeid(T));
    return tinfo ? detail::get_type_override(this_ptr, tinfo, name) : function();
}

#define PYBIND11_OVERRIDE_IMPL(ret_type, cname, name, ...)                                        \
    do {                                                                                          \
        pybind11::gil_scoped_acquire gil;                                                         \
        pybind11::function override                                                               \
            = pybind11::get_override(static_cast<const cname *>(this), name);                     \
        if (override) {                                                                           \
            auto o = override(__VA_ARGS__);                                                       \
            if (pybind11::detail::cast_is_temporary_value_reference<ret_type>::value) {           \
                static pybind11::detail::override_caster_t<ret_type> caster;                      \
                return pybind11::detail::cast_ref<ret_type>(std::move(o), caster);                \
            }                                                                                     \
            return pybind11::detail::cast_safe<ret_type>(std::move(o));                           \
        }                                                                                         \
    } while (false)


#define PYBIND11_OVERRIDE_NAME(ret_type, cname, name, fn, ...)                                    \
    do {                                                                                          \
        PYBIND11_OVERRIDE_IMPL(PYBIND11_TYPE(ret_type), PYBIND11_TYPE(cname), name, __VA_ARGS__); \
        return cname::fn(__VA_ARGS__);                                                            \
    } while (false)


#define PYBIND11_OVERRIDE_PURE_NAME(ret_type, cname, name, fn, ...)                               \
    do {                                                                                          \
        PYBIND11_OVERRIDE_IMPL(PYBIND11_TYPE(ret_type), PYBIND11_TYPE(cname), name, __VA_ARGS__); \
        pybind11::pybind11_fail(                                                                  \
            "Tried to call pure virtual function \"" PYBIND11_STRINGIFY(cname) "::" name "\"");   \
    } while (false)


#define PYBIND11_OVERRIDE(ret_type, cname, fn, ...)                                               \
    PYBIND11_OVERRIDE_NAME(PYBIND11_TYPE(ret_type), PYBIND11_TYPE(cname), #fn, fn, __VA_ARGS__)


#define PYBIND11_OVERRIDE_PURE(ret_type, cname, fn, ...)                                          \
    PYBIND11_OVERRIDE_PURE_NAME(                                                                  \
        PYBIND11_TYPE(ret_type), PYBIND11_TYPE(cname), #fn, fn, __VA_ARGS__)



PYBIND11_DEPRECATED("get_type_overload has been deprecated")
inline function
get_type_overload(const void *this_ptr, const detail::type_info *this_type, const char *name) {
    return detail::get_type_override(this_ptr, this_type, name);
}

template <class T>
inline function get_overload(const T *this_ptr, const char *name) {
    return get_override(this_ptr, name);
}

#define PYBIND11_OVERLOAD_INT(ret_type, cname, name, ...)                                         \
    PYBIND11_OVERRIDE_IMPL(PYBIND11_TYPE(ret_type), PYBIND11_TYPE(cname), name, __VA_ARGS__)
#define PYBIND11_OVERLOAD_NAME(ret_type, cname, name, fn, ...)                                    \
    PYBIND11_OVERRIDE_NAME(PYBIND11_TYPE(ret_type), PYBIND11_TYPE(cname), name, fn, __VA_ARGS__)
#define PYBIND11_OVERLOAD_PURE_NAME(ret_type, cname, name, fn, ...)                               \
    PYBIND11_OVERRIDE_PURE_NAME(                                                                  \
        PYBIND11_TYPE(ret_type), PYBIND11_TYPE(cname), name, fn, __VA_ARGS__);
#define PYBIND11_OVERLOAD(ret_type, cname, fn, ...)                                               \
    PYBIND11_OVERRIDE(PYBIND11_TYPE(ret_type), PYBIND11_TYPE(cname), fn, __VA_ARGS__)
#define PYBIND11_OVERLOAD_PURE(ret_type, cname, fn, ...)                                          \
    PYBIND11_OVERRIDE_PURE(PYBIND11_TYPE(ret_type), PYBIND11_TYPE(cname), fn, __VA_ARGS__);

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)

#if defined(__GNUC__) && __GNUC__ == 7
#    pragma GCC diagnostic pop
#endif
