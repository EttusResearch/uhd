//
// Copyright 2017 Ettus Research (National Instruments)
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

#pragma once

#include "../adi/t_mykonos.h"
#include "ad937x_fir.hpp"
#include <boost/noncopyable.hpp>

// This class exists so that the entire mykonos config can be allocated and managed together
class ad937x_config_t  : public boost::noncopyable
{
    // The top level device struct contains all other structs, so everything is technically "public"
    // a user could technically modify the pointers in the structs, but we have no way of preventing that
public:
    ad937x_config_t(spiSettings_t* sps);
    //mykonosDevice_t * const device = &_device;
    mykonosDevice_t * device;

    ad937x_fir rx_fir_config;
    ad937x_fir tx_fir_config;
private:
    mykonosDevice_t _device;

    ad937x_fir _orx_fir_config;
    ad937x_fir _sniffer_rx_fir_config;

    // in general, this organization stinks
    // TODO: group and make more sense of these fields and pointers
    mykonosRxSettings_t _rx;
    mykonosTxSettings_t _tx;
    mykonosObsRxSettings_t _obsRx;
    mykonosAuxIo_t _auxIo;
    mykonosDigClocks_t _clocks;

    mykonosRxProfile_t _rxProfile;
    mykonosJesd204bFramerConfig_t _framer;
    mykonosRxGainControl_t _rxGainCtrl;
    mykonosAgcCfg_t _rxAgcCtrl;
    mykonosPeakDetAgcCfg_t _rxPeakAgc;
    mykonosPowerMeasAgcCfg_t _rxPowerAgc;

    mykonosTxProfile_t _txProfile;
    mykonosJesd204bDeframerConfig_t _deframer;

    mykonosRxProfile_t _orxProfile;
    mykonosORxGainControl_t _orxGainCtrl;
    mykonosAgcCfg_t _orxAgcCtrl;
    mykonosPeakDetAgcCfg_t _orxPeakAgc;
    mykonosPowerMeasAgcCfg_t _orxPowerAgc;

    mykonosRxProfile_t _snifferProfile;
    mykonosSnifferGainControl_t _snifferGainCtrl;
    mykonosJesd204bFramerConfig_t _orxFramer;

    mykonosGpio3v3_t _gpio3v3;
    mykonosGpioLowVoltage_t _gpio;
    mykonosArmGpioConfig_t _armGpio;

    void _init_pointers();
    void _assign_firs();
    void _assign_default_configuration();
};
