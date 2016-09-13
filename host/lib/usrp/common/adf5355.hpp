//
// Copyright 2015 Ettus Research LLC
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

#ifndef INCLUDED_ADF5355_HPP
#define INCLUDED_ADF5355_HPP

#include <boost/function.hpp>
#include <vector>

class adf5355_iface
{
public:
    typedef boost::shared_ptr<adf5355_iface> sptr;
    typedef boost::function<void(std::vector<boost::uint32_t>)> write_fn_t;

    static sptr make(write_fn_t write);

    virtual ~adf5355_iface() {}

    enum output_t { RF_OUTPUT_A, RF_OUTPUT_B };

    enum feedback_sel_t { FB_SEL_FUNDAMENTAL, FB_SEL_DIVIDED };

    enum output_power_t { OUTPUT_POWER_M4DBM, OUTPUT_POWER_M1DBM, OUTPUT_POWER_2DBM, OUTPUT_POWER_5DBM };

    enum muxout_t { MUXOUT_3STATE, MUXOUT_DVDD, MUXOUT_DGND, MUXOUT_RDIV, MUXOUT_NDIV, MUXOUT_ALD, MUXOUT_DLD };

    virtual void set_reference_freq(double fref, bool force = false) = 0;

    virtual void set_feedback_select(feedback_sel_t fb_sel) = 0;

    virtual void set_output_power(output_power_t power) = 0;

    virtual void set_output_enable(output_t output, bool enable) = 0;

    virtual void set_muxout_mode(muxout_t mode) = 0;

    virtual double set_frequency(double target_freq, double freq_resolution, bool flush = false) = 0;

    virtual void commit(void) = 0;
};

#endif // INCLUDED_ADF5355_HPP
