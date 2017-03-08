#pragma once

#include "../adi/t_mykonos.h"
#include "ad937x_fir.h"
#include <boost/noncopyable.hpp>

// This class exists so that the entire mykonos config can be allocated and managed together
class ad937x_device  : public boost::noncopyable
{
    // The top level device struct contains all other structs, so everything is technically "public"
    // a user could technically modify the pointers in the structs, but we have no way of preventing that
public:
    ad937x_device();
    mykonosDevice_t * const device = &_device;

private:
    mykonosDevice_t _device;

    // in general, this organization stinks
    // TODO: group and make more sense of these fields and pointers
    spiSettings_t _spiSettings;
    mykonosRxSettings_t _rx;
    mykonosTxSettings_t _tx;
    mykonosObsRxSettings_t _obsRx;
    mykonosAuxIo_t _auxIo;
    mykonosDigClocks_t _clocks;

    ad937x_fir _rx_fir_config;
    mykonosRxProfile_t _rxProfile;
    std::vector<uint16_t> _customRxAdcProfile;
    mykonosJesd204bFramerConfig_t _framer;
    mykonosRxGainControl_t _rxGainCtrl;
    mykonosAgcCfg_t _rxAgcCtrl;
    mykonosPeakDetAgcCfg_t _rxPeakAgc;
    mykonosPowerMeasAgcCfg_t _rxPowerAgc;

    ad937x_fir _tx_fir_config;
    mykonosTxProfile_t _txProfile;
    mykonosJesd204bDeframerConfig_t _deframer;
    mykonosDpdConfig_t _dpdConfig;
    mykonosClgcConfig_t _clgcConfig;
    mykonosVswrConfig_t _vswrConfig;

    ad937x_fir _orx_fir_config;
    mykonosRxProfile_t _orxProfile;
    mykonosORxGainControl_t _orxGainCtrl;
    mykonosAgcCfg_t _orxAgcCtrl;
    mykonosPeakDetAgcCfg_t _orxPeakAgc;
    mykonosPowerMeasAgcCfg_t _orxPowerAgc;

    ad937x_fir _sniffer_rx_fir_config;
    mykonosRxProfile_t _snifferProfile;
    mykonosSnifferGainControl_t _snifferGainCtrl;
    mykonosJesd204bFramerConfig_t _orxFramer;
    std::vector<uint16_t> _customORxAdcProfile;

    mykonosGpio3v3_t _gpio3v3;
    mykonosGpioLowVoltage_t _gpio;
    mykonosArmGpioConfig_t _armGpio;

    void _init_pointers();
    void _assign_firs();
    void _assign_default_configuration();
};
