/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "pybind11.h"
#include "complex.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <numeric>
#include <sstream>
#include <string>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>


static_assert(sizeof(::pybind11::ssize_t) == sizeof(Py_intptr_t), "ssize_t != Py_intptr_t");
static_assert(std::is_signed<Py_intptr_t>::value, "Py_intptr_t must be signed");


PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)

class array;

PYBIND11_NAMESPACE_BEGIN(detail)

template <>
struct handle_type_name<array> {
    static constexpr auto name = const_name("numpy.ndarray");
};

template <typename type, typename SFINAE = void>
struct npy_format_descriptor;

struct PyArrayDescr_Proxy {
    PyObject_HEAD
    PyObject *typeobj;
    char kind;
    char type;
    char byteorder;
    char flags;
    int type_num;
    int elsize;
    int alignment;
    char *subarray;
    PyObject *fields;
    PyObject *names;
};

struct PyArray_Proxy {
    PyObject_HEAD
    char *data;
    int nd;
    ssize_t *dimensions;
    ssize_t *strides;
    PyObject *base;
    PyObject *descr;
    int flags;
};

struct PyVoidScalarObject_Proxy {
    PyObject_VAR_HEAD char *obval;
    PyArrayDescr_Proxy *descr;
    int flags;
    PyObject *base;
};

struct numpy_type_info {
    PyObject *dtype_ptr;
    std::string format_str;
};

struct numpy_internals {
    std::unordered_map<std::type_index, numpy_type_info> registered_dtypes;

    numpy_type_info *get_type_info(const std::type_info &tinfo, bool throw_if_missing = true) {
        auto it = registered_dtypes.find(std::type_index(tinfo));
        if (it != registered_dtypes.end()) {
            return &(it->second);
        }
        if (throw_if_missing) {
            pybind11_fail(std::string("NumPy type info missing for ") + tinfo.name());
        }
        return nullptr;
    }

    template <typename T>
    numpy_type_info *get_type_info(bool throw_if_missing = true) {
        return get_type_info(typeid(typename std::remove_cv<T>::type), throw_if_missing);
    }
};

PYBIND11_NOINLINE void load_numpy_internals(numpy_internals *&ptr) {
    ptr = &get_or_create_shared_data<numpy_internals>("_numpy_internals");
}

inline numpy_internals &get_numpy_internals() {
    static numpy_internals *ptr = nullptr;
    if (!ptr) {
        load_numpy_internals(ptr);
    }
    return *ptr;
}

template <typename T>
struct same_size {
    template <typename U>
    using as = bool_constant<sizeof(T) == sizeof(U)>;
};

template <typename Concrete>
constexpr int platform_lookup() {
    return -1;
}


template <typename Concrete, typename T, typename... Ts, typename... Ints>
constexpr int platform_lookup(int I, Ints... Is) {
    return sizeof(Concrete) == sizeof(T) ? I : platform_lookup<Concrete, Ts...>(Is...);
}

struct npy_api {
    enum constants {
        NPY_ARRAY_C_CONTIGUOUS_ = 0x0001,
        NPY_ARRAY_F_CONTIGUOUS_ = 0x0002,
        NPY_ARRAY_OWNDATA_ = 0x0004,
        NPY_ARRAY_FORCECAST_ = 0x0010,
        NPY_ARRAY_ENSUREARRAY_ = 0x0040,
        NPY_ARRAY_ALIGNED_ = 0x0100,
        NPY_ARRAY_WRITEABLE_ = 0x0400,
        NPY_BOOL_ = 0,
        NPY_BYTE_,
        NPY_UBYTE_,
        NPY_SHORT_,
        NPY_USHORT_,
        NPY_INT_,
        NPY_UINT_,
        NPY_LONG_,
        NPY_ULONG_,
        NPY_LONGLONG_,
        NPY_ULONGLONG_,
        NPY_FLOAT_,
        NPY_DOUBLE_,
        NPY_LONGDOUBLE_,
        NPY_CFLOAT_,
        NPY_CDOUBLE_,
        NPY_CLONGDOUBLE_,
        NPY_OBJECT_ = 17,
        NPY_STRING_,
        NPY_UNICODE_,
        NPY_VOID_,

        NPY_INT8_ = NPY_BYTE_,
        NPY_UINT8_ = NPY_UBYTE_,
        NPY_INT16_ = NPY_SHORT_,
        NPY_UINT16_ = NPY_USHORT_,



        NPY_INT32_
        = platform_lookup<std::int32_t, long, int, short>(NPY_LONG_, NPY_INT_, NPY_SHORT_),
        NPY_UINT32_ = platform_lookup<std::uint32_t, unsigned long, unsigned int, unsigned short>(
            NPY_ULONG_, NPY_UINT_, NPY_USHORT_),
        NPY_INT64_
        = platform_lookup<std::int64_t, long, long long, int>(NPY_LONG_, NPY_LONGLONG_, NPY_INT_),
        NPY_UINT64_
        = platform_lookup<std::uint64_t, unsigned long, unsigned long long, unsigned int>(
            NPY_ULONG_, NPY_ULONGLONG_, NPY_UINT_),
    };

    struct PyArray_Dims {
        Py_intptr_t *ptr;
        int len;
    };

    static npy_api &get() {
        static npy_api api = lookup();
        return api;
    }

    bool PyArray_Check_(PyObject *obj) const {
        return PyObject_TypeCheck(obj, PyArray_Type_) != 0;
    }
    bool PyArrayDescr_Check_(PyObject *obj) const {
        return PyObject_TypeCheck(obj, PyArrayDescr_Type_) != 0;
    }

    unsigned int (*PyArray_GetNDArrayCFeatureVersion_)();
    PyObject *(*PyArray_DescrFromType_)(int);
    PyObject *(*PyArray_NewFromDescr_)(PyTypeObject *,
                                       PyObject *,
                                       int,
                                       Py_intptr_t const *,
                                       Py_intptr_t const *,
                                       void *,
                                       int,
                                       PyObject *);

    PyObject *(*PyArray_DescrNewFromType_)(int);
    int (*PyArray_CopyInto_)(PyObject *, PyObject *);
    PyObject *(*PyArray_NewCopy_)(PyObject *, int);
    PyTypeObject *PyArray_Type_;
    PyTypeObject *PyVoidArrType_Type_;
    PyTypeObject *PyArrayDescr_Type_;
    PyObject *(*PyArray_DescrFromScalar_)(PyObject *);
    PyObject *(*PyArray_FromAny_)(PyObject *, PyObject *, int, int, int, PyObject *);
    int (*PyArray_DescrConverter_)(PyObject *, PyObject **);
    bool (*PyArray_EquivTypes_)(PyObject *, PyObject *);
    int (*PyArray_GetArrayParamsFromObject_)(PyObject *,
                                             PyObject *,
                                             unsigned char,
                                             PyObject **,
                                             int *,
                                             Py_intptr_t *,
                                             PyObject **,
                                             PyObject *);
    PyObject *(*PyArray_Squeeze_)(PyObject *);

    int (*PyArray_SetBaseObject_)(PyObject *, PyObject *);
    PyObject *(*PyArray_Resize_)(PyObject *, PyArray_Dims *, int, int);
    PyObject *(*PyArray_Newshape_)(PyObject *, PyArray_Dims *, int);
    PyObject *(*PyArray_View_)(PyObject *, PyObject *, PyObject *);

private:
    enum functions {
        API_PyArray_GetNDArrayCFeatureVersion = 211,
        API_PyArray_Type = 2,
        API_PyArrayDescr_Type = 3,
        API_PyVoidArrType_Type = 39,
        API_PyArray_DescrFromType = 45,
        API_PyArray_DescrFromScalar = 57,
        API_PyArray_FromAny = 69,
        API_PyArray_Resize = 80,
        API_PyArray_CopyInto = 82,
        API_PyArray_NewCopy = 85,
        API_PyArray_NewFromDescr = 94,
        API_PyArray_DescrNewFromType = 96,
        API_PyArray_Newshape = 135,
        API_PyArray_Squeeze = 136,
        API_PyArray_View = 137,
        API_PyArray_DescrConverter = 174,
        API_PyArray_EquivTypes = 182,
        API_PyArray_GetArrayParamsFromObject = 278,
        API_PyArray_SetBaseObject = 282
    };

    static npy_api lookup() {
        module_ m = module_::import("numpy.core.multiarray");
        auto c = m.attr("_ARRAY_API");
        void **api_ptr = (void **) PyCapsule_GetPointer(c.ptr(), nullptr);
        npy_api api;
#define DECL_NPY_API(Func) api.Func##_ = (decltype(api.Func##_)) api_ptr[API_##Func];
        DECL_NPY_API(PyArray_GetNDArrayCFeatureVersion);
        if (api.PyArray_GetNDArrayCFeatureVersion_() < 0x7) {
            pybind11_fail("pybind11 numpy support requires numpy >= 1.7.0");
        }
        DECL_NPY_API(PyArray_Type);
        DECL_NPY_API(PyVoidArrType_Type);
        DECL_NPY_API(PyArrayDescr_Type);
        DECL_NPY_API(PyArray_DescrFromType);
        DECL_NPY_API(PyArray_DescrFromScalar);
        DECL_NPY_API(PyArray_FromAny);
        DECL_NPY_API(PyArray_Resize);
        DECL_NPY_API(PyArray_CopyInto);
        DECL_NPY_API(PyArray_NewCopy);
        DECL_NPY_API(PyArray_NewFromDescr);
        DECL_NPY_API(PyArray_DescrNewFromType);
        DECL_NPY_API(PyArray_Newshape);
        DECL_NPY_API(PyArray_Squeeze);
        DECL_NPY_API(PyArray_View);
        DECL_NPY_API(PyArray_DescrConverter);
        DECL_NPY_API(PyArray_EquivTypes);
        DECL_NPY_API(PyArray_GetArrayParamsFromObject);
        DECL_NPY_API(PyArray_SetBaseObject);

#undef DECL_NPY_API
        return api;
    }
};

inline PyArray_Proxy *array_proxy(void *ptr) { return reinterpret_cast<PyArray_Proxy *>(ptr); }

inline const PyArray_Proxy *array_proxy(const void *ptr) {
    return reinterpret_cast<const PyArray_Proxy *>(ptr);
}

inline PyArrayDescr_Proxy *array_descriptor_proxy(PyObject *ptr) {
    return reinterpret_cast<PyArrayDescr_Proxy *>(ptr);
}

inline const PyArrayDescr_Proxy *array_descriptor_proxy(const PyObject *ptr) {
    return reinterpret_cast<const PyArrayDescr_Proxy *>(ptr);
}

inline bool check_flags(const void *ptr, int flag) {
    return (flag == (array_proxy(ptr)->flags & flag));
}

template <typename T>
struct is_std_array : std::false_type {};
template <typename T, size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};
template <typename T>
struct is_complex : std::false_type {};
template <typename T>
struct is_complex<std::complex<T>> : std::true_type {};

template <typename T>
struct array_info_scalar {
    using type = T;
    static constexpr bool is_array = false;
    static constexpr bool is_empty = false;
    static constexpr auto extents = const_name("");
    static void append_extents(list &  ) {}
};



template <typename T>
struct array_info : array_info_scalar<T> {};
template <typename T, size_t N>
struct array_info<std::array<T, N>> {
    using type = typename array_info<T>::type;
    static constexpr bool is_array = true;
    static constexpr bool is_empty = (N == 0) || array_info<T>::is_empty;
    static constexpr size_t extent = N;


    static void append_extents(list &shape) {
        shape.append(N);
        array_info<T>::append_extents(shape);
    }

    static constexpr auto extents = const_name<array_info<T>::is_array>(
        concat(const_name<N>(), array_info<T>::extents), const_name<N>());
};


template <size_t N>
struct array_info<char[N]> : array_info_scalar<char[N]> {};
template <size_t N>
struct array_info<std::array<char, N>> : array_info_scalar<std::array<char, N>> {};
template <typename T, size_t N>
struct array_info<T[N]> : array_info<std::array<T, N>> {};
template <typename T>
using remove_all_extents_t = typename array_info<T>::type;

template <typename T>
using is_pod_struct
    = all_of<std::is_standard_layout<T>,

#if defined(__GLIBCXX__)                                                                          \
    && (__GLIBCXX__ < 20150422 || __GLIBCXX__ == 20150426 || __GLIBCXX__ == 20150623              \
        || __GLIBCXX__ == 20150626 || __GLIBCXX__ == 20160803)


             std::is_trivially_destructible<T>,
             satisfies_any_of<T, std::has_trivial_copy_constructor, std::has_trivial_copy_assign>,
#else
             std::is_trivially_copyable<T>,
#endif
             satisfies_none_of<T,
                               std::is_reference,
                               std::is_array,
                               is_std_array,
                               std::is_arithmetic,
                               is_complex,
                               std::is_enum>>;


template <typename T>
using is_pod = all_of<std::is_standard_layout<T>, std::is_trivial<T>>;

template <ssize_t Dim = 0, typename Strides>
ssize_t byte_offset_unsafe(const Strides &) {
    return 0;
}
template <ssize_t Dim = 0, typename Strides, typename... Ix>
ssize_t byte_offset_unsafe(const Strides &strides, ssize_t i, Ix... index) {
    return i * strides[Dim] + byte_offset_unsafe<Dim + 1>(strides, index...);
}


template <typename T, ssize_t Dims>
class unchecked_reference {
protected:
    static constexpr bool Dynamic = Dims < 0;
    const unsigned char *data_;


    conditional_t<Dynamic, const ssize_t *, std::array<ssize_t, (size_t) Dims>> shape_, strides_;
    const ssize_t dims_;

    friend class pybind11::array;

    template <bool Dyn = Dynamic>
    unchecked_reference(const void *data,
                        const ssize_t *shape,
                        const ssize_t *strides,
                        enable_if_t<!Dyn, ssize_t>)
        : data_{reinterpret_cast<const unsigned char *>(data)}, dims_{Dims} {
        for (size_t i = 0; i < (size_t) dims_; i++) {
            shape_[i] = shape[i];
            strides_[i] = strides[i];
        }
    }

    template <bool Dyn = Dynamic>
    unchecked_reference(const void *data,
                        const ssize_t *shape,
                        const ssize_t *strides,
                        enable_if_t<Dyn, ssize_t> dims)
        : data_{reinterpret_cast<const unsigned char *>(data)}, shape_{shape}, strides_{strides},
          dims_{dims} {}

public:

    template <typename... Ix>
    const T &operator()(Ix... index) const {
        static_assert(ssize_t{sizeof...(Ix)} == Dims || Dynamic,
                      "Invalid number of indices for unchecked array reference");
        return *reinterpret_cast<const T *>(data_
                                            + byte_offset_unsafe(strides_, ssize_t(index)...));
    }

    template <ssize_t D = Dims, typename = enable_if_t<D == 1 || Dynamic>>
    const T &operator[](ssize_t index) const {
        return operator()(index);
    }


    template <typename... Ix>
    const T *data(Ix... ix) const {
        return &operator()(ssize_t(ix)...);
    }


    constexpr static ssize_t itemsize() { return sizeof(T); }


    ssize_t shape(ssize_t dim) const { return shape_[(size_t) dim]; }


    ssize_t ndim() const { return dims_; }



    template <bool Dyn = Dynamic>
    enable_if_t<!Dyn, ssize_t> size() const {
        return std::accumulate(
            shape_.begin(), shape_.end(), (ssize_t) 1, std::multiplies<ssize_t>());
    }
    template <bool Dyn = Dynamic>
    enable_if_t<Dyn, ssize_t> size() const {
        return std::accumulate(shape_, shape_ + ndim(), (ssize_t) 1, std::multiplies<ssize_t>());
    }




    ssize_t nbytes() const { return size() * itemsize(); }
};

template <typename T, ssize_t Dims>
class unchecked_mutable_reference : public unchecked_reference<T, Dims> {
    friend class pybind11::array;
    using ConstBase = unchecked_reference<T, Dims>;
    using ConstBase::ConstBase;
    using ConstBase::Dynamic;

public:

    using ConstBase::operator();
    using ConstBase::operator[];


    template <typename... Ix>
    T &operator()(Ix... index) {
        static_assert(ssize_t{sizeof...(Ix)} == Dims || Dynamic,
                      "Invalid number of indices for unchecked array reference");
        return const_cast<T &>(ConstBase::operator()(index...));
    }

    template <ssize_t D = Dims, typename = enable_if_t<D == 1 || Dynamic>>
    T &operator[](ssize_t index) {
        return operator()(index);
    }


    template <typename... Ix>
    T *mutable_data(Ix... ix) {
        return &operator()(ssize_t(ix)...);
    }
};

template <typename T, ssize_t Dim>
struct type_caster<unchecked_reference<T, Dim>> {
    static_assert(Dim == 0 && Dim > 0  ,
                  "unchecked array proxy object is not castable");
};
template <typename T, ssize_t Dim>
struct type_caster<unchecked_mutable_reference<T, Dim>>
    : type_caster<unchecked_reference<T, Dim>> {};

PYBIND11_NAMESPACE_END(detail)

class dtype : public object {
public:
    PYBIND11_OBJECT_DEFAULT(dtype, object, detail::npy_api::get().PyArrayDescr_Check_);

    explicit dtype(const buffer_info &info) {
        dtype descr(_dtype_from_pep3118()(pybind11::str(info.format)));

        m_ptr = descr.strip_padding(info.itemsize != 0 ? info.itemsize : descr.itemsize())
                    .release()
                    .ptr();
    }

    explicit dtype(const pybind11::str &format) : dtype(from_args(format)) {}

    explicit dtype(const std::string &format) : dtype(pybind11::str(format)) {}

    explicit dtype(const char *format) : dtype(pybind11::str(format)) {}

    dtype(list names, list formats, list offsets, ssize_t itemsize) {
        dict args;
        args["names"] = std::move(names);
        args["formats"] = std::move(formats);
        args["offsets"] = std::move(offsets);
        args["itemsize"] = pybind11::int_(itemsize);
        m_ptr = from_args(args).release().ptr();
    }

    explicit dtype(int typenum)
        : object(detail::npy_api::get().PyArray_DescrFromType_(typenum), stolen_t{}) {
        if (m_ptr == nullptr) {
            throw error_already_set();
        }
    }


    static dtype from_args(const object &args) {
        PyObject *ptr = nullptr;
        if ((detail::npy_api::get().PyArray_DescrConverter_(args.ptr(), &ptr) == 0) || !ptr) {
            throw error_already_set();
        }
        return reinterpret_steal<dtype>(ptr);
    }


    template <typename T>
    static dtype of() {
        return detail::npy_format_descriptor<typename std::remove_cv<T>::type>::dtype();
    }


    ssize_t itemsize() const { return detail::array_descriptor_proxy(m_ptr)->elsize; }


    bool has_fields() const { return detail::array_descriptor_proxy(m_ptr)->names != nullptr; }



    char kind() const { return detail::array_descriptor_proxy(m_ptr)->kind; }



    char char_() const {



        return detail::array_descriptor_proxy(m_ptr)->type;
    }


    int num() const {



        return detail::array_descriptor_proxy(m_ptr)->type_num;
    }


    char byteorder() const { return detail::array_descriptor_proxy(m_ptr)->byteorder; }


    int alignment() const { return detail::array_descriptor_proxy(m_ptr)->alignment; }


    char flags() const { return detail::array_descriptor_proxy(m_ptr)->flags; }

private:
    static object _dtype_from_pep3118() {
        static PyObject *obj = module_::import("numpy.core._internal")
                                   .attr("_dtype_from_pep3118")
                                   .cast<object>()
                                   .release()
                                   .ptr();
        return reinterpret_borrow<object>(obj);
    }

    dtype strip_padding(ssize_t itemsize) {


        if (!has_fields()) {
            return *this;
        }

        struct field_descr {
            pybind11::str name;
            object format;
            pybind11::int_ offset;
            field_descr(pybind11::str &&name, object &&format, pybind11::int_ &&offset)
                : name{std::move(name)}, format{std::move(format)}, offset{std::move(offset)} {};
        };
        auto field_dict = attr("fields").cast<dict>();
        std::vector<field_descr> field_descriptors;
        field_descriptors.reserve(field_dict.size());

        for (auto field : field_dict.attr("items")()) {
            auto spec = field.cast<tuple>();
            auto name = spec[0].cast<pybind11::str>();
            auto spec_fo = spec[1].cast<tuple>();
            auto format = spec_fo[0].cast<dtype>();
            auto offset = spec_fo[1].cast<pybind11::int_>();
            if ((len(name) == 0u) && format.kind() == 'V') {
                continue;
            }
            field_descriptors.emplace_back(
                std::move(name), format.strip_padding(format.itemsize()), std::move(offset));
        }

        std::sort(field_descriptors.begin(),
                  field_descriptors.end(),
                  [](const field_descr &a, const field_descr &b) {
                      return a.offset.cast<int>() < b.offset.cast<int>();
                  });

        list names, formats, offsets;
        for (auto &descr : field_descriptors) {
            names.append(std::move(descr.name));
            formats.append(std::move(descr.format));
            offsets.append(std::move(descr.offset));
        }
        return dtype(std::move(names), std::move(formats), std::move(offsets), itemsize);
    }
};

class array : public buffer {
public:
    PYBIND11_OBJECT_CVT(array, buffer, detail::npy_api::get().PyArray_Check_, raw_array)

    enum {
        c_style = detail::npy_api::NPY_ARRAY_C_CONTIGUOUS_,
        f_style = detail::npy_api::NPY_ARRAY_F_CONTIGUOUS_,
        forcecast = detail::npy_api::NPY_ARRAY_FORCECAST_
    };

    array() : array(0, static_cast<const double *>(nullptr)) {}

    using ShapeContainer = detail::any_container<ssize_t>;
    using StridesContainer = detail::any_container<ssize_t>;


    array(const pybind11::dtype &dt,
          ShapeContainer shape,
          StridesContainer strides,
          const void *ptr = nullptr,
          handle base = handle()) {

        if (strides->empty()) {
            *strides = detail::c_strides(*shape, dt.itemsize());
        }

        auto ndim = shape->size();
        if (ndim != strides->size()) {
            pybind11_fail("NumPy: shape ndim doesn't match strides ndim");
        }
        auto descr = dt;

        int flags = 0;
        if (base && ptr) {
            if (isinstance<array>(base)) {

                flags = reinterpret_borrow<array>(base).flags()
                        & ~detail::npy_api::NPY_ARRAY_OWNDATA_;
            } else {

                flags = detail::npy_api::NPY_ARRAY_WRITEABLE_;
            }
        }

        auto &api = detail::npy_api::get();
        auto tmp = reinterpret_steal<object>(api.PyArray_NewFromDescr_(
            api.PyArray_Type_,
            descr.release().ptr(),
            (int) ndim,

            reinterpret_cast<Py_intptr_t *>(shape->data()),
            reinterpret_cast<Py_intptr_t *>(strides->data()),
            const_cast<void *>(ptr),
            flags,
            nullptr));
        if (!tmp) {
            throw error_already_set();
        }
        if (ptr) {
            if (base) {
                api.PyArray_SetBaseObject_(tmp.ptr(), base.inc_ref().ptr());
            } else {
                tmp = reinterpret_steal<object>(
                    api.PyArray_NewCopy_(tmp.ptr(), -1  ));
            }
        }
        m_ptr = tmp.release().ptr();
    }

    array(const pybind11::dtype &dt,
          ShapeContainer shape,
          const void *ptr = nullptr,
          handle base = handle())
        : array(dt, std::move(shape), {}, ptr, base) {}

    template <typename T,
              typename
              = detail::enable_if_t<std::is_integral<T>::value && !std::is_same<bool, T>::value>>
    array(const pybind11::dtype &dt, T count, const void *ptr = nullptr, handle base = handle())
        : array(dt, {{count}}, ptr, base) {}

    template <typename T>
    array(ShapeContainer shape, StridesContainer strides, const T *ptr, handle base = handle())
        : array(pybind11::dtype::of<T>(), std::move(shape), std::move(strides), ptr, base) {}

    template <typename T>
    array(ShapeContainer shape, const T *ptr, handle base = handle())
        : array(std::move(shape), {}, ptr, base) {}

    template <typename T>
    explicit array(ssize_t count, const T *ptr, handle base = handle())
        : array({count}, {}, ptr, base) {}

    explicit array(const buffer_info &info, handle base = handle())
        : array(pybind11::dtype(info), info.shape, info.strides, info.ptr, base) {}


    pybind11::dtype dtype() const {
        return reinterpret_borrow<pybind11::dtype>(detail::array_proxy(m_ptr)->descr);
    }


    ssize_t size() const {
        return std::accumulate(shape(), shape() + ndim(), (ssize_t) 1, std::multiplies<ssize_t>());
    }


    ssize_t itemsize() const {
        return detail::array_descriptor_proxy(detail::array_proxy(m_ptr)->descr)->elsize;
    }


    ssize_t nbytes() const { return size() * itemsize(); }


    ssize_t ndim() const { return detail::array_proxy(m_ptr)->nd; }


    object base() const { return reinterpret_borrow<object>(detail::array_proxy(m_ptr)->base); }


    const ssize_t *shape() const { return detail::array_proxy(m_ptr)->dimensions; }


    ssize_t shape(ssize_t dim) const {
        if (dim >= ndim()) {
            fail_dim_check(dim, "invalid axis");
        }
        return shape()[dim];
    }


    const ssize_t *strides() const { return detail::array_proxy(m_ptr)->strides; }


    ssize_t strides(ssize_t dim) const {
        if (dim >= ndim()) {
            fail_dim_check(dim, "invalid axis");
        }
        return strides()[dim];
    }


    int flags() const { return detail::array_proxy(m_ptr)->flags; }


    bool writeable() const {
        return detail::check_flags(m_ptr, detail::npy_api::NPY_ARRAY_WRITEABLE_);
    }


    bool owndata() const {
        return detail::check_flags(m_ptr, detail::npy_api::NPY_ARRAY_OWNDATA_);
    }



    template <typename... Ix>
    const void *data(Ix... index) const {
        return static_cast<const void *>(detail::array_proxy(m_ptr)->data + offset_at(index...));
    }




    template <typename... Ix>
    void *mutable_data(Ix... index) {
        check_writeable();
        return static_cast<void *>(detail::array_proxy(m_ptr)->data + offset_at(index...));
    }



    template <typename... Ix>
    ssize_t offset_at(Ix... index) const {
        if ((ssize_t) sizeof...(index) > ndim()) {
            fail_dim_check(sizeof...(index), "too many indices for an array");
        }
        return byte_offset(ssize_t(index)...);
    }

    ssize_t offset_at() const { return 0; }



    template <typename... Ix>
    ssize_t index_at(Ix... index) const {
        return offset_at(index...) / itemsize();
    }


    template <typename T, ssize_t Dims = -1>
    detail::unchecked_mutable_reference<T, Dims> mutable_unchecked() & {
        if (PYBIND11_SILENCE_MSVC_C4127(Dims >= 0) && ndim() != Dims) {
            throw std::domain_error("array has incorrect number of dimensions: "
                                    + std::to_string(ndim()) + "; expected "
                                    + std::to_string(Dims));
        }
        return detail::unchecked_mutable_reference<T, Dims>(
            mutable_data(), shape(), strides(), ndim());
    }


    template <typename T, ssize_t Dims = -1>
    detail::unchecked_reference<T, Dims> unchecked() const & {
        if (PYBIND11_SILENCE_MSVC_C4127(Dims >= 0) && ndim() != Dims) {
            throw std::domain_error("array has incorrect number of dimensions: "
                                    + std::to_string(ndim()) + "; expected "
                                    + std::to_string(Dims));
        }
        return detail::unchecked_reference<T, Dims>(data(), shape(), strides(), ndim());
    }


    array squeeze() {
        auto &api = detail::npy_api::get();
        return reinterpret_steal<array>(api.PyArray_Squeeze_(m_ptr));
    }




    void resize(ShapeContainer new_shape, bool refcheck = true) {
        detail::npy_api::PyArray_Dims d
            = {
               reinterpret_cast<Py_intptr_t *>(new_shape->data()),
               int(new_shape->size())};

        auto new_array = reinterpret_steal<object>(
            detail::npy_api::get().PyArray_Resize_(m_ptr, &d, int(refcheck), -1));
        if (!new_array) {
            throw error_already_set();
        }
        if (isinstance<array>(new_array)) {
            *this = std::move(new_array);
        }
    }


    array reshape(ShapeContainer new_shape) {
        detail::npy_api::PyArray_Dims d
            = {reinterpret_cast<Py_intptr_t *>(new_shape->data()), int(new_shape->size())};
        auto new_array
            = reinterpret_steal<array>(detail::npy_api::get().PyArray_Newshape_(m_ptr, &d, 0));
        if (!new_array) {
            throw error_already_set();
        }
        return new_array;
    }






    array view(const std::string &dtype) {
        auto &api = detail::npy_api::get();
        auto new_view = reinterpret_steal<array>(api.PyArray_View_(
            m_ptr, dtype::from_args(pybind11::str(dtype)).release().ptr(), nullptr));
        if (!new_view) {
            throw error_already_set();
        }
        return new_view;
    }



    static array ensure(handle h, int ExtraFlags = 0) {
        auto result = reinterpret_steal<array>(raw_array(h.ptr(), ExtraFlags));
        if (!result) {
            PyErr_Clear();
        }
        return result;
    }

protected:
    template <typename, typename>
    friend struct detail::npy_format_descriptor;

    void fail_dim_check(ssize_t dim, const std::string &msg) const {
        throw index_error(msg + ": " + std::to_string(dim) + " (ndim = " + std::to_string(ndim())
                          + ')');
    }

    template <typename... Ix>
    ssize_t byte_offset(Ix... index) const {
        check_dimensions(index...);
        return detail::byte_offset_unsafe(strides(), ssize_t(index)...);
    }

    void check_writeable() const {
        if (!writeable()) {
            throw std::domain_error("array is not writeable");
        }
    }

    template <typename... Ix>
    void check_dimensions(Ix... index) const {
        check_dimensions_impl(ssize_t(0), shape(), ssize_t(index)...);
    }

    void check_dimensions_impl(ssize_t, const ssize_t *) const {}

    template <typename... Ix>
    void check_dimensions_impl(ssize_t axis, const ssize_t *shape, ssize_t i, Ix... index) const {
        if (i >= *shape) {
            throw index_error(std::string("index ") + std::to_string(i)
                              + " is out of bounds for axis " + std::to_string(axis)
                              + " with size " + std::to_string(*shape));
        }
        check_dimensions_impl(axis + 1, shape + 1, index...);
    }


    static PyObject *raw_array(PyObject *ptr, int ExtraFlags = 0) {
        if (ptr == nullptr) {
            PyErr_SetString(PyExc_ValueError, "cannot create a pybind11::array from a nullptr");
            return nullptr;
        }
        return detail::npy_api::get().PyArray_FromAny_(
            ptr, nullptr, 0, 0, detail::npy_api::NPY_ARRAY_ENSUREARRAY_ | ExtraFlags, nullptr);
    }
};

template <typename T, int ExtraFlags = array::forcecast>
class array_t : public array {
private:
    struct private_ctor {};

    array_t(private_ctor,
            ShapeContainer &&shape,
            StridesContainer &&strides,
            const T *ptr,
            handle base)
        : array(std::move(shape), std::move(strides), ptr, base) {}

public:
    static_assert(!detail::array_info<T>::is_array, "Array types cannot be used with array_t");

    using value_type = T;

    array_t() : array(0, static_cast<const T *>(nullptr)) {}
    array_t(handle h, borrowed_t) : array(h, borrowed_t{}) {}
    array_t(handle h, stolen_t) : array(h, stolen_t{}) {}

    PYBIND11_DEPRECATED("Use array_t<T>::ensure() instead")
    array_t(handle h, bool is_borrowed) : array(raw_array_t(h.ptr()), stolen_t{}) {
        if (!m_ptr) {
            PyErr_Clear();
        }
        if (!is_borrowed) {
            Py_XDECREF(h.ptr());
        }
    }


    array_t(const object &o) : array(raw_array_t(o.ptr()), stolen_t{}) {
        if (!m_ptr) {
            throw error_already_set();
        }
    }

    explicit array_t(const buffer_info &info, handle base = handle()) : array(info, base) {}

    array_t(ShapeContainer shape,
            StridesContainer strides,
            const T *ptr = nullptr,
            handle base = handle())
        : array(std::move(shape), std::move(strides), ptr, base) {}

    explicit array_t(ShapeContainer shape, const T *ptr = nullptr, handle base = handle())
        : array_t(private_ctor{},
                  std::move(shape),
                  (ExtraFlags & f_style) != 0 ? detail::f_strides(*shape, itemsize())
                                              : detail::c_strides(*shape, itemsize()),
                  ptr,
                  base) {}

    explicit array_t(ssize_t count, const T *ptr = nullptr, handle base = handle())
        : array({count}, {}, ptr, base) {}

    constexpr ssize_t itemsize() const { return sizeof(T); }

    template <typename... Ix>
    ssize_t index_at(Ix... index) const {
        return offset_at(index...) / itemsize();
    }

    template <typename... Ix>
    const T *data(Ix... index) const {
        return static_cast<const T *>(array::data(index...));
    }

    template <typename... Ix>
    T *mutable_data(Ix... index) {
        return static_cast<T *>(array::mutable_data(index...));
    }


    template <typename... Ix>
    const T &at(Ix... index) const {
        if ((ssize_t) sizeof...(index) != ndim()) {
            fail_dim_check(sizeof...(index), "index dimension mismatch");
        }
        return *(static_cast<const T *>(array::data())
                 + byte_offset(ssize_t(index)...) / itemsize());
    }


    template <typename... Ix>
    T &mutable_at(Ix... index) {
        if ((ssize_t) sizeof...(index) != ndim()) {
            fail_dim_check(sizeof...(index), "index dimension mismatch");
        }
        return *(static_cast<T *>(array::mutable_data())
                 + byte_offset(ssize_t(index)...) / itemsize());
    }


    template <ssize_t Dims = -1>
    detail::unchecked_mutable_reference<T, Dims> mutable_unchecked() & {
        return array::mutable_unchecked<T, Dims>();
    }


    template <ssize_t Dims = -1>
    detail::unchecked_reference<T, Dims> unchecked() const & {
        return array::unchecked<T, Dims>();
    }



    static array_t ensure(handle h) {
        auto result = reinterpret_steal<array_t>(raw_array_t(h.ptr()));
        if (!result) {
            PyErr_Clear();
        }
        return result;
    }

    static bool check_(handle h) {
        const auto &api = detail::npy_api::get();
        return api.PyArray_Check_(h.ptr())
               && api.PyArray_EquivTypes_(detail::array_proxy(h.ptr())->descr,
                                          dtype::of<T>().ptr())
               && detail::check_flags(h.ptr(), ExtraFlags & (array::c_style | array::f_style));
    }

protected:

    static PyObject *raw_array_t(PyObject *ptr) {
        if (ptr == nullptr) {
            PyErr_SetString(PyExc_ValueError, "cannot create a pybind11::array_t from a nullptr");
            return nullptr;
        }
        return detail::npy_api::get().PyArray_FromAny_(ptr,
                                                       dtype::of<T>().release().ptr(),
                                                       0,
                                                       0,
                                                       detail::npy_api::NPY_ARRAY_ENSUREARRAY_
                                                           | ExtraFlags,
                                                       nullptr);
    }
};

template <typename T>
struct format_descriptor<T, detail::enable_if_t<detail::is_pod_struct<T>::value>> {
    static std::string format() {
        return detail::npy_format_descriptor<typename std::remove_cv<T>::type>::format();
    }
};

template <size_t N>
struct format_descriptor<char[N]> {
    static std::string format() { return std::to_string(N) + 's'; }
};
template <size_t N>
struct format_descriptor<std::array<char, N>> {
    static std::string format() { return std::to_string(N) + 's'; }
};

template <typename T>
struct format_descriptor<T, detail::enable_if_t<std::is_enum<T>::value>> {
    static std::string format() {
        return format_descriptor<
            typename std::remove_cv<typename std::underlying_type<T>::type>::type>::format();
    }
};

template <typename T>
struct format_descriptor<T, detail::enable_if_t<detail::array_info<T>::is_array>> {
    static std::string format() {
        using namespace detail;
        static constexpr auto extents = const_name("(") + array_info<T>::extents + const_name(")");
        return extents.text + format_descriptor<remove_all_extents_t<T>>::format();
    }
};

PYBIND11_NAMESPACE_BEGIN(detail)
template <typename T, int ExtraFlags>
struct pyobject_caster<array_t<T, ExtraFlags>> {
    using type = array_t<T, ExtraFlags>;

    bool load(handle src, bool convert) {
        if (!convert && !type::check_(src)) {
            return false;
        }
        value = type::ensure(src);
        return static_cast<bool>(value);
    }

    static handle cast(const handle &src, return_value_policy  , handle  ) {
        return src.inc_ref();
    }
    PYBIND11_TYPE_CASTER(type, handle_type_name<type>::name);
};

template <typename T>
struct compare_buffer_info<T, detail::enable_if_t<detail::is_pod_struct<T>::value>> {
    static bool compare(const buffer_info &b) {
        return npy_api::get().PyArray_EquivTypes_(dtype::of<T>().ptr(), dtype(b).ptr());
    }
};

template <typename T, typename = void>
struct npy_format_descriptor_name;

template <typename T>
struct npy_format_descriptor_name<T, enable_if_t<std::is_integral<T>::value>> {
    static constexpr auto name = const_name<std::is_same<T, bool>::value>(
        const_name("bool"),
        const_name<std::is_signed<T>::value>("numpy.int", "numpy.uint")
            + const_name<sizeof(T) * 8>());
};

template <typename T>
struct npy_format_descriptor_name<T, enable_if_t<std::is_floating_point<T>::value>> {
    static constexpr auto name = const_name < std::is_same<T, float>::value
                                 || std::is_same<T, const float>::value
                                 || std::is_same<T, double>::value
                                 || std::is_same<T, const double>::value
                                        > (const_name("numpy.float") + const_name<sizeof(T) * 8>(),
                                           const_name("numpy.longdouble"));
};

template <typename T>
struct npy_format_descriptor_name<T, enable_if_t<is_complex<T>::value>> {
    static constexpr auto name = const_name < std::is_same<typename T::value_type, float>::value
                                 || std::is_same<typename T::value_type, const float>::value
                                 || std::is_same<typename T::value_type, double>::value
                                 || std::is_same<typename T::value_type, const double>::value
                                        > (const_name("numpy.complex")
                                               + const_name<sizeof(typename T::value_type) * 16>(),
                                           const_name("numpy.longcomplex"));
};

template <typename T>
struct npy_format_descriptor<
    T,
    enable_if_t<satisfies_any_of<T, std::is_arithmetic, is_complex>::value>>
    : npy_format_descriptor_name<T> {
private:

    constexpr static const int values[15] = {npy_api::NPY_BOOL_,
                                             npy_api::NPY_BYTE_,
                                             npy_api::NPY_UBYTE_,
                                             npy_api::NPY_INT16_,
                                             npy_api::NPY_UINT16_,
                                             npy_api::NPY_INT32_,
                                             npy_api::NPY_UINT32_,
                                             npy_api::NPY_INT64_,
                                             npy_api::NPY_UINT64_,
                                             npy_api::NPY_FLOAT_,
                                             npy_api::NPY_DOUBLE_,
                                             npy_api::NPY_LONGDOUBLE_,
                                             npy_api::NPY_CFLOAT_,
                                             npy_api::NPY_CDOUBLE_,
                                             npy_api::NPY_CLONGDOUBLE_};

public:
    static constexpr int value = values[detail::is_fmt_numeric<T>::index];

    static pybind11::dtype dtype() {
        if (auto *ptr = npy_api::get().PyArray_DescrFromType_(value)) {
            return reinterpret_steal<pybind11::dtype>(ptr);
        }
        pybind11_fail("Unsupported buffer format!");
    }
};

#define PYBIND11_DECL_CHAR_FMT                                                                    \
    static constexpr auto name = const_name("S") + const_name<N>();                               \
    static pybind11::dtype dtype() {                                                              \
        return pybind11::dtype(std::string("S") + std::to_string(N));                             \
    }
template <size_t N>
struct npy_format_descriptor<char[N]> {
    PYBIND11_DECL_CHAR_FMT
};
template <size_t N>
struct npy_format_descriptor<std::array<char, N>> {
    PYBIND11_DECL_CHAR_FMT
};
#undef PYBIND11_DECL_CHAR_FMT

template <typename T>
struct npy_format_descriptor<T, enable_if_t<array_info<T>::is_array>> {
private:
    using base_descr = npy_format_descriptor<typename array_info<T>::type>;

public:
    static_assert(!array_info<T>::is_empty, "Zero-sized arrays are not supported");

    static constexpr auto name
        = const_name("(") + array_info<T>::extents + const_name(")") + base_descr::name;
    static pybind11::dtype dtype() {
        list shape;
        array_info<T>::append_extents(shape);
        return pybind11::dtype::from_args(
            pybind11::make_tuple(base_descr::dtype(), std::move(shape)));
    }
};

template <typename T>
struct npy_format_descriptor<T, enable_if_t<std::is_enum<T>::value>> {
private:
    using base_descr = npy_format_descriptor<typename std::underlying_type<T>::type>;

public:
    static constexpr auto name = base_descr::name;
    static pybind11::dtype dtype() { return base_descr::dtype(); }
};

struct field_descriptor {
    const char *name;
    ssize_t offset;
    ssize_t size;
    std::string format;
    dtype descr;
};

PYBIND11_NOINLINE void register_structured_dtype(any_container<field_descriptor> fields,
                                                 const std::type_info &tinfo,
                                                 ssize_t itemsize,
                                                 bool (*direct_converter)(PyObject *, void *&)) {

    auto &numpy_internals = get_numpy_internals();
    if (numpy_internals.get_type_info(tinfo, false)) {
        pybind11_fail("NumPy: dtype is already registered");
    }



    std::vector<field_descriptor> ordered_fields(std::move(fields));
    std::sort(
        ordered_fields.begin(),
        ordered_fields.end(),
        [](const field_descriptor &a, const field_descriptor &b) { return a.offset < b.offset; });

    list names, formats, offsets;
    for (auto &field : ordered_fields) {
        if (!field.descr) {
            pybind11_fail(std::string("NumPy: unsupported field dtype: `") + field.name + "` @ "
                          + tinfo.name());
        }
        names.append(pybind11::str(field.name));
        formats.append(field.descr);
        offsets.append(pybind11::int_(field.offset));
    }
    auto *dtype_ptr
        = pybind11::dtype(std::move(names), std::move(formats), std::move(offsets), itemsize)
              .release()
              .ptr();








    ssize_t offset = 0;
    std::ostringstream oss;





    oss << "^T{";
    for (auto &field : ordered_fields) {
        if (field.offset > offset) {
            oss << (field.offset - offset) << 'x';
        }
        oss << field.format << ':' << field.name << ':';
        offset = field.offset + field.size;
    }
    if (itemsize > offset) {
        oss << (itemsize - offset) << 'x';
    }
    oss << '}';
    auto format_str = oss.str();


    auto &api = npy_api::get();
    auto arr = array(buffer_info(nullptr, itemsize, format_str, 1));
    if (!api.PyArray_EquivTypes_(dtype_ptr, arr.dtype().ptr())) {
        pybind11_fail("NumPy: invalid buffer descriptor!");
    }

    auto tindex = std::type_index(tinfo);
    numpy_internals.registered_dtypes[tindex] = {dtype_ptr, std::move(format_str)};
    get_internals().direct_conversions[tindex].push_back(direct_converter);
}

template <typename T, typename SFINAE>
struct npy_format_descriptor {
    static_assert(is_pod_struct<T>::value,
                  "Attempt to use a non-POD or unimplemented POD type as a numpy dtype");

    static constexpr auto name = make_caster<T>::name;

    static pybind11::dtype dtype() { return reinterpret_borrow<pybind11::dtype>(dtype_ptr()); }

    static std::string format() {
        static auto format_str = get_numpy_internals().get_type_info<T>(true)->format_str;
        return format_str;
    }

    static void register_dtype(any_container<field_descriptor> fields) {
        register_structured_dtype(std::move(fields),
                                  typeid(typename std::remove_cv<T>::type),
                                  sizeof(T),
                                  &direct_converter);
    }

private:
    static PyObject *dtype_ptr() {
        static PyObject *ptr = get_numpy_internals().get_type_info<T>(true)->dtype_ptr;
        return ptr;
    }

    static bool direct_converter(PyObject *obj, void *&value) {
        auto &api = npy_api::get();
        if (!PyObject_TypeCheck(obj, api.PyVoidArrType_Type_)) {
            return false;
        }
        if (auto descr = reinterpret_steal<object>(api.PyArray_DescrFromScalar_(obj))) {
            if (api.PyArray_EquivTypes_(dtype_ptr(), descr.ptr())) {
                value = ((PyVoidScalarObject_Proxy *) obj)->obval;
                return true;
            }
        }
        return false;
    }
};

#ifdef __CLION_IDE__
#    define PYBIND11_NUMPY_DTYPE(Type, ...) ((void) 0)
#    define PYBIND11_NUMPY_DTYPE_EX(Type, ...) ((void) 0)
#else

#    define PYBIND11_FIELD_DESCRIPTOR_EX(T, Field, Name)                                          \
        ::pybind11::detail::field_descriptor {                                                    \
            Name, offsetof(T, Field), sizeof(decltype(std::declval<T>().Field)),                  \
                ::pybind11::format_descriptor<decltype(std::declval<T>().Field)>::format(),       \
                ::pybind11::detail::npy_format_descriptor<                                        \
                    decltype(std::declval<T>().Field)>::dtype()                                   \
        }


#    define PYBIND11_FIELD_DESCRIPTOR(T, Field) PYBIND11_FIELD_DESCRIPTOR_EX(T, Field, #    Field)



#    define PYBIND11_EVAL0(...) __VA_ARGS__
#    define PYBIND11_EVAL1(...) PYBIND11_EVAL0(PYBIND11_EVAL0(PYBIND11_EVAL0(__VA_ARGS__)))
#    define PYBIND11_EVAL2(...) PYBIND11_EVAL1(PYBIND11_EVAL1(PYBIND11_EVAL1(__VA_ARGS__)))
#    define PYBIND11_EVAL3(...) PYBIND11_EVAL2(PYBIND11_EVAL2(PYBIND11_EVAL2(__VA_ARGS__)))
#    define PYBIND11_EVAL4(...) PYBIND11_EVAL3(PYBIND11_EVAL3(PYBIND11_EVAL3(__VA_ARGS__)))
#    define PYBIND11_EVAL(...) PYBIND11_EVAL4(PYBIND11_EVAL4(PYBIND11_EVAL4(__VA_ARGS__)))
#    define PYBIND11_MAP_END(...)
#    define PYBIND11_MAP_OUT
#    define PYBIND11_MAP_COMMA ,
#    define PYBIND11_MAP_GET_END() 0, PYBIND11_MAP_END
#    define PYBIND11_MAP_NEXT0(test, next, ...) next PYBIND11_MAP_OUT
#    define PYBIND11_MAP_NEXT1(test, next) PYBIND11_MAP_NEXT0(test, next, 0)
#    define PYBIND11_MAP_NEXT(test, next) PYBIND11_MAP_NEXT1(PYBIND11_MAP_GET_END test, next)
#    if defined(_MSC_VER)                                                                         \
        && !defined(__clang__)
#        define PYBIND11_MAP_LIST_NEXT1(test, next)                                               \
            PYBIND11_EVAL0(PYBIND11_MAP_NEXT0(test, PYBIND11_MAP_COMMA next, 0))
#    else
#        define PYBIND11_MAP_LIST_NEXT1(test, next)                                               \
            PYBIND11_MAP_NEXT0(test, PYBIND11_MAP_COMMA next, 0)
#    endif
#    define PYBIND11_MAP_LIST_NEXT(test, next)                                                    \
        PYBIND11_MAP_LIST_NEXT1(PYBIND11_MAP_GET_END test, next)
#    define PYBIND11_MAP_LIST0(f, t, x, peek, ...)                                                \
        f(t, x) PYBIND11_MAP_LIST_NEXT(peek, PYBIND11_MAP_LIST1)(f, t, peek, __VA_ARGS__)
#    define PYBIND11_MAP_LIST1(f, t, x, peek, ...)                                                \
        f(t, x) PYBIND11_MAP_LIST_NEXT(peek, PYBIND11_MAP_LIST0)(f, t, peek, __VA_ARGS__)

#    define PYBIND11_MAP_LIST(f, t, ...)                                                          \
        PYBIND11_EVAL(PYBIND11_MAP_LIST1(f, t, __VA_ARGS__, (), 0))

#    define PYBIND11_NUMPY_DTYPE(Type, ...)                                                       \
        ::pybind11::detail::npy_format_descriptor<Type>::register_dtype(                          \
            ::std::vector<::pybind11::detail::field_descriptor>{                                  \
                PYBIND11_MAP_LIST(PYBIND11_FIELD_DESCRIPTOR, Type, __VA_ARGS__)})

#    if defined(_MSC_VER) && !defined(__clang__)
#        define PYBIND11_MAP2_LIST_NEXT1(test, next)                                              \
            PYBIND11_EVAL0(PYBIND11_MAP_NEXT0(test, PYBIND11_MAP_COMMA next, 0))
#    else
#        define PYBIND11_MAP2_LIST_NEXT1(test, next)                                              \
            PYBIND11_MAP_NEXT0(test, PYBIND11_MAP_COMMA next, 0)
#    endif
#    define PYBIND11_MAP2_LIST_NEXT(test, next)                                                   \
        PYBIND11_MAP2_LIST_NEXT1(PYBIND11_MAP_GET_END test, next)
#    define PYBIND11_MAP2_LIST0(f, t, x1, x2, peek, ...)                                          \
        f(t, x1, x2) PYBIND11_MAP2_LIST_NEXT(peek, PYBIND11_MAP2_LIST1)(f, t, peek, __VA_ARGS__)
#    define PYBIND11_MAP2_LIST1(f, t, x1, x2, peek, ...)                                          \
        f(t, x1, x2) PYBIND11_MAP2_LIST_NEXT(peek, PYBIND11_MAP2_LIST0)(f, t, peek, __VA_ARGS__)

#    define PYBIND11_MAP2_LIST(f, t, ...)                                                         \
        PYBIND11_EVAL(PYBIND11_MAP2_LIST1(f, t, __VA_ARGS__, (), 0))

#    define PYBIND11_NUMPY_DTYPE_EX(Type, ...)                                                    \
        ::pybind11::detail::npy_format_descriptor<Type>::register_dtype(                          \
            ::std::vector<::pybind11::detail::field_descriptor>{                                  \
                PYBIND11_MAP2_LIST(PYBIND11_FIELD_DESCRIPTOR_EX, Type, __VA_ARGS__)})

#endif

class common_iterator {
public:
    using container_type = std::vector<ssize_t>;
    using value_type = container_type::value_type;
    using size_type = container_type::size_type;

    common_iterator() : m_strides() {}

    common_iterator(void *ptr, const container_type &strides, const container_type &shape)
        : p_ptr(reinterpret_cast<char *>(ptr)), m_strides(strides.size()) {
        m_strides.back() = static_cast<value_type>(strides.back());
        for (size_type i = m_strides.size() - 1; i != 0; --i) {
            size_type j = i - 1;
            auto s = static_cast<value_type>(shape[i]);
            m_strides[j] = strides[j] + m_strides[i] - strides[i] * s;
        }
    }

    void increment(size_type dim) { p_ptr += m_strides[dim]; }

    void *data() const { return p_ptr; }

private:
    char *p_ptr{nullptr};
    container_type m_strides;
};

template <size_t N>
class multi_array_iterator {
public:
    using container_type = std::vector<ssize_t>;

    multi_array_iterator(const std::array<buffer_info, N> &buffers, const container_type &shape)
        : m_shape(shape.size()), m_index(shape.size(), 0), m_common_iterator() {


        for (size_t i = 0; i < shape.size(); ++i) {
            m_shape[i] = shape[i];
        }

        container_type strides(shape.size());
        for (size_t i = 0; i < N; ++i) {
            init_common_iterator(buffers[i], shape, m_common_iterator[i], strides);
        }
    }

    multi_array_iterator &operator++() {
        for (size_t j = m_index.size(); j != 0; --j) {
            size_t i = j - 1;
            if (++m_index[i] != m_shape[i]) {
                increment_common_iterator(i);
                break;
            }
            m_index[i] = 0;
        }
        return *this;
    }

    template <size_t K, class T = void>
    T *data() const {
        return reinterpret_cast<T *>(m_common_iterator[K].data());
    }

private:
    using common_iter = common_iterator;

    void init_common_iterator(const buffer_info &buffer,
                              const container_type &shape,
                              common_iter &iterator,
                              container_type &strides) {
        auto buffer_shape_iter = buffer.shape.rbegin();
        auto buffer_strides_iter = buffer.strides.rbegin();
        auto shape_iter = shape.rbegin();
        auto strides_iter = strides.rbegin();

        while (buffer_shape_iter != buffer.shape.rend()) {
            if (*shape_iter == *buffer_shape_iter) {
                *strides_iter = *buffer_strides_iter;
            } else {
                *strides_iter = 0;
            }

            ++buffer_shape_iter;
            ++buffer_strides_iter;
            ++shape_iter;
            ++strides_iter;
        }

        std::fill(strides_iter, strides.rend(), 0);
        iterator = common_iter(buffer.ptr, strides, shape);
    }

    void increment_common_iterator(size_t dim) {
        for (auto &iter : m_common_iterator) {
            iter.increment(dim);
        }
    }

    container_type m_shape;
    container_type m_index;
    std::array<common_iter, N> m_common_iterator;
};

enum class broadcast_trivial { non_trivial, c_trivial, f_trivial };





template <size_t N>
broadcast_trivial
broadcast(const std::array<buffer_info, N> &buffers, ssize_t &ndim, std::vector<ssize_t> &shape) {
    ndim = std::accumulate(
        buffers.begin(), buffers.end(), ssize_t(0), [](ssize_t res, const buffer_info &buf) {
            return std::max(res, buf.ndim);
        });

    shape.clear();
    shape.resize((size_t) ndim, 1);



    for (size_t i = 0; i < N; ++i) {
        auto res_iter = shape.rbegin();
        auto end = buffers[i].shape.rend();
        for (auto shape_iter = buffers[i].shape.rbegin(); shape_iter != end;
             ++shape_iter, ++res_iter) {
            const auto &dim_size_in = *shape_iter;
            auto &dim_size_out = *res_iter;



            if (dim_size_out == 1) {
                dim_size_out = dim_size_in;
            } else if (dim_size_in != 1 && dim_size_in != dim_size_out) {
                pybind11_fail("pybind11::vectorize: incompatible size/dimension of inputs!");
            }
        }
    }

    bool trivial_broadcast_c = true;
    bool trivial_broadcast_f = true;
    for (size_t i = 0; i < N && (trivial_broadcast_c || trivial_broadcast_f); ++i) {
        if (buffers[i].size == 1) {
            continue;
        }


        if (buffers[i].ndim != ndim) {
            return broadcast_trivial::non_trivial;
        }


        if (!std::equal(buffers[i].shape.cbegin(), buffers[i].shape.cend(), shape.cbegin())) {
            return broadcast_trivial::non_trivial;
        }


        if (trivial_broadcast_c) {
            ssize_t expect_stride = buffers[i].itemsize;
            auto end = buffers[i].shape.crend();
            for (auto shape_iter = buffers[i].shape.crbegin(),
                      stride_iter = buffers[i].strides.crbegin();
                 trivial_broadcast_c && shape_iter != end;
                 ++shape_iter, ++stride_iter) {
                if (expect_stride == *stride_iter) {
                    expect_stride *= *shape_iter;
                } else {
                    trivial_broadcast_c = false;
                }
            }
        }


        if (trivial_broadcast_f) {
            ssize_t expect_stride = buffers[i].itemsize;
            auto end = buffers[i].shape.cend();
            for (auto shape_iter = buffers[i].shape.cbegin(),
                      stride_iter = buffers[i].strides.cbegin();
                 trivial_broadcast_f && shape_iter != end;
                 ++shape_iter, ++stride_iter) {
                if (expect_stride == *stride_iter) {
                    expect_stride *= *shape_iter;
                } else {
                    trivial_broadcast_f = false;
                }
            }
        }
    }

    return trivial_broadcast_c   ? broadcast_trivial::c_trivial
           : trivial_broadcast_f ? broadcast_trivial::f_trivial
                                 : broadcast_trivial::non_trivial;
}

template <typename T>
struct vectorize_arg {
    static_assert(!std::is_rvalue_reference<T>::value,
                  "Functions with rvalue reference arguments cannot be vectorized");

    using call_type = remove_reference_t<T>;

    static constexpr bool vectorize
        = satisfies_any_of<call_type, std::is_arithmetic, is_complex, is_pod>::value
          && satisfies_none_of<call_type,
                               std::is_pointer,
                               std::is_array,
                               is_std_array,
                               std::is_enum>::value
          && (!std::is_reference<T>::value
              || (std::is_lvalue_reference<T>::value && std::is_const<call_type>::value));

    using type = conditional_t<vectorize, array_t<remove_cv_t<call_type>, array::forcecast>, T>;
};


template <typename Func, typename Return, typename... Args>
struct vectorize_returned_array {
    using Type = array_t<Return>;

    static Type create(broadcast_trivial trivial, const std::vector<ssize_t> &shape) {
        if (trivial == broadcast_trivial::f_trivial) {
            return array_t<Return, array::f_style>(shape);
        }
        return array_t<Return>(shape);
    }

    static Return *mutable_data(Type &array) { return array.mutable_data(); }

    static Return call(Func &f, Args &...args) { return f(args...); }

    static void call(Return *out, size_t i, Func &f, Args &...args) { out[i] = f(args...); }
};


template <typename Func, typename... Args>
struct vectorize_returned_array<Func, void, Args...> {
    using Type = none;

    static Type create(broadcast_trivial, const std::vector<ssize_t> &) { return none(); }

    static void *mutable_data(Type &) { return nullptr; }

    static detail::void_type call(Func &f, Args &...args) {
        f(args...);
        return {};
    }

    static void call(void *, size_t, Func &f, Args &...args) { f(args...); }
};

template <typename Func, typename Return, typename... Args>
struct vectorize_helper {


#ifdef __CUDACC__
public:
#else
private:
#endif

    static constexpr size_t N = sizeof...(Args);
    static constexpr size_t NVectorized = constexpr_sum(vectorize_arg<Args>::vectorize...);
    static_assert(
        NVectorized >= 1,
        "pybind11::vectorize(...) requires a function with at least one vectorizable argument");

public:
    template <typename T,

              typename = detail::enable_if_t<
                  !std::is_same<vectorize_helper, typename std::decay<T>::type>::value>>
    explicit vectorize_helper(T &&f) : f(std::forward<T>(f)) {}

    object operator()(typename vectorize_arg<Args>::type... args) {
        return run(args...,
                   make_index_sequence<N>(),
                   select_indices<vectorize_arg<Args>::vectorize...>(),
                   make_index_sequence<NVectorized>());
    }

private:
    remove_reference_t<Func> f;



    using arg_call_types = std::tuple<typename vectorize_arg<Args>::call_type...>;
    template <size_t Index>
    using param_n_t = typename std::tuple_element<Index, arg_call_types>::type;

    using returned_array = vectorize_returned_array<Func, Return, Args...>;








    template <size_t... Index, size_t... VIndex, size_t... BIndex>
    object run(typename vectorize_arg<Args>::type &...args,
               index_sequence<Index...> i_seq,
               index_sequence<VIndex...> vi_seq,
               index_sequence<BIndex...> bi_seq) {




        std::array<void *, N> params{{&args...}};


        std::array<buffer_info, NVectorized> buffers{
            {reinterpret_cast<array *>(params[VIndex])->request()...}};


        ssize_t nd = 0;
        std::vector<ssize_t> shape(0);
        auto trivial = broadcast(buffers, nd, shape);
        auto ndim = (size_t) nd;

        size_t size
            = std::accumulate(shape.begin(), shape.end(), (size_t) 1, std::multiplies<size_t>());



        if (size == 1 && ndim == 0) {
            PYBIND11_EXPAND_SIDE_EFFECTS(params[VIndex] = buffers[BIndex].ptr);
            return cast(
                returned_array::call(f, *reinterpret_cast<param_n_t<Index> *>(params[Index])...));
        }

        auto result = returned_array::create(trivial, shape);

        if (size == 0) {
            return std::move(result);
        }


        auto *mutable_data = returned_array::mutable_data(result);
        if (trivial == broadcast_trivial::non_trivial) {
            apply_broadcast(buffers, params, mutable_data, size, shape, i_seq, vi_seq, bi_seq);
        } else {
            apply_trivial(buffers, params, mutable_data, size, i_seq, vi_seq, bi_seq);
        }

        return std::move(result);
    }

    template <size_t... Index, size_t... VIndex, size_t... BIndex>
    void apply_trivial(std::array<buffer_info, NVectorized> &buffers,
                       std::array<void *, N> &params,
                       Return *out,
                       size_t size,
                       index_sequence<Index...>,
                       index_sequence<VIndex...>,
                       index_sequence<BIndex...>) {




        std::array<std::pair<unsigned char *&, const size_t>, NVectorized> vecparams{
            {std::pair<unsigned char *&, const size_t>(
                reinterpret_cast<unsigned char *&>(params[VIndex] = buffers[BIndex].ptr),
                buffers[BIndex].size == 1 ? 0 : sizeof(param_n_t<VIndex>))...}};

        for (size_t i = 0; i < size; ++i) {
            returned_array::call(
                out, i, f, *reinterpret_cast<param_n_t<Index> *>(params[Index])...);
            for (auto &x : vecparams) {
                x.first += x.second;
            }
        }
    }

    template <size_t... Index, size_t... VIndex, size_t... BIndex>
    void apply_broadcast(std::array<buffer_info, NVectorized> &buffers,
                         std::array<void *, N> &params,
                         Return *out,
                         size_t size,
                         const std::vector<ssize_t> &output_shape,
                         index_sequence<Index...>,
                         index_sequence<VIndex...>,
                         index_sequence<BIndex...>) {

        multi_array_iterator<NVectorized> input_iter(buffers, output_shape);

        for (size_t i = 0; i < size; ++i, ++input_iter) {
            PYBIND11_EXPAND_SIDE_EFFECTS((params[VIndex] = input_iter.template data<BIndex>()));
            returned_array::call(
                out, i, f, *reinterpret_cast<param_n_t<Index> *>(std::get<Index>(params))...);
        }
    }
};

template <typename Func, typename Return, typename... Args>
vectorize_helper<Func, Return, Args...> vectorize_extractor(const Func &f, Return (*)(Args...)) {
    return detail::vectorize_helper<Func, Return, Args...>(f);
}

template <typename T, int Flags>
struct handle_type_name<array_t<T, Flags>> {
    static constexpr auto name
        = const_name("numpy.ndarray[") + npy_format_descriptor<T>::name + const_name("]");
};

PYBIND11_NAMESPACE_END(detail)


template <typename Return, typename... Args>
detail::vectorize_helper<Return (*)(Args...), Return, Args...> vectorize(Return (*f)(Args...)) {
    return detail::vectorize_helper<Return (*)(Args...), Return, Args...>(f);
}


template <typename Func, detail::enable_if_t<detail::is_lambda<Func>::value, int> = 0>
auto vectorize(Func &&f)
    -> decltype(detail::vectorize_extractor(std::forward<Func>(f),
                                            (detail::function_signature_t<Func> *) nullptr)) {
    return detail::vectorize_extractor(std::forward<Func>(f),
                                       (detail::function_signature_t<Func> *) nullptr);
}


template <typename Return,
          typename Class,
          typename... Args,
          typename Helper = detail::vectorize_helper<
              decltype(std::mem_fn(std::declval<Return (Class::*)(Args...)>())),
              Return,
              Class *,
              Args...>>
Helper vectorize(Return (Class::*f)(Args...)) {
    return Helper(std::mem_fn(f));
}


template <typename Return,
          typename Class,
          typename... Args,
          typename Helper = detail::vectorize_helper<
              decltype(std::mem_fn(std::declval<Return (Class::*)(Args...) const>())),
              Return,
              const Class *,
              Args...>>
Helper vectorize(Return (Class::*f)(Args...) const) {
    return Helper(std::mem_fn(f));
}

PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
