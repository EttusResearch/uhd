/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#define PYBIND11_VERSION_MAJOR 2
#define PYBIND11_VERSION_MINOR 6
#define PYBIND11_VERSION_PATCH 1

#define PYBIND11_NAMESPACE_BEGIN(name) namespace name {
#define PYBIND11_NAMESPACE_END(name) }




#if !defined(PYBIND11_NAMESPACE)
#  ifdef __GNUG__
#    define PYBIND11_NAMESPACE pybind11 __attribute__((visibility("hidden")))
#  else
#    define PYBIND11_NAMESPACE pybind11
#  endif
#endif

#if !(defined(_MSC_VER) && __cplusplus == 199711L) && !defined(__INTEL_COMPILER)
#  if __cplusplus >= 201402L
#    define PYBIND11_CPP14
#    if __cplusplus >= 201703L
#      define PYBIND11_CPP17
#    endif
#  endif
#elif defined(_MSC_VER) && __cplusplus == 199711L


#  if _MSVC_LANG >= 201402L
#    define PYBIND11_CPP14
#    if _MSVC_LANG > 201402L && _MSC_VER >= 1910
#      define PYBIND11_CPP17
#    endif
#  endif
#endif


#if defined(__INTEL_COMPILER)
#  if __INTEL_COMPILER < 1800
#    error pybind11 requires Intel C++ compiler v18 or newer
#  endif
#elif defined(__clang__) && !defined(__apple_build_version__)
#  if __clang_major__ < 3 || (__clang_major__ == 3 && __clang_minor__ < 3)
#    error pybind11 requires clang 3.3 or newer
#  endif
#elif defined(__clang__)


#  if __clang_major__ < 5
#    error pybind11 requires Xcode/clang 5.0 or newer
#  endif
#elif defined(__GNUG__)
#  if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
#    error pybind11 requires gcc 4.8 or newer
#  endif
#elif defined(_MSC_VER)


#  if _MSC_FULL_VER < 190024210
#    error pybind11 requires MSVC 2015 update 3 or newer
#  endif
#endif

#if !defined(PYBIND11_EXPORT)
#  if defined(WIN32) || defined(_WIN32)
#    define PYBIND11_EXPORT __declspec(dllexport)
#  else
#    define PYBIND11_EXPORT __attribute__ ((visibility("default")))
#  endif
#endif

#if defined(_MSC_VER)
#  define PYBIND11_NOINLINE __declspec(noinline)
#else
#  define PYBIND11_NOINLINE __attribute__ ((noinline))
#endif

#if defined(PYBIND11_CPP14)
#  define PYBIND11_DEPRECATED(reason) [[deprecated(reason)]]
#else
#  define PYBIND11_DEPRECATED(reason) __attribute__((deprecated(reason)))
#endif

#if defined(PYBIND11_CPP17)
#  define PYBIND11_MAYBE_UNUSED [[maybe_unused]]
#elif defined(_MSC_VER) && !defined(__clang__)
#  define PYBIND11_MAYBE_UNUSED
#else
#  define PYBIND11_MAYBE_UNUSED __attribute__ ((__unused__))
#endif


#if defined(_MSC_VER) && _MSC_VER >= 1900
#  define HAVE_SNPRINTF 1
#endif


#if defined(_MSC_VER)
#  if (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 4)
#    define HAVE_ROUND 1
#  endif
#  pragma warning(push)
#  pragma warning(disable: 4510 4610 4512 4005)
#  if defined(_DEBUG) && !defined(Py_DEBUG)
#    define PYBIND11_DEBUG_MARKER
#    undef _DEBUG
#  endif
#endif

#include <Python.h>
#include <frameobject.h>
#include <pythread.h>


#if defined(isalnum)
#  undef isalnum
#  undef isalpha
#  undef islower
#  undef isspace
#  undef isupper
#  undef tolower
#  undef toupper
#endif

#if defined(copysign)
#  undef copysign
#endif

#if defined(_MSC_VER)
#  if defined(PYBIND11_DEBUG_MARKER)
#    define _DEBUG
#    undef PYBIND11_DEBUG_MARKER
#  endif
#  pragma warning(pop)
#endif

#include <cstddef>
#include <cstring>
#include <forward_list>
#include <vector>
#include <string>
#include <stdexcept>
#include <exception>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <type_traits>

#if PY_MAJOR_VERSION >= 3
#define PYBIND11_INSTANCE_METHOD_NEW(ptr, class_) PyInstanceMethod_New(ptr)
#define PYBIND11_INSTANCE_METHOD_CHECK PyInstanceMethod_Check
#define PYBIND11_INSTANCE_METHOD_GET_FUNCTION PyInstanceMethod_GET_FUNCTION
#define PYBIND11_BYTES_CHECK PyBytes_Check
#define PYBIND11_BYTES_FROM_STRING PyBytes_FromString
#define PYBIND11_BYTES_FROM_STRING_AND_SIZE PyBytes_FromStringAndSize
#define PYBIND11_BYTES_AS_STRING_AND_SIZE PyBytes_AsStringAndSize
#define PYBIND11_BYTES_AS_STRING PyBytes_AsString
#define PYBIND11_BYTES_SIZE PyBytes_Size
#define PYBIND11_LONG_CHECK(o) PyLong_Check(o)
#define PYBIND11_LONG_AS_LONGLONG(o) PyLong_AsLongLong(o)
#define PYBIND11_LONG_FROM_SIGNED(o) PyLong_FromSsize_t((ssize_t) o)
#define PYBIND11_LONG_FROM_UNSIGNED(o) PyLong_FromSize_t((size_t) o)
#define PYBIND11_BYTES_NAME "bytes"
#define PYBIND11_STRING_NAME "str"
#define PYBIND11_SLICE_OBJECT PyObject
#define PYBIND11_FROM_STRING PyUnicode_FromString
#define PYBIND11_STR_TYPE ::pybind11::str
#define PYBIND11_BOOL_ATTR "__bool__"
#define PYBIND11_NB_BOOL(ptr) ((ptr)->nb_bool)
#define PYBIND11_BUILTINS_MODULE "builtins"


#define PYBIND11_PLUGIN_IMPL(name) \
    extern "C" PYBIND11_MAYBE_UNUSED PYBIND11_EXPORT PyObject *PyInit_##name(); \
    extern "C" PYBIND11_EXPORT PyObject *PyInit_##name()

#else
#define PYBIND11_INSTANCE_METHOD_NEW(ptr, class_) PyMethod_New(ptr, nullptr, class_)
#define PYBIND11_INSTANCE_METHOD_CHECK PyMethod_Check
#define PYBIND11_INSTANCE_METHOD_GET_FUNCTION PyMethod_GET_FUNCTION
#define PYBIND11_BYTES_CHECK PyString_Check
#define PYBIND11_BYTES_FROM_STRING PyString_FromString
#define PYBIND11_BYTES_FROM_STRING_AND_SIZE PyString_FromStringAndSize
#define PYBIND11_BYTES_AS_STRING_AND_SIZE PyString_AsStringAndSize
#define PYBIND11_BYTES_AS_STRING PyString_AsString
#define PYBIND11_BYTES_SIZE PyString_Size
#define PYBIND11_LONG_CHECK(o) (PyInt_Check(o) || PyLong_Check(o))
#define PYBIND11_LONG_AS_LONGLONG(o) (PyInt_Check(o) ? (long long) PyLong_AsLong(o) : PyLong_AsLongLong(o))
#define PYBIND11_LONG_FROM_SIGNED(o) PyInt_FromSsize_t((ssize_t) o)
#define PYBIND11_LONG_FROM_UNSIGNED(o) PyInt_FromSize_t((size_t) o)
#define PYBIND11_BYTES_NAME "str"
#define PYBIND11_STRING_NAME "unicode"
#define PYBIND11_SLICE_OBJECT PySliceObject
#define PYBIND11_FROM_STRING PyString_FromString
#define PYBIND11_STR_TYPE ::pybind11::bytes
#define PYBIND11_BOOL_ATTR "__nonzero__"
#define PYBIND11_NB_BOOL(ptr) ((ptr)->nb_nonzero)
#define PYBIND11_BUILTINS_MODULE "__builtin__"


#define PYBIND11_PLUGIN_IMPL(name) \
    static PyObject *pybind11_init_wrapper();                           \
    extern "C" PYBIND11_MAYBE_UNUSED PYBIND11_EXPORT void init##name(); \
    extern "C" PYBIND11_EXPORT void init##name() {                      \
        (void)pybind11_init_wrapper();                                  \
    }                                                                   \
    PyObject *pybind11_init_wrapper()
#endif

#if PY_VERSION_HEX >= 0x03050000 && PY_VERSION_HEX < 0x03050200
extern "C" {
    struct _Py_atomic_address { void *value; };
    PyAPI_DATA(_Py_atomic_address) _PyThreadState_Current;
}
#endif

#define PYBIND11_TRY_NEXT_OVERLOAD ((PyObject *) 1)
#define PYBIND11_STRINGIFY(x) #x
#define PYBIND11_TOSTRING(x) PYBIND11_STRINGIFY(x)
#define PYBIND11_CONCAT(first, second) first##second
#define PYBIND11_ENSURE_INTERNALS_READY \
    pybind11::detail::get_internals();

#define PYBIND11_CHECK_PYTHON_VERSION \
    {                                                                          \
        const char *compiled_ver = PYBIND11_TOSTRING(PY_MAJOR_VERSION)         \
            "." PYBIND11_TOSTRING(PY_MINOR_VERSION);                           \
        const char *runtime_ver = Py_GetVersion();                             \
        size_t len = std::strlen(compiled_ver);                                \
        if (std::strncmp(runtime_ver, compiled_ver, len) != 0                  \
                || (runtime_ver[len] >= '0' && runtime_ver[len] <= '9')) {     \
            PyErr_Format(PyExc_ImportError,                                    \
                "Python version mismatch: module was compiled for Python %s, " \
                "but the interpreter version is incompatible: %s.",            \
                compiled_ver, runtime_ver);                                    \
            return nullptr;                                                    \
        }                                                                      \
    }

#define PYBIND11_CATCH_INIT_EXCEPTIONS \
        catch (pybind11::error_already_set &e) {                               \
            PyErr_SetString(PyExc_ImportError, e.what());                      \
            return nullptr;                                                    \
        } catch (const std::exception &e) {                                    \
            PyErr_SetString(PyExc_ImportError, e.what());                      \
            return nullptr;                                                    \
        }                                                                      \


#define PYBIND11_PLUGIN(name)                                                  \
    PYBIND11_DEPRECATED("PYBIND11_PLUGIN is deprecated, use PYBIND11_MODULE")  \
    static PyObject *pybind11_init();                                          \
    PYBIND11_PLUGIN_IMPL(name) {                                               \
        PYBIND11_CHECK_PYTHON_VERSION                                          \
        PYBIND11_ENSURE_INTERNALS_READY                                        \
        try {                                                                  \
            return pybind11_init();                                            \
        } PYBIND11_CATCH_INIT_EXCEPTIONS                                       \
    }                                                                          \
    PyObject *pybind11_init()


#define PYBIND11_MODULE(name, variable)                                        \
    static ::pybind11::module_::module_def                                     \
        PYBIND11_CONCAT(pybind11_module_def_, name) PYBIND11_MAYBE_UNUSED;     \
    PYBIND11_MAYBE_UNUSED                                                      \
    static void PYBIND11_CONCAT(pybind11_init_, name)(::pybind11::module_ &);  \
    PYBIND11_PLUGIN_IMPL(name) {                                               \
        PYBIND11_CHECK_PYTHON_VERSION                                          \
        PYBIND11_ENSURE_INTERNALS_READY                                        \
        auto m = ::pybind11::module_::create_extension_module(                 \
            PYBIND11_TOSTRING(name), nullptr,                                  \
            &PYBIND11_CONCAT(pybind11_module_def_, name));                     \
        try {                                                                  \
            PYBIND11_CONCAT(pybind11_init_, name)(m);                          \
            return m.ptr();                                                    \
        } PYBIND11_CATCH_INIT_EXCEPTIONS                                       \
    }                                                                          \
    void PYBIND11_CONCAT(pybind11_init_, name)(::pybind11::module_ &variable)


PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)

using ssize_t = Py_ssize_t;
using size_t  = std::size_t;


enum class return_value_policy : uint8_t {

    automatic = 0,


    automatic_reference,


    take_ownership,


    copy,


    move,


    reference,


    reference_internal
};

PYBIND11_NAMESPACE_BEGIN(detail)

inline static constexpr int log2(size_t n, int k = 0) { return (n <= 1) ? k : log2(n >> 1, k + 1); }


inline static constexpr size_t size_in_ptrs(size_t s) { return 1 + ((s - 1) >> log2(sizeof(void *))); }


constexpr size_t instance_simple_holder_in_ptrs() {
    static_assert(sizeof(std::shared_ptr<int>) >= sizeof(std::unique_ptr<int>),
            "pybind assumes std::shared_ptrs are at least as big as std::unique_ptrs");
    return size_in_ptrs(sizeof(std::shared_ptr<int>));
}


struct type_info;
struct value_and_holder;

struct nonsimple_values_and_holders {
    void **values_and_holders;
    uint8_t *status;
};


struct instance {
    PyObject_HEAD

    union {
        void *simple_value_holder[1 + instance_simple_holder_in_ptrs()];
        nonsimple_values_and_holders nonsimple;
    };

    PyObject *weakrefs;

    bool owned : 1;

    bool simple_layout : 1;

    bool simple_holder_constructed : 1;

    bool simple_instance_registered : 1;

    bool has_patients : 1;


    void allocate_layout();


    void deallocate_layout();




    value_and_holder get_value_and_holder(const type_info *find_type = nullptr, bool throw_if_missing = true);


    static constexpr uint8_t status_holder_constructed  = 1;
    static constexpr uint8_t status_instance_registered = 2;
};

static_assert(std::is_standard_layout<instance>::value, "Internal error: `pybind11::detail::instance` is not standard layout!");


#if defined(PYBIND11_CPP14) && (!defined(_MSC_VER) || _MSC_VER >= 1910)
using std::enable_if_t;
using std::conditional_t;
using std::remove_cv_t;
using std::remove_reference_t;
#else
template <bool B, typename T = void> using enable_if_t = typename std::enable_if<B, T>::type;
template <bool B, typename T, typename F> using conditional_t = typename std::conditional<B, T, F>::type;
template <typename T> using remove_cv_t = typename std::remove_cv<T>::type;
template <typename T> using remove_reference_t = typename std::remove_reference<T>::type;
#endif


#if defined(PYBIND11_CPP14)
using std::index_sequence;
using std::make_index_sequence;
#else
template<size_t ...> struct index_sequence  { };
template<size_t N, size_t ...S> struct make_index_sequence_impl : make_index_sequence_impl <N - 1, N - 1, S...> { };
template<size_t ...S> struct make_index_sequence_impl <0, S...> { using type = index_sequence<S...>; };
template<size_t N> using make_index_sequence = typename make_index_sequence_impl<N>::type;
#endif


template <typename ISeq, size_t, bool...> struct select_indices_impl { using type = ISeq; };
template <size_t... IPrev, size_t I, bool B, bool... Bs> struct select_indices_impl<index_sequence<IPrev...>, I, B, Bs...>
    : select_indices_impl<conditional_t<B, index_sequence<IPrev..., I>, index_sequence<IPrev...>>, I + 1, Bs...> {};
template <bool... Bs> using select_indices = typename select_indices_impl<index_sequence<>, 0, Bs...>::type;


template <bool B> using bool_constant = std::integral_constant<bool, B>;
template <typename T> struct negation : bool_constant<!T::value> { };




#if defined(__PGIC__) || defined(__INTEL_COMPILER)
template<typename... > using void_t = void;
#else
template <typename...> struct void_t_impl { using type = void; };
template <typename... Ts> using void_t = typename void_t_impl<Ts...>::type;
#endif



#if defined(__cpp_fold_expressions) && !(defined(_MSC_VER) && (_MSC_VER < 1916))
template <class... Ts> using all_of = bool_constant<(Ts::value && ...)>;
template <class... Ts> using any_of = bool_constant<(Ts::value || ...)>;
#elif !defined(_MSC_VER)
template <bool...> struct bools {};
template <class... Ts> using all_of = std::is_same<
    bools<Ts::value..., true>,
    bools<true, Ts::value...>>;
template <class... Ts> using any_of = negation<all_of<negation<Ts>...>>;
#else


template <class... Ts> using all_of = std::conjunction<Ts...>;
template <class... Ts> using any_of = std::disjunction<Ts...>;
#endif
template <class... Ts> using none_of = negation<any_of<Ts...>>;

template <class T, template<class> class... Predicates> using satisfies_all_of = all_of<Predicates<T>...>;
template <class T, template<class> class... Predicates> using satisfies_any_of = any_of<Predicates<T>...>;
template <class T, template<class> class... Predicates> using satisfies_none_of = none_of<Predicates<T>...>;


template <typename T> struct remove_class { };
template <typename C, typename R, typename... A> struct remove_class<R (C::*)(A...)> { using type = R (A...); };
template <typename C, typename R, typename... A> struct remove_class<R (C::*)(A...) const> { using type = R (A...); };


template <typename T> struct intrinsic_type                       { using type = T; };
template <typename T> struct intrinsic_type<const T>              { using type = typename intrinsic_type<T>::type; };
template <typename T> struct intrinsic_type<T*>                   { using type = typename intrinsic_type<T>::type; };
template <typename T> struct intrinsic_type<T&>                   { using type = typename intrinsic_type<T>::type; };
template <typename T> struct intrinsic_type<T&&>                  { using type = typename intrinsic_type<T>::type; };
template <typename T, size_t N> struct intrinsic_type<const T[N]> { using type = typename intrinsic_type<T>::type; };
template <typename T, size_t N> struct intrinsic_type<T[N]>       { using type = typename intrinsic_type<T>::type; };
template <typename T> using intrinsic_t = typename intrinsic_type<T>::type;


struct void_type { };


template <typename...> struct type_list { };


#ifdef __cpp_fold_expressions
template <typename... Ts> constexpr size_t constexpr_sum(Ts... ns) { return (0 + ... + size_t{ns}); }
#else
constexpr size_t constexpr_sum() { return 0; }
template <typename T, typename... Ts>
constexpr size_t constexpr_sum(T n, Ts... ns) { return size_t{n} + constexpr_sum(ns...); }
#endif

PYBIND11_NAMESPACE_BEGIN(constexpr_impl)

constexpr int first(int i) { return i; }
template <typename T, typename... Ts>
constexpr int first(int i, T v, Ts... vs) { return v ? i : first(i + 1, vs...); }

constexpr int last(int  , int result) { return result; }
template <typename T, typename... Ts>
constexpr int last(int i, int result, T v, Ts... vs) { return last(i + 1, v ? i : result, vs...); }
PYBIND11_NAMESPACE_END(constexpr_impl)



template <template<typename> class Predicate, typename... Ts>
constexpr int constexpr_first() { return constexpr_impl::first(0, Predicate<Ts>::value...); }


template <template<typename> class Predicate, typename... Ts>
constexpr int constexpr_last() { return constexpr_impl::last(0, -1, Predicate<Ts>::value...); }


template <size_t N, typename T, typename... Ts>
struct pack_element { using type = typename pack_element<N - 1, Ts...>::type; };
template <typename T, typename... Ts>
struct pack_element<0, T, Ts...> { using type = T; };



template <template<typename> class Predicate, typename Default, typename... Ts>
struct exactly_one {
    static constexpr auto found = constexpr_sum(Predicate<Ts>::value...);
    static_assert(found <= 1, "Found more than one type matching the predicate");

    static constexpr auto index = found ? constexpr_first<Predicate, Ts...>() : 0;
    using type = conditional_t<found, typename pack_element<index, Ts...>::type, Default>;
};
template <template<typename> class P, typename Default>
struct exactly_one<P, Default> { using type = Default; };

template <template<typename> class Predicate, typename Default, typename... Ts>
using exactly_one_t = typename exactly_one<Predicate, Default, Ts...>::type;


template <typename T, typename...  > struct deferred_type { using type = T; };
template <typename T, typename... Us> using deferred_t = typename deferred_type<T, Us...>::type;



template <typename Base, typename Derived> using is_strict_base_of = bool_constant<
    std::is_base_of<Base, Derived>::value && !std::is_same<Base, Derived>::value>;




template <typename Base, typename Derived> using is_accessible_base_of = bool_constant<
    (std::is_same<Base, Derived>::value || std::is_base_of<Base, Derived>::value) && std::is_convertible<Derived *, Base *>::value>;

template <template<typename...> class Base>
struct is_template_base_of_impl {
    template <typename... Us> static std::true_type check(Base<Us...> *);
    static std::false_type check(...);
};



template <template<typename...> class Base, typename T>
#if !defined(_MSC_VER)
using is_template_base_of = decltype(is_template_base_of_impl<Base>::check((intrinsic_t<T>*)nullptr));
#else
struct is_template_base_of : decltype(is_template_base_of_impl<Base>::check((intrinsic_t<T>*)nullptr)) { };
#endif



template <template<typename...> class Class, typename T>
struct is_instantiation : std::false_type { };
template <template<typename...> class Class, typename... Us>
struct is_instantiation<Class, Class<Us...>> : std::true_type { };


template <typename T> using is_shared_ptr = is_instantiation<std::shared_ptr, T>;


template <typename T, typename = void> struct is_input_iterator : std::false_type {};
template <typename T>
struct is_input_iterator<T, void_t<decltype(*std::declval<T &>()), decltype(++std::declval<T &>())>>
    : std::true_type {};

template <typename T> using is_function_pointer = bool_constant<
    std::is_pointer<T>::value && std::is_function<typename std::remove_pointer<T>::type>::value>;

template <typename F> struct strip_function_object {
    using type = typename remove_class<decltype(&F::operator())>::type;
};


template <typename Function, typename F = remove_reference_t<Function>>
using function_signature_t = conditional_t<
    std::is_function<F>::value,
    F,
    typename conditional_t<
        std::is_pointer<F>::value || std::is_member_pointer<F>::value,
        std::remove_pointer<F>,
        strip_function_object<F>
    >::type
>;




template <typename T> using is_lambda = satisfies_none_of<remove_reference_t<T>,
        std::is_function, std::is_pointer, std::is_member_pointer>;


inline void ignore_unused(const int *) { }


#ifdef __cpp_fold_expressions
#define PYBIND11_EXPAND_SIDE_EFFECTS(PATTERN) (((PATTERN), void()), ...)
#else
using expand_side_effects = bool[];
#define PYBIND11_EXPAND_SIDE_EFFECTS(PATTERN) (void)pybind11::detail::expand_side_effects{ ((PATTERN), void(), false)..., false }
#endif

PYBIND11_NAMESPACE_END(detail)


class builtin_exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;

    virtual void set_error() const = 0;
};

#define PYBIND11_RUNTIME_EXCEPTION(name, type) \
    class name : public builtin_exception { public: \
        using builtin_exception::builtin_exception; \
        name() : name("") { } \
        void set_error() const override { PyErr_SetString(type, what()); } \
    };

PYBIND11_RUNTIME_EXCEPTION(stop_iteration, PyExc_StopIteration)
PYBIND11_RUNTIME_EXCEPTION(index_error, PyExc_IndexError)
PYBIND11_RUNTIME_EXCEPTION(key_error, PyExc_KeyError)
PYBIND11_RUNTIME_EXCEPTION(value_error, PyExc_ValueError)
PYBIND11_RUNTIME_EXCEPTION(type_error, PyExc_TypeError)
PYBIND11_RUNTIME_EXCEPTION(buffer_error, PyExc_BufferError)
PYBIND11_RUNTIME_EXCEPTION(import_error, PyExc_ImportError)
PYBIND11_RUNTIME_EXCEPTION(cast_error, PyExc_RuntimeError)
PYBIND11_RUNTIME_EXCEPTION(reference_cast_error, PyExc_RuntimeError)

[[noreturn]] PYBIND11_NOINLINE inline void pybind11_fail(const char *reason) { throw std::runtime_error(reason); }
[[noreturn]] PYBIND11_NOINLINE inline void pybind11_fail(const std::string &reason) { throw std::runtime_error(reason); }

template <typename T, typename SFINAE = void> struct format_descriptor { };

PYBIND11_NAMESPACE_BEGIN(detail)





template <typename T, typename SFINAE = void> struct is_fmt_numeric { static constexpr bool value = false; };
template <typename T> struct is_fmt_numeric<T, enable_if_t<std::is_arithmetic<T>::value>> {
    static constexpr bool value = true;
    static constexpr int index = std::is_same<T, bool>::value ? 0 : 1 + (
        std::is_integral<T>::value ? detail::log2(sizeof(T))*2 + std::is_unsigned<T>::value : 8 + (
        std::is_same<T, double>::value ? 1 : std::is_same<T, long double>::value ? 2 : 0));
};
PYBIND11_NAMESPACE_END(detail)

template <typename T> struct format_descriptor<T, detail::enable_if_t<std::is_arithmetic<T>::value>> {
    static constexpr const char c = "?bBhHiIqQfdg"[detail::is_fmt_numeric<T>::index];
    static constexpr const char value[2] = { c, '\0' };
    static std::string format() { return std::string(1, c); }
};

#if !defined(PYBIND11_CPP17)

template <typename T> constexpr const char format_descriptor<
    T, detail::enable_if_t<std::is_arithmetic<T>::value>>::value[2];

#endif


struct error_scope {
    PyObject *type, *value, *trace;
    error_scope() { PyErr_Fetch(&type, &value, &trace); }
    ~error_scope() { PyErr_Restore(type, value, trace); }
};


struct nodelete { template <typename T> void operator()(T*) { } };

PYBIND11_NAMESPACE_BEGIN(detail)
template <typename... Args>
struct overload_cast_impl {
    constexpr overload_cast_impl() {};

    template <typename Return>
    constexpr auto operator()(Return (*pf)(Args...)) const noexcept
                              -> decltype(pf) { return pf; }

    template <typename Return, typename Class>
    constexpr auto operator()(Return (Class::*pmf)(Args...), std::false_type = {}) const noexcept
                              -> decltype(pmf) { return pmf; }

    template <typename Return, typename Class>
    constexpr auto operator()(Return (Class::*pmf)(Args...) const, std::true_type) const noexcept
                              -> decltype(pmf) { return pmf; }
};
PYBIND11_NAMESPACE_END(detail)


#if defined(PYBIND11_CPP14)
#define PYBIND11_OVERLOAD_CAST 1



template <typename... Args>
static constexpr detail::overload_cast_impl<Args...> overload_cast = {};

#endif




static constexpr auto const_ = std::true_type{};

#if !defined(PYBIND11_CPP14)
template <typename... Args> struct overload_cast {
    static_assert(detail::deferred_t<std::false_type, Args...>::value,
                  "pybind11::overload_cast<...> requires compiling in C++14 mode");
};
#endif

PYBIND11_NAMESPACE_BEGIN(detail)




template <typename T>
class any_container {
    std::vector<T> v;
public:
    any_container() = default;


    template <typename It, typename = enable_if_t<is_input_iterator<It>::value>>
    any_container(It first, It last) : v(first, last) { }


    template <typename Container, typename = enable_if_t<std::is_convertible<decltype(*std::begin(std::declval<const Container &>())), T>::value>>
    any_container(const Container &c) : any_container(std::begin(c), std::end(c)) { }



    template <typename TIn, typename = enable_if_t<std::is_convertible<TIn, T>::value>>
    any_container(const std::initializer_list<TIn> &c) : any_container(c.begin(), c.end()) { }


    any_container(std::vector<T> &&v) : v(std::move(v)) { }


    operator std::vector<T> &&() && { return std::move(v); }


    std::vector<T> &operator*() { return v; }
    const std::vector<T> &operator*() const { return v; }


    std::vector<T> *operator->() { return &v; }
    const std::vector<T> *operator->() const { return &v; }
};


std::string get_fully_qualified_tp_name(PyTypeObject*);

PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
