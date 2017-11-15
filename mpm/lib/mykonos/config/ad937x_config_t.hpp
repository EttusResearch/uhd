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
// Allocates and links the entire mykonos config struct in a single class
class ad937x_config_t  : public boost::noncopyable
{
public:
    ad937x_config_t(spiSettings_t* sps);
    mykonosDevice_t * device;

    ad937x_fir rx_fir_config;
    ad937x_fir tx_fir_config;

    static const size_t DEFAULT_TX_FIR_SIZE = 32;
    static const size_t DEFAULT_RX_FIR_SIZE = 48;

    static const int32_t DEFAULT_TX_FIR_GAIN = 6;
    static const int32_t DEFAULT_RX_FIR_GAIN = -6;

    static const int16_t DEFAULT_TX_FIR[DEFAULT_TX_FIR_SIZE];
    static const int16_t DEFAULT_RX_FIR[DEFAULT_RX_FIR_SIZE];
    static const int16_t DEFAULT_OBSRX_FIR[DEFAULT_RX_FIR_SIZE];
    static const int16_t DEFAULT_SNIFFER_FIR[DEFAULT_RX_FIR_SIZE];
    void    set_rx_pll_use_external_lo(uint8_t val);
    uint8_t get_rx_pll_use_external_lo();
    void    set_tx_pll_use_external_lo(uint8_t val);
    uint8_t get_tx_pll_use_external_lo();

private:
    // The top level device struct is non-const and contains all other structs, so everything is "public"
    // a user could technically modify the pointers in the structs, but we have no way of preventing that
    mykonosDevice_t _device;

    ad937x_fir _orx_fir_config;
    ad937x_fir _sniffer_rx_fir_config;

    // General structs
    mykonosRxSettings_t _rx;
    mykonosTxSettings_t _tx;
    mykonosObsRxSettings_t _obsRx;
    mykonosAuxIo_t _auxIo;
    mykonosDigClocks_t _clocks;

    // RX structs
    mykonosRxProfile_t _rxProfile;
    mykonosJesd204bFramerConfig_t _framer;
    mykonosRxGainControl_t _rxGainCtrl;
    mykonosAgcCfg_t _rxAgcCtrl;
    mykonosPeakDetAgcCfg_t _rxPeakAgc;
    mykonosPowerMeasAgcCfg_t _rxPowerAgc;

    // TX structs
    mykonosTxProfile_t _txProfile;
    mykonosJesd204bDeframerConfig_t _deframer;

    // ObsRX structs
    mykonosRxProfile_t _orxProfile;
    mykonosORxGainControl_t _orxGainCtrl;
    mykonosAgcCfg_t _orxAgcCtrl;
    mykonosPeakDetAgcCfg_t _orxPeakAgc;
    mykonosPowerMeasAgcCfg_t _orxPowerAgc;

    // Sniffer RX structs
    mykonosRxProfile_t _snifferProfile;
    mykonosSnifferGainControl_t _snifferGainCtrl;
    mykonosJesd204bFramerConfig_t _orxFramer;

    // GPIO structs
    mykonosGpio3v3_t _gpio3v3;
    mykonosGpioLowVoltage_t _gpio;
    mykonosArmGpioConfig_t _armGpio;

    // private initialization helper functions
    void _init_pointers();
};
