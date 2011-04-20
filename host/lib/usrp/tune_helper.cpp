//
// Copyright 2010-2011 Ettus Research LLC
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
#include <uhd/utils/algorithm.hpp>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Tune Helper Functions
 **********************************************************************/
static tune_result_t tune_xx_subdev_and_dsp(
    dboard_iface::unit_t unit,
    wax::obj subdev, wax::obj dsp,
    const tune_request_t &tune_request
){
    wax::obj subdev_freq_proxy = subdev[SUBDEV_PROP_FREQ];
    wax::obj dsp_freq_proxy = dsp[DSP_PROP_FREQ_SHIFT];

    //------------------------------------------------------------------
    //-- calculate the LO offset, only used with automatic policy
    //------------------------------------------------------------------
    double lo_offset = 0.0;
    if (subdev[SUBDEV_PROP_USE_LO_OFFSET].as<bool>()){
        //If the local oscillator will be in the passband, use an offset.
        //But constrain the LO offset by the width of the filter bandwidth.
        double rate = dsp[DSP_PROP_HOST_RATE].as<double>();
        double bw = subdev[SUBDEV_PROP_BANDWIDTH].as<double>();
        if (bw > rate) lo_offset = std::min((bw - rate)/2, rate/2);
    }

    //------------------------------------------------------------------
    //-- set the intermediate frequency depending upon the IF policy
    //------------------------------------------------------------------
    double target_inter_freq = 0.0;
    switch (tune_request.inter_freq_policy){
    case tune_request_t::POLICY_AUTO:
        target_inter_freq = tune_request.target_freq + lo_offset;
        subdev_freq_proxy = target_inter_freq;
        break;

    case tune_request_t::POLICY_MANUAL:
        target_inter_freq = tune_request.inter_freq;
        subdev_freq_proxy = target_inter_freq;
        break;

    case tune_request_t::POLICY_NONE: break; //does not set
    }
    double actual_inter_freq = subdev_freq_proxy.as<double>();

    //------------------------------------------------------------------
    //-- calculate the dsp freq, only used with automatic policy
    //------------------------------------------------------------------
    double target_dsp_freq = actual_inter_freq - tune_request.target_freq;

    //invert the sign on the dsp freq given the following conditions
    if (unit == dboard_iface::UNIT_TX) target_dsp_freq *= -1.0;

    //------------------------------------------------------------------
    //-- set the dsp frequency depending upon the dsp frequency policy
    //------------------------------------------------------------------
    switch (tune_request.dsp_freq_policy){
    case tune_request_t::POLICY_AUTO:
        dsp_freq_proxy = target_dsp_freq;
        break;

    case tune_request_t::POLICY_MANUAL:
        target_dsp_freq = tune_request.dsp_freq;
        dsp_freq_proxy = target_dsp_freq;
        break;

    case tune_request_t::POLICY_NONE: break; //does not set
    }
    double actual_dsp_freq = dsp_freq_proxy.as<double>();

    //------------------------------------------------------------------
    //-- load and return the tune result
    //------------------------------------------------------------------
    tune_result_t tune_result;
    tune_result.target_inter_freq = target_inter_freq;
    tune_result.actual_inter_freq = actual_inter_freq;
    tune_result.target_dsp_freq = target_dsp_freq;
    tune_result.actual_dsp_freq = actual_dsp_freq;
    return tune_result;
}

static double derive_freq_from_xx_subdev_and_dsp(
    dboard_iface::unit_t unit, wax::obj subdev, wax::obj dsp
){
    //extract actual dsp and IF frequencies
    double actual_inter_freq = subdev[SUBDEV_PROP_FREQ].as<double>();
    double actual_dsp_freq = dsp[DSP_PROP_FREQ_SHIFT].as<double>();

    //invert the sign on the dsp freq given the following conditions
    if (unit == dboard_iface::UNIT_TX) actual_dsp_freq *= -1.0;

    return actual_inter_freq - actual_dsp_freq;
}

/***********************************************************************
 * RX Tune
 **********************************************************************/
tune_result_t usrp::tune_rx_subdev_and_dsp(
    wax::obj subdev, wax::obj ddc, const tune_request_t &tune_request
){
    return tune_xx_subdev_and_dsp(dboard_iface::UNIT_RX, subdev, ddc, tune_request);
}

double usrp::derive_freq_from_rx_subdev_and_dsp(
    wax::obj subdev, wax::obj ddc
){
    return derive_freq_from_xx_subdev_and_dsp(dboard_iface::UNIT_RX, subdev, ddc);
}

/***********************************************************************
 * TX Tune
 **********************************************************************/
tune_result_t usrp::tune_tx_subdev_and_dsp(
    wax::obj subdev, wax::obj duc, const tune_request_t &tune_request
){
    return tune_xx_subdev_and_dsp(dboard_iface::UNIT_TX, subdev, duc, tune_request);
}

double usrp::derive_freq_from_tx_subdev_and_dsp(
    wax::obj subdev, wax::obj duc
){
    return derive_freq_from_xx_subdev_and_dsp(dboard_iface::UNIT_TX, subdev, duc);
}
