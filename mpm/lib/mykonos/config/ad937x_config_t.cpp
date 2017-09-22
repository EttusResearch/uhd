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

#include "ad937x_config_t.hpp"
#include "ad937x_default_config.hpp"

const int16_t ad937x_config_t::DEFAULT_TX_FIR[DEFAULT_TX_FIR_SIZE] =
    {   -94,  -26,  282,  177, -438, -368,  756,  732,-1170,-1337, 1758, 2479,-2648,-5088, 4064,16760,
      16759, 4110,-4881,-2247, 2888, 1917,-1440,-1296,  745,  828, -358, -474,  164,  298,  -16,  -94 };
const int16_t ad937x_config_t::DEFAULT_RX_FIR[DEFAULT_RX_FIR_SIZE] =
    {   -20,    6,   66,   22, -128,  -54,  240,  126, -402, -248,  634,  444, -956, -756, 1400, 1244,
      -2028,-2050, 2978, 3538,-4646,-7046, 9536,30880,30880, 9536,-7046,-4646, 3538, 2978,-2050,-2028,
       1244, 1400, -756, -956,  444,  634, -248, -402,  126,  240,  -54, -128,   22,   66,    6,  -20 };
const int16_t ad937x_config_t::DEFAULT_OBSRX_FIR[DEFAULT_RX_FIR_SIZE] =
    {   -14,  -19,   44,   41,  -89,  -95,  175,  178, -303, -317,  499,  527, -779, -843, 1184, 1317,
      -1781,-2059, 2760, 3350,-4962,-7433, 9822,32154,32154, 9822,-7433,-4962, 3350, 2760,-2059,-1781,
       1317, 1184, -843, -779,  527,  499, -317, -303,  178,  175,  -95,  -89,   41,   44,  -19,  -14 };
const int16_t ad937x_config_t::DEFAULT_SNIFFER_FIR[DEFAULT_RX_FIR_SIZE] =
    {    -1,   -5,  -14,  -23,  -16,   24,   92,  137,   80, -120, -378, -471, -174,  507, 1174, 1183,
         98,-1771,-3216,-2641,  942, 7027,13533,17738,17738,13533, 7027,  942,-2641,-3216,-1771,   98,
       1183, 1174,  507, -174, -471, -378, -120,   80,  137,   92,   24,  -16,  -23,  -14,   -5,   -1 };

ad937x_config_t::ad937x_config_t(spiSettings_t* sps) :
    _rx(DEFAULT_RX_SETTINGS),
    _rxProfile(DEFAULT_RX_PROFILE),
    _framer(DEFAULT_FRAMER),
    _rxGainCtrl(DEFAULT_RX_GAIN),
    _rxPeakAgc(DEFAULT_RX_PEAK_AGC),
    _rxPowerAgc(DEFAULT_RX_POWER_AGC),
    _rxAgcCtrl(DEFAULT_RX_AGC_CTRL),

    _tx(DEFAULT_TX_SETTINGS),
    _txProfile(DEFAULT_TX_PROFILE),
    _deframer(DEFAULT_DEFRAMER),

    // TODO: Remove if ADI ever fixes this
    // The TX bring up requires a valid ORX profile
    // https://github.com/EttusResearch/uhddev/blob/f0f8f58471c3fed94279c32f00e9f8da7db40efd/mpm/lib/mykonos/adi/mykonos.c#L16590

    _obsRx(DEFAULT_ORX_SETTINGS),
    _orxFramer(DEFAULT_ORX_FRAMER),
    _orxProfile(DEFAULT_ORX_PROFILE),
    _orxGainCtrl(DEFAULT_ORX_GAIN),
    _orxPeakAgc(DEFAULT_ORX_PEAK_AGC),
    _orxPowerAgc(DEFAULT_ORX_POWER_AGC),
    _orxAgcCtrl(DEFAULT_ORX_AGC_CTRL),

    // TODO: Remove if ADI ever fixes this
    // ORX bring up requires a valid sniffer gain control struct
    // https://github.com/EttusResearch/uhddev/blob/f0f8f58471c3fed94279c32f00e9f8da7db40efd/mpm/lib/mykonos/adi/mykonos.c#L5752

    _snifferGainCtrl(DEFAULT_SNIFFER_GAIN),

    _armGpio(DEFAULT_ARM_GPIO),
    _gpio3v3(DEFAULT_GPIO_3V3),
    _gpio(DEFAULT_GPIO),

    _auxIo(DEFAULT_AUX_IO),
    _clocks(DEFAULT_CLOCKS),

    tx_fir_config(6, std::vector<int16_t>(DEFAULT_TX_FIR, DEFAULT_TX_FIR + DEFAULT_TX_FIR_SIZE)),
    rx_fir_config(-6, std::vector<int16_t>(DEFAULT_RX_FIR, DEFAULT_RX_FIR + DEFAULT_RX_FIR_SIZE)),
    _orx_fir_config(-6, std::vector<int16_t>(DEFAULT_OBSRX_FIR, DEFAULT_OBSRX_FIR + DEFAULT_RX_FIR_SIZE)),
    _sniffer_rx_fir_config(-6, std::vector<int16_t>(DEFAULT_SNIFFER_FIR, DEFAULT_SNIFFER_FIR + DEFAULT_RX_FIR_SIZE))
{
    _device.spiSettings = sps;

    _init_pointers();

    device = &_device;
}

// This function sets up all the pointers in all of our local members that represent the device struct
// This function should only be called during construction.
void ad937x_config_t::_init_pointers()
{
    _device.rx = &_rx;
    _device.tx = &_tx;
    _device.obsRx = &_obsRx;
    _device.auxIo = &_auxIo;
    _device.clocks = &_clocks;

    _rx.rxProfile = &_rxProfile;
    _rx.framer = &_framer;
    _rx.rxGainCtrl = &_rxGainCtrl;
    _rx.rxAgcCtrl = &_rxAgcCtrl;
    _rxProfile.rxFir = rx_fir_config.fir;
    _rxProfile.customAdcProfile = nullptr;
    _rxAgcCtrl.peakAgc = &_rxPeakAgc;
    _rxAgcCtrl.powerAgc = &_rxPowerAgc;

    _tx.txProfile = &_txProfile;
    _txProfile.txFir = tx_fir_config.fir;
    _tx.deframer = &_deframer;

    // AD9373
    _tx.dpdConfig = nullptr;
    _tx.clgcConfig = nullptr;
    _tx.vswrConfig = nullptr;

    // TODO: ideally we set none of this information and leave the profile as nullptr
    // Check that the API supports this
    _obsRx.orxProfile = &_orxProfile;
    _obsRx.orxGainCtrl = &_orxGainCtrl;
    _obsRx.orxAgcCtrl = &_orxAgcCtrl;
    _orxProfile.rxFir = _orx_fir_config.fir;
    _orxProfile.customAdcProfile = nullptr;
    _orxAgcCtrl.peakAgc = &_orxPeakAgc;
    _orxAgcCtrl.powerAgc = &_orxPowerAgc;

    _obsRx.snifferProfile = &_snifferProfile;
    _snifferProfile.rxFir = _sniffer_rx_fir_config.fir;
    _obsRx.snifferGainCtrl = &_snifferGainCtrl;
    // sniffer has no AGC ctrl, so leave as null
    _obsRx.framer = &_orxFramer;

    _auxIo.gpio3v3 = &_gpio3v3;
    _auxIo.gpio = &_gpio;
    _auxIo.armGpio = &_armGpio;
}

