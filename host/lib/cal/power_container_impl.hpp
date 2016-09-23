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

#ifndef INCLUDED_UHD_CAL_POWER_CONTAINER_IMPL_HPP
#define INCLUDED_UHD_CAL_POWER_CONTAINER_IMPL_HPP

#include "interpolation.ipp"
#include <uhd/cal/power_container.hpp>

namespace uhd {
namespace cal {

enum interp_mode_t
{
    BILINEAR,  //! linear interpolation
    NEAREST    //! nearest neighbor interpolation
};

class power_container_impl : public power_container {
public:
    typedef std::map<std::vector<double>, double> container_t;

    power_container_impl();

    double get(const std::vector<double> &args);
    const metadata_t &get_metadata();

    void add(const double output, const std::vector<double> &args);
    void add_metadata(const metadata_t &data);

private:
    // Container data to be serialized
    size_t _nargs;
    metadata_t _metadata;
    interp_mode_t _mode;
    container_t _data;

    interp<double, double> _interpolator;

    void verify_nargs(const std::vector<double> &args);

protected:
    friend class boost::serialization::access;

    void serialize(iarchive_type & ar, const unsigned int) {
        ar & _nargs;
        ar & _metadata;
        ar & _mode;
        ar & _data;
    }

    void serialize(oarchive_type & ar, const unsigned int) {
        ar & _nargs;
        ar & _metadata;
        ar & _mode;
        ar & _data;
    }
};

} // namespace cal
} // namespace uhd

#endif /* INCLUDED_UHD_CAL_POWER_CONTAINER_IMPL_HPP */
