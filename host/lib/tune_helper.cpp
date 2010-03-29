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

#include <uhd/utils/tune_helper.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/props.hpp>
#include <cmath>

using namespace uhd;

/***********************************************************************
 * Tune Helper Function
 **********************************************************************/
static tune_result_t tune_xx_subdev_and_dxc(
    bool is_tx,
    wax::obj subdev, wax::obj dxc,
    double target_freq, double lo_offset
){
    wax::obj subdev_freq_proxy = subdev[SUBDEV_PROP_FREQ];
    bool subdev_quadrature = subdev[SUBDEV_PROP_QUADRATURE].as<bool>();
    bool subdev_spectrum_inverted = subdev[SUBDEV_PROP_SPECTRUM_INVERTED].as<bool>();
    wax::obj dxc_freq_proxy = dxc[std::string("freq")];
    double dxc_sample_rate = dxc[std::string("if_rate")].as<double>();

    // Ask the d'board to tune as closely as it can to target_freq+lo_offset
    double target_inter_freq = target_freq + lo_offset;
    subdev_freq_proxy = target_inter_freq;
    double actual_inter_freq = subdev_freq_proxy.as<double>();

    // Calculate the DDC setting that will downconvert the baseband from the
    // daughterboard to our target frequency.
    double delta_freq = target_freq - actual_inter_freq;
    double delta_sign = std::signum(delta_freq);
    delta_freq *= delta_sign;
    delta_freq = std::fmod(delta_freq, dxc_sample_rate);
    bool inverted = delta_freq > dxc_sample_rate/2.0;
    double target_dxc_freq = inverted? (delta_freq - dxc_sample_rate) : (-delta_freq);
    target_dxc_freq *= delta_sign;

    // If the spectrum is inverted, and the daughterboard doesn't do
    // quadrature downconversion, we can fix the inversion by flipping the
    // sign of the dxc_freq...  (This only happens using the basic_rx board)
    if (subdev_spectrum_inverted){
        inverted = not inverted;
    }
    if (inverted and not subdev_quadrature){
        target_dxc_freq *= -1.0;
        inverted = not inverted;
    }
    // down conversion versus up conversion, fight!
    // your mother is ugly and your going down...
    target_dxc_freq *= (is_tx)? -1.0 : +1.0;

    dxc_freq_proxy = target_dxc_freq;
    double actual_dxc_freq = dxc_freq_proxy.as<double>();

    //return some kind of tune result tuple/struct
    tune_result_t tune_result;
    tune_result.target_inter_freq = target_inter_freq;
    tune_result.actual_inter_freq = actual_inter_freq;
    tune_result.target_dxc_freq = target_dxc_freq;
    tune_result.actual_dxc_freq = actual_dxc_freq;
    tune_result.spectrum_inverted = inverted;
    return tune_result;
}

/***********************************************************************
 * RX Tune
 **********************************************************************/
tune_result_t uhd::tune_rx_subdev_and_ddc(
    wax::obj subdev, wax::obj ddc,
    double target_freq, double lo_offset
){
    bool is_tx = false;
    return tune_xx_subdev_and_dxc(is_tx, subdev, ddc, target_freq, lo_offset);
}

tune_result_t uhd::tune_rx_subdev_and_ddc(
    wax::obj subdev, wax::obj ddc,
    double target_freq
){
    double lo_offset = 0.0;
    //if the local oscillator will be in the passband, use an offset
    if (subdev[SUBDEV_PROP_LO_INTERFERES].as<bool>()){
        lo_offset = 2.0*ddc[std::string("bb_rate")].as<double>();
    }
    return tune_rx_subdev_and_ddc(subdev, ddc, target_freq, lo_offset);
}

/***********************************************************************
 * TX Tune
 **********************************************************************/
tune_result_t uhd::tune_tx_subdev_and_duc(
    wax::obj subdev, wax::obj duc,
    double target_freq, double lo_offset
){
    bool is_tx = true;
    return tune_xx_subdev_and_dxc(is_tx, subdev, duc, target_freq, lo_offset);
}

tune_result_t uhd::tune_tx_subdev_and_duc(
    wax::obj subdev, wax::obj duc,
    double target_freq
){
    double lo_offset = 0.0;
    //if the local oscillator will be in the passband, use an offset
    if (subdev[SUBDEV_PROP_LO_INTERFERES].as<bool>()){
        lo_offset = 2.0*duc[std::string("bb_rate")].as<double>();
    }
    return tune_tx_subdev_and_duc(subdev, duc, target_freq, lo_offset);
}
