//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_INTERPOLATION_HPP
#define INCLUDED_UHD_INTERPOLATION_HPP

#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <map>
#include <cmath>

namespace uhd {
namespace cal {

template<typename in_type, typename out_type>
struct interp
{
public:
    typedef std::vector<in_type> args_t;
    typedef std::map<args_t, out_type> container_t;

    /*!
     * Nearest neighbor interpolation given a mapping: R^n -> R
     *
     * 1) search for the nearest point in R^n
     * 2) find the nearest output scalars in R
     *
     * \param data input data container
     * \param args input data point
     * \returns interpolated output value
     */
    const out_type nn_interp(container_t &data, const args_t &args);

    /*!
     * Bilinear interpolation given a mapping: R^2 -> R
     *
     * 1) search for 4 surrounding points in R^2
     * 2) find the output scalars in R
     * 3) solve the system of equations given our input mappings
     *
     * \param data input data container
     * \param args input data point
     * \returns interpolated output value
     */
    const out_type bl_interp(container_t &data, const args_t &args);

private:
    /*!
     * Calculate the distance between two points
     */
    static in_type calc_dist(const args_t &a, const args_t &b);
};

} // namespace cal
} // namespace uhd

#endif  /* INCLUDED_UHD_INTERPOLATION_HPP */
