/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "detail/common.h"
#include "buffer_info.h"
#include <utility>
#include <type_traits>

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)


class handle; class object;
class str; class iterator;
class type;
struct arg; struct arg_v;

PYBIND11_NAMESPACE_BEGIN(detail)
class args_proxy;
inline bool isinstance_generic(handle obj, const std::type_info &tp);


template <typename Policy> class accessor;
namespace accessor_policies {
    struct obj_attr;
    struct str_attr;
    struct generic_item;
    struct sequence_item;
    struct list_item;
    struct tuple_item;
}
using obj_attr_accessor = accessor<accessor_policies::obj_attr>;
using str_attr_accessor = accessor<accessor_policies::str_attr>;
using item_accessor = accessor<accessor_policies::generic_item>;
using sequence_accessor = accessor<accessor_policies::sequence_item>;
using list_accessor = accessor<accessor_policies::list_item>;
using tuple_accessor = accessor<accessor_policies::tuple_item>;


class pyobject_tag { };
template <typename T> using is_pyobject = std::is_base_of<pyobject_tag, remove_reference_t<T>>;


template <typename Derived>
class object_api : public pyobject_tag {
    const Derived &derived() const { return static_cast<const Derived &>(*this); }

public:

    iterator begin() const;

    iterator end() const;


    item_accessor operator[](handle key) const;

    item_accessor operator[](const char *key) const;


    obj_attr_accessor attr(handle key) const;

    str_attr_accessor attr(const char *key) const;


    args_proxy operator*() const;


    template <typename T> bool contains(T &&item) const;


    template <return_value_policy policy = return_value_policy::automatic_reference, typename... Args>
    object operator()(Args &&...args) const;
    template <return_value_policy policy = return_value_policy::automatic_reference, typename... Args>
    PYBIND11_DEPRECATED("call(...) was deprecated in favor of operator()(...)")
        object call(Args&&... args) const;


    bool is(object_api const& other) const { return derived().ptr() == other.derived().ptr(); }

    bool is_none() const { return derived().ptr() == Py_None; }

    bool equal(object_api const &other) const      { return rich_compare(other, Py_EQ); }
    bool not_equal(object_api const &other) const  { return rich_compare(other, Py_NE); }
    bool operator<(object_api const &other) const  { return rich_compare(other, Py_LT); }
    bool operator<=(object_api const &other) const { return rich_compare(other, Py_LE); }
    bool operator>(object_api const &other) const  { return rich_compare(other, Py_GT); }
    bool operator>=(object_api const &other) const { return rich_compare(other, Py_GE); }

    object operator-() const;
    object operator~() const;
    object operator+(object_api const &other) const;
    object operator+=(object_api const &other) const;
    object operator-(object_api const &other) const;
    object operator-=(object_api const &other) const;
    object operator*(object_api const &other) const;
    object operator*=(object_api const &other) const;
    object operator/(object_api const &other) const;
    object operator/=(object_api const &other) const;
    object operator|(object_api const &other) const;
    object operator|=(object_api const &other) const;
    object operator&(object_api const &other) const;
    object operator&=(object_api const &other) const;
    object operator^(object_api const &other) const;
    object operator^=(object_api const &other) const;
    object operator<<(object_api const &other) const;
    object operator<<=(object_api const &other) const;
    object operator>>(object_api const &other) const;
    object operator>>=(object_api const &other) const;

    PYBIND11_DEPRECATED("Use py::str(obj) instead")
    pybind11::str str() const;


    str_attr_accessor doc() const;


    int ref_count() const { return static_cast<int>(Py_REFCNT(derived().ptr())); }


    handle get_type() const;

private:
    bool rich_compare(object_api const &other, int value) const;
};

PYBIND11_NAMESPACE_END(detail)


class handle : public detail::object_api<handle> {
public:

    handle() = default;

    handle(PyObject *ptr) : m_ptr(ptr) { }


    PyObject *ptr() const { return m_ptr; }
    PyObject *&ptr() { return m_ptr; }


    const handle& inc_ref() const & { Py_XINCREF(m_ptr); return *this; }


    const handle& dec_ref() const & { Py_XDECREF(m_ptr); return *this; }


    template <typename T> T cast() const;

    explicit operator bool() const { return m_ptr != nullptr; }

    PYBIND11_DEPRECATED("Use obj1.is(obj2) instead")
    bool operator==(const handle &h) const { return m_ptr == h.m_ptr; }
    PYBIND11_DEPRECATED("Use !obj1.is(obj2) instead")
    bool operator!=(const handle &h) const { return m_ptr != h.m_ptr; }
    PYBIND11_DEPRECATED("Use handle::operator bool() instead")
    bool check() const { return m_ptr != nullptr; }
protected:
    PyObject *m_ptr = nullptr;
};


class object : public handle {
public:
    object() = default;
    PYBIND11_DEPRECATED("Use reinterpret_borrow<object>() or reinterpret_steal<object>()")
    object(handle h, bool is_borrowed) : handle(h) { if (is_borrowed) inc_ref(); }

    object(const object &o) : handle(o) { inc_ref(); }

    object(object &&other) noexcept { m_ptr = other.m_ptr; other.m_ptr = nullptr; }

    ~object() { dec_ref(); }


    handle release() {
      PyObject *tmp = m_ptr;
      m_ptr = nullptr;
      return handle(tmp);
    }

    object& operator=(const object &other) {
        other.inc_ref();
        dec_ref();
        m_ptr = other.m_ptr;
        return *this;
    }

    object& operator=(object &&other) noexcept {
        if (this != &other) {
            handle temp(m_ptr);
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
            temp.dec_ref();
        }
        return *this;
    }


    template <typename T> T cast() const &;

    template <typename T> T cast() &&;

protected:

    struct borrowed_t { };
    struct stolen_t { };

    template <typename T> friend T reinterpret_borrow(handle);
    template <typename T> friend T reinterpret_steal(handle);

public:

    object(handle h, borrowed_t) : handle(h) { inc_ref(); }
    object(handle h, stolen_t) : handle(h) { }
};


template <typename T> T reinterpret_borrow(handle h) { return {h, object::borrowed_t{}}; }


template <typename T> T reinterpret_steal(handle h) { return {h, object::stolen_t{}}; }

PYBIND11_NAMESPACE_BEGIN(detail)
inline std::string error_string();
PYBIND11_NAMESPACE_END(detail)





class error_already_set : public std::runtime_error {
public:


    error_already_set() : std::runtime_error(detail::error_string()) {
        PyErr_Fetch(&m_type.ptr(), &m_value.ptr(), &m_trace.ptr());
    }

    error_already_set(const error_already_set &) = default;
    error_already_set(error_already_set &&) = default;

    inline ~error_already_set() override;




    void restore() { PyErr_Restore(m_type.release().ptr(), m_value.release().ptr(), m_trace.release().ptr()); }







    void discard_as_unraisable(object err_context) {
        restore();
        PyErr_WriteUnraisable(err_context.ptr());
    }
    void discard_as_unraisable(const char *err_context) {
        discard_as_unraisable(reinterpret_steal<object>(PYBIND11_FROM_STRING(err_context)));
    }


    PYBIND11_DEPRECATED("Use of error_already_set.clear() is deprecated")
    void clear() {}




    bool matches(handle exc) const { return PyErr_GivenExceptionMatches(m_type.ptr(), exc.ptr()); }

    const object& type() const { return m_type; }
    const object& value() const { return m_value; }
    const object& trace() const { return m_trace; }

private:
    object m_type, m_value, m_trace;
};




template <typename T, detail::enable_if_t<std::is_base_of<object, T>::value, int> = 0>
bool isinstance(handle obj) { return T::check_(obj); }

template <typename T, detail::enable_if_t<!std::is_base_of<object, T>::value, int> = 0>
bool isinstance(handle obj) { return detail::isinstance_generic(obj, typeid(T)); }

template <> inline bool isinstance<handle>(handle) = delete;
template <> inline bool isinstance<object>(handle obj) { return obj.ptr() != nullptr; }



inline bool isinstance(handle obj, handle type) {
    const auto result = PyObject_IsInstance(obj.ptr(), type.ptr());
    if (result == -1)
        throw error_already_set();
    return result != 0;
}



inline bool hasattr(handle obj, handle name) {
    return PyObject_HasAttr(obj.ptr(), name.ptr()) == 1;
}

inline bool hasattr(handle obj, const char *name) {
    return PyObject_HasAttrString(obj.ptr(), name) == 1;
}

inline void delattr(handle obj, handle name) {
    if (PyObject_DelAttr(obj.ptr(), name.ptr()) != 0) { throw error_already_set(); }
}

inline void delattr(handle obj, const char *name) {
    if (PyObject_DelAttrString(obj.ptr(), name) != 0) { throw error_already_set(); }
}

inline object getattr(handle obj, handle name) {
    PyObject *result = PyObject_GetAttr(obj.ptr(), name.ptr());
    if (!result) { throw error_already_set(); }
    return reinterpret_steal<object>(result);
}

inline object getattr(handle obj, const char *name) {
    PyObject *result = PyObject_GetAttrString(obj.ptr(), name);
    if (!result) { throw error_already_set(); }
    return reinterpret_steal<object>(result);
}

inline object getattr(handle obj, handle name, handle default_) {
    if (PyObject *result = PyObject_GetAttr(obj.ptr(), name.ptr())) {
        return reinterpret_steal<object>(result);
    } else {
        PyErr_Clear();
        return reinterpret_borrow<object>(default_);
    }
}

inline object getattr(handle obj, const char *name, handle default_) {
    if (PyObject *result = PyObject_GetAttrString(obj.ptr(), name)) {
        return reinterpret_steal<object>(result);
    } else {
        PyErr_Clear();
        return reinterpret_borrow<object>(default_);
    }
}

inline void setattr(handle obj, handle name, handle value) {
    if (PyObject_SetAttr(obj.ptr(), name.ptr(), value.ptr()) != 0) { throw error_already_set(); }
}

inline void setattr(handle obj, const char *name, handle value) {
    if (PyObject_SetAttrString(obj.ptr(), name, value.ptr()) != 0) { throw error_already_set(); }
}

inline ssize_t hash(handle obj) {
    auto h = PyObject_Hash(obj.ptr());
    if (h == -1) { throw error_already_set(); }
    return h;
}



PYBIND11_NAMESPACE_BEGIN(detail)
inline handle get_function(handle value) {
    if (value) {
#if PY_MAJOR_VERSION >= 3
        if (PyInstanceMethod_Check(value.ptr()))
            value = PyInstanceMethod_GET_FUNCTION(value.ptr());
        else
#endif
        if (PyMethod_Check(value.ptr()))
            value = PyMethod_GET_FUNCTION(value.ptr());
    }
    return value;
}




template <typename T, enable_if_t<is_pyobject<T>::value, int> = 0>
auto object_or_cast(T &&o) -> decltype(std::forward<T>(o)) { return std::forward<T>(o); }

template <typename T, enable_if_t<!is_pyobject<T>::value, int> = 0>
object object_or_cast(T &&o);

inline handle object_or_cast(PyObject *ptr) { return ptr; }

template <typename Policy>
class accessor : public object_api<accessor<Policy>> {
    using key_type = typename Policy::key_type;

public:
    accessor(handle obj, key_type key) : obj(obj), key(std::move(key)) { }
    accessor(const accessor &) = default;
    accessor(accessor &&) = default;



    void operator=(const accessor &a) && { std::move(*this).operator=(handle(a)); }
    void operator=(const accessor &a) & { operator=(handle(a)); }

    template <typename T> void operator=(T &&value) && {
        Policy::set(obj, key, object_or_cast(std::forward<T>(value)));
    }
    template <typename T> void operator=(T &&value) & {
        get_cache() = reinterpret_borrow<object>(object_or_cast(std::forward<T>(value)));
    }

    template <typename T = Policy>
    PYBIND11_DEPRECATED("Use of obj.attr(...) as bool is deprecated in favor of pybind11::hasattr(obj, ...)")
    explicit operator enable_if_t<std::is_same<T, accessor_policies::str_attr>::value ||
            std::is_same<T, accessor_policies::obj_attr>::value, bool>() const {
        return hasattr(obj, key);
    }
    template <typename T = Policy>
    PYBIND11_DEPRECATED("Use of obj[key] as bool is deprecated in favor of obj.contains(key)")
    explicit operator enable_if_t<std::is_same<T, accessor_policies::generic_item>::value, bool>() const {
        return obj.contains(key);
    }

    operator object() const { return get_cache(); }
    PyObject *ptr() const { return get_cache().ptr(); }
    template <typename T> T cast() const { return get_cache().template cast<T>(); }

private:
    object &get_cache() const {
        if (!cache) { cache = Policy::get(obj, key); }
        return cache;
    }

private:
    handle obj;
    key_type key;
    mutable object cache;
};

PYBIND11_NAMESPACE_BEGIN(accessor_policies)
struct obj_attr {
    using key_type = object;
    static object get(handle obj, handle key) { return getattr(obj, key); }
    static void set(handle obj, handle key, handle val) { setattr(obj, key, val); }
};

struct str_attr {
    using key_type = const char *;
    static object get(handle obj, const char *key) { return getattr(obj, key); }
    static void set(handle obj, const char *key, handle val) { setattr(obj, key, val); }
};

struct generic_item {
    using key_type = object;

    static object get(handle obj, handle key) {
        PyObject *result = PyObject_GetItem(obj.ptr(), key.ptr());
        if (!result) { throw error_already_set(); }
        return reinterpret_steal<object>(result);
    }

    static void set(handle obj, handle key, handle val) {
        if (PyObject_SetItem(obj.ptr(), key.ptr(), val.ptr()) != 0) { throw error_already_set(); }
    }
};

struct sequence_item {
    using key_type = size_t;

    static object get(handle obj, size_t index) {
        PyObject *result = PySequence_GetItem(obj.ptr(), static_cast<ssize_t>(index));
        if (!result) { throw error_already_set(); }
        return reinterpret_steal<object>(result);
    }

    static void set(handle obj, size_t index, handle val) {

        if (PySequence_SetItem(obj.ptr(), static_cast<ssize_t>(index), val.ptr()) != 0) {
            throw error_already_set();
        }
    }
};

struct list_item {
    using key_type = size_t;

    static object get(handle obj, size_t index) {
        PyObject *result = PyList_GetItem(obj.ptr(), static_cast<ssize_t>(index));
        if (!result) { throw error_already_set(); }
        return reinterpret_borrow<object>(result);
    }

    static void set(handle obj, size_t index, handle val) {

        if (PyList_SetItem(obj.ptr(), static_cast<ssize_t>(index), val.inc_ref().ptr()) != 0) {
            throw error_already_set();
        }
    }
};

struct tuple_item {
    using key_type = size_t;

    static object get(handle obj, size_t index) {
        PyObject *result = PyTuple_GetItem(obj.ptr(), static_cast<ssize_t>(index));
        if (!result) { throw error_already_set(); }
        return reinterpret_borrow<object>(result);
    }

    static void set(handle obj, size_t index, handle val) {

        if (PyTuple_SetItem(obj.ptr(), static_cast<ssize_t>(index), val.inc_ref().ptr()) != 0) {
            throw error_already_set();
        }
    }
};
PYBIND11_NAMESPACE_END(accessor_policies)


template <typename Policy>
class generic_iterator : public Policy {
    using It = generic_iterator;

public:
    using difference_type = ssize_t;
    using iterator_category = typename Policy::iterator_category;
    using value_type = typename Policy::value_type;
    using reference = typename Policy::reference;
    using pointer = typename Policy::pointer;

    generic_iterator() = default;
    generic_iterator(handle seq, ssize_t index) : Policy(seq, index) { }

    reference operator*() const { return Policy::dereference(); }
    reference operator[](difference_type n) const { return *(*this + n); }
    pointer operator->() const { return **this; }

    It &operator++() { Policy::increment(); return *this; }
    It operator++(int) { auto copy = *this; Policy::increment(); return copy; }
    It &operator--() { Policy::decrement(); return *this; }
    It operator--(int) { auto copy = *this; Policy::decrement(); return copy; }
    It &operator+=(difference_type n) { Policy::advance(n); return *this; }
    It &operator-=(difference_type n) { Policy::advance(-n); return *this; }

    friend It operator+(const It &a, difference_type n) { auto copy = a; return copy += n; }
    friend It operator+(difference_type n, const It &b) { return b + n; }
    friend It operator-(const It &a, difference_type n) { auto copy = a; return copy -= n; }
    friend difference_type operator-(const It &a, const It &b) { return a.distance_to(b); }

    friend bool operator==(const It &a, const It &b) { return a.equal(b); }
    friend bool operator!=(const It &a, const It &b) { return !(a == b); }
    friend bool operator< (const It &a, const It &b) { return b - a > 0; }
    friend bool operator> (const It &a, const It &b) { return b < a; }
    friend bool operator>=(const It &a, const It &b) { return !(a < b); }
    friend bool operator<=(const It &a, const It &b) { return !(a > b); }
};

PYBIND11_NAMESPACE_BEGIN(iterator_policies)

template <typename T>
struct arrow_proxy {
    T value;

    arrow_proxy(T &&value) : value(std::move(value)) { }
    T *operator->() const { return &value; }
};


class sequence_fast_readonly {
protected:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = handle;
    using reference = const handle;
    using pointer = arrow_proxy<const handle>;

    sequence_fast_readonly(handle obj, ssize_t n) : ptr(PySequence_Fast_ITEMS(obj.ptr()) + n) { }

    reference dereference() const { return *ptr; }
    void increment() { ++ptr; }
    void decrement() { --ptr; }
    void advance(ssize_t n) { ptr += n; }
    bool equal(const sequence_fast_readonly &b) const { return ptr == b.ptr; }
    ssize_t distance_to(const sequence_fast_readonly &b) const { return ptr - b.ptr; }

private:
    PyObject **ptr;
};


class sequence_slow_readwrite {
protected:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = object;
    using reference = sequence_accessor;
    using pointer = arrow_proxy<const sequence_accessor>;

    sequence_slow_readwrite(handle obj, ssize_t index) : obj(obj), index(index) { }

    reference dereference() const { return {obj, static_cast<size_t>(index)}; }
    void increment() { ++index; }
    void decrement() { --index; }
    void advance(ssize_t n) { index += n; }
    bool equal(const sequence_slow_readwrite &b) const { return index == b.index; }
    ssize_t distance_to(const sequence_slow_readwrite &b) const { return index - b.index; }

private:
    handle obj;
    ssize_t index;
};


class dict_readonly {
protected:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<handle, handle>;
    using reference = const value_type;
    using pointer = arrow_proxy<const value_type>;

    dict_readonly() = default;
    dict_readonly(handle obj, ssize_t pos) : obj(obj), pos(pos) { increment(); }

    reference dereference() const { return {key, value}; }
    void increment() { if (!PyDict_Next(obj.ptr(), &pos, &key, &value)) { pos = -1; } }
    bool equal(const dict_readonly &b) const { return pos == b.pos; }

private:
    handle obj;
    PyObject *key = nullptr, *value = nullptr;
    ssize_t pos = -1;
};
PYBIND11_NAMESPACE_END(iterator_policies)

#if !defined(PYPY_VERSION)
using tuple_iterator = generic_iterator<iterator_policies::sequence_fast_readonly>;
using list_iterator = generic_iterator<iterator_policies::sequence_fast_readonly>;
#else
using tuple_iterator = generic_iterator<iterator_policies::sequence_slow_readwrite>;
using list_iterator = generic_iterator<iterator_policies::sequence_slow_readwrite>;
#endif

using sequence_iterator = generic_iterator<iterator_policies::sequence_slow_readwrite>;
using dict_iterator = generic_iterator<iterator_policies::dict_readonly>;

inline bool PyIterable_Check(PyObject *obj) {
    PyObject *iter = PyObject_GetIter(obj);
    if (iter) {
        Py_DECREF(iter);
        return true;
    } else {
        PyErr_Clear();
        return false;
    }
}

inline bool PyNone_Check(PyObject *o) { return o == Py_None; }
inline bool PyEllipsis_Check(PyObject *o) { return o == Py_Ellipsis; }

inline bool PyUnicode_Check_Permissive(PyObject *o) { return PyUnicode_Check(o) || PYBIND11_BYTES_CHECK(o); }

inline bool PyStaticMethod_Check(PyObject *o) { return o->ob_type == &PyStaticMethod_Type; }

class kwargs_proxy : public handle {
public:
    explicit kwargs_proxy(handle h) : handle(h) { }
};

class args_proxy : public handle {
public:
    explicit args_proxy(handle h) : handle(h) { }
    kwargs_proxy operator*() const { return kwargs_proxy(*this); }
};


template <typename T> using is_keyword = std::is_base_of<arg, T>;
template <typename T> using is_s_unpacking = std::is_same<args_proxy, T>;
template <typename T> using is_ds_unpacking = std::is_same<kwargs_proxy, T>;
template <typename T> using is_positional = satisfies_none_of<T,
    is_keyword, is_s_unpacking, is_ds_unpacking
>;
template <typename T> using is_keyword_or_ds = satisfies_any_of<T, is_keyword, is_ds_unpacking>;


template <return_value_policy policy = return_value_policy::automatic_reference>
class simple_collector;
template <return_value_policy policy = return_value_policy::automatic_reference>
class unpacking_collector;

PYBIND11_NAMESPACE_END(detail)





#define PYBIND11_OBJECT_COMMON(Name, Parent, CheckFun) \
    public: \
        PYBIND11_DEPRECATED("Use reinterpret_borrow<"#Name">() or reinterpret_steal<"#Name">()") \
        Name(handle h, bool is_borrowed) : Parent(is_borrowed ? Parent(h, borrowed_t{}) : Parent(h, stolen_t{})) { } \
        Name(handle h, borrowed_t) : Parent(h, borrowed_t{}) { } \
        Name(handle h, stolen_t) : Parent(h, stolen_t{}) { } \
        PYBIND11_DEPRECATED("Use py::isinstance<py::python_type>(obj) instead") \
        bool check() const { return m_ptr != nullptr && (bool) CheckFun(m_ptr); } \
        static bool check_(handle h) { return h.ptr() != nullptr && CheckFun(h.ptr()); } \
        template <typename Policy_> \
        Name(const ::pybind11::detail::accessor<Policy_> &a) : Name(object(a)) { }

#define PYBIND11_OBJECT_CVT(Name, Parent, CheckFun, ConvertFun) \
    PYBIND11_OBJECT_COMMON(Name, Parent, CheckFun) \
      \
    Name(const object &o) \
    : Parent(check_(o) ? o.inc_ref().ptr() : ConvertFun(o.ptr()), stolen_t{}) \
    { if (!m_ptr) throw error_already_set(); } \
    Name(object &&o) \
    : Parent(check_(o) ? o.release().ptr() : ConvertFun(o.ptr()), stolen_t{}) \
    { if (!m_ptr) throw error_already_set(); }

#define PYBIND11_OBJECT_CHECK_FAILED(Name, o) \
    ::pybind11::type_error("Object of type '" + \
                           ::pybind11::detail::get_fully_qualified_tp_name(Py_TYPE(o.ptr())) + \
                           "' is not an instance of '" #Name "'")

#define PYBIND11_OBJECT(Name, Parent, CheckFun) \
    PYBIND11_OBJECT_COMMON(Name, Parent, CheckFun) \
      \
    Name(const object &o) : Parent(o) \
    { if (o && !check_(o)) throw PYBIND11_OBJECT_CHECK_FAILED(Name, o); } \
    Name(object &&o) : Parent(std::move(o)) \
    { if (o && !check_(o)) throw PYBIND11_OBJECT_CHECK_FAILED(Name, o); }

#define PYBIND11_OBJECT_DEFAULT(Name, Parent, CheckFun) \
    PYBIND11_OBJECT(Name, Parent, CheckFun) \
    Name() : Parent() { }





class iterator : public object {
public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = ssize_t;
    using value_type = handle;
    using reference = const handle;
    using pointer = const handle *;

    PYBIND11_OBJECT_DEFAULT(iterator, object, PyIter_Check)

    iterator& operator++() {
        advance();
        return *this;
    }

    iterator operator++(int) {
        auto rv = *this;
        advance();
        return rv;
    }

    reference operator*() const {
        if (m_ptr && !value.ptr()) {
            auto& self = const_cast<iterator &>(*this);
            self.advance();
        }
        return value;
    }

    pointer operator->() const { operator*(); return &value; }


    static iterator sentinel() { return {}; }

    friend bool operator==(const iterator &a, const iterator &b) { return a->ptr() == b->ptr(); }
    friend bool operator!=(const iterator &a, const iterator &b) { return a->ptr() != b->ptr(); }

private:
    void advance() {
        value = reinterpret_steal<object>(PyIter_Next(m_ptr));
        if (PyErr_Occurred()) { throw error_already_set(); }
    }

private:
    object value = {};
};



class type : public object {
public:
    PYBIND11_OBJECT(type, object, PyType_Check)


    static handle handle_of(handle h) { return handle((PyObject*) Py_TYPE(h.ptr())); }


    static type of(handle h) { return type(type::handle_of(h), borrowed_t{}); }





    template<typename T>
    static handle handle_of();




    template<typename T>
    static type of() {return type(type::handle_of<T>(), borrowed_t{}); }
};

class iterable : public object {
public:
    PYBIND11_OBJECT_DEFAULT(iterable, object, detail::PyIterable_Check)
};

class bytes;

class str : public object {
public:
    PYBIND11_OBJECT_CVT(str, object, detail::PyUnicode_Check_Permissive, raw_str)

    str(const char *c, size_t n)
        : object(PyUnicode_FromStringAndSize(c, (ssize_t) n), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate string object!");
    }


    str(const char *c = "")
        : object(PyUnicode_FromString(c), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate string object!");
    }

    str(const std::string &s) : str(s.data(), s.size()) { }

    explicit str(const bytes &b);


    explicit str(handle h) : object(raw_str(h.ptr()), stolen_t{}) { if (!m_ptr) throw error_already_set(); }

    operator std::string() const {
        object temp = *this;
        if (PyUnicode_Check(m_ptr)) {
            temp = reinterpret_steal<object>(PyUnicode_AsUTF8String(m_ptr));
            if (!temp)
                pybind11_fail("Unable to extract string contents! (encoding issue)");
        }
        char *buffer;
        ssize_t length;
        if (PYBIND11_BYTES_AS_STRING_AND_SIZE(temp.ptr(), &buffer, &length))
            pybind11_fail("Unable to extract string contents! (invalid type)");
        return std::string(buffer, (size_t) length);
    }

    template <typename... Args>
    str format(Args &&...args) const {
        return attr("format")(std::forward<Args>(args)...);
    }

private:

    static PyObject *raw_str(PyObject *op) {
        PyObject *str_value = PyObject_Str(op);
#if PY_MAJOR_VERSION < 3
        if (!str_value) throw error_already_set();
        PyObject *unicode = PyUnicode_FromEncodedObject(str_value, "utf-8", nullptr);
        Py_XDECREF(str_value); str_value = unicode;
#endif
        return str_value;
    }
};


inline namespace literals {

inline str operator"" _s(const char *s, size_t size) { return {s, size}; }
}



class bytes : public object {
public:
    PYBIND11_OBJECT(bytes, object, PYBIND11_BYTES_CHECK)


    bytes(const char *c = "")
        : object(PYBIND11_BYTES_FROM_STRING(c), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate bytes object!");
    }

    bytes(const char *c, size_t n)
        : object(PYBIND11_BYTES_FROM_STRING_AND_SIZE(c, (ssize_t) n), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate bytes object!");
    }


    bytes(const std::string &s) : bytes(s.data(), s.size()) { }

    explicit bytes(const pybind11::str &s);

    operator std::string() const {
        char *buffer;
        ssize_t length;
        if (PYBIND11_BYTES_AS_STRING_AND_SIZE(m_ptr, &buffer, &length))
            pybind11_fail("Unable to extract bytes contents!");
        return std::string(buffer, (size_t) length);
    }
};




inline bytes::bytes(const pybind11::str &s) {
    object temp = s;
    if (PyUnicode_Check(s.ptr())) {
        temp = reinterpret_steal<object>(PyUnicode_AsUTF8String(s.ptr()));
        if (!temp)
            pybind11_fail("Unable to extract string contents! (encoding issue)");
    }
    char *buffer;
    ssize_t length;
    if (PYBIND11_BYTES_AS_STRING_AND_SIZE(temp.ptr(), &buffer, &length))
        pybind11_fail("Unable to extract string contents! (invalid type)");
    auto obj = reinterpret_steal<object>(PYBIND11_BYTES_FROM_STRING_AND_SIZE(buffer, length));
    if (!obj)
        pybind11_fail("Could not allocate bytes object!");
    m_ptr = obj.release().ptr();
}

inline str::str(const bytes& b) {
    char *buffer;
    ssize_t length;
    if (PYBIND11_BYTES_AS_STRING_AND_SIZE(b.ptr(), &buffer, &length))
        pybind11_fail("Unable to extract bytes contents!");
    auto obj = reinterpret_steal<object>(PyUnicode_FromStringAndSize(buffer, (ssize_t) length));
    if (!obj)
        pybind11_fail("Could not allocate string object!");
    m_ptr = obj.release().ptr();
}



class none : public object {
public:
    PYBIND11_OBJECT(none, object, detail::PyNone_Check)
    none() : object(Py_None, borrowed_t{}) { }
};

class ellipsis : public object {
public:
    PYBIND11_OBJECT(ellipsis, object, detail::PyEllipsis_Check)
    ellipsis() : object(Py_Ellipsis, borrowed_t{}) { }
};

class bool_ : public object {
public:
    PYBIND11_OBJECT_CVT(bool_, object, PyBool_Check, raw_bool)
    bool_() : object(Py_False, borrowed_t{}) { }

    bool_(bool value) : object(value ? Py_True : Py_False, borrowed_t{}) { }
    operator bool() const { return m_ptr && PyLong_AsLong(m_ptr) != 0; }

private:

    static PyObject *raw_bool(PyObject *op) {
        const auto value = PyObject_IsTrue(op);
        if (value == -1) return nullptr;
        return handle(value ? Py_True : Py_False).inc_ref().ptr();
    }
};

PYBIND11_NAMESPACE_BEGIN(detail)




template <typename Unsigned>
Unsigned as_unsigned(PyObject *o) {
    if (sizeof(Unsigned) <= sizeof(unsigned long)
#if PY_VERSION_HEX < 0x03000000
            || PyInt_Check(o)
#endif
    ) {
        unsigned long v = PyLong_AsUnsignedLong(o);
        return v == (unsigned long) -1 && PyErr_Occurred() ? (Unsigned) -1 : (Unsigned) v;
    }
    else {
        unsigned long long v = PyLong_AsUnsignedLongLong(o);
        return v == (unsigned long long) -1 && PyErr_Occurred() ? (Unsigned) -1 : (Unsigned) v;
    }
}
PYBIND11_NAMESPACE_END(detail)

class int_ : public object {
public:
    PYBIND11_OBJECT_CVT(int_, object, PYBIND11_LONG_CHECK, PyNumber_Long)
    int_() : object(PyLong_FromLong(0), stolen_t{}) { }

    template <typename T,
              detail::enable_if_t<std::is_integral<T>::value, int> = 0>
    int_(T value) {
        if (sizeof(T) <= sizeof(long)) {
            if (std::is_signed<T>::value)
                m_ptr = PyLong_FromLong((long) value);
            else
                m_ptr = PyLong_FromUnsignedLong((unsigned long) value);
        } else {
            if (std::is_signed<T>::value)
                m_ptr = PyLong_FromLongLong((long long) value);
            else
                m_ptr = PyLong_FromUnsignedLongLong((unsigned long long) value);
        }
        if (!m_ptr) pybind11_fail("Could not allocate int object!");
    }

    template <typename T,
              detail::enable_if_t<std::is_integral<T>::value, int> = 0>
    operator T() const {
        return std::is_unsigned<T>::value
            ? detail::as_unsigned<T>(m_ptr)
            : sizeof(T) <= sizeof(long)
              ? (T) PyLong_AsLong(m_ptr)
              : (T) PYBIND11_LONG_AS_LONGLONG(m_ptr);
    }
};

class float_ : public object {
public:
    PYBIND11_OBJECT_CVT(float_, object, PyFloat_Check, PyNumber_Float)

    float_(float value) : object(PyFloat_FromDouble((double) value), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate float object!");
    }
    float_(double value = .0) : object(PyFloat_FromDouble((double) value), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate float object!");
    }
    operator float() const { return (float) PyFloat_AsDouble(m_ptr); }
    operator double() const { return (double) PyFloat_AsDouble(m_ptr); }
};

class weakref : public object {
public:
    PYBIND11_OBJECT_DEFAULT(weakref, object, PyWeakref_Check)
    explicit weakref(handle obj, handle callback = {})
        : object(PyWeakref_NewRef(obj.ptr(), callback.ptr()), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate weak reference!");
    }
};

class slice : public object {
public:
    PYBIND11_OBJECT_DEFAULT(slice, object, PySlice_Check)
    slice(ssize_t start_, ssize_t stop_, ssize_t step_) {
        int_ start(start_), stop(stop_), step(step_);
        m_ptr = PySlice_New(start.ptr(), stop.ptr(), step.ptr());
        if (!m_ptr) pybind11_fail("Could not allocate slice object!");
    }
    bool compute(size_t length, size_t *start, size_t *stop, size_t *step,
                 size_t *slicelength) const {
        return PySlice_GetIndicesEx((PYBIND11_SLICE_OBJECT *) m_ptr,
                                    (ssize_t) length, (ssize_t *) start,
                                    (ssize_t *) stop, (ssize_t *) step,
                                    (ssize_t *) slicelength) == 0;
    }
    bool compute(ssize_t length, ssize_t *start, ssize_t *stop, ssize_t *step,
      ssize_t *slicelength) const {
      return PySlice_GetIndicesEx((PYBIND11_SLICE_OBJECT *) m_ptr,
          length, start,
          stop, step,
          slicelength) == 0;
    }
};

class capsule : public object {
public:
    PYBIND11_OBJECT_DEFAULT(capsule, object, PyCapsule_CheckExact)
    PYBIND11_DEPRECATED("Use reinterpret_borrow<capsule>() or reinterpret_steal<capsule>()")
    capsule(PyObject *ptr, bool is_borrowed) : object(is_borrowed ? object(ptr, borrowed_t{}) : object(ptr, stolen_t{})) { }

    explicit capsule(const void *value, const char *name = nullptr, void (*destructor)(PyObject *) = nullptr)
        : object(PyCapsule_New(const_cast<void *>(value), name, destructor), stolen_t{}) {
        if (!m_ptr)
            pybind11_fail("Could not allocate capsule object!");
    }

    PYBIND11_DEPRECATED("Please pass a destructor that takes a void pointer as input")
    capsule(const void *value, void (*destruct)(PyObject *))
        : object(PyCapsule_New(const_cast<void*>(value), nullptr, destruct), stolen_t{}) {
        if (!m_ptr)
            pybind11_fail("Could not allocate capsule object!");
    }

    capsule(const void *value, void (*destructor)(void *)) {
        m_ptr = PyCapsule_New(const_cast<void *>(value), nullptr, [](PyObject *o) {
            auto destructor = reinterpret_cast<void (*)(void *)>(PyCapsule_GetContext(o));
            void *ptr = PyCapsule_GetPointer(o, nullptr);
            destructor(ptr);
        });

        if (!m_ptr)
            pybind11_fail("Could not allocate capsule object!");

        if (PyCapsule_SetContext(m_ptr, (void *) destructor) != 0)
            pybind11_fail("Could not set capsule context!");
    }

    capsule(void (*destructor)()) {
        m_ptr = PyCapsule_New(reinterpret_cast<void *>(destructor), nullptr, [](PyObject *o) {
            auto destructor = reinterpret_cast<void (*)()>(PyCapsule_GetPointer(o, nullptr));
            destructor();
        });

        if (!m_ptr)
            pybind11_fail("Could not allocate capsule object!");
    }

    template <typename T> operator T *() const {
        return get_pointer<T>();
    }


    template<typename T = void>
    T* get_pointer() const {
        auto name = this->name();
        T *result = static_cast<T *>(PyCapsule_GetPointer(m_ptr, name));
        if (!result) pybind11_fail("Unable to extract capsule contents!");
        return result;
    }


    void set_pointer(const void *value) {
        if (PyCapsule_SetPointer(m_ptr, const_cast<void *>(value)) != 0)
            pybind11_fail("Could not set capsule pointer");
    }

    const char *name() const { return PyCapsule_GetName(m_ptr); }
};

class tuple : public object {
public:
    PYBIND11_OBJECT_CVT(tuple, object, PyTuple_Check, PySequence_Tuple)
    explicit tuple(size_t size = 0) : object(PyTuple_New((ssize_t) size), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate tuple object!");
    }
    size_t size() const { return (size_t) PyTuple_Size(m_ptr); }
    bool empty() const { return size() == 0; }
    detail::tuple_accessor operator[](size_t index) const { return {*this, index}; }
    detail::item_accessor operator[](handle h) const { return object::operator[](h); }
    detail::tuple_iterator begin() const { return {*this, 0}; }
    detail::tuple_iterator end() const { return {*this, PyTuple_GET_SIZE(m_ptr)}; }
};

class dict : public object {
public:
    PYBIND11_OBJECT_CVT(dict, object, PyDict_Check, raw_dict)
    dict() : object(PyDict_New(), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate dict object!");
    }
    template <typename... Args,
              typename = detail::enable_if_t<detail::all_of<detail::is_keyword_or_ds<Args>...>::value>,

              typename collector = detail::deferred_t<detail::unpacking_collector<>, Args...>>
    explicit dict(Args &&...args) : dict(collector(std::forward<Args>(args)...).kwargs()) { }

    size_t size() const { return (size_t) PyDict_Size(m_ptr); }
    bool empty() const { return size() == 0; }
    detail::dict_iterator begin() const { return {*this, 0}; }
    detail::dict_iterator end() const { return {}; }
    void clear() const { PyDict_Clear(ptr()); }
    template <typename T> bool contains(T &&key) const {
        return PyDict_Contains(m_ptr, detail::object_or_cast(std::forward<T>(key)).ptr()) == 1;
    }

private:

    static PyObject *raw_dict(PyObject *op) {
        if (PyDict_Check(op))
            return handle(op).inc_ref().ptr();
        return PyObject_CallFunctionObjArgs((PyObject *) &PyDict_Type, op, nullptr);
    }
};

class sequence : public object {
public:
    PYBIND11_OBJECT_DEFAULT(sequence, object, PySequence_Check)
    size_t size() const {
        ssize_t result = PySequence_Size(m_ptr);
        if (result == -1)
            throw error_already_set();
        return (size_t) result;
    }
    bool empty() const { return size() == 0; }
    detail::sequence_accessor operator[](size_t index) const { return {*this, index}; }
    detail::item_accessor operator[](handle h) const { return object::operator[](h); }
    detail::sequence_iterator begin() const { return {*this, 0}; }
    detail::sequence_iterator end() const { return {*this, PySequence_Size(m_ptr)}; }
};

class list : public object {
public:
    PYBIND11_OBJECT_CVT(list, object, PyList_Check, PySequence_List)
    explicit list(size_t size = 0) : object(PyList_New((ssize_t) size), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate list object!");
    }
    size_t size() const { return (size_t) PyList_Size(m_ptr); }
    bool empty() const { return size() == 0; }
    detail::list_accessor operator[](size_t index) const { return {*this, index}; }
    detail::item_accessor operator[](handle h) const { return object::operator[](h); }
    detail::list_iterator begin() const { return {*this, 0}; }
    detail::list_iterator end() const { return {*this, PyList_GET_SIZE(m_ptr)}; }
    template <typename T> void append(T &&val) const {
        PyList_Append(m_ptr, detail::object_or_cast(std::forward<T>(val)).ptr());
    }
    template <typename T> void insert(size_t index, T &&val) const {
        PyList_Insert(m_ptr, static_cast<ssize_t>(index),
            detail::object_or_cast(std::forward<T>(val)).ptr());
    }
};

class args : public tuple { PYBIND11_OBJECT_DEFAULT(args, tuple, PyTuple_Check) };
class kwargs : public dict { PYBIND11_OBJECT_DEFAULT(kwargs, dict, PyDict_Check)  };

class set : public object {
public:
    PYBIND11_OBJECT_CVT(set, object, PySet_Check, PySet_New)
    set() : object(PySet_New(nullptr), stolen_t{}) {
        if (!m_ptr) pybind11_fail("Could not allocate set object!");
    }
    size_t size() const { return (size_t) PySet_Size(m_ptr); }
    bool empty() const { return size() == 0; }
    template <typename T> bool add(T &&val) const {
        return PySet_Add(m_ptr, detail::object_or_cast(std::forward<T>(val)).ptr()) == 0;
    }
    void clear() const { PySet_Clear(m_ptr); }
    template <typename T> bool contains(T &&val) const {
        return PySet_Contains(m_ptr, detail::object_or_cast(std::forward<T>(val)).ptr()) == 1;
    }
};

class function : public object {
public:
    PYBIND11_OBJECT_DEFAULT(function, object, PyCallable_Check)
    handle cpp_function() const {
        handle fun = detail::get_function(m_ptr);
        if (fun && PyCFunction_Check(fun.ptr()))
            return fun;
        return handle();
    }
    bool is_cpp_function() const { return (bool) cpp_function(); }
};

class staticmethod : public object {
public:
    PYBIND11_OBJECT_CVT(staticmethod, object, detail::PyStaticMethod_Check, PyStaticMethod_New)
};

class buffer : public object {
public:
    PYBIND11_OBJECT_DEFAULT(buffer, object, PyObject_CheckBuffer)

    buffer_info request(bool writable = false) const {
        int flags = PyBUF_STRIDES | PyBUF_FORMAT;
        if (writable) flags |= PyBUF_WRITABLE;
        auto *view = new Py_buffer();
        if (PyObject_GetBuffer(m_ptr, view, flags) != 0) {
            delete view;
            throw error_already_set();
        }
        return buffer_info(view);
    }
};

class memoryview : public object {
public:
    PYBIND11_OBJECT_CVT(memoryview, object, PyMemoryView_Check, PyMemoryView_FromObject)


    explicit memoryview(const buffer_info& info) {
        if (!info.view())
            pybind11_fail("Prohibited to create memoryview without Py_buffer");

        m_ptr = (info.view()->obj) ?
            PyMemoryView_FromObject(info.view()->obj) :
            PyMemoryView_FromBuffer(info.view());
        if (!m_ptr)
            pybind11_fail("Unable to create memoryview from buffer descriptor");
    }


    static memoryview from_buffer(
        void *ptr, ssize_t itemsize, const char *format,
        detail::any_container<ssize_t> shape,
        detail::any_container<ssize_t> strides, bool readonly = false);

    static memoryview from_buffer(
        const void *ptr, ssize_t itemsize, const char *format,
        detail::any_container<ssize_t> shape,
        detail::any_container<ssize_t> strides) {
        return memoryview::from_buffer(
            const_cast<void*>(ptr), itemsize, format, shape, strides, true);
    }

    template<typename T>
    static memoryview from_buffer(
        T *ptr, detail::any_container<ssize_t> shape,
        detail::any_container<ssize_t> strides, bool readonly = false) {
        return memoryview::from_buffer(
            reinterpret_cast<void*>(ptr), sizeof(T),
            format_descriptor<T>::value, shape, strides, readonly);
    }

    template<typename T>
    static memoryview from_buffer(
        const T *ptr, detail::any_container<ssize_t> shape,
        detail::any_container<ssize_t> strides) {
        return memoryview::from_buffer(
            const_cast<T*>(ptr), shape, strides, true);
    }

#if PY_MAJOR_VERSION >= 3

    static memoryview from_memory(void *mem, ssize_t size, bool readonly = false) {
        PyObject* ptr = PyMemoryView_FromMemory(
            reinterpret_cast<char*>(mem), size,
            (readonly) ? PyBUF_READ : PyBUF_WRITE);
        if (!ptr)
            pybind11_fail("Could not allocate memoryview object!");
        return memoryview(object(ptr, stolen_t{}));
    }

    static memoryview from_memory(const void *mem, ssize_t size) {
        return memoryview::from_memory(const_cast<void*>(mem), size, true);
    }
#endif
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
inline memoryview memoryview::from_buffer(
    void *ptr, ssize_t itemsize, const char* format,
    detail::any_container<ssize_t> shape,
    detail::any_container<ssize_t> strides, bool readonly) {
    size_t ndim = shape->size();
    if (ndim != strides->size())
        pybind11_fail("memoryview: shape length doesn't match strides length");
    ssize_t size = ndim ? 1 : 0;
    for (size_t i = 0; i < ndim; ++i)
        size *= (*shape)[i];
    Py_buffer view;
    view.buf = ptr;
    view.obj = nullptr;
    view.len = size * itemsize;
    view.readonly = static_cast<int>(readonly);
    view.itemsize = itemsize;
    view.format = const_cast<char*>(format);
    view.ndim = static_cast<int>(ndim);
    view.shape = shape->data();
    view.strides = strides->data();
    view.suboffsets = nullptr;
    view.internal = nullptr;
    PyObject* obj = PyMemoryView_FromBuffer(&view);
    if (!obj)
        throw error_already_set();
    return memoryview(object(obj, stolen_t{}));
}
#endif






inline size_t len(handle h) {
    ssize_t result = PyObject_Length(h.ptr());
    if (result < 0)
        throw error_already_set();
    return (size_t) result;
}



inline size_t len_hint(handle h) {
#if PY_VERSION_HEX >= 0x03040000
    ssize_t result = PyObject_LengthHint(h.ptr(), 0);
#else
    ssize_t result = PyObject_Length(h.ptr());
#endif
    if (result < 0) {


        PyErr_Clear();
        return 0;
    }
    return (size_t) result;
}

inline str repr(handle h) {
    PyObject *str_value = PyObject_Repr(h.ptr());
    if (!str_value) throw error_already_set();
#if PY_MAJOR_VERSION < 3
    PyObject *unicode = PyUnicode_FromEncodedObject(str_value, "utf-8", nullptr);
    Py_XDECREF(str_value); str_value = unicode;
    if (!str_value) throw error_already_set();
#endif
    return reinterpret_steal<str>(str_value);
}

inline iterator iter(handle obj) {
    PyObject *result = PyObject_GetIter(obj.ptr());
    if (!result) { throw error_already_set(); }
    return reinterpret_steal<iterator>(result);
}


PYBIND11_NAMESPACE_BEGIN(detail)
template <typename D> iterator object_api<D>::begin() const { return iter(derived()); }
template <typename D> iterator object_api<D>::end() const { return iterator::sentinel(); }
template <typename D> item_accessor object_api<D>::operator[](handle key) const {
    return {derived(), reinterpret_borrow<object>(key)};
}
template <typename D> item_accessor object_api<D>::operator[](const char *key) const {
    return {derived(), pybind11::str(key)};
}
template <typename D> obj_attr_accessor object_api<D>::attr(handle key) const {
    return {derived(), reinterpret_borrow<object>(key)};
}
template <typename D> str_attr_accessor object_api<D>::attr(const char *key) const {
    return {derived(), key};
}
template <typename D> args_proxy object_api<D>::operator*() const {
    return args_proxy(derived().ptr());
}
template <typename D> template <typename T> bool object_api<D>::contains(T &&item) const {
    return attr("__contains__")(std::forward<T>(item)).template cast<bool>();
}

template <typename D>
pybind11::str object_api<D>::str() const { return pybind11::str(derived()); }

template <typename D>
str_attr_accessor object_api<D>::doc() const { return attr("__doc__"); }

template <typename D>
handle object_api<D>::get_type() const { return type::handle_of(derived()); }

template <typename D>
bool object_api<D>::rich_compare(object_api const &other, int value) const {
    int rv = PyObject_RichCompareBool(derived().ptr(), other.derived().ptr(), value);
    if (rv == -1)
        throw error_already_set();
    return rv == 1;
}

#define PYBIND11_MATH_OPERATOR_UNARY(op, fn)                                   \
    template <typename D> object object_api<D>::op() const {                   \
        object result = reinterpret_steal<object>(fn(derived().ptr()));        \
        if (!result.ptr())                                                     \
            throw error_already_set();                                         \
        return result;                                                         \
    }

#define PYBIND11_MATH_OPERATOR_BINARY(op, fn)                                  \
    template <typename D>                                                      \
    object object_api<D>::op(object_api const &other) const {                  \
        object result = reinterpret_steal<object>(                             \
            fn(derived().ptr(), other.derived().ptr()));                       \
        if (!result.ptr())                                                     \
            throw error_already_set();                                         \
        return result;                                                         \
    }

PYBIND11_MATH_OPERATOR_UNARY (operator~,   PyNumber_Invert)
PYBIND11_MATH_OPERATOR_UNARY (operator-,   PyNumber_Negative)
PYBIND11_MATH_OPERATOR_BINARY(operator+,   PyNumber_Add)
PYBIND11_MATH_OPERATOR_BINARY(operator+=,  PyNumber_InPlaceAdd)
PYBIND11_MATH_OPERATOR_BINARY(operator-,   PyNumber_Subtract)
PYBIND11_MATH_OPERATOR_BINARY(operator-=,  PyNumber_InPlaceSubtract)
PYBIND11_MATH_OPERATOR_BINARY(operator*,   PyNumber_Multiply)
PYBIND11_MATH_OPERATOR_BINARY(operator*=,  PyNumber_InPlaceMultiply)
PYBIND11_MATH_OPERATOR_BINARY(operator/,   PyNumber_TrueDivide)
PYBIND11_MATH_OPERATOR_BINARY(operator/=,  PyNumber_InPlaceTrueDivide)
PYBIND11_MATH_OPERATOR_BINARY(operator|,   PyNumber_Or)
PYBIND11_MATH_OPERATOR_BINARY(operator|=,  PyNumber_InPlaceOr)
PYBIND11_MATH_OPERATOR_BINARY(operator&,   PyNumber_And)
PYBIND11_MATH_OPERATOR_BINARY(operator&=,  PyNumber_InPlaceAnd)
PYBIND11_MATH_OPERATOR_BINARY(operator^,   PyNumber_Xor)
PYBIND11_MATH_OPERATOR_BINARY(operator^=,  PyNumber_InPlaceXor)
PYBIND11_MATH_OPERATOR_BINARY(operator<<,  PyNumber_Lshift)
PYBIND11_MATH_OPERATOR_BINARY(operator<<=, PyNumber_InPlaceLshift)
PYBIND11_MATH_OPERATOR_BINARY(operator>>,  PyNumber_Rshift)
PYBIND11_MATH_OPERATOR_BINARY(operator>>=, PyNumber_InPlaceRshift)

#undef PYBIND11_MATH_OPERATOR_UNARY
#undef PYBIND11_MATH_OPERATOR_BINARY

PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
