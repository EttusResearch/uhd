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

ad937x_config_t::ad937x_config_t(spiSettings_t* sps)    
{
    _device.spiSettings = sps;

    _assign_default_configuration();
    _init_pointers();
}

void ad937x_config_t::_assign_default_configuration()
{
    // This is a pretty poor way of doing this, but the ADI structs force you
    // to write spaghetti code sometimes.  This sets none of the required pointers,
    // relying on a call to _init_pointers() to set all of them after the fact.
    // TODO: do this better
    _rx = DEFAULT_RX_SETTINGS;

    _rxProfile = DEFAULT_RX_PROFILE;
    _framer = DEFAULT_FRAMER;
    _rxGainCtrl = DEFAULT_RX_GAIN;
    _rxPeakAgc = DEFAULT_RX_PEAK_AGC;
    _rxPowerAgc = DEFAULT_RX_POWER_AGC;
    _rxAgcCtrl = DEFAULT_RX_AGC_CTRL;

    _tx = DEFAULT_TX_SETTINGS;

    _txProfile = DEFAULT_TX_PROFILE;
    _deframer = DEFAULT_DEFRAMER;

    // TODO: Likely an ADI bug.
    // The TX bring up, for no reason, requires a valid ORX profile
    // https://github.com/EttusResearch/uhddev/blob/eb2bcf9001b8e39eca8e62f0d6d5283895fae50d/embedded/libraries/mykonos/adi/mykonos.c#L16236

    _obsRx = DEFAULT_ORX_SETTINGS;
    _orxFramer = DEFAULT_ORX_FRAMER;
    _orxProfile = DEFAULT_ORX_PROFILE;
    _orxGainCtrl = DEFAULT_ORX_GAIN;
    _orxPeakAgc = DEFAULT_ORX_PEAK_AGC;
    _orxPowerAgc = DEFAULT_ORX_POWER_AGC;
    _orxAgcCtrl = DEFAULT_ORX_AGC_CTRL;

    _armGpio = DEFAULT_ARM_GPIO;
    _gpio3v3 = DEFAULT_GPIO_3V3;
    _gpio = DEFAULT_GPIO;
    _auxIo = DEFAULT_AUX_IO;

    _clocks = DEFAULT_CLOCKS;

    _assign_firs();
}

void ad937x_config_t::_init_pointers()
{
    _device.spiSettings = &_spiSettings;
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
    _obsRx.orxAgcCtrl = nullptr;
    _obsRx.framer = &_orxFramer;

    _auxIo.gpio3v3 = &_gpio3v3;
    _auxIo.gpio = &_gpio;
    _auxIo.armGpio = &_armGpio;
}

void ad937x_config_t::_assign_firs()
{
    // TODO: get default filters here
    tx_fir_config.set_fir(6, { 32, 0 });
    rx_fir_config.set_fir( -6,{ 48, 0 });
    _orx_fir_config.set_fir( -6, { 48, 0 });
    _sniffer_rx_fir_config.set_fir( -6, { 48, 0 });
}

