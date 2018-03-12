//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "ad937x_config_t.hpp"
#include "ad937x_default_config.hpp"

const int16_t ad937x_config_t::DEFAULT_TX_FIR[DEFAULT_TX_FIR_SIZE] =
    {   -94,  -26,  282,  177, -438, -368,  756,  732,-1170,-1337, 1758, 2479,-2648,-5088, 4064,16760,
      16759, 4110,-4881,-2247, 2888, 1917,-1440,-1296,  745,  828, -358, -474,  164,  298,  -16,  -94 };

const int16_t ad937x_config_t::DEFAULT_TX_FIR_15366[DEFAULT_TX_FIR_SIZE] =
    {     4,  -16,   -5,   75,  -13, -229,   85,  547, -293,-1158,  738, 2290,-1640,-4805, 3687,17108,
      17108, 3687,-4805,-1640, 2290,  738,-1158, -293,  547,   85, -229,  -13,   75,   -5,  -16,    4 };

const int16_t ad937x_config_t::DEFAULT_RX_FIR[DEFAULT_RX_FIR_SIZE] =
    {   -20,    6,   66,   22, -128,  -54,  240,  126, -402, -248,  634,  444, -956, -756, 1400, 1244,
      -2028,-2050, 2978, 3538,-4646,-7046, 9536,30880,30880, 9536,-7046,-4646, 3538, 2978,-2050,-2028,
       1244, 1400, -756, -956,  444,  634, -248, -402,  126,  240,  -54, -128,   22,   66,    6,  -20 };

const int16_t ad937x_config_t::DEFAULT_RX_FIR_15366[DEFAULT_RX_FIR_SIZE] =
    {   -16,  -22,   18,   74,   24, -132, -152,  132,  372,   38, -598, -474,  638, 1178, -206,-1952,
       -984, 2362, 3152,-1612,-6544,-2164,12806,26836,26836,12806,-2164,-6544,-1612, 3152, 2362, -984,
      -1952, -206, 1178,  638, -474, -598,   38,  372,  132, -152, -132,   24,   74,   18,  -22,  -16 };

const int16_t ad937x_config_t::DEFAULT_OBSRX_FIR[DEFAULT_RX_FIR_SIZE] =
    {   -14,  -19,   44,   41,  -89,  -95,  175,  178, -303, -317,  499,  527, -779, -843, 1184, 1317,
      -1781,-2059, 2760, 3350,-4962,-7433, 9822,32154,32154, 9822,-7433,-4962, 3350, 2760,-2059,-1781,
       1317, 1184, -843, -779,  527,  499, -317, -303,  178,  175,  -95,  -89,   41,   44,  -19,  -14 };

const int16_t ad937x_config_t::DEFAULT_OBSRX_FIR_15366[DEFAULT_RX_FIR_SIZE] =
    {    -2,    3,   12,  -19,  -28,   44,   74,  -92, -169,  150,  353, -203, -671,  203, 1179,  -66,
      -1952, -347, 3153, 1307,-5595,-4820,11323,29525,29525,11323,-4820,-5595, 1307, 3153, -347,-1952,
        -66, 1179,  203, -671, -203,  353,  150, -169,  -92,   74,   44,  -28,  -19,   12,    3,   -2 };

const int16_t ad937x_config_t::DEFAULT_SNIFFER_FIR[DEFAULT_RX_FIR_SIZE] =
    {    -1,   -5,  -14,  -23,  -16,   24,   92,  137,   80, -120, -378, -471, -174,  507, 1174, 1183,
         98,-1771,-3216,-2641,  942, 7027,13533,17738,17738,13533, 7027,  942,-2641,-3216,-1771,   98,
       1183, 1174,  507, -174, -471, -378, -120,   80,  137,   92,   24,  -16,  -23,  -14,   -5,   -1 };

const int16_t ad937x_config_t::DEFAULT_SNIFFER_FIR_15366[DEFAULT_RX_FIR_SIZE] =
    {   10,    31,   59,   71,   30,  -92, -283, -456, -466, -175,  440, 1192, 1683, 1444,  198,-1871,
     -3988, -4942,-3512,  958, 8118,16519,23993,28395,28395,23993,16519, 8118,  958,-3512,-4942,-3988,
     -1871,   198, 1444, 1683, 1192,  440, -175, -466, -456, -283,  -92,   30,   71,   59,   31,   10 };

ad937x_config_t::ad937x_config_t(spiSettings_t* sps, const size_t deserializer_lane_xbar) :
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

    tx_fir_config(DEFAULT_TX_FIR_GAIN, std::vector<int16_t>(DEFAULT_TX_FIR, DEFAULT_TX_FIR + DEFAULT_TX_FIR_SIZE)),
    rx_fir_config(DEFAULT_RX_FIR_GAIN, std::vector<int16_t>(DEFAULT_RX_FIR, DEFAULT_RX_FIR + DEFAULT_RX_FIR_SIZE)),
    _orx_fir_config(DEFAULT_RX_FIR_GAIN, std::vector<int16_t>(DEFAULT_OBSRX_FIR, DEFAULT_OBSRX_FIR + DEFAULT_RX_FIR_SIZE)),
    _sniffer_rx_fir_config(DEFAULT_RX_FIR_GAIN, std::vector<int16_t>(DEFAULT_SNIFFER_FIR, DEFAULT_SNIFFER_FIR + DEFAULT_RX_FIR_SIZE))
{
    _device.spiSettings = sps;
    _deframer.deserializerLaneCrossbar = deserializer_lane_xbar;

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

