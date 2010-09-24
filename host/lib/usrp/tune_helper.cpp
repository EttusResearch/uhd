//
// Copyright 2010 Ettus Research LLC
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

#include <uhd/usrp/tune_helper.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <uhd/usrp/dboard_iface.hpp> //unit_t
#include <boost/math/special_functions/sign.hpp>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Tune Helper Functions
 **********************************************************************/
static tune_result_t tune_xx_subdev_and_dxc(
    dboard_iface::unit_t unit,
    wax::obj subdev, wax::obj dxc, size_t chan,
    double target_freq, double lo_offset
){
    wax::obj subdev_freq_proxy = subdev[SUBDEV_PROP_FREQ];
    std::string freq_name = dxc[DSP_PROP_FREQ_SHIFT_NAMES].as<prop_names_t>().at(chan);
    wax::obj dxc_freq_proxy = dxc[named_prop_t(DSP_PROP_FREQ_SHIFT, freq_name)];
    double dxc_sample_rate = dxc[DSP_PROP_CODEC_RATE].as<double>();

    // Ask the d'board to tune as closely as it can to target_freq+lo_offset
    double target_inter_freq = target_freq + lo_offset;
    subdev_freq_proxy = target_inter_freq;
    double actual_inter_freq = subdev_freq_proxy.as<double>();

    //perform the correction correction for dxc rates outside of nyquist
    double delta_freq = std::fmod(target_freq - actual_inter_freq, dxc_sample_rate);
    bool outside_of_nyquist = std::abs(delta_freq) > dxc_sample_rate/2.0;
    double target_dxc_freq = (outside_of_nyquist)?
        boost::math::sign(delta_freq)*dxc_sample_rate - delta_freq : -delta_freq;

    //invert the sign on the dxc freq given the following conditions
    if (unit == dboard_iface::UNIT_TX) target_dxc_freq *= -1.0;

    dxc_freq_proxy = target_dxc_freq;
    double actual_dxc_freq = dxc_freq_proxy.as<double>();

    //load and return the tune result
    tune_result_t tune_result;
    tune_result.target_inter_freq = target_inter_freq;
    tune_result.actual_inter_freq = actual_inter_freq;
    tune_result.target_dsp_freq = target_dxc_freq;
    tune_result.actual_dsp_freq = actual_dxc_freq;
    return tune_result;
}

static double derive_freq_from_xx_subdev_and_dxc(
    dboard_iface::unit_t unit,
    wax::obj subdev, wax::obj dxc, size_t chan
){
    //extract actual dsp and IF frequencies
    double actual_inter_freq = subdev[SUBDEV_PROP_FREQ].as<double>();
    std::string freq_name = dxc[DSP_PROP_FREQ_SHIFT_NAMES].as<prop_names_t>().at(chan);
    double actual_dxc_freq = dxc[named_prop_t(DSP_PROP_FREQ_SHIFT, freq_name)].as<double>();

    //invert the sign on the dxc freq given the following conditions
    if (unit == dboard_iface::UNIT_TX) actual_dxc_freq *= -1.0;

    return actual_inter_freq - actual_dxc_freq;
}

/***********************************************************************
 * RX Tune
 **********************************************************************/
tune_result_t usrp::tune_rx_subdev_and_dsp(
    wax::obj subdev, wax::obj ddc, size_t chan,
    double target_freq, double lo_offset
){
    return tune_xx_subdev_and_dxc(dboard_iface::UNIT_RX, subdev, ddc, chan, target_freq, lo_offset);
}

tune_result_t usrp::tune_rx_subdev_and_dsp(
    wax::obj subdev, wax::obj ddc,
    size_t chan, double target_freq
){
    double lo_offset = 0.0;
    //if the local oscillator will be in the passband, use an offset
    if (subdev[SUBDEV_PROP_USE_LO_OFFSET].as<bool>()){
        lo_offset = 2.0*ddc[DSP_PROP_HOST_RATE].as<double>();
    }
    return tune_rx_subdev_and_dsp(subdev, ddc, chan, target_freq, lo_offset);
}

double usrp::derive_freq_from_rx_subdev_and_dsp(
    wax::obj subdev, wax::obj ddc, size_t chan
){
    return derive_freq_from_xx_subdev_and_dxc(dboard_iface::UNIT_RX, subdev, ddc, chan);
}

/***********************************************************************
 * TX Tune
 **********************************************************************/
tune_result_t usrp::tune_tx_subdev_and_dsp(
    wax::obj subdev, wax::obj duc, size_t chan,
    double target_freq, double lo_offset
){
    return tune_xx_subdev_and_dxc(dboard_iface::UNIT_TX, subdev, duc, chan, target_freq, lo_offset);
}

tune_result_t usrp::tune_tx_subdev_and_dsp(
    wax::obj subdev, wax::obj duc,
    size_t chan, double target_freq
){
    double lo_offset = 0.0;
    //if the local oscillator will be in the passband, use an offset
    if (subdev[SUBDEV_PROP_USE_LO_OFFSET].as<bool>()){
        lo_offset = 2.0*duc[DSP_PROP_HOST_RATE].as<double>();
    }
    return tune_tx_subdev_and_dsp(subdev, duc, chan, target_freq, lo_offset);
}

double usrp::derive_freq_from_tx_subdev_and_dsp(
    wax::obj subdev, wax::obj duc, size_t chan
){
    return derive_freq_from_xx_subdev_and_dxc(dboard_iface::UNIT_TX, subdev, duc, chan);
}
