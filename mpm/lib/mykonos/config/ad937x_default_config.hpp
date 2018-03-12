//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../adi/t_mykonos.h"

// This file is more or less the static config provided by a run of the eval software
// except all pointers have been changed to nullptr
// Hopefully this helps the compiler use these as purely constants
// The pointers should be filled in if these data structures are to be actually used with the API

static const mykonosRxSettings_t DEFAULT_RX_SETTINGS =
{
    nullptr,        // Rx datapath profile, 3dB corner frequencies, and digital filter enables
    nullptr,        // Rx JESD204b framer configuration structure
    nullptr,        // Rx Gain control settings structure
    nullptr,        // Rx AGC control settings structure
    RX1_RX2,        // The desired Rx Channels to enable during initialization
    0,              // Internal LO = 0, external LO*2 = 1
    3500000000U,    // Rx PLL LO Frequency (internal or external LO)
    0               // Flag to choose if complex baseband or real IF data are selected for Rx and ObsRx paths. Where, if > 0 = real IF data, '0' = zero IF (IQ) data
};

static const mykonosRxProfile_t DEFAULT_RX_PROFILE =
{                   // Rx 100MHz, IQrate 125MSPS, Dec5
    1,              // The divider used to generate the ADC clock
    nullptr,        // Pointer to Rx FIR filter structure
    2,              // Rx FIR decimation (1,2,4)
    5,              // Decimation of Dec5 or Dec4 filter (5,4)
    1,              // If set, and DEC5 filter used, will use a higher rejection DEC5 FIR filter (1=Enabled, 0=Disabled)
    1,              // RX Half band 1 decimation (1 or 2)
    125000,         // Rx IQ data rate in kHz
    100000000,      // The Rx RF passband bandwidth for the profile
    102000,         // Rx BBF 3dB corner in kHz
    NULL            // pointer to custom ADC profile
};

static const mykonosJesd204bFramerConfig_t DEFAULT_FRAMER =
{
    0,              // JESD204B Configuration Bank ID -extension to Device ID (Valid 0..15)
    0,              // JESD204B Configuration Device ID - link identification number. (Valid 0..255)
    0,              // JESD204B Configuration starting Lane ID.  If more than one lane used, each lane will increment from the Lane0 ID. (Valid 0..31)
    4,              // number of ADCs (0, 2, or 4) - 2 ADCs per receive chain
    20,             // number of frames in a multiframe (default=32), F*K must be a multiple of 4. (F=2*M/numberOfLanes)
    1,              // scrambling off if framerScramble= 0, if framerScramble>0 scramble is enabled.
    1,              // 0=use internal SYSREF, 1= use external SYSREF
    0x0F,           // serializerLanesEnabled - bit per lane, [0] = Lane0 enabled, [1] = Lane1 enabled
    0x4B,           // serializerLaneCrossbar
    26,             // serializerAmplitude - default 22 (valid (0-31)
    0,              // preEmphasis - < default 4 (valid 0 - 7)
    0,              // invertLanePolarity - default 0 ([0] will invert lane [0], bit1 will invert lane1)
    0,              // lmfcOffset - LMFC offset value for deterministic latency setting
    0,              // Flag for determining if SYSREF on relink should be set. Where, if > 0 = set, 0 = not set
    0,              // Flag for determining if auto channel select for the xbar should be set. Where, if > 0 = set, '0' = not set
    0,              // Selects SYNCb input source. Where, 0 = use RXSYNCB for this framer, 1 = use OBSRX_SYNCB for this framer
    1,              // Flag for determining if CMOS mode for RX Sync signal is used. Where, if > 0 = CMOS, '0' = LVDS
    0,              // Selects framer bit repeat or oversampling mode for lane rate matching. Where, 0 = bitRepeat mode (changes effective lanerate), 1 = overSample (maintains same lane rate between ObsRx framer and Rx framer and oversamples the ADC samples)
    1               // Flag for determining if API will calculate the appropriate settings for framer lane outputs to physical lanes. Where, if '0' = API will set automatic lane crossbar, '1' = set to manual mode and the value in serializerLaneCrossbar will be used
};

static const mykonosRxGainControl_t DEFAULT_RX_GAIN =
{
    MGC,            // Current Rx gain control mode setting
    255,            // Rx1 Gain Index, can be used in different ways for manual and AGC gain control
    255,            // Rx2 Gain Index, can be used in different ways for manual and AGC gain control
    255,            // Max gain index for the currently loaded Rx1 Gain table
    195,            // Min gain index for the currently loaded Rx1 Gain table
    255,            // Max gain index for the currently loaded Rx2 Gain table
    195,            // Min gain index for the currently loaded Rx2 Gain table
    0,              // Stores Rx1 RSSI value read back from the Mykonos
    0               // Stores Rx2 RSSI value read back from the Mykonos
};

static const mykonosPeakDetAgcCfg_t DEFAULT_RX_PEAK_AGC =
{
    0x1F,   // apdHighThresh:
    0x16,   // apdLowThresh
    0xB5,   // hb2HighThresh
    0x80,   // hb2LowThresh
    0x40,   // hb2VeryLowThresh
    0x06,   // apdHighThreshExceededCnt
    0x04,   // apdLowThreshExceededCnt
    0x06,   // hb2HighThreshExceededCnt
    0x04,   // hb2LowThreshExceededCnt
    0x04,   // hb2VeryLowThreshExceededCnt
    0x4,    // apdHighGainStepAttack
    0x2,    // apdLowGainStepRecovery
    0x4,    // hb2HighGainStepAttack
    0x2,    // hb2LowGainStepRecovery
    0x4,    // hb2VeryLowGainStepRecovery
    0x1,    // apdFastAttack
    0x1,    // hb2FastAttack
    0x1,    // hb2OverloadDetectEnable
    0x1,    // hb2OverloadDurationCnt
    0x1     // hb2OverloadThreshCnt
};

static const mykonosPowerMeasAgcCfg_t DEFAULT_RX_POWER_AGC =
{
    0x01,   // pmdUpperHighThresh
    0x03,   // pmdUpperLowThresh
    0x0C,   // pmdLowerHighThresh
    0x04,   // pmdLowerLowThresh
    0x4,    // pmdUpperHighGainStepAttack
    0x2,    // pmdUpperLowGainStepAttack
    0x2,    // pmdLowerHighGainStepRecovery
    0x4,    // pmdLowerLowGainStepRecovery
    0x08,   // pmdMeasDuration
    0x02    // pmdMeasConfig
};

static const mykonosAgcCfg_t DEFAULT_RX_AGC_CTRL =
{
    255,    // AGC peak wait time
    195,    // agcRx1MinGainIndex
    255,    // agcRx2MaxGainIndex
    195,    // agcRx2MinGainIndex:
    255,    // agcObsRxMaxGainIndex
    203,    // agcObsRxMinGainIndex
    1,      // agcObsRxSelect
    1,      // agcPeakThresholdMode
    1,      // agcLowThsPreventGainIncrease
    30720,  // agcGainUpdateCounter
    3,      // agcSlowLoopSettlingDelay
    2,      // agcPeakWaitTime
    0,      // agcResetOnRxEnable
    0,      // agcEnableSyncPulseForGainCounter
    nullptr,// *peakAgc
    nullptr // *powerAgc
};

static const mykonosTxSettings_t DEFAULT_TX_SETTINGS =
{
    nullptr,        // Tx datapath profile, 3dB corner frequencies, and digital filter enables
    nullptr,        // Mykonos JESD204b deframer config for the Tx data path
    TX1_TX2,        // The desired Tx channels to enable during initialization
    0,              // Internal LO=0, external LO*2 if =1
    3500000000U,    // Tx PLL LO frequency (internal or external LO)
    TXATTEN_0P05_DB,// Initial and current Tx1 Attenuation
    10000,          // Initial and current Tx1 Attenuation mdB
    10000,          // Initial and current Tx2 Attenuation mdB
    nullptr,        // DPD,CLGC,VSWR settings. Only valid for AD9373 device, set pointer to NULL otherwise
    nullptr,        // CLGC Config Structure. Only valid for AD9373 device, set pointer to NULL otherwise
    nullptr         // VSWR Config Structure. Only valid for AD9373 device, set pointer to NULL otherwise
};

static const mykonosTxProfile_t DEFAULT_TX_PROFILE =
{                   // Tx 20/100MHz, IQrate 122.88MHz, Dec5
    DACDIV_2p5,     // The divider used to generate the DAC clock
    nullptr,        // Pointer to Tx FIR filter structure
    2,              // The Tx digital FIR filter interpolation (1,2,4)
    2,              // Tx Halfband1 filter interpolation (1,2)
    1,              // Tx Halfband2 filter interpolation (1,2)
    1,              // TxInputHbInterpolation (1,2)
    125000,         // Tx IQ data rate in kHz
    20000000,       // Primary Signal BW
    102000000,      // The Tx RF passband bandwidth for the profile
    722000,         // The DAC filter 3dB corner in kHz
    51000,          // Tx BBF 3dB corner in kHz
    0               // Enable DPD, only valid for AD9373
};

static const mykonosJesd204bDeframerConfig_t DEFAULT_DEFRAMER =
{
    0,              // bankId extension to Device ID (Valid 0..15)
    0,              // deviceId  link identification number. (Valid 0..255)
    0,              // lane0Id Lane0 ID. (Valid 0..31)
    4,              // M  number of DACss (0, 2, or 4) - 2 DACs per transmit chain
    20,             // K  #frames in a multiframe (default=32), F*K=multiple of 4. (F=2*M/numberOfLanes)
    0,              // Scrambling off if scramble = 0, if framerScramble > 0 scrambling is enabled
    1,              // External SYSREF select. 0 = use internal SYSREF, 1 = external SYSREF
    0x0F,           // Deserializer lane select bit field. Where, [0] = Lane0 enabled, [1] = Lane1 enabled, etc
    0xD2,           // Lane crossbar to map physical lanes to deframer lane inputs [1:0] = Deframer Input 0 Lane section, [3:2] = Deframer Input 1 lane select, etc
    1,              // Equalizer setting. Applied to all deserializer lanes. Range is 0..4
    0,              // PN inversion per each lane.  bit[0] = 1 Invert PN of Lane 0, bit[1] = Invert PN of Lane 1, etc).
    0,              // LMFC offset value to adjust deterministic latency. Range is 0..31
    0,              // Flag for determining if SYSREF on relink should be set. Where, if > 0 = set, '0' = not set
    0,              // Flag for determining if auto channel select for the xbar should be set. Where, if > 0 = set, '0' = not set
    1,              // Flag for determining if CMOS mode for TX Sync signal is used. Where, if > 0 = CMOS, '0' = LVDS
    1,              // Flag for determining if API will calculate the appropriate settings for deframer lane in to physical lanes. Where, if '0' = API will set automatic lane crossbar, '1' = set to manual mode and the value in deserializerLaneCrossbar will be used
};

static const mykonosObsRxSettings_t DEFAULT_ORX_SETTINGS =
{
    nullptr,        // ORx datapath profile, 3dB corner frequencies, and digital filter enables
    nullptr,        // ObsRx gain control settings structure
    nullptr,        // ORx AGC control settings structure
    nullptr,        // Sniffer datapath profile, 3dB corner frequencies, and digital filter enables
    nullptr,        // SnRx gain control settings structure
    nullptr,        // ObsRx JESD204b framer configuration structure
    MYK_ORX1,       // obsRxChannel  TODO: fix this garbage please
    OBSLO_TX_PLL,   // (obsRxLoSource) The Obs Rx mixer can use the Tx Synth(TX_PLL) or Sniffer Synth (SNIFFER_PLL)
    2600000000U,    // SnRx PLL LO frequency in Hz
    0,              // Flag to choose if complex baseband or real IF data are selected for Rx and ObsRx paths. Where if > 0 = real IF data, '0' = complex data
    nullptr,        // Custom Loopback ADC profile to set the bandwidth of the ADC response
    OBS_RXOFF       // Default ObsRx channel to enter when radioOn called
};

static const mykonosJesd204bFramerConfig_t DEFAULT_ORX_FRAMER =
{
    0,              // JESD204B Configuration Bank ID -extension to Device ID (Valid 0..15)
    0,              // JESD204B Configuration Device ID - link identification number. (Valid 0..255)
    0,              // JESD204B Configuration starting Lane ID.  If more than one lane used, each lane will increment from the Lane0 ID. (Valid 0..31)
    2,              // number of ADCs (0, 2, or 4) - 2 ADCs per receive chain
    32,             // number of frames in a multiframe (default=32), F*K must be a multiple of 4. (F=2*M/numberOfLanes)
    1,              // scrambling off if framerScramble= 0, if framerScramble>0 scramble is enabled.
    1,              // 0=use internal SYSREF, 1= use external SYSREF
    0x00,           // serializerLanesEnabled - bit per lane, [0] = Lane0 enabled, [1] = Lane1 enabled
    0xE4,           // Lane crossbar to map framer lane outputs to physical lanes
    22,             // serializerAmplitude - default 22 (valid (0-31)
    4,              // preEmphasis - < default 4 (valid 0 - 7)
    0,              // invertLanePolarity - default 0 ([0] will invert lane [0], bit1 will invert lane1)
    0,              // lmfcOffset - LMFC_Offset offset value for deterministic latency setting
    0,              // Flag for determining if SYSREF on relink should be set. Where, if > 0 = set, 0 = not set
    0,              // Flag for determining if auto channel select for the xbar should be set. Where, if > 0 = set, '0' = not set
    1,              // Selects SYNCb input source. Where, 0 = use RXSYNCB for this framer, 1 = use OBSRX_SYNCB for this framer
    0,              // Flag for determining if CMOS mode for RX Sync signal is used. Where, if > 0 = CMOS, '0' = LVDS
    1,              // Selects framer bit repeat or oversampling mode for lane rate matching. Where, 0 = bitRepeat mode (changes effective lanerate), 1 = overSample (maintains same lane rate between ObsRx framer and Rx framer and oversamples the ADC samples)
    1               // Flag for determining if API will calculate the appropriate settings for framer lane outputs to physical lanes. Where, if '0' = API will set automatic lane crossbar, '1' = set to manual mode and the value in serializerLaneCrossbar will be used
};

static const mykonosORxGainControl_t DEFAULT_ORX_GAIN =
{
    MGC,            // Current ORx gain control mode setting
    255,            // ORx1 Gain Index, can be used in different ways for manual and AGC gain control
    255,            // ORx2 Gain Index, can be used in different ways for manual and AGC gain control
    255,            // Max gain index for the currently loaded ORx Gain table
    237             // Min gain index for the currently loaded ORx Gain table
};

static const mykonosAgcCfg_t DEFAULT_ORX_AGC_CTRL =
{
    255,    // agcRx1MaxGainIndex
    195,    // agcRx1MinGainIndex
    255,    // agcRx2MaxGainIndex
    195,    // agcRx2MinGainIndex:
    255,    // agcObsRxMaxGainIndex
    203,    // agcObsRxMinGainIndex
    1,      // agcObsRxSelect
    1,      // agcPeakThresholdMode
    1,      // agcLowThsPreventGainIncrease
    30720,  // agcGainUpdateCounter
    3,      // agcSlowLoopSettlingDelay
    4,      // agcPeakWaitTime
    0,      // agcResetOnRxEnable
    0,      // agcEnableSyncPulseForGainCounter
    nullptr,// *peakAgc
    nullptr // *powerAgc
};

static const mykonosPeakDetAgcCfg_t DEFAULT_ORX_PEAK_AGC =
{
    0x2A,   // apdHighThresh:
    0x16,   // apdLowThresh
    0xB5,   // hb2HighThresh
    0x72,   // hb2LowThresh
    0x40,   // hb2VeryLowThresh
    0x03,   // apdHighThreshExceededCnt
    0x03,   // apdLowThreshExceededCnt
    0x03,   // hb2HighThreshExceededCnt
    0x03,   // hb2LowThreshExceededCnt
    0x03,   // hb2VeryLowThreshExceededCnt
    0x4,    // apdHighGainStepAttack
    0x2,    // apdLowGainStepRecovery
    0x4,    // hb2HighGainStepAttack
    0x2,    // hb2LowGainStepRecovery
    0x4,    // hb2VeryLowGainStepRecovery
    0x0,    // apdFastAttack
    0x0,    // hb2FastAttack
    0x1,    // hb2OverloadDetectEnable
    0x1,    // hb2OverloadDurationCnt
    0x1     // hb2OverloadThreshCnt
};

static const mykonosPowerMeasAgcCfg_t DEFAULT_ORX_POWER_AGC =
{
    0x01,   // pmdUpperHighThresh
    0x03,   // pmdUpperLowThresh
    0x0C,   // pmdLowerHighThresh
    0x04,   // pmdLowerLowThresh
    0x0,    // pmdUpperHighGainStepAttack
    0x0,    // pmdUpperLowGainStepAttack
    0x0,    // pmdLowerHighGainStepRecovery
    0x0,    // pmdLowerLowGainStepRecovery
    0x08,   // pmdMeasDuration
    0x02    // pmdMeasConfig
};

static const mykonosSnifferGainControl_t DEFAULT_SNIFFER_GAIN =
{
    MGC,    // Current Sniffer gain control mode setting
    255,    // Current Sniffer gain index. Can be used differently for Manual Gain control/AGC
    255,    // Max gain index for the currently loaded Sniffer Gain table
    203     // Min gain index for the currently loaded Sniffer Gain table
};

static const mykonosRxProfile_t DEFAULT_ORX_PROFILE =
{// ORX 100MHz, IQrate 125MSPS, Dec5
    1,              // The divider used to generate the ADC clock
    nullptr,        // Pointer to Rx FIR filter structure or NULL
    2,              // Rx FIR decimation (1,2,4)
    5,              // Decimation of Dec5 or Dec4 filter (5,4)
    0,              // If set, and DEC5 filter used, will use a higher rejection DEC5 FIR filter (1=Enabled, 0=Disabled)
    1,              // RX Half band 1 decimation (1 or 2)
    125000,         // Rx IQ data rate in kHz
    100000000,      // The Rx RF passband bandwidth for the profile
    102000,         // Rx BBF 3dB corner in kHz
    nullptr         // pointer to custom ADC profile
};

static const mykonosArmGpioConfig_t DEFAULT_ARM_GPIO =
{
    1,  // useRx2EnablePin; //!< 0= RX1_ENABLE controls RX1 and RX2, 1 = separate RX1_ENABLE/RX2_ENABLE pins
    1,  // useTx2EnablePin; //!< 0= TX1_ENABLE controls TX1 and TX2, 1 = separate TX1_ENABLE/TX2_ENABLE pins
    0,  // txRxPinMode;     //!< 0= ARM command mode, 1 = Pin mode to power up Tx/Rx chains
    0,  // orxPinMode;      //!< 0= ARM command mode, 1 = Pin mode to power up ObsRx receiver

    //Mykonos ARM input GPIO pins -- Only valid if orxPinMode = 1
    0,  // orxTriggerPin; //!< Select desired GPIO pin (valid 4-15)
    0,  // orxMode2Pin;   //!< Select desired GPIO pin (valid 0-18)
    0,  // orxMode1Pin;   //!< Select desired GPIO pin (valid 0-18)
    0,  // orxMode0Pin;   //!< Select desired GPIO pin (valid 0-18)

    // Mykonos ARM output GPIO pins  --  always available, even when pin mode not enabled
    0,  // rx1EnableAck;  //!< Select desired GPIO pin (0-15), [4] = Output Enable
    0,  // rx2EnableAck;  //!< Select desired GPIO pin (0-15), [4] = Output Enable
    0,  // tx1EnableAck;  //!< Select desired GPIO pin (0-15), [4] = Output Enable
    0,  // tx2EnableAck;  //!< Select desired GPIO pin (0-15), [4] = Output Enable
    0,  // orx1EnableAck;  //!< Select desired GPIO pin (0-15), [4] = Output Enable
    0,  // orx2EnableAck;  //!< Select desired GPIO pin (0-15), [4] = Output Enable
    0,  // srxEnableAck;  //!< Select desired GPIO pin (0-15), [4] = Output Enable
    0   // txObsSelect;   //!< Select desired GPIO pin (0-15), [4] = Output Enable
        // When 2Tx are used with only 1 ORx input, this GPIO tells the BBIC which Tx channel is
        // active for calibrations, so BBIC can route correct RF Tx path into the single ORx input
};

static const mykonosGpio3v3_t DEFAULT_GPIO_3V3 =
{
    0,                      //!< Oe per pin, 1=output, 0 = input
    GPIO3V3_BITBANG_MODE,   //!< Mode for GPIO3V3[3:0]
    GPIO3V3_BITBANG_MODE,   //!< Mode for GPIO3V3[7:4]
    GPIO3V3_BITBANG_MODE,   //!< Mode for GPIO3V3[11:8]
};

static const mykonosGpioLowVoltage_t DEFAULT_GPIO =
{
    0,                      // Oe per pin, 1=output, 0 = input
    GPIO_MONITOR_MODE,      // Mode for GPIO[3:0]
    GPIO_MONITOR_MODE,      // Mode for GPIO[7:4]
    GPIO_MONITOR_MODE,      // Mode for GPIO[11:8]
    GPIO_MONITOR_MODE,      // Mode for GPIO[15:12]
    GPIO_MONITOR_MODE,      // Mode for GPIO[18:16]
};

static const mykonosAuxIo_t DEFAULT_AUX_IO =
{
    0,                          // auxDacEnable uint16_t
    { 0,0,0,0,0,0,0,0,0,0 },    // auxDacValue uint16[10]
    { 0,0,0,0,0,0,0,0,0,0 },    // auxDacSlope uint8[10]
    { 0,0,0,0,0,0,0,0,0,0 },    // auxDacVref uint8[10]
    nullptr,                    // *mykonosGpio3v3_t
    nullptr,                    // *mykonosGpioLowVoltage_t
    nullptr                     // *mykonosArmGpioConfig_t
};

static const mykonosDigClocks_t DEFAULT_CLOCKS =
{
    125000,         // CLKPLL and device reference clock frequency in kHz
    10000000,       // CLKPLL VCO frequency in kHz
    VCODIV_2,       // CLKPLL VCO divider
    4               // CLKPLL high speed clock divider
};


