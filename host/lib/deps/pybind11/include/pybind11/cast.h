/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "pytypes.h"
#include "detail/typeid.h"
#include "detail/descr.h"
#include "detail/internals.h"
#include <array>
#include <limits>
#include <tuple>
#include <type_traits>

#if defined(PYBIND11_CPP17)
#  if defined(__has_include)
#    if __has_include(<string_view>)
#      define PYBIND11_HAS_STRING_VIEW
#    endif
#  elif defined(_MSC_VER)
#    define PYBIND11_HAS_STRING_VIEW
#  endif
#endif
#ifdef PYBIND11_HAS_STRING_VIEW
#include <string_view>
#endif

#if defined(__cpp_lib_char8_t) && __cpp_lib_char8_t >= 201811L
#  define PYBIND11_HAS_U8STRING
#endif

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)



class loader_life_support {
public:

    loader_life_support() {
        get_internals().loader_patient_stack.push_back(nullptr);
    }


    ~loader_life_support() {
        auto &stack = get_internals().loader_patient_stack;
        if (stack.empty())
            pybind11_fail("loader_life_support: internal error");

        auto ptr = stack.back();
        stack.pop_back();
        Py_CLEAR(ptr);


        if (stack.capacity() > 16 && !stack.empty() && stack.capacity() / stack.size() > 2)
            stack.shrink_to_fit();
    }



    PYBIND11_NOINLINE static void add_patient(handle h) {
        auto &stack = get_internals().loader_patient_stack;
        if (stack.empty())
            throw cast_error("When called outside a bound function, py::cast() cannot "
                             "do Python -> C++ conversions which require the creation "
                             "of temporary values");

        auto &list_ptr = stack.back();
        if (list_ptr == nullptr) {
            list_ptr = PyList_New(1);
            if (!list_ptr)
                pybind11_fail("loader_life_support: error allocating list");
            PyList_SET_ITEM(list_ptr, 0, h.inc_ref().ptr());
        } else {
            auto result = PyList_Append(list_ptr, h.ptr());
            if (result == -1)
                pybind11_fail("loader_life_support: error adding patient");
        }
    }
};




inline std::pair<decltype(internals::registered_types_py)::iterator, bool> all_type_info_get_cache(PyTypeObject *type);


PYBIND11_NOINLINE inline void all_type_info_populate(PyTypeObject *t, std::vector<type_info *> &bases) {
    std::vector<PyTypeObject *> check;
    for (handle parent : reinterpret_borrow<tuple>(t->tp_bases))
        check.push_back((PyTypeObject *) parent.ptr());

    auto const &type_dict = get_internals().registered_types_py;
    for (size_t i = 0; i < check.size(); i++) {
        auto type = check[i];

        if (!PyType_Check((PyObject *) type)) continue;


        auto it = type_dict.find(type);
        if (it != type_dict.end()) {




            for (auto *tinfo : it->second) {



                bool found = false;
                for (auto *known : bases) {
                    if (known == tinfo) { found = true; break; }
                }
                if (!found) bases.push_back(tinfo);
            }
        }
        else if (type->tp_bases) {


            if (i + 1 == check.size()) {



                check.pop_back();
                i--;
            }
            for (handle parent : reinterpret_borrow<tuple>(type->tp_bases))
                check.push_back((PyTypeObject *) parent.ptr());
        }
    }
}


inline const std::vector<detail::type_info *> &all_type_info(PyTypeObject *type) {
    auto ins = all_type_info_get_cache(type);
    if (ins.second)

        all_type_info_populate(type, ins.first->second);

    return ins.first->second;
}


PYBIND11_NOINLINE inline detail::type_info* get_type_info(PyTypeObject *type) {
    auto &bases = all_type_info(type);
    if (bases.empty())
        return nullptr;
    if (bases.size() > 1)
        pybind11_fail("pybind11::detail::get_type_info: type has multiple pybind11-registered bases");
    return bases.front();
}

inline detail::type_info *get_local_type_info(const std::type_index &tp) {
    auto &locals = registered_local_types_cpp();
    auto it = locals.find(tp);
    if (it != locals.end())
        return it->second;
    return nullptr;
}

inline detail::type_info *get_global_type_info(const std::type_index &tp) {
    auto &types = get_internals().registered_types_cpp;
    auto it = types.find(tp);
    if (it != types.end())
        return it->second;
    return nullptr;
}


PYBIND11_NOINLINE inline detail::type_info *get_type_info(const std::type_index &tp,
                                                          bool throw_if_missing = false) {
    if (auto ltype = get_local_type_info(tp))
        return ltype;
    if (auto gtype = get_global_type_info(tp))
        return gtype;

    if (throw_if_missing) {
        std::string tname = tp.name();
        detail::clean_type_id(tname);
        pybind11_fail("pybind11::detail::get_type_info: unable to find type info for \"" + tname + "\"");
    }
    return nullptr;
}

PYBIND11_NOINLINE inline handle get_type_handle(const std::type_info &tp, bool throw_if_missing) {
    detail::type_info *type_info = get_type_info(tp, throw_if_missing);
    return handle(type_info ? ((PyObject *) type_info->type) : nullptr);
}

struct value_and_holder {
    instance *inst = nullptr;
    size_t index = 0u;
    const detail::type_info *type = nullptr;
    void **vh = nullptr;


    value_and_holder(instance *i, const detail::type_info *type, size_t vpos, size_t index) :
        inst{i}, index{index}, type{type},
        vh{inst->simple_layout ? inst->simple_value_holder : &inst->nonsimple.values_and_holders[vpos]}
    {}


    value_and_holder() = default;


    value_and_holder(size_t index) : index{index} {}

    template <typename V = void> V *&value_ptr() const {
        return reinterpret_cast<V *&>(vh[0]);
    }

    explicit operator bool() const { return value_ptr(); }

    template <typename H> H &holder() const {
        return reinterpret_cast<H &>(vh[1]);
    }
    bool holder_constructed() const {
        return inst->simple_layout
            ? inst->simple_holder_constructed
            : inst->nonsimple.status[index] & instance::status_holder_constructed;
    }
    void set_holder_constructed(bool v = true) {
        if (inst->simple_layout)
            inst->simple_holder_constructed = v;
        else if (v)
            inst->nonsimple.status[index] |= instance::status_holder_constructed;
        else
            inst->nonsimple.status[index] &= (uint8_t) ~instance::status_holder_constructed;
    }
    bool instance_registered() const {
        return inst->simple_layout
            ? inst->simple_instance_registered
            : inst->nonsimple.status[index] & instance::status_instance_registered;
    }
    void set_instance_registered(bool v = true) {
        if (inst->simple_layout)
            inst->simple_instance_registered = v;
        else if (v)
            inst->nonsimple.status[index] |= instance::status_instance_registered;
        else
            inst->nonsimple.status[index] &= (uint8_t) ~instance::status_instance_registered;
    }
};


struct values_and_holders {
private:
    instance *inst;
    using type_vec = std::vector<detail::type_info *>;
    const type_vec &tinfo;

public:
    values_and_holders(instance *inst) : inst{inst}, tinfo(all_type_info(Py_TYPE(inst))) {}

    struct iterator {
    private:
        instance *inst = nullptr;
        const type_vec *types = nullptr;
        value_and_holder curr;
        friend struct values_and_holders;
        iterator(instance *inst, const type_vec *tinfo)
            : inst{inst}, types{tinfo},
            curr(inst  ,
                 types->empty() ? nullptr : (*types)[0]  ,
                 0,
                 0  )
        {}

        iterator(size_t end) : curr(end) {}
    public:
        bool operator==(const iterator &other) const { return curr.index == other.curr.index; }
        bool operator!=(const iterator &other) const { return curr.index != other.curr.index; }
        iterator &operator++() {
            if (!inst->simple_layout)
                curr.vh += 1 + (*types)[curr.index]->holder_size_in_ptrs;
            ++curr.index;
            curr.type = curr.index < types->size() ? (*types)[curr.index] : nullptr;
            return *this;
        }
        value_and_holder &operator*() { return curr; }
        value_and_holder *operator->() { return &curr; }
    };

    iterator begin() { return iterator(inst, &tinfo); }
    iterator end() { return iterator(tinfo.size()); }

    iterator find(const type_info *find_type) {
        auto it = begin(), endit = end();
        while (it != endit && it->type != find_type) ++it;
        return it;
    }

    size_t size() { return tinfo.size(); }
};


PYBIND11_NOINLINE inline value_and_holder instance::get_value_and_holder(const type_info *find_type  , bool throw_if_missing  ) {

    if (!find_type || Py_TYPE(this) == find_type->type)
        return value_and_holder(this, find_type, 0, 0);

    detail::values_and_holders vhs(this);
    auto it = vhs.find(find_type);
    if (it != vhs.end())
        return *it;

    if (!throw_if_missing)
        return value_and_holder();

#if defined(NDEBUG)
    pybind11_fail("pybind11::detail::instance::get_value_and_holder: "
            "type is not a pybind11 base of the given instance "
            "(compile in debug mode for type details)");
#else
    pybind11_fail("pybind11::detail::instance::get_value_and_holder: `" +
            get_fully_qualified_tp_name(find_type->type) + "' is not a pybind11 base of the given `" +
            get_fully_qualified_tp_name(Py_TYPE(this)) + "' instance");
#endif
}

PYBIND11_NOINLINE inline void instance::allocate_layout() {
    auto &tinfo = all_type_info(Py_TYPE(this));

    const size_t n_types = tinfo.size();

    if (n_types == 0)
        pybind11_fail("instance allocation failed: new instance has no pybind11-registered base types");

    simple_layout =
        n_types == 1 && tinfo.front()->holder_size_in_ptrs <= instance_simple_holder_in_ptrs();


    if (simple_layout) {
        simple_value_holder[0] = nullptr;
        simple_holder_constructed = false;
        simple_instance_registered = false;
    }
    else {




        size_t space = 0;
        for (auto t : tinfo) {
            space += 1;
            space += t->holder_size_in_ptrs;
        }
        size_t flags_at = space;
        space += size_in_ptrs(n_types);






#if PY_VERSION_HEX >= 0x03050000
        nonsimple.values_and_holders = (void **) PyMem_Calloc(space, sizeof(void *));
        if (!nonsimple.values_and_holders) throw std::bad_alloc();
#else
        nonsimple.values_and_holders = (void **) PyMem_New(void *, space);
        if (!nonsimple.values_and_holders) throw std::bad_alloc();
        std::memset(nonsimple.values_and_holders, 0, space * sizeof(void *));
#endif
        nonsimple.status = reinterpret_cast<uint8_t *>(&nonsimple.values_and_holders[flags_at]);
    }
    owned = true;
}

PYBIND11_NOINLINE inline void instance::deallocate_layout() {
    if (!simple_layout)
        PyMem_Free(nonsimple.values_and_holders);
}

PYBIND11_NOINLINE inline bool isinstance_generic(handle obj, const std::type_info &tp) {
    handle type = detail::get_type_handle(tp, false);
    if (!type)
        return false;
    return isinstance(obj, type);
}

PYBIND11_NOINLINE inline std::string error_string() {
    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_RuntimeError, "Unknown internal error occurred");
        return "Unknown internal error occurred";
    }

    error_scope scope;

    std::string errorString;
    if (scope.type) {
        errorString += handle(scope.type).attr("__name__").cast<std::string>();
        errorString += ": ";
    }
    if (scope.value)
        errorString += (std::string) str(scope.value);

    PyErr_NormalizeException(&scope.type, &scope.value, &scope.trace);

#if PY_MAJOR_VERSION >= 3
    if (scope.trace != nullptr)
        PyException_SetTraceback(scope.value, scope.trace);
#endif

#if !defined(PYPY_VERSION)
    if (scope.trace) {
        auto *trace = (PyTracebackObject *) scope.trace;


        while (trace->tb_next)
            trace = trace->tb_next;

        PyFrameObject *frame = trace->tb_frame;
        errorString += "\n\nAt:\n";
        while (frame) {
            int lineno = PyFrame_GetLineNumber(frame);
            errorString +=
                "  " + handle(frame->f_code->co_filename).cast<std::string>() +
                "(" + std::to_string(lineno) + "): " +
                handle(frame->f_code->co_name).cast<std::string>() + "\n";
            frame = frame->f_back;
        }
    }
#endif

    return errorString;
}

PYBIND11_NOINLINE inline handle get_object_handle(const void *ptr, const detail::type_info *type ) {
    auto &instances = get_internals().registered_instances;
    auto range = instances.equal_range(ptr);
    for (auto it = range.first; it != range.second; ++it) {
        for (const auto &vh : values_and_holders(it->second)) {
            if (vh.type == type)
                return handle((PyObject *) it->second);
        }
    }
    return handle();
}

inline PyThreadState *get_thread_state_unchecked() {
#if defined(PYPY_VERSION)
    return PyThreadState_GET();
#elif PY_VERSION_HEX < 0x03000000
    return _PyThreadState_Current;
#elif PY_VERSION_HEX < 0x03050000
    return (PyThreadState*) _Py_atomic_load_relaxed(&_PyThreadState_Current);
#elif PY_VERSION_HEX < 0x03050200
    return (PyThreadState*) _PyThreadState_Current.value;
#else
    return _PyThreadState_UncheckedGet();
#endif
}


inline void keep_alive_impl(handle nurse, handle patient);
inline PyObject *make_new_instance(PyTypeObject *type);

class type_caster_generic {
public:
    PYBIND11_NOINLINE type_caster_generic(const std::type_info &type_info)
        : typeinfo(get_type_info(type_info)), cpptype(&type_info) { }

    type_caster_generic(const type_info *typeinfo)
        : typeinfo(typeinfo), cpptype(typeinfo ? typeinfo->cpptype : nullptr) { }

    bool load(handle src, bool convert) {
        return load_impl<type_caster_generic>(src, convert);
    }

    PYBIND11_NOINLINE static handle cast(const void *_src, return_value_policy policy, handle parent,
                                         const detail::type_info *tinfo,
                                         void *(*copy_constructor)(const void *),
                                         void *(*move_constructor)(const void *),
                                         const void *existing_holder = nullptr) {
        if (!tinfo)
            return handle();

        void *src = const_cast<void *>(_src);
        if (src == nullptr)
            return none().release();

        auto it_instances = get_internals().registered_instances.equal_range(src);
        for (auto it_i = it_instances.first; it_i != it_instances.second; ++it_i) {
            for (auto instance_type : detail::all_type_info(Py_TYPE(it_i->second))) {
                if (instance_type && same_type(*instance_type->cpptype, *tinfo->cpptype))
                    return handle((PyObject *) it_i->second).inc_ref();
            }
        }

        auto inst = reinterpret_steal<object>(make_new_instance(tinfo->type));
        auto wrapper = reinterpret_cast<instance *>(inst.ptr());
        wrapper->owned = false;
        void *&valueptr = values_and_holders(wrapper).begin()->value_ptr();

        switch (policy) {
            case return_value_policy::automatic:
            case return_value_policy::take_ownership:
                valueptr = src;
                wrapper->owned = true;
                break;

            case return_value_policy::automatic_reference:
            case return_value_policy::reference:
                valueptr = src;
                wrapper->owned = false;
                break;

            case return_value_policy::copy:
                if (copy_constructor)
                    valueptr = copy_constructor(src);
                else {
#if defined(NDEBUG)
                    throw cast_error("return_value_policy = copy, but type is "
                                     "non-copyable! (compile in debug mode for details)");
#else
                    std::string type_name(tinfo->cpptype->name());
                    detail::clean_type_id(type_name);
                    throw cast_error("return_value_policy = copy, but type " +
                                     type_name + " is non-copyable!");
#endif
                }
                wrapper->owned = true;
                break;

            case return_value_policy::move:
                if (move_constructor)
                    valueptr = move_constructor(src);
                else if (copy_constructor)
                    valueptr = copy_constructor(src);
                else {
#if defined(NDEBUG)
                    throw cast_error("return_value_policy = move, but type is neither "
                                     "movable nor copyable! "
                                     "(compile in debug mode for details)");
#else
                    std::string type_name(tinfo->cpptype->name());
                    detail::clean_type_id(type_name);
                    throw cast_error("return_value_policy = move, but type " +
                                     type_name + " is neither movable nor copyable!");
#endif
                }
                wrapper->owned = true;
                break;

            case return_value_policy::reference_internal:
                valueptr = src;
                wrapper->owned = false;
                keep_alive_impl(inst, parent);
                break;

            default:
                throw cast_error("unhandled return_value_policy: should not happen!");
        }

        tinfo->init_instance(wrapper, existing_holder);

        return inst.release();
    }


    void load_value(value_and_holder &&v_h) {
        auto *&vptr = v_h.value_ptr();

        if (vptr == nullptr) {
            auto *type = v_h.type ? v_h.type : typeinfo;
            if (type->operator_new) {
                vptr = type->operator_new(type->type_size);
            } else {
                #if defined(__cpp_aligned_new) && (!defined(_MSC_VER) || _MSC_VER >= 1912)
                    if (type->type_align > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
                        vptr = ::operator new(type->type_size,
                                              std::align_val_t(type->type_align));
                    else
                #endif
                vptr = ::operator new(type->type_size);
            }
        }
        value = vptr;
    }
    bool try_implicit_casts(handle src, bool convert) {
        for (auto &cast : typeinfo->implicit_casts) {
            type_caster_generic sub_caster(*cast.first);
            if (sub_caster.load(src, convert)) {
                value = cast.second(sub_caster.value);
                return true;
            }
        }
        return false;
    }
    bool try_direct_conversions(handle src) {
        for (auto &converter : *typeinfo->direct_conversions) {
            if (converter(src.ptr(), value))
                return true;
        }
        return false;
    }
    void check_holder_compat() {}

    PYBIND11_NOINLINE static void *local_load(PyObject *src, const type_info *ti) {
        auto caster = type_caster_generic(ti);
        if (caster.load(src, false))
            return caster.value;
        return nullptr;
    }



    PYBIND11_NOINLINE bool try_load_foreign_module_local(handle src) {
        constexpr auto *local_key = PYBIND11_MODULE_LOCAL_ID;
        const auto pytype = type::handle_of(src);
        if (!hasattr(pytype, local_key))
            return false;

        type_info *foreign_typeinfo = reinterpret_borrow<capsule>(getattr(pytype, local_key));

        if (foreign_typeinfo->module_local_load == &local_load
            || (cpptype && !same_type(*cpptype, *foreign_typeinfo->cpptype)))
            return false;

        if (auto result = foreign_typeinfo->module_local_load(src.ptr(), foreign_typeinfo)) {
            value = result;
            return true;
        }
        return false;
    }




    template <typename ThisT>
    PYBIND11_NOINLINE bool load_impl(handle src, bool convert) {
        if (!src) return false;
        if (!typeinfo) return try_load_foreign_module_local(src);
        if (src.is_none()) {

            if (!convert) return false;
            value = nullptr;
            return true;
        }

        auto &this_ = static_cast<ThisT &>(*this);
        this_.check_holder_compat();

        PyTypeObject *srctype = Py_TYPE(src.ptr());



        if (srctype == typeinfo->type) {
            this_.load_value(reinterpret_cast<instance *>(src.ptr())->get_value_and_holder());
            return true;
        }

        else if (PyType_IsSubtype(srctype, typeinfo->type)) {
            auto &bases = all_type_info(srctype);
            bool no_cpp_mi = typeinfo->simple_type;







            if (bases.size() == 1 && (no_cpp_mi || bases.front()->type == typeinfo->type)) {
                this_.load_value(reinterpret_cast<instance *>(src.ptr())->get_value_and_holder());
                return true;
            }



            else if (bases.size() > 1) {
                for (auto base : bases) {
                    if (no_cpp_mi ? PyType_IsSubtype(base->type, typeinfo->type) : base->type == typeinfo->type) {
                        this_.load_value(reinterpret_cast<instance *>(src.ptr())->get_value_and_holder(base));
                        return true;
                    }
                }
            }




            if (this_.try_implicit_casts(src, convert))
                return true;
        }


        if (convert) {
            for (auto &converter : typeinfo->implicit_conversions) {
                auto temp = reinterpret_steal<object>(converter(src.ptr(), typeinfo->type));
                if (load_impl<ThisT>(temp, false)) {
                    loader_life_support::add_patient(temp);
                    return true;
                }
            }
            if (this_.try_direct_conversions(src))
                return true;
        }


        if (typeinfo->module_local) {
            if (auto gtype = get_global_type_info(*typeinfo->cpptype)) {
                typeinfo = gtype;
                return load(src, false);
            }
        }


        return try_load_foreign_module_local(src);
    }





    PYBIND11_NOINLINE static std::pair<const void *, const type_info *> src_and_type(
            const void *src, const std::type_info &cast_type, const std::type_info *rtti_type = nullptr) {
        if (auto *tpi = get_type_info(cast_type))
            return {src, const_cast<const type_info *>(tpi)};


        std::string tname = rtti_type ? rtti_type->name() : cast_type.name();
        detail::clean_type_id(tname);
        std::string msg = "Unregistered type : " + tname;
        PyErr_SetString(PyExc_TypeError, msg.c_str());
        return {nullptr, nullptr};
    }

    const type_info *typeinfo = nullptr;
    const std::type_info *cpptype = nullptr;
    void *value = nullptr;
};


template <typename T>
using cast_op_type =
    conditional_t<std::is_pointer<remove_reference_t<T>>::value,
        typename std::add_pointer<intrinsic_t<T>>::type,
        typename std::add_lvalue_reference<intrinsic_t<T>>::type>;


template <typename T>
using movable_cast_op_type =
    conditional_t<std::is_pointer<typename std::remove_reference<T>::type>::value,
        typename std::add_pointer<intrinsic_t<T>>::type,
    conditional_t<std::is_rvalue_reference<T>::value,
        typename std::add_rvalue_reference<intrinsic_t<T>>::type,
        typename std::add_lvalue_reference<intrinsic_t<T>>::type>>;



template <typename T, typename SFINAE = void> struct is_copy_constructible : std::is_copy_constructible<T> {};




template <typename Container> struct is_copy_constructible<Container, enable_if_t<all_of<
        std::is_copy_constructible<Container>,
        std::is_same<typename Container::value_type &, typename Container::reference>,

        negation<std::is_same<Container, typename Container::value_type>>
    >::value>> : is_copy_constructible<typename Container::value_type> {};




template <typename T1, typename T2> struct is_copy_constructible<std::pair<T1, T2>>
    : all_of<is_copy_constructible<T1>, is_copy_constructible<T2>> {};


template <typename T, typename SFINAE = void> struct is_copy_assignable : std::is_copy_assignable<T> {};
template <typename Container> struct is_copy_assignable<Container, enable_if_t<all_of<
        std::is_copy_assignable<Container>,
        std::is_same<typename Container::value_type &, typename Container::reference>
    >::value>> : is_copy_assignable<typename Container::value_type> {};
template <typename T1, typename T2> struct is_copy_assignable<std::pair<T1, T2>>
    : all_of<is_copy_assignable<T1>, is_copy_assignable<T2>> {};

PYBIND11_NAMESPACE_END(detail)






















template <typename itype, typename SFINAE = void>
struct polymorphic_type_hook_base
{
    static const void *get(const itype *src, const std::type_info*&) { return src; }
};
template <typename itype>
struct polymorphic_type_hook_base<itype, detail::enable_if_t<std::is_polymorphic<itype>::value>>
{
    static const void *get(const itype *src, const std::type_info*& type) {
        type = src ? &typeid(*src) : nullptr;
        return dynamic_cast<const void*>(src);
    }
};
template <typename itype, typename SFINAE = void>
struct polymorphic_type_hook : public polymorphic_type_hook_base<itype> {};

PYBIND11_NAMESPACE_BEGIN(detail)


template <typename type> class type_caster_base : public type_caster_generic {
    using itype = intrinsic_t<type>;

public:
    static constexpr auto name = _<type>();

    type_caster_base() : type_caster_base(typeid(type)) { }
    explicit type_caster_base(const std::type_info &info) : type_caster_generic(info) { }

    static handle cast(const itype &src, return_value_policy policy, handle parent) {
        if (policy == return_value_policy::automatic || policy == return_value_policy::automatic_reference)
            policy = return_value_policy::copy;
        return cast(&src, policy, parent);
    }

    static handle cast(itype &&src, return_value_policy, handle parent) {
        return cast(&src, return_value_policy::move, parent);
    }




    static std::pair<const void *, const type_info *> src_and_type(const itype *src) {
        auto &cast_type = typeid(itype);
        const std::type_info *instance_type = nullptr;
        const void *vsrc = polymorphic_type_hook<itype>::get(src, instance_type);
        if (instance_type && !same_type(cast_type, *instance_type)) {








            if (const auto *tpi = get_type_info(*instance_type))
                return {vsrc, tpi};
        }


        return type_caster_generic::src_and_type(src, cast_type, instance_type);
    }

    static handle cast(const itype *src, return_value_policy policy, handle parent) {
        auto st = src_and_type(src);
        return type_caster_generic::cast(
            st.first, policy, parent, st.second,
            make_copy_constructor(src), make_move_constructor(src));
    }

    static handle cast_holder(const itype *src, const void *holder) {
        auto st = src_and_type(src);
        return type_caster_generic::cast(
            st.first, return_value_policy::take_ownership, {}, st.second,
            nullptr, nullptr, holder);
    }

    template <typename T> using cast_op_type = detail::cast_op_type<T>;

    operator itype*() { return (type *) value; }
    operator itype&() { if (!value) throw reference_cast_error(); return *((itype *) value); }

protected:
    using Constructor = void *(*)(const void *);


    template <typename T, typename = enable_if_t<is_copy_constructible<T>::value>>
    static auto make_copy_constructor(const T *x) -> decltype(new T(*x), Constructor{}) {
        return [](const void *arg) -> void * {
            return new T(*reinterpret_cast<const T *>(arg));
        };
    }

    template <typename T, typename = enable_if_t<std::is_move_constructible<T>::value>>
    static auto make_move_constructor(const T *x) -> decltype(new T(std::move(*const_cast<T *>(x))), Constructor{}) {
        return [](const void *arg) -> void * {
            return new T(std::move(*const_cast<T *>(reinterpret_cast<const T *>(arg))));
        };
    }

    static Constructor make_copy_constructor(...) { return nullptr; }
    static Constructor make_move_constructor(...) { return nullptr; }
};

template <typename type, typename SFINAE = void> class type_caster : public type_caster_base<type> { };
template <typename type> using make_caster = type_caster<intrinsic_t<type>>;


template <typename T> typename make_caster<T>::template cast_op_type<T> cast_op(make_caster<T> &caster) {
    return caster.operator typename make_caster<T>::template cast_op_type<T>();
}
template <typename T> typename make_caster<T>::template cast_op_type<typename std::add_rvalue_reference<T>::type>
cast_op(make_caster<T> &&caster) {
    return std::move(caster).operator
        typename make_caster<T>::template cast_op_type<typename std::add_rvalue_reference<T>::type>();
}

template <typename type> class type_caster<std::reference_wrapper<type>> {
private:
    using caster_t = make_caster<type>;
    caster_t subcaster;
    using subcaster_cast_op_type = typename caster_t::template cast_op_type<type>;
    static_assert(std::is_same<typename std::remove_const<type>::type &, subcaster_cast_op_type>::value,
            "std::reference_wrapper<T> caster requires T to have a caster with an `T &` operator");
public:
    bool load(handle src, bool convert) { return subcaster.load(src, convert); }
    static constexpr auto name = caster_t::name;
    static handle cast(const std::reference_wrapper<type> &src, return_value_policy policy, handle parent) {

        if (policy == return_value_policy::take_ownership || policy == return_value_policy::automatic)
            policy = return_value_policy::automatic_reference;
        return caster_t::cast(&src.get(), policy, parent);
    }
    template <typename T> using cast_op_type = std::reference_wrapper<type>;
    operator std::reference_wrapper<type>() { return subcaster.operator subcaster_cast_op_type&(); }
};

#define PYBIND11_TYPE_CASTER(type, py_name) \
    protected: \
        type value; \
    public: \
        static constexpr auto name = py_name; \
        template <typename T_, enable_if_t<std::is_same<type, remove_cv_t<T_>>::value, int> = 0> \
        static handle cast(T_ *src, return_value_policy policy, handle parent) { \
            if (!src) return none().release(); \
            if (policy == return_value_policy::take_ownership) { \
                auto h = cast(std::move(*src), policy, parent); delete src; return h; \
            } else { \
                return cast(*src, policy, parent); \
            } \
        } \
        operator type*() { return &value; } \
        operator type&() { return value; } \
        operator type&&() && { return std::move(value); } \
        template <typename T_> using cast_op_type = pybind11::detail::movable_cast_op_type<T_>


template <typename CharT> using is_std_char_type = any_of<
    std::is_same<CharT, char>,
#if defined(PYBIND11_HAS_U8STRING)
    std::is_same<CharT, char8_t>,
#endif
    std::is_same<CharT, char16_t>,
    std::is_same<CharT, char32_t>,
    std::is_same<CharT, wchar_t>
>;


template <typename T>
struct type_caster<T, enable_if_t<std::is_arithmetic<T>::value && !is_std_char_type<T>::value>> {
    using _py_type_0 = conditional_t<sizeof(T) <= sizeof(long), long, long long>;
    using _py_type_1 = conditional_t<std::is_signed<T>::value, _py_type_0, typename std::make_unsigned<_py_type_0>::type>;
    using py_type = conditional_t<std::is_floating_point<T>::value, double, _py_type_1>;
public:

    bool load(handle src, bool convert) {
        py_type py_value;

        if (!src)
            return false;

        if (std::is_floating_point<T>::value) {
            if (convert || PyFloat_Check(src.ptr()))
                py_value = (py_type) PyFloat_AsDouble(src.ptr());
            else
                return false;
        } else if (PyFloat_Check(src.ptr())) {
            return false;
        } else if (std::is_unsigned<py_type>::value) {
            py_value = as_unsigned<py_type>(src.ptr());
        } else {
            py_value = sizeof(T) <= sizeof(long)
                ? (py_type) PyLong_AsLong(src.ptr())
                : (py_type) PYBIND11_LONG_AS_LONGLONG(src.ptr());
        }


        bool py_err = py_value == (py_type) -1 && PyErr_Occurred();



        if (py_err || (std::is_integral<T>::value && sizeof(py_type) != sizeof(T) && py_value != (py_type) (T) py_value)) {
            bool type_error = py_err && PyErr_ExceptionMatches(
#if PY_VERSION_HEX < 0x03000000 && !defined(PYPY_VERSION)
                PyExc_SystemError
#else
                PyExc_TypeError
#endif
            );
            PyErr_Clear();
            if (type_error && convert && PyNumber_Check(src.ptr())) {
                auto tmp = reinterpret_steal<object>(std::is_floating_point<T>::value
                                                     ? PyNumber_Float(src.ptr())
                                                     : PyNumber_Long(src.ptr()));
                PyErr_Clear();
                return load(tmp, false);
            }
            return false;
        }

        value = (T) py_value;
        return true;
    }

    template<typename U = T>
    static typename std::enable_if<std::is_floating_point<U>::value, handle>::type
    cast(U src, return_value_policy  , handle  ) {
        return PyFloat_FromDouble((double) src);
    }

    template<typename U = T>
    static typename std::enable_if<!std::is_floating_point<U>::value && std::is_signed<U>::value && (sizeof(U) <= sizeof(long)), handle>::type
    cast(U src, return_value_policy  , handle  ) {
        return PYBIND11_LONG_FROM_SIGNED((long) src);
    }

    template<typename U = T>
    static typename std::enable_if<!std::is_floating_point<U>::value && std::is_unsigned<U>::value && (sizeof(U) <= sizeof(unsigned long)), handle>::type
    cast(U src, return_value_policy  , handle  ) {
        return PYBIND11_LONG_FROM_UNSIGNED((unsigned long) src);
    }

    template<typename U = T>
    static typename std::enable_if<!std::is_floating_point<U>::value && std::is_signed<U>::value && (sizeof(U) > sizeof(long)), handle>::type
    cast(U src, return_value_policy  , handle  ) {
        return PyLong_FromLongLong((long long) src);
    }

    template<typename U = T>
    static typename std::enable_if<!std::is_floating_point<U>::value && std::is_unsigned<U>::value && (sizeof(U) > sizeof(unsigned long)), handle>::type
    cast(U src, return_value_policy  , handle  ) {
        return PyLong_FromUnsignedLongLong((unsigned long long) src);
    }

    PYBIND11_TYPE_CASTER(T, _<std::is_integral<T>::value>("int", "float"));
};

template<typename T> struct void_caster {
public:
    bool load(handle src, bool) {
        if (src && src.is_none())
            return true;
        return false;
    }
    static handle cast(T, return_value_policy  , handle  ) {
        return none().inc_ref();
    }
    PYBIND11_TYPE_CASTER(T, _("None"));
};

template <> class type_caster<void_type> : public void_caster<void_type> {};

template <> class type_caster<void> : public type_caster<void_type> {
public:
    using type_caster<void_type>::cast;

    bool load(handle h, bool) {
        if (!h) {
            return false;
        } else if (h.is_none()) {
            value = nullptr;
            return true;
        }


        if (isinstance<capsule>(h)) {
            value = reinterpret_borrow<capsule>(h);
            return true;
        }


        auto &bases = all_type_info((PyTypeObject *) type::handle_of(h).ptr());
        if (bases.size() == 1) {
            value = values_and_holders(reinterpret_cast<instance *>(h.ptr())).begin()->value_ptr();
            return true;
        }


        return false;
    }

    static handle cast(const void *ptr, return_value_policy  , handle  ) {
        if (ptr)
            return capsule(ptr).release();
        else
            return none().inc_ref();
    }

    template <typename T> using cast_op_type = void*&;
    operator void *&() { return value; }
    static constexpr auto name = _("capsule");
private:
    void *value = nullptr;
};

template <> class type_caster<std::nullptr_t> : public void_caster<std::nullptr_t> { };

template <> class type_caster<bool> {
public:
    bool load(handle src, bool convert) {
        if (!src) return false;
        else if (src.ptr() == Py_True) { value = true; return true; }
        else if (src.ptr() == Py_False) { value = false; return true; }
        else if (convert || !strcmp("numpy.bool_", Py_TYPE(src.ptr())->tp_name)) {


            Py_ssize_t res = -1;
            if (src.is_none()) {
                res = 0;
            }
            #if defined(PYPY_VERSION)

            else if (hasattr(src, PYBIND11_BOOL_ATTR)) {
                res = PyObject_IsTrue(src.ptr());
            }
            #else


            else if (auto tp_as_number = src.ptr()->ob_type->tp_as_number) {
                if (PYBIND11_NB_BOOL(tp_as_number)) {
                    res = (*PYBIND11_NB_BOOL(tp_as_number))(src.ptr());
                }
            }
            #endif
            if (res == 0 || res == 1) {
                value = (bool) res;
                return true;
            } else {
                PyErr_Clear();
            }
        }
        return false;
    }
    static handle cast(bool src, return_value_policy  , handle  ) {
        return handle(src ? Py_True : Py_False).inc_ref();
    }
    PYBIND11_TYPE_CASTER(bool, _("bool"));
};


template <typename StringType, bool IsView = false> struct string_caster {
    using CharT = typename StringType::value_type;



    static_assert(!std::is_same<CharT, char>::value || sizeof(CharT) == 1, "Unsupported char size != 1");
#if defined(PYBIND11_HAS_U8STRING)
    static_assert(!std::is_same<CharT, char8_t>::value || sizeof(CharT) == 1, "Unsupported char8_t size != 1");
#endif
    static_assert(!std::is_same<CharT, char16_t>::value || sizeof(CharT) == 2, "Unsupported char16_t size != 2");
    static_assert(!std::is_same<CharT, char32_t>::value || sizeof(CharT) == 4, "Unsupported char32_t size != 4");

    static_assert(!std::is_same<CharT, wchar_t>::value || sizeof(CharT) == 2 || sizeof(CharT) == 4,
            "Unsupported wchar_t size != 2/4");
    static constexpr size_t UTF_N = 8 * sizeof(CharT);

    bool load(handle src, bool) {
#if PY_MAJOR_VERSION < 3
        object temp;
#endif
        handle load_src = src;
        if (!src) {
            return false;
        } else if (!PyUnicode_Check(load_src.ptr())) {
#if PY_MAJOR_VERSION >= 3
            return load_bytes(load_src);
#else
            if (std::is_same<CharT, char>::value) {
                return load_bytes(load_src);
            }


            if (!PYBIND11_BYTES_CHECK(load_src.ptr()))
                return false;

            temp = reinterpret_steal<object>(PyUnicode_FromObject(load_src.ptr()));
            if (!temp) { PyErr_Clear(); return false; }
            load_src = temp;
#endif
        }

        auto utfNbytes = reinterpret_steal<object>(PyUnicode_AsEncodedString(
            load_src.ptr(), UTF_N == 8 ? "utf-8" : UTF_N == 16 ? "utf-16" : "utf-32", nullptr));
        if (!utfNbytes) { PyErr_Clear(); return false; }

        const auto *buffer = reinterpret_cast<const CharT *>(PYBIND11_BYTES_AS_STRING(utfNbytes.ptr()));
        size_t length = (size_t) PYBIND11_BYTES_SIZE(utfNbytes.ptr()) / sizeof(CharT);
        if (UTF_N > 8) { buffer++; length--; }
        value = StringType(buffer, length);


        if (IsView)
            loader_life_support::add_patient(utfNbytes);

        return true;
    }

    static handle cast(const StringType &src, return_value_policy  , handle  ) {
        const char *buffer = reinterpret_cast<const char *>(src.data());
        auto nbytes = ssize_t(src.size() * sizeof(CharT));
        handle s = decode_utfN(buffer, nbytes);
        if (!s) throw error_already_set();
        return s;
    }

    PYBIND11_TYPE_CASTER(StringType, _(PYBIND11_STRING_NAME));

private:
    static handle decode_utfN(const char *buffer, ssize_t nbytes) {
#if !defined(PYPY_VERSION)
        return
            UTF_N == 8  ? PyUnicode_DecodeUTF8(buffer, nbytes, nullptr) :
            UTF_N == 16 ? PyUnicode_DecodeUTF16(buffer, nbytes, nullptr, nullptr) :
                          PyUnicode_DecodeUTF32(buffer, nbytes, nullptr, nullptr);
#else


        return PyUnicode_Decode(buffer, nbytes, UTF_N == 8 ? "utf-8" : UTF_N == 16 ? "utf-16" : "utf-32", nullptr);
#endif
    }




    template <typename C = CharT>
    bool load_bytes(enable_if_t<std::is_same<C, char>::value, handle> src) {
        if (PYBIND11_BYTES_CHECK(src.ptr())) {


            const char *bytes = PYBIND11_BYTES_AS_STRING(src.ptr());
            if (bytes) {
                value = StringType(bytes, (size_t) PYBIND11_BYTES_SIZE(src.ptr()));
                return true;
            }
        }

        return false;
    }

    template <typename C = CharT>
    bool load_bytes(enable_if_t<!std::is_same<C, char>::value, handle>) { return false; }
};

template <typename CharT, class Traits, class Allocator>
struct type_caster<std::basic_string<CharT, Traits, Allocator>, enable_if_t<is_std_char_type<CharT>::value>>
    : string_caster<std::basic_string<CharT, Traits, Allocator>> {};

#ifdef PYBIND11_HAS_STRING_VIEW
template <typename CharT, class Traits>
struct type_caster<std::basic_string_view<CharT, Traits>, enable_if_t<is_std_char_type<CharT>::value>>
    : string_caster<std::basic_string_view<CharT, Traits>, true> {};
#endif



template <typename CharT> struct type_caster<CharT, enable_if_t<is_std_char_type<CharT>::value>> {
    using StringType = std::basic_string<CharT>;
    using StringCaster = type_caster<StringType>;
    StringCaster str_caster;
    bool none = false;
    CharT one_char = 0;
public:
    bool load(handle src, bool convert) {
        if (!src) return false;
        if (src.is_none()) {

            if (!convert) return false;
            none = true;
            return true;
        }
        return str_caster.load(src, convert);
    }

    static handle cast(const CharT *src, return_value_policy policy, handle parent) {
        if (src == nullptr) return pybind11::none().inc_ref();
        return StringCaster::cast(StringType(src), policy, parent);
    }

    static handle cast(CharT src, return_value_policy policy, handle parent) {
        if (std::is_same<char, CharT>::value) {
            handle s = PyUnicode_DecodeLatin1((const char *) &src, 1, nullptr);
            if (!s) throw error_already_set();
            return s;
        }
        return StringCaster::cast(StringType(1, src), policy, parent);
    }

    operator CharT*() { return none ? nullptr : const_cast<CharT *>(static_cast<StringType &>(str_caster).c_str()); }
    operator CharT&() {
        if (none)
            throw value_error("Cannot convert None to a character");

        auto &value = static_cast<StringType &>(str_caster);
        size_t str_len = value.size();
        if (str_len == 0)
            throw value_error("Cannot convert empty string to a character");






        if (StringCaster::UTF_N == 8 && str_len > 1 && str_len <= 4) {
            auto v0 = static_cast<unsigned char>(value[0]);
            size_t char0_bytes = !(v0 & 0x80) ? 1 :
                (v0 & 0xE0) == 0xC0 ? 2 :
                (v0 & 0xF0) == 0xE0 ? 3 :
                4;

            if (char0_bytes == str_len) {

                if (char0_bytes == 2 && (v0 & 0xFC) == 0xC0) {
                    one_char = static_cast<CharT>(((v0 & 3) << 6) + (static_cast<unsigned char>(value[1]) & 0x3F));
                    return one_char;
                }

                throw value_error("Character code point not in range(0x100)");
            }
        }




        else if (StringCaster::UTF_N == 16 && str_len == 2) {
            one_char = static_cast<CharT>(value[0]);
            if (one_char >= 0xD800 && one_char < 0xE000)
                throw value_error("Character code point not in range(0x10000)");
        }

        if (str_len != 1)
            throw value_error("Expected a character, but multi-character string found");

        one_char = value[0];
        return one_char;
    }

    static constexpr auto name = _(PYBIND11_STRING_NAME);
    template <typename _T> using cast_op_type = pybind11::detail::cast_op_type<_T>;
};


template <template<typename...> class Tuple, typename... Ts> class tuple_caster {
    using type = Tuple<Ts...>;
    static constexpr auto size = sizeof...(Ts);
    using indices = make_index_sequence<size>;
public:

    bool load(handle src, bool convert) {
        if (!isinstance<sequence>(src))
            return false;
        const auto seq = reinterpret_borrow<sequence>(src);
        if (seq.size() != size)
            return false;
        return load_impl(seq, convert, indices{});
    }

    template <typename T>
    static handle cast(T &&src, return_value_policy policy, handle parent) {
        return cast_impl(std::forward<T>(src), policy, parent, indices{});
    }


    template <typename T>
    static handle cast(T *src, return_value_policy policy, handle parent) {
        if (!src) return none().release();
        if (policy == return_value_policy::take_ownership) {
            auto h = cast(std::move(*src), policy, parent); delete src; return h;
        } else {
            return cast(*src, policy, parent);
        }
    }

    static constexpr auto name = _("Tuple[") + concat(make_caster<Ts>::name...) + _("]");

    template <typename T> using cast_op_type = type;

    operator type() & { return implicit_cast(indices{}); }
    operator type() && { return std::move(*this).implicit_cast(indices{}); }

protected:
    template <size_t... Is>
    type implicit_cast(index_sequence<Is...>) & { return type(cast_op<Ts>(std::get<Is>(subcasters))...); }
    template <size_t... Is>
    type implicit_cast(index_sequence<Is...>) && { return type(cast_op<Ts>(std::move(std::get<Is>(subcasters)))...); }

    static constexpr bool load_impl(const sequence &, bool, index_sequence<>) { return true; }

    template <size_t... Is>
    bool load_impl(const sequence &seq, bool convert, index_sequence<Is...>) {
#ifdef __cpp_fold_expressions
        if ((... || !std::get<Is>(subcasters).load(seq[Is], convert)))
            return false;
#else
        for (bool r : {std::get<Is>(subcasters).load(seq[Is], convert)...})
            if (!r)
                return false;
#endif
        return true;
    }


    template <typename T, size_t... Is>
    static handle cast_impl(T &&src, return_value_policy policy, handle parent, index_sequence<Is...>) {
        std::array<object, size> entries{{
            reinterpret_steal<object>(make_caster<Ts>::cast(std::get<Is>(std::forward<T>(src)), policy, parent))...
        }};
        for (const auto &entry: entries)
            if (!entry)
                return handle();
        tuple result(size);
        int counter = 0;
        for (auto & entry: entries)
            PyTuple_SET_ITEM(result.ptr(), counter++, entry.release().ptr());
        return result.release();
    }

    Tuple<make_caster<Ts>...> subcasters;
};

template <typename T1, typename T2> class type_caster<std::pair<T1, T2>>
    : public tuple_caster<std::pair, T1, T2> {};

template <typename... Ts> class type_caster<std::tuple<Ts...>>
    : public tuple_caster<std::tuple, Ts...> {};



template <typename T>
struct holder_helper {
    static auto get(const T &p) -> decltype(p.get()) { return p.get(); }
};


template <typename type, typename holder_type>
struct copyable_holder_caster : public type_caster_base<type> {
public:
    using base = type_caster_base<type>;
    static_assert(std::is_base_of<base, type_caster<type>>::value,
            "Holder classes are only supported for custom types");
    using base::base;
    using base::cast;
    using base::typeinfo;
    using base::value;

    bool load(handle src, bool convert) {
        return base::template load_impl<copyable_holder_caster<type, holder_type>>(src, convert);
    }

    explicit operator type*() { return this->value; }


    explicit operator type&() { return *(static_cast<type *>(this->value)); }
    explicit operator holder_type*() { return std::addressof(holder); }
    explicit operator holder_type&() { return holder; }

    static handle cast(const holder_type &src, return_value_policy, handle) {
        const auto *ptr = holder_helper<holder_type>::get(src);
        return type_caster_base<type>::cast_holder(ptr, &src);
    }

protected:
    friend class type_caster_generic;
    void check_holder_compat() {
        if (typeinfo->default_holder)
            throw cast_error("Unable to load a custom holder type from a default-holder instance");
    }

    bool load_value(value_and_holder &&v_h) {
        if (v_h.holder_constructed()) {
            value = v_h.value_ptr();
            holder = v_h.template holder<holder_type>();
            return true;
        } else {
            throw cast_error("Unable to cast from non-held to held instance (T& to Holder<T>) "
#if defined(NDEBUG)
                             "(compile in debug mode for type information)");
#else
                             "of type '" + type_id<holder_type>() + "''");
#endif
        }
    }

    template <typename T = holder_type, detail::enable_if_t<!std::is_constructible<T, const T &, type*>::value, int> = 0>
    bool try_implicit_casts(handle, bool) { return false; }

    template <typename T = holder_type, detail::enable_if_t<std::is_constructible<T, const T &, type*>::value, int> = 0>
    bool try_implicit_casts(handle src, bool convert) {
        for (auto &cast : typeinfo->implicit_casts) {
            copyable_holder_caster sub_caster(*cast.first);
            if (sub_caster.load(src, convert)) {
                value = cast.second(sub_caster.value);
                holder = holder_type(sub_caster.holder, (type *) value);
                return true;
            }
        }
        return false;
    }

    static bool try_direct_conversions(handle) { return false; }


    holder_type holder;
};


template <typename T>
class type_caster<std::shared_ptr<T>> : public copyable_holder_caster<T, std::shared_ptr<T>> { };

template <typename type, typename holder_type>
struct move_only_holder_caster {
    static_assert(std::is_base_of<type_caster_base<type>, type_caster<type>>::value,
            "Holder classes are only supported for custom types");

    static handle cast(holder_type &&src, return_value_policy, handle) {
        auto *ptr = holder_helper<holder_type>::get(src);
        return type_caster_base<type>::cast_holder(ptr, std::addressof(src));
    }
    static constexpr auto name = type_caster_base<type>::name;
};

template <typename type, typename deleter>
class type_caster<std::unique_ptr<type, deleter>>
    : public move_only_holder_caster<type, std::unique_ptr<type, deleter>> { };

template <typename type, typename holder_type>
using type_caster_holder = conditional_t<is_copy_constructible<holder_type>::value,
                                         copyable_holder_caster<type, holder_type>,
                                         move_only_holder_caster<type, holder_type>>;

template <typename T, bool Value = false> struct always_construct_holder { static constexpr bool value = Value; };


#define PYBIND11_DECLARE_HOLDER_TYPE(type, holder_type, ...) \
    namespace pybind11 { namespace detail { \
    template <typename type> \
    struct always_construct_holder<holder_type> : always_construct_holder<void, ##__VA_ARGS__>  { }; \
    template <typename type> \
    class type_caster<holder_type, enable_if_t<!is_shared_ptr<holder_type>::value>> \
        : public type_caster_holder<type, holder_type> { }; \
    }}


template <typename base, typename holder> struct is_holder_type :
    std::is_base_of<detail::type_caster_holder<base, holder>, detail::type_caster<holder>> {};

template <typename base, typename deleter> struct is_holder_type<base, std::unique_ptr<base, deleter>> :
    std::true_type {};

template <typename T> struct handle_type_name { static constexpr auto name = _<T>(); };
template <> struct handle_type_name<bytes> { static constexpr auto name = _(PYBIND11_BYTES_NAME); };
template <> struct handle_type_name<int_> { static constexpr auto name = _("int"); };
template <> struct handle_type_name<iterable> { static constexpr auto name = _("Iterable"); };
template <> struct handle_type_name<iterator> { static constexpr auto name = _("Iterator"); };
template <> struct handle_type_name<none> { static constexpr auto name = _("None"); };
template <> struct handle_type_name<args> { static constexpr auto name = _("*args"); };
template <> struct handle_type_name<kwargs> { static constexpr auto name = _("**kwargs"); };

template <typename type>
struct pyobject_caster {
    template <typename T = type, enable_if_t<std::is_same<T, handle>::value, int> = 0>
    bool load(handle src, bool  ) { value = src; return static_cast<bool>(value); }

    template <typename T = type, enable_if_t<std::is_base_of<object, T>::value, int> = 0>
    bool load(handle src, bool  ) {
        if (!isinstance<type>(src))
            return false;
        value = reinterpret_borrow<type>(src);
        return true;
    }

    static handle cast(const handle &src, return_value_policy  , handle  ) {
        return src.inc_ref();
    }
    PYBIND11_TYPE_CASTER(type, handle_type_name<type>::name);
};

template <typename T>
class type_caster<T, enable_if_t<is_pyobject<T>::value>> : public pyobject_caster<T> { };










template <typename T> using move_is_plain_type = satisfies_none_of<T,
    std::is_void, std::is_pointer, std::is_reference, std::is_const
>;
template <typename T, typename SFINAE = void> struct move_always : std::false_type {};
template <typename T> struct move_always<T, enable_if_t<all_of<
    move_is_plain_type<T>,
    negation<is_copy_constructible<T>>,
    std::is_move_constructible<T>,
    std::is_same<decltype(std::declval<make_caster<T>>().operator T&()), T&>
>::value>> : std::true_type {};
template <typename T, typename SFINAE = void> struct move_if_unreferenced : std::false_type {};
template <typename T> struct move_if_unreferenced<T, enable_if_t<all_of<
    move_is_plain_type<T>,
    negation<move_always<T>>,
    std::is_move_constructible<T>,
    std::is_same<decltype(std::declval<make_caster<T>>().operator T&()), T&>
>::value>> : std::true_type {};
template <typename T> using move_never = none_of<move_always<T>, move_if_unreferenced<T>>;





template <typename type> using cast_is_temporary_value_reference = bool_constant<
    (std::is_reference<type>::value || std::is_pointer<type>::value) &&
    !std::is_base_of<type_caster_generic, make_caster<type>>::value &&
    !std::is_same<intrinsic_t<type>, void>::value
>;




template <typename Return, typename SFINAE = void> struct return_value_policy_override {
    static return_value_policy policy(return_value_policy p) { return p; }
};

template <typename Return> struct return_value_policy_override<Return,
        detail::enable_if_t<std::is_base_of<type_caster_generic, make_caster<Return>>::value, void>> {
    static return_value_policy policy(return_value_policy p) {
        return !std::is_lvalue_reference<Return>::value &&
               !std::is_pointer<Return>::value
                   ? return_value_policy::move : p;
    }
};


template <typename T, typename SFINAE> type_caster<T, SFINAE> &load_type(type_caster<T, SFINAE> &conv, const handle &handle) {
    if (!conv.load(handle, true)) {
#if defined(NDEBUG)
        throw cast_error("Unable to cast Python instance to C++ type (compile in debug mode for details)");
#else
        throw cast_error("Unable to cast Python instance of type " +
            (std::string) str(type::handle_of(handle)) + " to C++ type '" + type_id<T>() + "'");
#endif
    }
    return conv;
}

template <typename T> make_caster<T> load_type(const handle &handle) {
    make_caster<T> conv;
    load_type(conv, handle);
    return conv;
}

PYBIND11_NAMESPACE_END(detail)


template <typename T, detail::enable_if_t<!detail::is_pyobject<T>::value, int> = 0>
T cast(const handle &handle) {
    using namespace detail;
    static_assert(!cast_is_temporary_value_reference<T>::value,
            "Unable to cast type to reference: value is local to type caster");
    return cast_op<T>(load_type<T>(handle));
}


template <typename T, detail::enable_if_t<detail::is_pyobject<T>::value, int> = 0>
T cast(const handle &handle) { return T(reinterpret_borrow<object>(handle)); }


template <typename T, detail::enable_if_t<!detail::is_pyobject<T>::value, int> = 0>
object cast(T &&value, return_value_policy policy = return_value_policy::automatic_reference,
            handle parent = handle()) {
    using no_ref_T = typename std::remove_reference<T>::type;
    if (policy == return_value_policy::automatic)
        policy = std::is_pointer<no_ref_T>::value ? return_value_policy::take_ownership :
                 std::is_lvalue_reference<T>::value ? return_value_policy::copy : return_value_policy::move;
    else if (policy == return_value_policy::automatic_reference)
        policy = std::is_pointer<no_ref_T>::value ? return_value_policy::reference :
                 std::is_lvalue_reference<T>::value ? return_value_policy::copy : return_value_policy::move;
    return reinterpret_steal<object>(detail::make_caster<T>::cast(std::forward<T>(value), policy, parent));
}

template <typename T> T handle::cast() const { return pybind11::cast<T>(*this); }
template <> inline void handle::cast() const { return; }

template <typename T>
detail::enable_if_t<!detail::move_never<T>::value, T> move(object &&obj) {
    if (obj.ref_count() > 1)
#if defined(NDEBUG)
        throw cast_error("Unable to cast Python instance to C++ rvalue: instance has multiple references"
            " (compile in debug mode for details)");
#else
        throw cast_error("Unable to move from Python " + (std::string) str(type::handle_of(obj)) +
                " instance to C++ " + type_id<T>() + " instance: instance has multiple references");
#endif


    T ret = std::move(detail::load_type<T>(obj).operator T&());
    return ret;
}






template <typename T> detail::enable_if_t<detail::move_always<T>::value, T> cast(object &&object) {
    return move<T>(std::move(object));
}
template <typename T> detail::enable_if_t<detail::move_if_unreferenced<T>::value, T> cast(object &&object) {
    if (object.ref_count() > 1)
        return cast<T>(object);
    else
        return move<T>(std::move(object));
}
template <typename T> detail::enable_if_t<detail::move_never<T>::value, T> cast(object &&object) {
    return cast<T>(object);
}

template <typename T> T object::cast() const & { return pybind11::cast<T>(*this); }
template <typename T> T object::cast() && { return pybind11::cast<T>(std::move(*this)); }
template <> inline void object::cast() const & { return; }
template <> inline void object::cast() && { return; }

PYBIND11_NAMESPACE_BEGIN(detail)


template <typename T, enable_if_t<!is_pyobject<T>::value, int>>
object object_or_cast(T &&o) { return pybind11::cast(std::forward<T>(o)); }

struct override_unused {};
template <typename ret_type> using override_caster_t = conditional_t<
    cast_is_temporary_value_reference<ret_type>::value, make_caster<ret_type>, override_unused>;



template <typename T> enable_if_t<cast_is_temporary_value_reference<T>::value, T> cast_ref(object &&o, make_caster<T> &caster) {
    return cast_op<T>(load_type(caster, o));
}
template <typename T> enable_if_t<!cast_is_temporary_value_reference<T>::value, T> cast_ref(object &&, override_unused &) {
    pybind11_fail("Internal error: cast_ref fallback invoked"); }




template <typename T> enable_if_t<!cast_is_temporary_value_reference<T>::value, T> cast_safe(object &&o) {
    return pybind11::cast<T>(std::move(o)); }
template <typename T> enable_if_t<cast_is_temporary_value_reference<T>::value, T> cast_safe(object &&) {
    pybind11_fail("Internal error: cast_safe fallback invoked"); }
template <> inline void cast_safe<void>(object &&) {}

PYBIND11_NAMESPACE_END(detail)

template <return_value_policy policy = return_value_policy::automatic_reference>
tuple make_tuple() { return tuple(0); }

template <return_value_policy policy = return_value_policy::automatic_reference,
          typename... Args> tuple make_tuple(Args&&... args_) {
    constexpr size_t size = sizeof...(Args);
    std::array<object, size> args {
        { reinterpret_steal<object>(detail::make_caster<Args>::cast(
            std::forward<Args>(args_), policy, nullptr))... }
    };
    for (size_t i = 0; i < args.size(); i++) {
        if (!args[i]) {
#if defined(NDEBUG)
            throw cast_error("make_tuple(): unable to convert arguments to Python object (compile in debug mode for details)");
#else
            std::array<std::string, size> argtypes { {type_id<Args>()...} };
            throw cast_error("make_tuple(): unable to convert argument of type '" +
                argtypes[i] + "' to Python object");
#endif
        }
    }
    tuple result(size);
    int counter = 0;
    for (auto &arg_value : args)
        PyTuple_SET_ITEM(result.ptr(), counter++, arg_value.release().ptr());
    return result;
}



struct arg {

    constexpr explicit arg(const char *name = nullptr) : name(name), flag_noconvert(false), flag_none(true) { }

    template <typename T> arg_v operator=(T &&value) const;

    arg &noconvert(bool flag = true) { flag_noconvert = flag; return *this; }

    arg &none(bool flag = true) { flag_none = flag; return *this; }

    const char *name;
    bool flag_noconvert : 1;
    bool flag_none : 1;
};



struct arg_v : arg {
private:
    template <typename T>
    arg_v(arg &&base, T &&x, const char *descr = nullptr)
        : arg(base),
          value(reinterpret_steal<object>(
              detail::make_caster<T>::cast(x, return_value_policy::automatic, {})
          )),
          descr(descr)
#if !defined(NDEBUG)
        , type(type_id<T>())
#endif
    { }

public:

    template <typename T>
    arg_v(const char *name, T &&x, const char *descr = nullptr)
        : arg_v(arg(name), std::forward<T>(x), descr) { }


    template <typename T>
    arg_v(const arg &base, T &&x, const char *descr = nullptr)
        : arg_v(arg(base), std::forward<T>(x), descr) { }


    arg_v &noconvert(bool flag = true) { arg::noconvert(flag); return *this; }


    arg_v &none(bool flag = true) { arg::none(flag); return *this; }


    object value;

    const char *descr;
#if !defined(NDEBUG)

    std::string type;
#endif
};




struct kw_only {};




struct pos_only {};

template <typename T>
arg_v arg::operator=(T &&value) const { return {std::move(*this), std::forward<T>(value)}; }


template <typename  > using arg_t = arg_v;

inline namespace literals {

constexpr arg operator"" _a(const char *name, size_t) { return arg(name); }
}

PYBIND11_NAMESPACE_BEGIN(detail)


struct function_record;


struct function_call {
    function_call(const function_record &f, handle p);


    const function_record &func;


    std::vector<handle> args;


    std::vector<bool> args_convert;



    object args_ref, kwargs_ref;


    handle parent;


    handle init_self;
};



template <typename... Args>
class argument_loader {
    using indices = make_index_sequence<sizeof...(Args)>;

    template <typename Arg> using argument_is_args   = std::is_same<intrinsic_t<Arg>, args>;
    template <typename Arg> using argument_is_kwargs = std::is_same<intrinsic_t<Arg>, kwargs>;

    static constexpr auto args_pos = constexpr_first<argument_is_args, Args...>() - (int) sizeof...(Args),
                        kwargs_pos = constexpr_first<argument_is_kwargs, Args...>() - (int) sizeof...(Args);

    static constexpr bool args_kwargs_are_last = kwargs_pos >= - 1 && args_pos >= kwargs_pos - 1;

    static_assert(args_kwargs_are_last, "py::args/py::kwargs are only permitted as the last argument(s) of a function");

public:
    static constexpr bool has_kwargs = kwargs_pos < 0;
    static constexpr bool has_args = args_pos < 0;

    static constexpr auto arg_names = concat(type_descr(make_caster<Args>::name)...);

    bool load_args(function_call &call) {
        return load_impl_sequence(call, indices{});
    }

    template <typename Return, typename Guard, typename Func>
    enable_if_t<!std::is_void<Return>::value, Return> call(Func &&f) && {
        return std::move(*this).template call_impl<Return>(std::forward<Func>(f), indices{}, Guard{});
    }

    template <typename Return, typename Guard, typename Func>
    enable_if_t<std::is_void<Return>::value, void_type> call(Func &&f) && {
        std::move(*this).template call_impl<Return>(std::forward<Func>(f), indices{}, Guard{});
        return void_type();
    }

private:

    static bool load_impl_sequence(function_call &, index_sequence<>) { return true; }

    template <size_t... Is>
    bool load_impl_sequence(function_call &call, index_sequence<Is...>) {
#ifdef __cpp_fold_expressions
        if ((... || !std::get<Is>(argcasters).load(call.args[Is], call.args_convert[Is])))
            return false;
#else
        for (bool r : {std::get<Is>(argcasters).load(call.args[Is], call.args_convert[Is])...})
            if (!r)
                return false;
#endif
        return true;
    }

    template <typename Return, typename Func, size_t... Is, typename Guard>
    Return call_impl(Func &&f, index_sequence<Is...>, Guard &&) && {
        return std::forward<Func>(f)(cast_op<Args>(std::move(std::get<Is>(argcasters)))...);
    }

    std::tuple<make_caster<Args>...> argcasters;
};



template <return_value_policy policy>
class simple_collector {
public:
    template <typename... Ts>
    explicit simple_collector(Ts &&...values)
        : m_args(pybind11::make_tuple<policy>(std::forward<Ts>(values)...)) { }

    const tuple &args() const & { return m_args; }
    dict kwargs() const { return {}; }

    tuple args() && { return std::move(m_args); }


    object call(PyObject *ptr) const {
        PyObject *result = PyObject_CallObject(ptr, m_args.ptr());
        if (!result)
            throw error_already_set();
        return reinterpret_steal<object>(result);
    }

private:
    tuple m_args;
};


template <return_value_policy policy>
class unpacking_collector {
public:
    template <typename... Ts>
    explicit unpacking_collector(Ts &&...values) {


        auto args_list = list();
        int _[] = { 0, (process(args_list, std::forward<Ts>(values)), 0)... };
        ignore_unused(_);

        m_args = std::move(args_list);
    }

    const tuple &args() const & { return m_args; }
    const dict &kwargs() const & { return m_kwargs; }

    tuple args() && { return std::move(m_args); }
    dict kwargs() && { return std::move(m_kwargs); }


    object call(PyObject *ptr) const {
        PyObject *result = PyObject_Call(ptr, m_args.ptr(), m_kwargs.ptr());
        if (!result)
            throw error_already_set();
        return reinterpret_steal<object>(result);
    }

private:
    template <typename T>
    void process(list &args_list, T &&x) {
        auto o = reinterpret_steal<object>(detail::make_caster<T>::cast(std::forward<T>(x), policy, {}));
        if (!o) {
#if defined(NDEBUG)
            argument_cast_error();
#else
            argument_cast_error(std::to_string(args_list.size()), type_id<T>());
#endif
        }
        args_list.append(o);
    }

    void process(list &args_list, detail::args_proxy ap) {
        for (auto a : ap)
            args_list.append(a);
    }

    void process(list & , arg_v a) {
        if (!a.name)
#if defined(NDEBUG)
            nameless_argument_error();
#else
            nameless_argument_error(a.type);
#endif

        if (m_kwargs.contains(a.name)) {
#if defined(NDEBUG)
            multiple_values_error();
#else
            multiple_values_error(a.name);
#endif
        }
        if (!a.value) {
#if defined(NDEBUG)
            argument_cast_error();
#else
            argument_cast_error(a.name, a.type);
#endif
        }
        m_kwargs[a.name] = a.value;
    }

    void process(list & , detail::kwargs_proxy kp) {
        if (!kp)
            return;
        for (auto k : reinterpret_borrow<dict>(kp)) {
            if (m_kwargs.contains(k.first)) {
#if defined(NDEBUG)
                multiple_values_error();
#else
                multiple_values_error(str(k.first));
#endif
            }
            m_kwargs[k.first] = k.second;
        }
    }

    [[noreturn]] static void nameless_argument_error() {
        throw type_error("Got kwargs without a name; only named arguments "
                         "may be passed via py::arg() to a python function call. "
                         "(compile in debug mode for details)");
    }
    [[noreturn]] static void nameless_argument_error(std::string type) {
        throw type_error("Got kwargs without a name of type '" + type + "'; only named "
                         "arguments may be passed via py::arg() to a python function call. ");
    }
    [[noreturn]] static void multiple_values_error() {
        throw type_error("Got multiple values for keyword argument "
                         "(compile in debug mode for details)");
    }

    [[noreturn]] static void multiple_values_error(std::string name) {
        throw type_error("Got multiple values for keyword argument '" + name + "'");
    }

    [[noreturn]] static void argument_cast_error() {
        throw cast_error("Unable to convert call argument to Python object "
                         "(compile in debug mode for details)");
    }

    [[noreturn]] static void argument_cast_error(std::string name, std::string type) {
        throw cast_error("Unable to convert call argument '" + name
                         + "' of type '" + type + "' to Python object");
    }

private:
    tuple m_args;
    dict m_kwargs;
};


template <return_value_policy policy, typename... Args,
          typename = enable_if_t<all_of<is_positional<Args>...>::value>>
simple_collector<policy> collect_arguments(Args &&...args) {
    return simple_collector<policy>(std::forward<Args>(args)...);
}


template <return_value_policy policy, typename... Args,
          typename = enable_if_t<!all_of<is_positional<Args>...>::value>>
unpacking_collector<policy> collect_arguments(Args &&...args) {

    static_assert(
        constexpr_last<is_positional, Args...>() < constexpr_first<is_keyword_or_ds, Args...>()
        && constexpr_last<is_s_unpacking, Args...>() < constexpr_first<is_ds_unpacking, Args...>(),
        "Invalid function call: positional args must precede keywords and ** unpacking; "
        "* unpacking must precede ** unpacking"
    );
    return unpacking_collector<policy>(std::forward<Args>(args)...);
}

template <typename Derived>
template <return_value_policy policy, typename... Args>
object object_api<Derived>::operator()(Args &&...args) const {
    return detail::collect_arguments<policy>(std::forward<Args>(args)...).call(derived().ptr());
}

template <typename Derived>
template <return_value_policy policy, typename... Args>
object object_api<Derived>::call(Args &&...args) const {
    return operator()<policy>(std::forward<Args>(args)...);
}

PYBIND11_NAMESPACE_END(detail)


template<typename T>
handle type::handle_of() {
   static_assert(
      std::is_base_of<detail::type_caster_generic, detail::make_caster<T>>::value,
      "py::type::of<T> only supports the case where T is a registered C++ types."
    );

    return detail::get_type_handle(typeid(T), true);
}


#define PYBIND11_MAKE_OPAQUE(...) \
    namespace pybind11 { namespace detail { \
        template<> class type_caster<__VA_ARGS__> : public type_caster_base<__VA_ARGS__> { }; \
    }}



#define PYBIND11_TYPE(...) __VA_ARGS__

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
