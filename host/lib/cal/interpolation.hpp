//
// Copyright 2016 Ettus Research
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
