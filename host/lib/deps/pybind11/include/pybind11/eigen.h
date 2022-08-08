/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once



#include "numpy.h"






#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4127)
#    pragma warning(disable : 5054)

#endif

#include <Eigen/Core>
#include <Eigen/SparseCore>

#if defined(_MSC_VER)
#    pragma warning(pop)
#endif




static_assert(EIGEN_VERSION_AT_LEAST(3, 2, 7),
              "Eigen support in pybind11 requires Eigen >= 3.2.7");

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)


using EigenDStride = Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>;
template <typename MatrixType>
using EigenDRef = Eigen::Ref<MatrixType, 0, EigenDStride>;
template <typename MatrixType>
using EigenDMap = Eigen::Map<MatrixType, 0, EigenDStride>;

PYBIND11_NAMESPACE_BEGIN(detail)

#if EIGEN_VERSION_AT_LEAST(3, 3, 0)
using EigenIndex = Eigen::Index;
template <typename Scalar, int Flags, typename StorageIndex>
using EigenMapSparseMatrix = Eigen::Map<Eigen::SparseMatrix<Scalar, Flags, StorageIndex>>;
#else
using EigenIndex = EIGEN_DEFAULT_DENSE_INDEX_TYPE;
template <typename Scalar, int Flags, typename StorageIndex>
using EigenMapSparseMatrix = Eigen::MappedSparseMatrix<Scalar, Flags, StorageIndex>;
#endif


template <typename T>
using is_eigen_dense_map = all_of<is_template_base_of<Eigen::DenseBase, T>,
                                  std::is_base_of<Eigen::MapBase<T, Eigen::ReadOnlyAccessors>, T>>;
template <typename T>
using is_eigen_mutable_map = std::is_base_of<Eigen::MapBase<T, Eigen::WriteAccessors>, T>;
template <typename T>
using is_eigen_dense_plain
    = all_of<negation<is_eigen_dense_map<T>>, is_template_base_of<Eigen::PlainObjectBase, T>>;
template <typename T>
using is_eigen_sparse = is_template_base_of<Eigen::SparseMatrixBase, T>;




template <typename T>
using is_eigen_other
    = all_of<is_template_base_of<Eigen::EigenBase, T>,
             negation<any_of<is_eigen_dense_map<T>, is_eigen_dense_plain<T>, is_eigen_sparse<T>>>>;


template <bool EigenRowMajor>
struct EigenConformable {
    bool conformable = false;
    EigenIndex rows = 0, cols = 0;
    EigenDStride stride{0, 0};
    bool negativestrides = false;


    EigenConformable(bool fits = false) : conformable{fits} {}

    EigenConformable(EigenIndex r, EigenIndex c, EigenIndex rstride, EigenIndex cstride)
        : conformable{true}, rows{r}, cols{c},


          stride{EigenRowMajor ? (rstride > 0 ? rstride : 0)
                               : (cstride > 0 ? cstride : 0)  ,
                 EigenRowMajor ? (cstride > 0 ? cstride : 0)
                               : (rstride > 0 ? rstride : 0)  },
          negativestrides{rstride < 0 || cstride < 0} {}

    EigenConformable(EigenIndex r, EigenIndex c, EigenIndex stride)
        : EigenConformable(r, c, r == 1 ? c * stride : stride, c == 1 ? r : r * stride) {}

    template <typename props>
    bool stride_compatible() const {




        if (negativestrides) {
            return false;
        }
        if (rows == 0 || cols == 0) {
            return true;
        }
        return (props::inner_stride == Eigen::Dynamic || props::inner_stride == stride.inner()
                || (EigenRowMajor ? cols : rows) == 1)
               && (props::outer_stride == Eigen::Dynamic || props::outer_stride == stride.outer()
                   || (EigenRowMajor ? rows : cols) == 1);
    }

    operator bool() const { return conformable; }
};

template <typename Type>
struct eigen_extract_stride {
    using type = Type;
};
template <typename PlainObjectType, int MapOptions, typename StrideType>
struct eigen_extract_stride<Eigen::Map<PlainObjectType, MapOptions, StrideType>> {
    using type = StrideType;
};
template <typename PlainObjectType, int Options, typename StrideType>
struct eigen_extract_stride<Eigen::Ref<PlainObjectType, Options, StrideType>> {
    using type = StrideType;
};


template <typename Type_>
struct EigenProps {
    using Type = Type_;
    using Scalar = typename Type::Scalar;
    using StrideType = typename eigen_extract_stride<Type>::type;
    static constexpr EigenIndex rows = Type::RowsAtCompileTime, cols = Type::ColsAtCompileTime,
                                size = Type::SizeAtCompileTime;
    static constexpr bool row_major = Type::IsRowMajor,
                          vector
                          = Type::IsVectorAtCompileTime,
        fixed_rows = rows != Eigen::Dynamic, fixed_cols = cols != Eigen::Dynamic,
                          fixed = size != Eigen::Dynamic,
        dynamic = !fixed_rows && !fixed_cols;

    template <EigenIndex i, EigenIndex ifzero>
    using if_zero = std::integral_constant<EigenIndex, i == 0 ? ifzero : i>;
    static constexpr EigenIndex inner_stride
        = if_zero<StrideType::InnerStrideAtCompileTime, 1>::value,
        outer_stride = if_zero < StrideType::OuterStrideAtCompileTime,
        vector      ? size
        : row_major ? cols
                    : rows > ::value;
    static constexpr bool dynamic_stride
        = inner_stride == Eigen::Dynamic && outer_stride == Eigen::Dynamic;
    static constexpr bool requires_row_major
        = !dynamic_stride && !vector && (row_major ? inner_stride : outer_stride) == 1;
    static constexpr bool requires_col_major
        = !dynamic_stride && !vector && (row_major ? outer_stride : inner_stride) == 1;




    static EigenConformable<row_major> conformable(const array &a) {
        const auto dims = a.ndim();
        if (dims < 1 || dims > 2) {
            return false;
        }

        if (dims == 2) {

            EigenIndex np_rows = a.shape(0), np_cols = a.shape(1),
                       np_rstride = a.strides(0) / static_cast<ssize_t>(sizeof(Scalar)),
                       np_cstride = a.strides(1) / static_cast<ssize_t>(sizeof(Scalar));
            if ((PYBIND11_SILENCE_MSVC_C4127(fixed_rows) && np_rows != rows)
                || (PYBIND11_SILENCE_MSVC_C4127(fixed_cols) && np_cols != cols)) {
                return false;
            }

            return {np_rows, np_cols, np_rstride, np_cstride};
        }



        const EigenIndex n = a.shape(0),
                         stride = a.strides(0) / static_cast<ssize_t>(sizeof(Scalar));

        if (vector) {
            if (PYBIND11_SILENCE_MSVC_C4127(fixed) && size != n) {
                return false;
            }
            return {rows == 1 ? 1 : n, cols == 1 ? 1 : n, stride};
        }
        if (fixed) {

            return false;
        }
        if (fixed_cols) {


            if (cols != n) {
                return false;
            }
            return {1, n, stride};
        }
        if (PYBIND11_SILENCE_MSVC_C4127(fixed_rows) && rows != n) {
            return false;
        }
        return {n, 1, stride};
    }

    static constexpr bool show_writeable
        = is_eigen_dense_map<Type>::value && is_eigen_mutable_map<Type>::value;
    static constexpr bool show_order = is_eigen_dense_map<Type>::value;
    static constexpr bool show_c_contiguous = show_order && requires_row_major;
    static constexpr bool show_f_contiguous
        = !show_c_contiguous && show_order && requires_col_major;

    static constexpr auto descriptor
        = const_name("numpy.ndarray[") + npy_format_descriptor<Scalar>::name + const_name("[")
          + const_name<fixed_rows>(const_name<(size_t) rows>(), const_name("m")) + const_name(", ")
          + const_name<fixed_cols>(const_name<(size_t) cols>(), const_name("n")) + const_name("]")
          +






          const_name<show_writeable>(", flags.writeable", "")
          + const_name<show_c_contiguous>(", flags.c_contiguous", "")
          + const_name<show_f_contiguous>(", flags.f_contiguous", "") + const_name("]");
};



template <typename props>
handle
eigen_array_cast(typename props::Type const &src, handle base = handle(), bool writeable = true) {
    constexpr ssize_t elem_size = sizeof(typename props::Scalar);
    array a;
    if (props::vector) {
        a = array({src.size()}, {elem_size * src.innerStride()}, src.data(), base);
    } else {
        a = array({src.rows(), src.cols()},
                  {elem_size * src.rowStride(), elem_size * src.colStride()},
                  src.data(),
                  base);
    }

    if (!writeable) {
        array_proxy(a.ptr())->flags &= ~detail::npy_api::NPY_ARRAY_WRITEABLE_;
    }

    return a.release();
}





template <typename props, typename Type>
handle eigen_ref_array(Type &src, handle parent = none()) {


    return eigen_array_cast<props>(src, parent, !std::is_const<Type>::value);
}





template <typename props, typename Type, typename = enable_if_t<is_eigen_dense_plain<Type>::value>>
handle eigen_encapsulate(Type *src) {
    capsule base(src, [](void *o) { delete static_cast<Type *>(o); });
    return eigen_ref_array<props>(*src, base);
}



template <typename Type>
struct type_caster<Type, enable_if_t<is_eigen_dense_plain<Type>::value>> {
    using Scalar = typename Type::Scalar;
    using props = EigenProps<Type>;

    bool load(handle src, bool convert) {

        if (!convert && !isinstance<array_t<Scalar>>(src)) {
            return false;
        }


        auto buf = array::ensure(src);

        if (!buf) {
            return false;
        }

        auto dims = buf.ndim();
        if (dims < 1 || dims > 2) {
            return false;
        }

        auto fits = props::conformable(buf);
        if (!fits) {
            return false;
        }


        value = Type(fits.rows, fits.cols);
        auto ref = reinterpret_steal<array>(eigen_ref_array<props>(value));
        if (dims == 1) {
            ref = ref.squeeze();
        } else if (ref.ndim() == 1) {
            buf = buf.squeeze();
        }

        int result = detail::npy_api::get().PyArray_CopyInto_(ref.ptr(), buf.ptr());

        if (result < 0) {
            PyErr_Clear();
            return false;
        }

        return true;
    }

private:

    template <typename CType>
    static handle cast_impl(CType *src, return_value_policy policy, handle parent) {
        switch (policy) {
            case return_value_policy::take_ownership:
            case return_value_policy::automatic:
                return eigen_encapsulate<props>(src);
            case return_value_policy::move:
                return eigen_encapsulate<props>(new CType(std::move(*src)));
            case return_value_policy::copy:
                return eigen_array_cast<props>(*src);
            case return_value_policy::reference:
            case return_value_policy::automatic_reference:
                return eigen_ref_array<props>(*src);
            case return_value_policy::reference_internal:
                return eigen_ref_array<props>(*src, parent);
            default:
                throw cast_error("unhandled return_value_policy: should not happen!");
        };
    }

public:

    static handle cast(Type &&src, return_value_policy  , handle parent) {
        return cast_impl(&src, return_value_policy::move, parent);
    }

    static handle cast(const Type &&src, return_value_policy  , handle parent) {
        return cast_impl(&src, return_value_policy::move, parent);
    }

    static handle cast(Type &src, return_value_policy policy, handle parent) {
        if (policy == return_value_policy::automatic
            || policy == return_value_policy::automatic_reference) {
            policy = return_value_policy::copy;
        }
        return cast_impl(&src, policy, parent);
    }

    static handle cast(const Type &src, return_value_policy policy, handle parent) {
        if (policy == return_value_policy::automatic
            || policy == return_value_policy::automatic_reference) {
            policy = return_value_policy::copy;
        }
        return cast(&src, policy, parent);
    }

    static handle cast(Type *src, return_value_policy policy, handle parent) {
        return cast_impl(src, policy, parent);
    }

    static handle cast(const Type *src, return_value_policy policy, handle parent) {
        return cast_impl(src, policy, parent);
    }

    static constexpr auto name = props::descriptor;


    operator Type *() { return &value; }

    operator Type &() { return value; }

    operator Type &&() && { return std::move(value); }
    template <typename T>
    using cast_op_type = movable_cast_op_type<T>;

private:
    Type value;
};


template <typename MapType>
struct eigen_map_caster {
private:
    using props = EigenProps<MapType>;

public:






    static handle cast(const MapType &src, return_value_policy policy, handle parent) {
        switch (policy) {
            case return_value_policy::copy:
                return eigen_array_cast<props>(src);
            case return_value_policy::reference_internal:
                return eigen_array_cast<props>(src, parent, is_eigen_mutable_map<MapType>::value);
            case return_value_policy::reference:
            case return_value_policy::automatic:
            case return_value_policy::automatic_reference:
                return eigen_array_cast<props>(src, none(), is_eigen_mutable_map<MapType>::value);
            default:

                pybind11_fail("Invalid return_value_policy for Eigen Map/Ref/Block type");
        }
    }

    static constexpr auto name = props::descriptor;




    bool load(handle, bool) = delete;
    operator MapType() = delete;
    template <typename>
    using cast_op_type = MapType;
};


template <typename Type>
struct type_caster<Type, enable_if_t<is_eigen_dense_map<Type>::value>> : eigen_map_caster<Type> {};



template <typename PlainObjectType, typename StrideType>
struct type_caster<
    Eigen::Ref<PlainObjectType, 0, StrideType>,
    enable_if_t<is_eigen_dense_map<Eigen::Ref<PlainObjectType, 0, StrideType>>::value>>
    : public eigen_map_caster<Eigen::Ref<PlainObjectType, 0, StrideType>> {
private:
    using Type = Eigen::Ref<PlainObjectType, 0, StrideType>;
    using props = EigenProps<Type>;
    using Scalar = typename props::Scalar;
    using MapType = Eigen::Map<PlainObjectType, 0, StrideType>;
    using Array
        = array_t<Scalar,
                  array::forcecast
                      | ((props::row_major ? props::inner_stride : props::outer_stride) == 1
                             ? array::c_style
                         : (props::row_major ? props::outer_stride : props::inner_stride) == 1
                             ? array::f_style
                             : 0)>;
    static constexpr bool need_writeable = is_eigen_mutable_map<Type>::value;

    std::unique_ptr<MapType> map;
    std::unique_ptr<Type> ref;






    Array copy_or_ref;

public:
    bool load(handle src, bool convert) {


        bool need_copy = !isinstance<Array>(src);

        EigenConformable<props::row_major> fits;
        if (!need_copy) {


            auto aref = reinterpret_borrow<Array>(src);

            if (aref && (!need_writeable || aref.writeable())) {
                fits = props::conformable(aref);
                if (!fits) {
                    return false;
                }
                if (!fits.template stride_compatible<props>()) {
                    need_copy = true;
                } else {
                    copy_or_ref = std::move(aref);
                }
            } else {
                need_copy = true;
            }
        }

        if (need_copy) {



            if (!convert || need_writeable) {
                return false;
            }

            Array copy = Array::ensure(src);
            if (!copy) {
                return false;
            }
            fits = props::conformable(copy);
            if (!fits || !fits.template stride_compatible<props>()) {
                return false;
            }
            copy_or_ref = std::move(copy);
            loader_life_support::add_patient(copy_or_ref);
        }

        ref.reset();
        map.reset(new MapType(data(copy_or_ref),
                              fits.rows,
                              fits.cols,
                              make_stride(fits.stride.outer(), fits.stride.inner())));
        ref.reset(new Type(*map));

        return true;
    }


    operator Type *() { return ref.get(); }

    operator Type &() { return *ref; }
    template <typename _T>
    using cast_op_type = pybind11::detail::cast_op_type<_T>;

private:
    template <typename T = Type, enable_if_t<is_eigen_mutable_map<T>::value, int> = 0>
    Scalar *data(Array &a) {
        return a.mutable_data();
    }

    template <typename T = Type, enable_if_t<!is_eigen_mutable_map<T>::value, int> = 0>
    const Scalar *data(Array &a) {
        return a.data();
    }



    template <typename S>
    using stride_ctor_default = bool_constant<S::InnerStrideAtCompileTime != Eigen::Dynamic
                                              && S::OuterStrideAtCompileTime != Eigen::Dynamic
                                              && std::is_default_constructible<S>::value>;


    template <typename S>
    using stride_ctor_dual
        = bool_constant<!stride_ctor_default<S>::value
                        && std::is_constructible<S, EigenIndex, EigenIndex>::value>;


    template <typename S>
    using stride_ctor_outer
        = bool_constant<!any_of<stride_ctor_default<S>, stride_ctor_dual<S>>::value
                        && S::OuterStrideAtCompileTime == Eigen::Dynamic
                        && S::InnerStrideAtCompileTime != Eigen::Dynamic
                        && std::is_constructible<S, EigenIndex>::value>;
    template <typename S>
    using stride_ctor_inner
        = bool_constant<!any_of<stride_ctor_default<S>, stride_ctor_dual<S>>::value
                        && S::InnerStrideAtCompileTime == Eigen::Dynamic
                        && S::OuterStrideAtCompileTime != Eigen::Dynamic
                        && std::is_constructible<S, EigenIndex>::value>;

    template <typename S = StrideType, enable_if_t<stride_ctor_default<S>::value, int> = 0>
    static S make_stride(EigenIndex, EigenIndex) {
        return S();
    }
    template <typename S = StrideType, enable_if_t<stride_ctor_dual<S>::value, int> = 0>
    static S make_stride(EigenIndex outer, EigenIndex inner) {
        return S(outer, inner);
    }
    template <typename S = StrideType, enable_if_t<stride_ctor_outer<S>::value, int> = 0>
    static S make_stride(EigenIndex outer, EigenIndex) {
        return S(outer);
    }
    template <typename S = StrideType, enable_if_t<stride_ctor_inner<S>::value, int> = 0>
    static S make_stride(EigenIndex, EigenIndex inner) {
        return S(inner);
    }
};





template <typename Type>
struct type_caster<Type, enable_if_t<is_eigen_other<Type>::value>> {
protected:
    using Matrix
        = Eigen::Matrix<typename Type::Scalar, Type::RowsAtCompileTime, Type::ColsAtCompileTime>;
    using props = EigenProps<Matrix>;

public:
    static handle cast(const Type &src, return_value_policy  , handle  ) {
        handle h = eigen_encapsulate<props>(new Matrix(src));
        return h;
    }
    static handle cast(const Type *src, return_value_policy policy, handle parent) {
        return cast(*src, policy, parent);
    }

    static constexpr auto name = props::descriptor;




    bool load(handle, bool) = delete;
    operator Type() = delete;
    template <typename>
    using cast_op_type = Type;
};

template <typename Type>
struct type_caster<Type, enable_if_t<is_eigen_sparse<Type>::value>> {
    using Scalar = typename Type::Scalar;
    using StorageIndex = remove_reference_t<decltype(*std::declval<Type>().outerIndexPtr())>;
    using Index = typename Type::Index;
    static constexpr bool rowMajor = Type::IsRowMajor;

    bool load(handle src, bool) {
        if (!src) {
            return false;
        }

        auto obj = reinterpret_borrow<object>(src);
        object sparse_module = module_::import("scipy.sparse");
        object matrix_type = sparse_module.attr(rowMajor ? "csr_matrix" : "csc_matrix");

        if (!type::handle_of(obj).is(matrix_type)) {
            try {
                obj = matrix_type(obj);
            } catch (const error_already_set &) {
                return false;
            }
        }

        auto values = array_t<Scalar>((object) obj.attr("data"));
        auto innerIndices = array_t<StorageIndex>((object) obj.attr("indices"));
        auto outerIndices = array_t<StorageIndex>((object) obj.attr("indptr"));
        auto shape = pybind11::tuple((pybind11::object) obj.attr("shape"));
        auto nnz = obj.attr("nnz").cast<Index>();

        if (!values || !innerIndices || !outerIndices) {
            return false;
        }

        value = EigenMapSparseMatrix<Scalar,
                                     Type::Flags &(Eigen::RowMajor | Eigen::ColMajor),
                                     StorageIndex>(shape[0].cast<Index>(),
                                                   shape[1].cast<Index>(),
                                                   std::move(nnz),
                                                   outerIndices.mutable_data(),
                                                   innerIndices.mutable_data(),
                                                   values.mutable_data());

        return true;
    }

    static handle cast(const Type &src, return_value_policy  , handle  ) {
        const_cast<Type &>(src).makeCompressed();

        object matrix_type
            = module_::import("scipy.sparse").attr(rowMajor ? "csr_matrix" : "csc_matrix");

        array data(src.nonZeros(), src.valuePtr());
        array outerIndices((rowMajor ? src.rows() : src.cols()) + 1, src.outerIndexPtr());
        array innerIndices(src.nonZeros(), src.innerIndexPtr());

        return matrix_type(pybind11::make_tuple(
                               std::move(data), std::move(innerIndices), std::move(outerIndices)),
                           pybind11::make_tuple(src.rows(), src.cols()))
            .release();
    }

    PYBIND11_TYPE_CASTER(Type,
                         const_name<(Type::IsRowMajor) != 0>("scipy.sparse.csr_matrix[",
                                                             "scipy.sparse.csc_matrix[")
                             + npy_format_descriptor<Scalar>::name + const_name("]"));
};

PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
