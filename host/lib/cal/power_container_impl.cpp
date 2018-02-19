//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "power_container_impl.hpp"
#include <uhd/exception.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::cal;

power_container::sptr power_container::make()
{
    return power_container::sptr(new power_container_impl());
}

power_container_impl::power_container_impl() :
    _nargs(0), _mode(interp_mode_t::NEAREST)
{
    /* NOP */
}

double power_container_impl::get(const std::vector<double> &args)
{
    this->verify_nargs(args);
    switch (_mode)
    {
        case interp_mode_t::BILINEAR :
            return _interpolator.bl_interp(_data, args);
        case interp_mode_t::NEAREST  :
            return _interpolator.nn_interp(_data, args);
        default:
            return _interpolator.nn_interp(_data, args);
    }
}

void power_container_impl::add(const double output, const std::vector<double> &args)
{
    if (_nargs == 0)
    {
        _nargs = args.size();
        _mode = _nargs == 2 ? interp_mode_t::BILINEAR : interp_mode_t::NEAREST;
    }
    this->verify_nargs(args);
    _data[args] = output;
}

void power_container_impl::add_metadata(const power_container::metadata_t &data)
{
    _metadata = data;
}

const power_container_impl::metadata_t &power_container_impl::get_metadata()
{
    return _metadata;
}

void power_container_impl::verify_nargs(const std::vector<double> &args)
{
    // Check that the number of arguments expected are correct
    if (args.size() != _nargs) {
        throw uhd::assertion_error(str(boost::format(
            "power_container_impl: Expected %d number of arguments/values instead of %d")
            % _nargs % args.size()
        ));
    }
}
