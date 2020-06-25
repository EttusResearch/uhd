//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace pybind11 { namespace detail {
template <typename T>
struct type_caster<boost::optional<T>> : optional_caster<boost::optional<T>>
{
};
}} // namespace pybind11::detail

std::vector<uint8_t> pybytes_to_vector(const py::bytes& data)
{
    const std::string data_str = std::string(data);
    return std::vector<uint8_t>(data_str.cbegin(), data_str.cend());
}

py::bytes vector_to_pybytes(const std::vector<uint8_t>& data)
{
    return py::bytes(std::string(data.cbegin(), data.cend()));
}

std::vector<uint64_t> pybytes_to_u64_vector(const py::bytes& data)
{
    const std::string data_str = std::string(data);
    return std::vector<uint64_t>(data_str.cbegin(), data_str.cend());
}

py::bytes u64_vector_to_pybytes(const std::vector<uint64_t>& data)
{
    return py::bytes(std::string(data.cbegin(), data.cend()));
}
