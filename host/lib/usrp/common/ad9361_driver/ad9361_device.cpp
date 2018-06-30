//
// Copyright 2014 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "ad9361_filter_taps.h"
#include "ad9361_gain_tables.h"
#include "ad9361_synth_lut.h"
#include "ad9361_client.h"
#include "ad9361_device.h"
#define _USE_MATH_DEFINES
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>

#include <boost/scoped_array.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions.hpp>
#include <chrono>
#include <thread>
#include <cmath>
#include <stdint.h>

////////////////////////////////////////////////////////////
// the following macros evaluate to a compile time constant
// macros By Tom Torfs - donated to the public domain

/* turn a numeric literal into a hex constant
(avoids problems with leading zeroes)
8-bit constants max value 0x11111111, always fits in unsigned long
*/
#define HEX__(n) 0x##n##LU

/* 8-bit conversion function */
#define B8__(x) ((x&0x0000000FLU)?1:0) \
+((x&0x000000F0LU)?2:0) \
+((x&0x00000F00LU)?4:0) \
+((x&0x0000F000LU)?8:0) \
+((x&0x000F0000LU)?16:0) \
+((x&0x00F00000LU)?32:0) \
+((x&0x0F000000LU)?64:0) \
+((x&0xF0000000LU)?128:0)

/* for upto 8-bit binary constants */
#define B8(d) ((unsigned char)B8__(HEX__(d)))
////////////////////////////////////////////////////////////


namespace uhd { namespace usrp {

/* This is a simple comparison for very large double-precision floating
 * point numbers. It is used to prevent re-tunes for frequencies that are
 * the same but not 'exactly' because of data precision issues. */
// TODO: see if we can avoid the need for this function
int freq_is_nearly_equal(double a, double b) {
    return std::max(a,b) - std::min(a,b) < 1;
}

/***********************************************************************
 * Filter functions
 **********************************************************************/

/* This function takes in the calculated maximum number of FIR taps, and
 * returns a number of taps that makes AD9361 happy. */
int get_num_taps(int max_num_taps) {

    int num_taps = 0;
    int num_taps_list[] = {16, 32, 48, 64, 80, 96, 112, 128};
    int i;
    for(i = 1; i < 8; i++) {
        if(max_num_taps >= num_taps_list[i]) {
            continue;
        } else {
            num_taps = num_taps_list[i - 1];
            break;
        }
    } if(num_taps == 0) { num_taps = 128; }

    return num_taps;
}

const double ad9361_device_t::AD9361_MAX_GAIN        = 89.75;
const double ad9361_device_t::AD9361_MIN_CLOCK_RATE  = 220e3;
const double ad9361_device_t::AD9361_MAX_CLOCK_RATE  = 61.44e6;
const double ad9361_device_t::AD9361_CAL_VALID_WINDOW = 100e6;
// Max bandwdith is due to filter rolloff in analog filter stage
const double ad9361_device_t::AD9361_MIN_BW = 200e3;
const double ad9361_device_t::AD9361_MAX_BW = 56e6;

/* Startup RF frequencies */
const double ad9361_device_t::DEFAULT_RX_FREQ = 800e6;
const double ad9361_device_t::DEFAULT_TX_FREQ = 850e6;

/* Program either the RX or TX FIR filter.
 *
 * The process is the same for both filters, but the function must be told
 * how many taps are in the filter, and given a vector of the taps
 * themselves.  */

void ad9361_device_t::_program_fir_filter(direction_t direction, chain_t chain, int num_taps, uint16_t *coeffs)
{
    uint16_t base;

    /* RX and TX filters use largely identical sets of programming registers.
     Select the appropriate bank of registers here. */
    if (direction == RX) {
        base = 0x0f0;
    } else {
        base = 0x060;
    }

    /* Encode number of filter taps for programming register */
    uint8_t reg_numtaps = (((num_taps / 16) - 1) & 0x07) << 5;

    uint8_t reg_chain = 0;
    switch (chain) {
    case CHAIN_1:
        reg_chain = 0x01 << 3;
        break;
    case CHAIN_2:
        reg_chain = 0x02 << 3;
        break;
    default:
        reg_chain = 0x03 << 3;
    }

    /* Turn on the filter clock. */
    _io_iface->poke8(base + 5, reg_numtaps | reg_chain | 0x02);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    /* Zero the unused taps just in case they have stale data */
    int addr;
    for (addr = num_taps; addr < 128; addr++) {
        _io_iface->poke8(base + 0, addr);
        _io_iface->poke8(base + 1, 0x0);
        _io_iface->poke8(base + 2, 0x0);
        _io_iface->poke8(base + 5, reg_numtaps | reg_chain | (1 << 1) | (1 << 2));
        _io_iface->poke8(base + 4, 0x00);
        _io_iface->poke8(base + 4, 0x00);
    }

    /* Iterate through indirect programming of filter coeffs using ADI recomended procedure */
    for (addr = 0; addr < num_taps; addr++) {
        _io_iface->poke8(base + 0, addr);
        _io_iface->poke8(base + 1, (coeffs[addr]) & 0xff);
        _io_iface->poke8(base + 2, (coeffs[addr] >> 8) & 0xff);
        _io_iface->poke8(base + 5, reg_numtaps | reg_chain | (1 << 1) | (1 << 2));
        _io_iface->poke8(base + 4, 0x00);
        _io_iface->poke8(base + 4, 0x00);
    }

    /* UG-671 states (page 25) (paraphrased and clarified):
     " After the table has been programmed, write to register BASE+5 with the write bit D2 cleared and D1 high.
     Then, write to register BASE+5 again with D1 clear, thus ensuring that the write bit resets internally
     before the clock stops. Wait 4 sample clock periods after setting D2 high while that data writes into the table"
     */

    _io_iface->poke8(base + 5, reg_numtaps | reg_chain | (1 << 1));
    if (direction == RX) {
        _io_iface->poke8(base + 5, reg_numtaps | reg_chain );
        /* Rx Gain, set to prevent digital overflow/saturation in filters
           0:+6dB, 1:0dB, 2:-6dB, 3:-12dB
           page 35 of UG-671 */
        _io_iface->poke8(base + 6, 0x02); /* Also turn on -6dB Rx gain here, to stop filter overfow.*/
    } else {
        /* Tx Gain. bit[0]. set to prevent digital overflow/saturation in filters
           0: 0dB, 1:-6dB
           page 25 of UG-671 */
        _io_iface->poke8(base + 5, reg_numtaps | reg_chain );
    }
}


/* Program the RX FIR Filter. */
void ad9361_device_t::_setup_rx_fir(size_t num_taps, int32_t decimation)
{
    if (not (decimation == 1 or decimation == 2 or decimation == 4)) {
        throw uhd::runtime_error("[ad9361_device_t] Invalid Rx FIR decimation.");
    }
    boost::scoped_array<uint16_t> coeffs(new uint16_t[num_taps]);
    for (size_t i = 0; i < num_taps; i++) {
        switch (num_taps) {
        case 128:
            coeffs[i] = uint16_t((decimation==4) ? fir_128_x4_coeffs[i] : hb127_coeffs[i]);
            break;
        case 96:
            coeffs[i] = uint16_t((decimation==4) ? fir_96_x4_coeffs[i] : hb95_coeffs[i]);
            break;
        case 64:
            coeffs[i] = uint16_t((decimation==4) ? fir_64_x4_coeffs[i] : hb63_coeffs[i]);
            break;
        case 48:
            coeffs[i] = uint16_t((decimation==4) ? fir_48_x4_coeffs[i] : hb47_coeffs[i]);
            break;
        default:
            throw uhd::runtime_error("[ad9361_device_t] Unsupported number of Rx FIR taps.");
        }
    }

    _program_fir_filter(RX, CHAIN_BOTH, num_taps, coeffs.get());
}

/* Program the TX FIR Filter. */
void ad9361_device_t::_setup_tx_fir(size_t num_taps, int32_t interpolation)
{
    if (not (interpolation == 1 or interpolation == 2 or interpolation == 4)) {
        throw uhd::runtime_error("[ad9361_device_t] Invalid Tx FIR interpolation.");
    }
    if (interpolation == 1 and num_taps > 64) {
        throw uhd::runtime_error("[ad9361_device_t] Too many Tx FIR taps for interpolation value.");
    }
    boost::scoped_array<uint16_t> coeffs(new uint16_t[num_taps]);
    for (size_t i = 0; i < num_taps; i++) {
        switch (num_taps) {
        case 128:
            coeffs[i] = uint16_t((interpolation==4) ? fir_128_x4_coeffs[i] : hb127_coeffs[i]);
            break;
        case 96:
            coeffs[i] = uint16_t((interpolation==4) ? fir_96_x4_coeffs[i] : hb95_coeffs[i]);
            break;
        case 64:
            coeffs[i] = uint16_t((interpolation==4) ? fir_64_x4_coeffs[i] : hb63_coeffs[i]);
            break;
        case 48:
            coeffs[i] = uint16_t((interpolation==4) ? fir_48_x4_coeffs[i] : hb47_coeffs[i]);
            break;
        default:
            throw uhd::runtime_error("[ad9361_device_t] Unsupported number of Tx FIR taps.");
        }
    }

    _program_fir_filter(TX, CHAIN_BOTH, num_taps, coeffs.get());
}

/***********************************************************************
 * Calibration functions
 ***********************************************************************/

/* Calibrate and lock the BBPLL.
 *
 * This function should be called anytime the BBPLL is tuned. */
void ad9361_device_t::_calibrate_lock_bbpll()
{
    _io_iface->poke8(0x03F, 0x05); // Start the BBPLL calibration
    _io_iface->poke8(0x03F, 0x01); // Clear the 'start' bit

    /* Increase BBPLL KV and phase margin. */
    _io_iface->poke8(0x04c, 0x86);
    _io_iface->poke8(0x04d, 0x01);
    _io_iface->poke8(0x04d, 0x05);

    /* Wait for BBPLL lock. */
    size_t count = 0;
    while (!(_io_iface->peek8(0x05e) & 0x80)) {
        if (count > 1000) {
            throw uhd::runtime_error("[ad9361_device_t] BBPLL not locked");
            break;
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

/* Calibrate the synthesizer charge pumps.
 *
 * Technically, this calibration only needs to be done once, at device
 * initialization. */
void ad9361_device_t::_calibrate_synth_charge_pumps()
{
    /* If this function ever gets called, and the ENSM isn't already in the
     * ALERT state, then something has gone horribly wrong. */
    if ((_io_iface->peek8(0x017) & 0x0F) != 5) {
        throw uhd::runtime_error("[ad9361_device_t] AD9361 not in ALERT during cal");
    }

    /* Calibrate the RX synthesizer charge pump. */
    size_t count = 0;
    _io_iface->poke8(0x23d, 0x04);
    while (!(_io_iface->peek8(0x244) & 0x80)) {
        if (count > 5) {
            throw uhd::runtime_error("[ad9361_device_t] RX charge pump cal failure");
            break;
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    _io_iface->poke8(0x23d, 0x00);

    /* Calibrate the TX synthesizer charge pump. */
    count = 0;
    _io_iface->poke8(0x27d, 0x04);
    while (!(_io_iface->peek8(0x284) & 0x80)) {
        if (count > 5) {
            throw uhd::runtime_error("[ad9361_device_t] TX charge pump cal failure");
            break;
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    _io_iface->poke8(0x27d, 0x00);
}

/* Calibrate the analog BB RX filter.
 *
 * Note that the filter calibration depends heavily on the baseband
 * bandwidth, so this must be re-done after any change to the RX sample
 * rate.
 * UG570 Page 33 states that this filter should be calibrated to 1.4 * bbbw*/
double ad9361_device_t::_calibrate_baseband_rx_analog_filter(double req_rfbw)
{
    double bbbw = req_rfbw / 2.0;
    if(bbbw > _baseband_bw / 2.0)
    {
        UHD_LOGGER_DEBUG("AD936X")<< "baseband bandwidth too large for current sample rate. Setting bandwidth to: "<<_baseband_bw;
        bbbw = _baseband_bw / 2.0;
    }

    /* Baseband BW must be between 28e6 and 0.143e6.
     * Max filter BW is 39.2 MHz. 39.2 / 1.4 = 28
     * Min filter BW is 200kHz. 200 / 1.4 = 143 */
    if (bbbw > 28e6) {
        bbbw = 28e6;
    } else if (bbbw < 0.143e6) {
        bbbw = 0.143e6;
    }

    double rxtune_clk = ((1.4 * bbbw * 2 * M_PI) / M_LN2);
    _rx_bbf_tunediv = std::min<uint16_t>(511, uint16_t(std::ceil(_bbpll_freq / rxtune_clk)));
    _regs.bbftune_config = (_regs.bbftune_config & 0xFE)
            | ((_rx_bbf_tunediv >> 8) & 0x0001);

    double bbbw_mhz = bbbw / 1e6;
    double temp = ((bbbw_mhz - std::floor(bbbw_mhz)) * 1000) / 7.8125;
    uint8_t bbbw_khz = std::min<uint8_t>(127, uint8_t(std::floor(temp + 0.5)));

    /* Set corner frequencies and dividers. */
    _io_iface->poke8(0x1fb, (uint8_t) (bbbw_mhz));
    _io_iface->poke8(0x1fc, bbbw_khz);
    _io_iface->poke8(0x1f8, (_rx_bbf_tunediv & 0x00FF));
    _io_iface->poke8(0x1f9, _regs.bbftune_config);

    /* RX Mix Voltage settings - only change with apps engineer help. */
    _io_iface->poke8(0x1d5, 0x3f);
    _io_iface->poke8(0x1c0, 0x03);

    /* Enable RX1 & RX2 filter tuners. */
    _io_iface->poke8(0x1e2, 0x02);
    _io_iface->poke8(0x1e3, 0x02);

    /* Run the calibration! */
    size_t count = 0;
    _io_iface->poke8(0x016, 0x80);
    while (_io_iface->peek8(0x016) & 0x80) {
        if (count > 100) {
            throw uhd::runtime_error("[ad9361_device_t] RX baseband filter cal FAILURE");
            break;
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    /* Disable RX1 & RX2 filter tuners. */
    _io_iface->poke8(0x1e2, 0x03);
    _io_iface->poke8(0x1e3, 0x03);

    return bbbw;
}

/* Calibrate the analog BB TX filter.
 *
 * Note that the filter calibration depends heavily on the baseband
 * bandwidth, so this must be re-done after any change to the TX sample
 * rate.
 * UG570 Page 32 states that this filter should be calibrated to 1.6 * bbbw*/
double ad9361_device_t::_calibrate_baseband_tx_analog_filter(double req_rfbw)
{
    double bbbw = req_rfbw / 2.0;

    if(bbbw > _baseband_bw / 2.0)
    {
        UHD_LOGGER_DEBUG("AD936X")<< "baseband bandwidth too large for current sample rate. Setting bandwidth to: "<<_baseband_bw;
        bbbw = _baseband_bw / 2.0;
    }

    /* Baseband BW must be between 20e6 and 0.391e6.
     * Max filter BW is 32 MHz. 32 / 1.6 = 20
     * Min filter BW is 625 kHz. 625 / 1.6 = 391 */
    if (bbbw > 20e6) {
        bbbw = 20e6;
    } else if (bbbw < 0.391e6) {
        bbbw = 0.391e6;
    }

    double txtune_clk = ((1.6 * bbbw * 2 * M_PI) / M_LN2);
    uint16_t txbbfdiv = std::min<uint16_t>(511, uint16_t(std::ceil(_bbpll_freq / txtune_clk)));
    _regs.bbftune_mode = (_regs.bbftune_mode & 0xFE)
            | ((txbbfdiv >> 8) & 0x0001);

    /* Program the divider values. */
    _io_iface->poke8(0x0d6, (txbbfdiv & 0x00FF));
    _io_iface->poke8(0x0d7, _regs.bbftune_mode);

    /* Enable the filter tuner. */
    _io_iface->poke8(0x0ca, 0x22);

    /* Calibrate! */
    size_t count = 0;
    _io_iface->poke8(0x016, 0x40);
    while (_io_iface->peek8(0x016) & 0x40) {
        if (count > 100) {
            throw uhd::runtime_error("[ad9361_device_t] TX baseband filter cal FAILURE");
            break;
        }

        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    /* Disable the filter tuner. */
    _io_iface->poke8(0x0ca, 0x26);

    return bbbw;
}

/* Calibrate the secondary TX filter.
 *
 * This filter also depends on the TX sample rate, so if a rate change is
 * made, the previous calibration will no longer be valid.
 * UG570 Page 32 states that this filter should be calibrated to 5 * bbbw*/
double ad9361_device_t::_calibrate_secondary_tx_filter(double req_rfbw)
{
    double bbbw = req_rfbw / 2.0;

    if(bbbw > _baseband_bw / 2.0)
    {
        UHD_LOGGER_DEBUG("AD936X")<< "baseband bandwidth too large for current sample rate. Setting bandwidth to: "<<_baseband_bw;
        bbbw = _baseband_bw / 2.0;
    }

    /* Baseband BW must be between 20e6 and 0.54e6.
     * Max filter BW is 100 MHz. 100 / 5 = 20
     * Min filter BW is 2.7 MHz. 2.7 / 5 = 0.54 */
    if (bbbw > 20e6) {
        bbbw = 20e6;
    } else if (bbbw < 0.54e6) {
        bbbw = 0.54e6;
    }

    double bbbw_mhz = bbbw / 1e6;

    /* Start with a resistor value of 100 Ohms. */
    int res = 100;

    /* Calculate target corner frequency. */
    double corner_freq = 5 * bbbw_mhz * 2 * M_PI;

    /* Iterate through RC values to determine correct combination. */
    int cap = 0;
    int i;
    for (i = 0; i <= 3; i++) {
        cap = static_cast<int>(std::floor(0.5 + ((1 / ((corner_freq * res) * 1e6)) * 1e12)))
                - 12;

        if (cap <= 63) {
            break;
        }

        res = res * 2;
    }
    if (cap > 63) {
        cap = 63;
    }

    uint8_t reg0d0, reg0d1, reg0d2;

    /* Translate baseband bandwidths to register settings. */
    if ((bbbw_mhz * 2) <= 9) {
        reg0d0 = 0x59;
    } else if (((bbbw_mhz * 2) > 9) && ((bbbw_mhz * 2) <= 24)) {
        reg0d0 = 0x56;
    } else if ((bbbw_mhz * 2) > 24) {
        reg0d0 = 0x57;
    } else {
        reg0d0 = 0x00;
        throw uhd::runtime_error("[ad9361_device_t] Cal2ndTxFil: INVALID_CODE_PATH bad bbbw_mhz");
    }

    /* Translate resistor values to register settings. */
    if (res == 100) {
        reg0d1 = 0x0c;
    } else if (res == 200) {
        reg0d1 = 0x04;
    } else if (res == 400) {
        reg0d1 = 0x03;
    } else if (res == 800) {
        reg0d1 = 0x01;
    } else {
        reg0d1 = 0x0c;
    }

    reg0d2 = cap;

    /* Program the above-calculated values. Sweet. */
    _io_iface->poke8(0x0d2, reg0d2);
    _io_iface->poke8(0x0d1, reg0d1);
    _io_iface->poke8(0x0d0, reg0d0);

    return bbbw;
}

/* Calibrate the RX TIAs.
 *
 * Note that the values in the TIA register, after calibration, vary with
 * the RX gain settings.
 * We do not really program the BW here. Most settings are taken form the BB LPF registers
 * UG570 page 33 states that this filter should be calibrated to 2.5 * bbbw */
double ad9361_device_t::_calibrate_rx_TIAs(double req_rfbw)
{
    uint8_t reg1eb = _io_iface->peek8(0x1eb) & 0x3F;
    uint8_t reg1ec = _io_iface->peek8(0x1ec) & 0x7F;
    uint8_t reg1e6 = _io_iface->peek8(0x1e6) & 0x07;
    uint8_t reg1db = 0x00;
    uint8_t reg1dc = 0x00;
    uint8_t reg1dd = 0x00;
    uint8_t reg1de = 0x00;
    uint8_t reg1df = 0x00;

    double bbbw = req_rfbw / 2.0;

    if(bbbw > _baseband_bw / 2.0)
    {
        UHD_LOGGER_DEBUG("AD936X")<< "baseband bandwidth too large for current sample rate. Setting bandwidth to: "<<_baseband_bw;
        bbbw = _baseband_bw / 2.0;
    }

    /* Baseband BW must be between 28e6 and 0.4e6.
     * Max filter BW is 70 MHz. 70 / 2.5 = 28
     * Min filter BW is 1 MHz. 1 / 2.5 =  0.4*/
    if (bbbw > 28e6) {
        bbbw = 28e6;
    } else if (bbbw < 0.40e6) {
        bbbw = 0.40e6;
    }
    double ceil_bbbw_mhz = std::ceil(bbbw / 1e6);

    /* Do some crazy resistor and capacitor math. */
    int Cbbf = (reg1eb * 160) + (reg1ec * 10) + 140;
    int R2346 = 18300 * (reg1e6 & 0x07);
    double CTIA_fF = (Cbbf * R2346 * 0.56) / 3500;

    /* Translate baseband BW to register settings. */
    if (ceil_bbbw_mhz <= 3) {
        reg1db = 0xe0;
    } else if ((ceil_bbbw_mhz > 3) && (ceil_bbbw_mhz <= 10)) {
        reg1db = 0x60;
    } else if (ceil_bbbw_mhz > 10) {
        reg1db = 0x20;
    } else {
        throw uhd::runtime_error("[ad9361_device_t] CalRxTias: INVALID_CODE_PATH bad bbbw_mhz");
    }

    if (CTIA_fF > 2920) {
        reg1dc = 0x40;
        reg1de = 0x40;
        uint8_t temp = (uint8_t) std::min<uint8_t>(127,
                uint8_t(std::floor(0.5 + ((CTIA_fF - 400.0) / 320.0))));
        reg1dd = temp;
        reg1df = temp;
    } else {
        uint8_t temp = uint8_t(std::floor(0.5 + ((CTIA_fF - 400.0) / 40.0)) + 0x40);
        reg1dc = temp;
        reg1de = temp;
        reg1dd = 0;
        reg1df = 0;
    }

    /* w00t. Settings calculated. Program them and roll out. */
    _io_iface->poke8(0x1db, reg1db);
    _io_iface->poke8(0x1dd, reg1dd);
    _io_iface->poke8(0x1df, reg1df);
    _io_iface->poke8(0x1dc, reg1dc);
    _io_iface->poke8(0x1de, reg1de);

    return bbbw;
}

/* Setup the AD9361 ADC.
 *
 * There are 40 registers that control the ADC's operation, most of the
 * values of which must be derived mathematically, dependent on the current
 * setting of the BBPLL. Note that the order of calculation is critical, as
 * some of the 40 registers depend on the values in others. */
void ad9361_device_t::_setup_adc()
{
    double bbbw_mhz = (((_bbpll_freq / 1e6) / _rx_bbf_tunediv) * M_LN2) \
                  / (1.4 * 2 * M_PI);

    /* For calibration, baseband BW is half the complex BW, and must be
     * between 28e6 and 0.2e6. */
    if(bbbw_mhz > 28) {
        bbbw_mhz = 28;
    } else if (bbbw_mhz < 0.20) {
        bbbw_mhz = 0.20;
    }

    uint8_t rxbbf_c3_msb = _io_iface->peek8(0x1eb) & 0x3F;
    uint8_t rxbbf_c3_lsb = _io_iface->peek8(0x1ec) & 0x7F;
    uint8_t rxbbf_r2346 = _io_iface->peek8(0x1e6) & 0x07;

    double fsadc = _adcclock_freq / 1e6;

    /* Sort out the RC time constant for our baseband bandwidth... */
    double rc_timeconst = 0.0;
    if(bbbw_mhz < 18) {
        rc_timeconst = (1 / ((1.4 * 2 * M_PI) \
                            * (18300 * rxbbf_r2346)
                            * ((160e-15 * rxbbf_c3_msb)
                                + (10e-15 * rxbbf_c3_lsb) + 140e-15)
                            * (bbbw_mhz * 1e6)));
    } else {
        rc_timeconst = (1 / ((1.4 * 2 * M_PI) \
                            * (18300 * rxbbf_r2346)
                            * ((160e-15 * rxbbf_c3_msb)
                                + (10e-15 * rxbbf_c3_lsb) + 140e-15)
                            * (bbbw_mhz * 1e6) * (1 + (0.01 * (bbbw_mhz - 18)))));
    }

    double scale_res = sqrt(1 / rc_timeconst);
    double scale_cap = sqrt(1 / rc_timeconst);

    double scale_snr = (_adcclock_freq < 80e6) ? 1.0 : 1.584893192;
    double maxsnr = 640 / 160;

    /* Calculate the values for all 40 settings registers.
     *
     * DO NOT TOUCH THIS UNLESS YOU KNOW EXACTLY WHAT YOU ARE DOING. kthx.*/
    uint8_t data[40];
    data[0] = 0;    data[1] = 0; data[2] = 0; data[3] = 0x24;
    data[4] = 0x24; data[5] = 0; data[6] = 0;
    data[7] = std::min<uint8_t>(124, uint8_t(std::floor(-0.5
                    + (80.0 * scale_snr * scale_res
                    * std::min<double>(1.0, sqrt(maxsnr * fsadc / 640.0))))));
    double data007 = data[7];
    data[8] = std::min<uint8_t>(255, uint8_t(std::floor(0.5
                    + ((20.0 * (640.0 / fsadc) * ((data007 / 80.0))
                    / (scale_res * scale_cap))))));
    data[10] = std::min<uint8_t>(127, uint8_t(std::floor(-0.5 + (77.0 * scale_res
                    * std::min<double>(1.0, sqrt(maxsnr * fsadc / 640.0))))));
    double data010 = data[10];
    data[9] = std::min<uint8_t>(127, uint8_t(std::floor(0.8 * data010)));
    data[11] = std::min<uint8_t>(255, uint8_t(std::floor(0.5
                    + (20.0 * (640.0 / fsadc) * ((data010 / 77.0)
                    / (scale_res * scale_cap))))));
    data[12] = std::min<uint8_t>(127, uint8_t(std::floor(-0.5
                    + (80.0 * scale_res * std::min<double>(1.0,
                    sqrt(maxsnr * fsadc / 640.0))))));
    double data012 = data[12];
    data[13] = std::min<uint8_t>(255, uint8_t(std::floor(-1.5
                    + (20.0 * (640.0 / fsadc) * ((data012 / 80.0)
                    / (scale_res * scale_cap))))));
    data[14] = 21 * uint8_t(std::floor(0.1 * 640.0 / fsadc));
    data[15] = std::min<uint8_t>(127, uint8_t(1.025 * data007));
    double data015 = data[15];
    data[16] = std::min<uint8_t>(127, uint8_t(std::floor((data015
                    * (0.98 + (0.02 * std::max<double>(1.0,
                    (640.0 / fsadc) / maxsnr)))))));
    data[17] = data[15];
    data[18] = std::min<uint8_t>(127, uint8_t(0.975 * (data010)));
    double data018 = data[18];
    data[19] = std::min<uint8_t>(127, uint8_t(std::floor((data018
                    * (0.98 + (0.02 * std::max<double>(1.0,
                    (640.0 / fsadc) / maxsnr)))))));
    data[20] = data[18];
    data[21] = std::min<uint8_t>(127, uint8_t(0.975 * data012));
    double data021 = data[21];
    data[22] = std::min<uint8_t>(127, uint8_t(std::floor((data021
                    * (0.98 + (0.02 * std::max<double>(1.0,
                    (640.0 / fsadc) / maxsnr)))))));
    data[23] = data[21];
    data[24] = 0x2e;
    data[25] = uint8_t(std::floor(128.0 + std::min<double>(63.0,
                    63.0 * (fsadc / 640.0))));
    data[26] = uint8_t(std::floor(std::min<double>(63.0, 63.0 * (fsadc / 640.0)
                    * (0.92 + (0.08 * (640.0 / fsadc))))));
    data[27] = uint8_t(std::floor(std::min<double>(63.0,
                    32.0 * sqrt(fsadc / 640.0))));
    data[28] = uint8_t(std::floor(128.0 + std::min<double>(63.0,
                    63.0 * (fsadc / 640.0))));
    data[29] = uint8_t(std::floor(std::min<double>(63.0,
                    63.0 * (fsadc / 640.0)
                    * (0.92 + (0.08 * (640.0 / fsadc))))));
    data[30] = uint8_t(std::floor(std::min<double>(63.0,
                    32.0 * sqrt(fsadc / 640.0))));
    data[31] = uint8_t(std::floor(128.0 + std::min<double>(63.0,
                    63.0 * (fsadc / 640.0))));
    data[32] = uint8_t(std::floor(std::min<double>(63.0,
                    63.0 * (fsadc / 640.0) * (0.92
                    + (0.08 * (640.0 / fsadc))))));
    data[33] = uint8_t(std::floor(std::min<double>(63.0,
                    63.0 * sqrt(fsadc / 640.0))));
    data[34] = std::min<uint8_t>(127, uint8_t(std::floor(64.0
                    * sqrt(fsadc / 640.0))));
    data[35] = 0x40;
    data[36] = 0x40;
    data[37] = 0x2c;
    data[38] = 0x00;
    data[39] = 0x00;

    /* Program the registers! */
    for(size_t i = 0; i < 40; i++) {
        _io_iface->poke8(0x200+i, data[i]);
    }
}

/* Calibrate the baseband DC offset.
 * Disables tracking
 */
void ad9361_device_t::_calibrate_baseband_dc_offset()
{
    _io_iface->poke8(0x18b, 0x83); //Reset RF DC tracking flag

    _io_iface->poke8(0x193, 0x3f); // Calibration settings
    _io_iface->poke8(0x190, 0x0f); // Set tracking coefficient
    //write_ad9361_reg(device, 0x190, /*0x0f*//*0xDF*/0x80*1 | 0x40*1 | (16+8/*+4*/)); // Set tracking coefficient: don't *4 counter, do decim /4, increased gain shift
    _io_iface->poke8(0x194, 0x01); // More calibration settings

    /* Start that calibration, baby. */
    size_t count = 0;
    _io_iface->poke8(0x016, 0x01);
    while (_io_iface->peek8(0x016) & 0x01) {
        if (count > 100) {
            throw uhd::runtime_error("[ad9361_device_t] Baseband DC Offset Calibration Failure");
            break;
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

/* Calibrate the RF DC offset.
 * Disables tracking
 */
void ad9361_device_t::_calibrate_rf_dc_offset()
{
    /* Some settings are frequency-dependent. */
    if (_rx_freq < 4e9) {
        _io_iface->poke8(0x186, 0x32); // RF DC Offset count
        _io_iface->poke8(0x187, 0x24);
        _io_iface->poke8(0x188, 0x05);
    } else {
        _io_iface->poke8(0x186, 0x28); // RF DC Offset count
        _io_iface->poke8(0x187, 0x34);
        _io_iface->poke8(0x188, 0x06);
    }

    _io_iface->poke8(0x185, 0x20); // RF DC Offset wait count
    _io_iface->poke8(0x18b, 0x83); // Disable tracking
    _io_iface->poke8(0x189, 0x30);

    /* Run the calibration! */
    size_t count = 0;
    _io_iface->poke8(0x016, 0x02);
    while (_io_iface->peek8(0x016) & 0x02) {
        if (count > 200) {
            throw uhd::runtime_error("[ad9361_device_t] RF DC Offset Calibration Failure");
            break;
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    _io_iface->poke8(0x18b, 0x8d); // Enable RF DC tracking
}

void ad9361_device_t::_configure_bb_dc_tracking()
{
    if (_use_dc_offset_tracking)
        _io_iface->poke8(0x18b, 0xad); // Enable BB tracking
    else
        _io_iface->poke8(0x18b, 0x8d); // Disable BB tracking
}

void ad9361_device_t::_configure_rx_iq_tracking()
{
    if (_use_iq_balance_tracking)
        _io_iface->poke8(0x169, 0xcf); // Enable Rx IQ tracking
    else
        _io_iface->poke8(0x169, 0xc0); // Disable Rx IQ tracking
}

/* Single shot Rx quadrature calibration
 *
 * Procedure documented in "AD9361 Calibration Guide". Prior to calibration,
 * state should be set to ALERT, FDD, and Dual Synth Mode. Rx quadrature
 * tracking will be disabled, so run before or instead of enabling Rx
 * quadrature tracking.
 */
void ad9361_device_t::_calibrate_rx_quadrature()
{
    /* Configure RX Quadrature calibration settings. */
    _io_iface->poke8(0x168, 0x03); // Set tone level for cal
    _io_iface->poke8(0x16e, 0x25); // RX Gain index to use for cal
    _io_iface->poke8(0x16a, 0x75); // Set Kexp phase
    _io_iface->poke8(0x16b, 0x95); // Set Kexp amplitude
    _io_iface->poke8(0x057, 0x33); // Power down Tx mixer
    _io_iface->poke8(0x169, 0xc0); // Disable tracking and free run mode

    /* Place Tx LO within passband of Rx spectrum */
    double current_tx_freq = _tx_freq;
    _tune_helper(TX, _rx_freq + _rx_bb_lp_bw / 2.0);

    size_t count = 0;
    _io_iface->poke8(0x016, 0x20);
    while (_io_iface->peek8(0x016) & 0x20) {
        if (count > 1000) {
            throw uhd::runtime_error("[ad9361_device_t] Rx Quadrature Calibration Failure");
            break;
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    _io_iface->poke8(0x057, 0x30); // Re-enable Tx mixers

    _tune_helper(TX, current_tx_freq);
}

/* TX quadrature calibration routine.
 *
 * The TX quadrature needs to be done twice, once for each TX chain, with
 * only one register change in between. Thus, this function enacts the
 * calibrations, and it is called from calibrate_tx_quadrature. */
void ad9361_device_t::_tx_quadrature_cal_routine() {

    /* This is a weird process, but here is how it works:
     * 1) Read the calibrated NCO frequency bits out of 0A3.
     * 2) Write the two bits to the RX NCO freq part of 0A0.
     * 3) Re-read 0A3 to get bits [5:0] because maybe they changed?
     * 4) Update only the TX NCO freq bits in 0A3.
     * 5) Profit (I hope). */
    uint8_t reg0a3 = _io_iface->peek8(0x0a3);
    uint8_t nco_freq = (reg0a3 & 0xC0);
    _io_iface->poke8(0x0a0, 0x15 | (nco_freq >> 1));
    reg0a3 = _io_iface->peek8(0x0a3);
    _io_iface->poke8(0x0a3, (reg0a3 & 0x3F) | nco_freq);

    /* It is possible to reach a configuration that won't operate correctly,
     * where the two test tones used for quadrature calibration are outside
     * of the RX BBF, and therefore don't make it to the ADC. We will check
     * for that scenario here. */
    double max_cal_freq = (((_baseband_bw * _tfir_factor)
            * ((nco_freq >> 6) + 1)) / 32) * 2;
    double bbbw = _baseband_bw / 2.0; // bbbw represents the one-sided BW
    if (bbbw > 28e6) {
        bbbw = 28e6;
    } else if (bbbw < 0.20e6) {
        bbbw = 0.20e6;
    }
    if (max_cal_freq > bbbw)
        throw uhd::runtime_error("[ad9361_device_t] max_cal_freq > bbbw");

    _io_iface->poke8(0x0a1, 0x7B); // Set tracking coefficient
    _io_iface->poke8(0x0a9, 0xff); // Cal count
    _io_iface->poke8(0x0a2, 0x7f); // Cal Kexp
    _io_iface->poke8(0x0a5, 0x01); // Cal magnitude threshold VVVV
    _io_iface->poke8(0x0a6, 0x01);

    /* The gain table index used for calibration must be adjusted for the
     * mid-table to get a TIA index = 1 and LPF index = 0. */
    if (_rx_freq < 1300e6) {
        _io_iface->poke8(0x0aa, 0x22); // Cal gain table index
    } else {
        _io_iface->poke8(0x0aa, 0x25); // Cal gain table index
    }

    _io_iface->poke8(0x0a4, 0xf0); // Cal setting conut
    _io_iface->poke8(0x0ae, 0x00); // Cal LPF gain index (split mode)

    /* Now, calibrate the TX quadrature! */
    size_t count = 0;
    _io_iface->poke8(0x016, 0x10);
    while (_io_iface->peek8(0x016) & 0x10) {
        if (count > 100) {
            throw uhd::runtime_error("[ad9361_device_t] TX Quadrature Calibration Failure");
            break;
        }
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

/* Run the TX quadrature calibration.
 */
void ad9361_device_t::_calibrate_tx_quadrature()
{
    /* Make sure we are, in fact, in the ALERT state. If not, something is
     * terribly wrong in the driver execution flow. */
    if ((_io_iface->peek8(0x017) & 0x0F) != 5) {
        throw uhd::runtime_error("[ad9361_device_t] TX Quad Cal started, but not in ALERT");
    }

    /* Turn off free-running and continuous calibrations. Note that this
     * will get turned back on at the end of the RX calibration routine. */
    _io_iface->poke8(0x169, 0xc0);

    /* This calibration must be done in a certain order, and for both TX_A
     * and TX_B, separately. Store the original setting so that we can
     * restore it later. */
    uint8_t orig_reg_inputsel = _regs.inputsel;

    /***********************************************************************
     * TX1/2-A Calibration
     **********************************************************************/
    _regs.inputsel = _regs.inputsel & 0xBF;
    _io_iface->poke8(0x004, _regs.inputsel);

    _tx_quadrature_cal_routine();

    /***********************************************************************
     * TX1/2-B Calibration
     **********************************************************************/
    _regs.inputsel = _regs.inputsel | 0x40;
    _io_iface->poke8(0x004, _regs.inputsel);

    _tx_quadrature_cal_routine();

    /***********************************************************************
     * fin
     **********************************************************************/
    _regs.inputsel = orig_reg_inputsel;
    _io_iface->poke8(0x004, orig_reg_inputsel);
}


/***********************************************************************
 * Other Misc Setup Functions
 ***********************************************************************/

/* Program the mixer gain table.
 *
 * Note that this table is fixed for all frequency settings. */
void ad9361_device_t::_program_mixer_gm_subtable()
{
    uint8_t gain[] = { 0x78, 0x74, 0x70, 0x6C, 0x68, 0x64, 0x60, 0x5C, 0x58,
            0x54, 0x50, 0x4C, 0x48, 0x30, 0x18, 0x00 };
    uint8_t gm[] = { 0x00, 0x0D, 0x15, 0x1B, 0x21, 0x25, 0x29, 0x2C, 0x2F, 0x31,
            0x33, 0x34, 0x35, 0x3A, 0x3D, 0x3E };

    /* Start the clock. */
    _io_iface->poke8(0x13f, 0x02);

    /* Program the GM Sub-table. */
    int i;
    for (i = 15; i >= 0; i--) {
        _io_iface->poke8(0x138, i);
        _io_iface->poke8(0x139, gain[(15 - i)]);
        _io_iface->poke8(0x13A, 0x00);
        _io_iface->poke8(0x13B, gm[(15 - i)]);
        _io_iface->poke8(0x13F, 0x06);
        _io_iface->poke8(0x13C, 0x00);
        _io_iface->poke8(0x13C, 0x00);
    }

    /* Clear write bit and stop clock. */
    _io_iface->poke8(0x13f, 0x02);
    _io_iface->poke8(0x13C, 0x00);
    _io_iface->poke8(0x13C, 0x00);
    _io_iface->poke8(0x13f, 0x00);
}

/* Program the gain table.
 *
 * There are three different gain tables for different frequency ranges! */
void ad9361_device_t::_program_gain_table() {
    /* Figure out which gain table we should be using for our current
     * frequency band. */
    uint8_t (*gain_table)[3] = NULL;
    uint8_t new_gain_table;
    if (_rx_freq < 1300e6) {
        gain_table = gain_table_sub_1300mhz;
        new_gain_table = 1;
    } else if (_rx_freq < 4e9) {
        gain_table = gain_table_1300mhz_to_4000mhz;
        new_gain_table = 2;
    } else if (_rx_freq <= 6e9) {
        gain_table = gain_table_4000mhz_to_6000mhz;
        new_gain_table = 3;
    } else {
        new_gain_table = 1;
        throw uhd::runtime_error("[ad9361_device_t] Wrong _rx_freq value");
    }

    /* Only re-program the gain table if there has been a band change. */
    if (_curr_gain_table == new_gain_table) {
        return;
    } else {
        _curr_gain_table = new_gain_table;
    }

    /* Okay, we have to program a new gain table. Sucks, brah. Start the
     * gain table clock. */
    _io_iface->poke8(0x137, 0x1A);

    /* IT'S PROGRAMMING TIME. */
    uint8_t index = 0;
    for (; index < 77; index++) {
        _io_iface->poke8(0x130, index);
        _io_iface->poke8(0x131, gain_table[index][0]);
        _io_iface->poke8(0x132, gain_table[index][1]);
        _io_iface->poke8(0x133, gain_table[index][2]);
        _io_iface->poke8(0x137, 0x1E);
        _io_iface->poke8(0x134, 0x00);
        _io_iface->poke8(0x134, 0x00);
    }

    /* Everything above the 77th index is zero. */
    for (; index < 91; index++) {
        _io_iface->poke8(0x130, index);
        _io_iface->poke8(0x131, 0x00);
        _io_iface->poke8(0x132, 0x00);
        _io_iface->poke8(0x133, 0x00);
        _io_iface->poke8(0x137, 0x1E);
        _io_iface->poke8(0x134, 0x00);
        _io_iface->poke8(0x134, 0x00);
    }

    /* Clear the write bit and stop the gain clock. */
    _io_iface->poke8(0x137, 0x1A);
    _io_iface->poke8(0x134, 0x00);
    _io_iface->poke8(0x134, 0x00);
    _io_iface->poke8(0x137, 0x00);
}

/* Setup gain control registers.
 *
 * This really only needs to be done once, at initialization.
 * If AGC is used the mode select bits (Reg 0x0FA) must be written manually */
void ad9361_device_t::_setup_gain_control(bool agc)
{
    /* The AGC mode configuration should be good for all cases.
     * However, non AGC configuration still used for backward compatibility. */
    if (agc) {
        /*mode select bits must be written before hand!*/
        _io_iface->poke8(0x0FB, 0x08); // Table, Digital Gain, Man Gain Ctrl
        _io_iface->poke8(0x0FC, 0x23); // Incr Step Size, ADC Overrange Size
        _io_iface->poke8(0x0FD, 0x4C); // Max Full/LMT Gain Table Index
        _io_iface->poke8(0x0FE, 0x44); // Decr Step Size, Peak Overload Time
        _io_iface->poke8(0x100, 0x6F); // Max Digital Gain
        _io_iface->poke8(0x101, 0x0A); // Max Digital Gain
        _io_iface->poke8(0x103, 0x08); // Max Digital Gain
        _io_iface->poke8(0x104, 0x2F); // ADC Small Overload Threshold
        _io_iface->poke8(0x105, 0x3A); // ADC Large Overload Threshold
        _io_iface->poke8(0x106, 0x22); // Max Digital Gain
        _io_iface->poke8(0x107, 0x2B); // Large LMT Overload Threshold
        _io_iface->poke8(0x108, 0x31);
        _io_iface->poke8(0x111, 0x0A);
        _io_iface->poke8(0x11A, 0x1C);
        _io_iface->poke8(0x120, 0x0C);
        _io_iface->poke8(0x121, 0x44);
        _io_iface->poke8(0x122, 0x44);
        _io_iface->poke8(0x123, 0x11);
        _io_iface->poke8(0x124, 0xF5);
        _io_iface->poke8(0x125, 0x3B);
        _io_iface->poke8(0x128, 0x03);
        _io_iface->poke8(0x129, 0x56);
        _io_iface->poke8(0x12A, 0x22);
    } else {
        _io_iface->poke8(0x0FA, 0xE0); // Gain Control Mode Select
        _io_iface->poke8(0x0FB, 0x08); // Table, Digital Gain, Man Gain Ctrl
        _io_iface->poke8(0x0FC, 0x23); // Incr Step Size, ADC Overrange Size
        _io_iface->poke8(0x0FD, 0x4C); // Max Full/LMT Gain Table Index
        _io_iface->poke8(0x0FE, 0x44); // Decr Step Size, Peak Overload Time
        _io_iface->poke8(0x100, 0x6F); // Max Digital Gain
        _io_iface->poke8(0x104, 0x2F); // ADC Small Overload Threshold
        _io_iface->poke8(0x105, 0x3A); // ADC Large Overload Threshold
        _io_iface->poke8(0x107, 0x31); // Large LMT Overload Threshold
        _io_iface->poke8(0x108, 0x39); // Small LMT Overload Threshold
        _io_iface->poke8(0x109, 0x23); // Rx1 Full/LMT Gain Index
        _io_iface->poke8(0x10A, 0x58); // Rx1 LPF Gain Index
        _io_iface->poke8(0x10B, 0x00); // Rx1 Digital Gain Index
        _io_iface->poke8(0x10C, 0x23); // Rx2 Full/LMT Gain Index
        _io_iface->poke8(0x10D, 0x18); // Rx2 LPF Gain Index
        _io_iface->poke8(0x10E, 0x00); // Rx2 Digital Gain Index
        _io_iface->poke8(0x114, 0x30); // Low Power Threshold
        _io_iface->poke8(0x11A, 0x27); // Initial LMT Gain Limit
        _io_iface->poke8(0x081, 0x00); // Tx Symbol Gain Control
    }
}

/* Setup the RX or TX synthesizers.
 *
 * This setup depends on a fixed look-up table, which is stored in an
 * included header file. The table is indexed based on the passed VCO rate.
 */
void ad9361_device_t::_setup_synth(direction_t direction, double vcorate)
{
    /* The vcorates in the vco_index array represent lower boundaries for
     * rates. Once we find a match, we use that index to look-up the rest of
     * the register values in the LUT. */
    int vcoindex = 0;
    for (size_t i = 0; i < 53; i++) {
        vcoindex = i;
        if (vcorate > vco_index[i]) {
            break;
        }
    }
    if (vcoindex > 53)
        throw uhd::runtime_error("[ad9361_device_t] vcoindex > 53");

    /* Parse the values out of the LUT based on our calculated index... */
    uint8_t vco_output_level = synth_cal_lut[vcoindex][0];
    uint8_t vco_varactor = synth_cal_lut[vcoindex][1];
    uint8_t vco_bias_ref = synth_cal_lut[vcoindex][2];
    uint8_t vco_bias_tcf = synth_cal_lut[vcoindex][3];
    uint8_t vco_cal_offset = synth_cal_lut[vcoindex][4];
    uint8_t vco_varactor_ref = synth_cal_lut[vcoindex][5];
    uint8_t charge_pump_curr = synth_cal_lut[vcoindex][6];
    uint8_t loop_filter_c2 = synth_cal_lut[vcoindex][7];
    uint8_t loop_filter_c1 = synth_cal_lut[vcoindex][8];
    uint8_t loop_filter_r1 = synth_cal_lut[vcoindex][9];
    uint8_t loop_filter_c3 = synth_cal_lut[vcoindex][10];
    uint8_t loop_filter_r3 = synth_cal_lut[vcoindex][11];

    /* ... annnd program! */
    if (direction == RX) {
        _io_iface->poke8(0x23a, 0x40 | vco_output_level);
        _io_iface->poke8(0x239, 0xC0 | vco_varactor);
        _io_iface->poke8(0x242, vco_bias_ref | (vco_bias_tcf << 3));
        _io_iface->poke8(0x238, (vco_cal_offset << 3));
        _io_iface->poke8(0x245, 0x00);
        _io_iface->poke8(0x251, vco_varactor_ref);
        _io_iface->poke8(0x250, 0x70);
        _io_iface->poke8(0x23b, 0x80 | charge_pump_curr);
        _io_iface->poke8(0x23e, loop_filter_c1 | (loop_filter_c2 << 4));
        _io_iface->poke8(0x23f, loop_filter_c3 | (loop_filter_r1 << 4));
        _io_iface->poke8(0x240, loop_filter_r3);
    } else if (direction == TX) {
        _io_iface->poke8(0x27a, 0x40 | vco_output_level);
        _io_iface->poke8(0x279, 0xC0 | vco_varactor);
        _io_iface->poke8(0x282, vco_bias_ref | (vco_bias_tcf << 3));
        _io_iface->poke8(0x278, (vco_cal_offset << 3));
        _io_iface->poke8(0x285, 0x00);
        _io_iface->poke8(0x291, vco_varactor_ref);
        _io_iface->poke8(0x290, 0x70);
        _io_iface->poke8(0x27b, 0x80 | charge_pump_curr);
        _io_iface->poke8(0x27e, loop_filter_c1 | (loop_filter_c2 << 4));
        _io_iface->poke8(0x27f, loop_filter_c3 | (loop_filter_r1 << 4));
        _io_iface->poke8(0x280, loop_filter_r3);
    } else {
        throw uhd::runtime_error("[ad9361_device_t] [_setup_synth] INVALID_CODE_PATH");
    }
}


/* Tune the baseband VCO.
 *
 * This clock signal is what gets fed to the ADCs and DACs. This function is
 * not exported outside of this file, and is invoked based on the rate
 * fed to the public set_clock_rate function. */
double ad9361_device_t::_tune_bbvco(const double rate)
{
    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::_tune_bbvco] rate=%.10f\n") % rate;

    /* Let's not re-tune to the same frequency over and over... */
    if (freq_is_nearly_equal(rate, _req_coreclk)) {
        return _adcclock_freq;
    }

    _req_coreclk = rate;

    const double fref = 40e6;
    const int modulus = 2088960;
    const double vcomax = 1430e6;
    const double vcomin = 672e6;
    double vcorate;
    int vcodiv;

    /* Iterate over VCO dividers until appropriate divider is found. */
    int i = 1;
    for (; i <= 6; i++) {
        vcodiv = 1 << i;
        vcorate = rate * vcodiv;

        if (vcorate >= vcomin && vcorate <= vcomax)
            break;
    }
    if (i == 7)
        throw uhd::runtime_error("[ad9361_device_t] _tune_bbvco: wrong vcorate");

    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::_tune_bbvco] vcodiv=%d vcorate=%.10f\n") % vcodiv % vcorate;
    /* Fo = Fref * (Nint + Nfrac / mod) */
    int nint = static_cast<int>(vcorate / fref);
    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::_tune_bbvco] (nint)=%.10f\n") % (vcorate / fref);
    int nfrac = static_cast<int>(boost::math::round(((vcorate / fref) - (double) nint) * (double) modulus));
    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::_tune_bbvco] (nfrac)=%.10f\n") % (((vcorate / fref) - (double) nint) * (double) modulus);
    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::_tune_bbvco] nint=%d nfrac=%d\n") % nint % nfrac;
    double actual_vcorate = fref
            * ((double) nint + ((double) nfrac / (double) modulus));

    /* Scale CP current according to VCO rate */
    const double icp_baseline = 150e-6;
    const double freq_baseline = 1280e6;
    double icp = icp_baseline * (actual_vcorate / freq_baseline);
    int icp_reg = static_cast<int>(icp / 25e-6) - 1;

    _io_iface->poke8(0x045, 0x00);            // REFCLK / 1 to BBPLL
    _io_iface->poke8(0x046, icp_reg & 0x3F);  // CP current
    _io_iface->poke8(0x048, 0xe8);            // BBPLL loop filters
    _io_iface->poke8(0x049, 0x5b);            // BBPLL loop filters
    _io_iface->poke8(0x04a, 0x35);            // BBPLL loop filters

    _io_iface->poke8(0x04b, 0xe0);
    _io_iface->poke8(0x04e, 0x10);            // Max accuracy

    _io_iface->poke8(0x043, nfrac & 0xFF);         // Nfrac[7:0]
    _io_iface->poke8(0x042, (nfrac >> 8) & 0xFF);  // Nfrac[15:8]
    _io_iface->poke8(0x041, (nfrac >> 16) & 0xFF); // Nfrac[23:16]
    _io_iface->poke8(0x044, nint);                 // Nint

    _calibrate_lock_bbpll();

    _regs.bbpll = (_regs.bbpll & 0xF8) | i;

    _bbpll_freq = actual_vcorate;
    _adcclock_freq = (actual_vcorate / vcodiv);

    return _adcclock_freq;
}

/* This function re-programs all of the gains in the system.
 *
 * Because the gain values match to different gain indices based on the
 * current operating band, this function can be called to update all gain
 * settings to the appropriate index after a re-tune. */
void ad9361_device_t::_reprogram_gains()
{
    set_gain(RX, CHAIN_1,_rx1_gain);
    set_gain(RX, CHAIN_2,_rx2_gain);
    set_gain(TX, CHAIN_1,_tx1_gain);
    set_gain(TX, CHAIN_2,_tx2_gain);
}

/* This is the internal tune function, not available for a host call.
 *
 * Calculate the VCO settings for the requested frquency, and then either
 * tune the RX or TX VCO. */
double ad9361_device_t::_tune_helper(direction_t direction, const double value)
{
    /* The RFPLL runs from 6 GHz - 12 GHz */
    const double fref = 80e6;
    const int modulus = 8388593;
    const double vcomax = 12e9;
    const double vcomin = 6e9;
    double vcorate;
    int vcodiv;

    /* Iterate over VCO dividers until appropriate divider is found. */
    int i;
    for (i = 0; i <= 6; i++) {
        vcodiv = 2 << i;
        vcorate = value * vcodiv;
        if (vcorate >= vcomin && vcorate <= vcomax)
            break;
    }
    if (i == 7)
        throw uhd::runtime_error("[ad9361_device_t] RFVCO can't find valid VCO rate!");

    int nint = static_cast<int>(vcorate / fref);
    int nfrac = static_cast<int>(((vcorate / fref) - nint) * modulus);

    double actual_vcorate = fref * (nint + (double) (nfrac) / modulus);
    double actual_lo = actual_vcorate / vcodiv;

    if (direction == RX) {

        _req_rx_freq = value;

        /* Set band-specific settings. */
        if (value < _client_params->get_band_edge(AD9361_RX_BAND0)) {
            _regs.inputsel = (_regs.inputsel & 0xC0) | 0x30; // Port C, balanced
        } else if ((value
                >= _client_params->get_band_edge(AD9361_RX_BAND0))
                && (value
                        < _client_params->get_band_edge(AD9361_RX_BAND1))) {
            _regs.inputsel = (_regs.inputsel & 0xC0) | 0x0C; // Port B, balanced
        } else if ((value
                >= _client_params->get_band_edge(AD9361_RX_BAND1))
                && (value <= 6e9)) {
            _regs.inputsel = (_regs.inputsel & 0xC0) | 0x03; // Port A, balanced
        } else {
            throw uhd::runtime_error("[ad9361_device_t] [_tune_helper] INVALID_CODE_PATH");
        }

        _io_iface->poke8(0x004, _regs.inputsel);

        /* Store vcodiv setting. */
        _regs.vcodivs = (_regs.vcodivs & 0xF0) | (i & 0x0F);

        /* Setup the synthesizer. */
        _setup_synth(RX, actual_vcorate);

        /* Tune!!!! */
        _io_iface->poke8(0x233, nfrac & 0xFF);
        _io_iface->poke8(0x234, (nfrac >> 8) & 0xFF);
        _io_iface->poke8(0x235, (nfrac >> 16) & 0xFF);
        _io_iface->poke8(0x232, (nint >> 8) & 0xFF);
        _io_iface->poke8(0x231, nint & 0xFF);
        _io_iface->poke8(0x005, _regs.vcodivs);

        /* Lock the PLL! */
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if ((_io_iface->peek8(0x247) & 0x02) == 0) {
            throw uhd::runtime_error("[ad9361_device_t] RX PLL NOT LOCKED");
        }

        _rx_freq = actual_lo;

        return actual_lo;

    } else {

        _req_tx_freq = value;

        /* Set band-specific settings. */
        if (value < _client_params->get_band_edge(AD9361_TX_BAND0)) {
            _regs.inputsel = _regs.inputsel | 0x40;
        } else if ((value
                >= _client_params->get_band_edge(AD9361_TX_BAND0))
                && (value <= 6e9)) {
            _regs.inputsel = _regs.inputsel & 0xBF;
        } else {
            throw uhd::runtime_error("[ad9361_device_t] [_tune_helper] INVALID_CODE_PATH");
        }

        _io_iface->poke8(0x004, _regs.inputsel);

        /* Store vcodiv setting. */
        _regs.vcodivs = (_regs.vcodivs & 0x0F) | ((i & 0x0F) << 4);

        /* Setup the synthesizer. */
        _setup_synth(TX, actual_vcorate);

        /* Tune it, homey. */
        _io_iface->poke8(0x273, nfrac & 0xFF);
        _io_iface->poke8(0x274, (nfrac >> 8) & 0xFF);
        _io_iface->poke8(0x275, (nfrac >> 16) & 0xFF);
        _io_iface->poke8(0x272, (nint >> 8) & 0xFF);
        _io_iface->poke8(0x271, nint & 0xFF);
        _io_iface->poke8(0x005, _regs.vcodivs);

        /* Lock the PLL! */
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if ((_io_iface->peek8(0x287) & 0x02) == 0) {
            throw uhd::runtime_error("[ad9361_device_t] TX PLL NOT LOCKED");
        }

        _tx_freq = actual_lo;

        return actual_lo;
    }
}

/* Configure the various clock / sample rates in the RX and TX chains.
 *
 * Functionally, this function configures AD9361's RX and TX rates. For
 * a requested TX & RX rate, it sets the interpolation & decimation filters,
 * and tunes the VCO that feeds the ADCs and DACs.
 */
double ad9361_device_t::_setup_rates(const double rate)
{
    /* If we make it into this function, then we are tuning to a new rate.
     * Store the new rate. */
    _req_clock_rate = rate;
    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::_setup_rates] rate=%.6d\n") % rate;

    /* Set the decimation and interpolation values in the RX and TX chains.
     * This also switches filters in / out. Note that all transmitters and
     * receivers have to be turned on for the calibration portion of
     * bring-up, and then they will be switched out to reflect the actual
     * user-requested antenna selections. */
    int divfactor = 0;
    _tfir_factor = 0;
    _rfir_factor = 0;

    if (rate < 0.33e6) {
        // RX1 + RX2 enabled, 3, 2, 2, 4
        _regs.rxfilt = B8(11101111);

        // TX1 + TX2 enabled, 3, 2, 2, 4
        _regs.txfilt = B8(11101111);

        divfactor = 48;
        _tfir_factor = 4;
        _rfir_factor = 4;
    } else if (rate < 0.66e6) {
        // RX1 + RX2 enabled, 2, 2, 2, 4
        _regs.rxfilt = B8(11011111);

        // TX1 + TX2 enabled, 2, 2, 2, 4
        _regs.txfilt = B8(11011111);

        divfactor = 32;
        _tfir_factor = 4;
        _rfir_factor = 4;
    } else if (rate <= 20e6) {
        // RX1 + RX2 enabled, 2, 2, 2, 2
        _regs.rxfilt = B8(11011110);

        // TX1 + TX2 enabled, 2, 2, 2, 2
        _regs.txfilt = B8(11011110);

        divfactor = 16;
        _tfir_factor = 2;
        _rfir_factor = 2;
    } else if ((rate > 20e6) && (rate < 23e6)) {
        // RX1 + RX2 enabled, 3, 2, 2, 2
        _regs.rxfilt = B8(11101110);

        // TX1 + TX2 enabled, 3, 1, 2, 2
        _regs.txfilt = B8(11100110);

        divfactor = 24;
        _tfir_factor = 2;
        _rfir_factor = 2;
    } else if ((rate >= 23e6) && (rate < 41e6)) {
        // RX1 + RX2 enabled, 2, 2, 2, 2
        _regs.rxfilt = B8(11011110);

        // TX1 + TX2 enabled, 1, 2, 2, 2
        _regs.txfilt = B8(11001110);

        divfactor = 16;
        _tfir_factor = 2;
        _rfir_factor = 2;
    } else if ((rate >= 41e6) && (rate <= 58e6)) {
        // RX1 + RX2 enabled, 3, 1, 2, 2
        _regs.rxfilt = B8(11100110);

        // TX1 + TX2 enabled, 3, 1, 1, 2
        _regs.txfilt = B8(11100010);

        divfactor = 12;
        _tfir_factor = 2;
        _rfir_factor = 2;
    } else if ((rate > 58e6) && (rate <= 61.44e6)) {
        // RX1 + RX2 enabled, 2, 1, 2, 2
        _regs.rxfilt = B8(11001110);


        // TX1 + TX2 enabled, 2, 1, 1, 2
        _regs.txfilt = B8(11010010);

        divfactor = 8;
        _tfir_factor = 2;
        _rfir_factor = 2;
    } else {
        // should never get in here
        throw uhd::runtime_error("[ad9361_device_t] [_setup_rates] INVALID_CODE_PATH");
    }

    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::_setup_rates] divfactor=%d\n") % divfactor;

    /* Tune the BBPLL to get the ADC and DAC clocks. */
    const double adcclk = _tune_bbvco(rate * divfactor);
    double dacclk = adcclk;

    /* The DAC clock must be <= 336e6, and is either the ADC clock or 1/2 the
     * ADC clock.*/
    if (adcclk > 336e6) {
        /* Make the DAC clock = ADC/2 */
        _regs.bbpll = _regs.bbpll | 0x08;
        dacclk = adcclk / 2.0;
    } else {
        _regs.bbpll = _regs.bbpll & 0xF7;
    }

    /* Set the dividers / interpolators in AD9361. */
    _io_iface->poke8(0x002, _regs.txfilt);
    _io_iface->poke8(0x003, _regs.rxfilt);
    _io_iface->poke8(0x004, _regs.inputsel);
    _io_iface->poke8(0x00A, _regs.bbpll);

    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::_setup_rates] adcclk=%f\n") % adcclk;
    _baseband_bw = (adcclk / divfactor);

    /*
     The Tx & Rx FIR calculate 16 taps per clock cycle. This limits the number of available taps to the ratio of DAC_CLK/ADC_CLK
     to the input data rate multiplied by 16. For example, if the input data rate is 25 MHz and DAC_CLK is 100 MHz,
     then the ratio of DAC_CLK to the input data rate is 100/25 or 4. In this scenario, the total number of taps available is 64.

     Also, whilst the Rx FIR filter always has memory available for 128 taps, the Tx FIR Filter can only support a maximum length of 64 taps
     in 1x interpolation mode, and 128 taps in 2x & 4x modes.
     */
    const size_t max_tx_taps = std::min<size_t>(
            std::min<size_t>((16 * (int)((dacclk / rate) + 0.5)), 128),
            (_tfir_factor == 1) ? 64 : 128);
    const size_t max_rx_taps = std::min<size_t>((16 * (size_t)((adcclk / rate) + 0.5)),
            128);

    const size_t num_tx_taps = get_num_taps(max_tx_taps);
    const size_t num_rx_taps = get_num_taps(max_rx_taps);

    _setup_tx_fir(num_tx_taps,_tfir_factor);
    _setup_rx_fir(num_rx_taps,_rfir_factor);

    return _baseband_bw;
}

/***********************************************************************
 * Publicly exported functions to host calls
 **********************************************************************/
void ad9361_device_t::initialize()
{
    boost::lock_guard<boost::recursive_mutex> lock(_mutex);

    /* Initialize shadow registers. */
    _regs.vcodivs = 0x00;
    _regs.inputsel = 0x30;
    _regs.rxfilt = 0x00;
    _regs.txfilt = 0x00;
    _regs.bbpll = 0x02;
    _regs.bbftune_config = 0x1e;
    _regs.bbftune_mode = 0x1e;

    /* Initialize private VRQ fields. */
    _rx_freq = DEFAULT_RX_FREQ;
    _tx_freq = DEFAULT_TX_FREQ;
    _req_rx_freq = 0.0;
    _req_tx_freq = 0.0;
    _baseband_bw = 0.0;
    _req_clock_rate = 0.0;
    _req_coreclk = 0.0;
    _bbpll_freq = 0.0;
    _adcclock_freq = 0.0;
    _rx_bbf_tunediv = 0;
    _curr_gain_table = 0;
    _rx1_gain = 0;
    _rx2_gain = 0;
    _tx1_gain = 0;
    _tx2_gain = 0;
    _use_dc_offset_tracking = true;
    _use_iq_balance_tracking = true;
    _rx1_agc_mode = GAIN_MODE_SLOW_AGC;
    _rx2_agc_mode = GAIN_MODE_SLOW_AGC;
    _rx1_agc_enable = false;
    _rx2_agc_enable = false;
    _rx_analog_bw = 0;
    _tx_analog_bw = 0;
    _rx_tia_lp_bw = 0;
    _tx_sec_lp_bw = 0;
    _rx_bb_lp_bw = 0;
    _tx_bb_lp_bw = 0;

    /* Reset the device. */
    _io_iface->poke8(0x000, 0x01);
    _io_iface->poke8(0x000, 0x00);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    /* Check device ID to make sure iface works */
    uint32_t device_id = (_io_iface->peek8(0x037) & 0x8);
    if (device_id != 0x8) {
        throw uhd::runtime_error(str(boost::format("[ad9361_device_t::initialize] Device ID readback failure. Expected: 0x8, Received: 0x%x") % device_id));
    }

    /* There is not a WAT big enough for this. */
    _io_iface->poke8(0x3df, 0x01);

    _io_iface->poke8(0x2a6, 0x0e); // Enable master bias
    _io_iface->poke8(0x2a8, 0x0e); // Set bandgap trim

    /* Set RFPLL ref clock scale to REFCLK * 2 */
    _io_iface->poke8(0x2ab, 0x07);
    _io_iface->poke8(0x2ac, 0xff);

    /* Enable clocks. */
    switch (_client_params->get_clocking_mode()) {
    case clocking_mode_t::AD9361_XTAL_N_CLK_PATH: {
        _io_iface->poke8(0x009, 0x17);
    } break;

    case clocking_mode_t::AD9361_XTAL_P_CLK_PATH: {
        _io_iface->poke8(0x009, 0x07);
        _io_iface->poke8(0x292, 0x08);
        _io_iface->poke8(0x293, 0x80);
        _io_iface->poke8(0x294, 0x00);
        _io_iface->poke8(0x295, 0x14);
    } break;

    default:
        throw uhd::runtime_error("[ad9361_device_t] NOT IMPLEMENTED");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    /* Tune the BBPLL, write TX and RX FIRS. */
    _setup_rates(50e6);

    /* Setup data ports (FDD dual port DDR):
     *      FDD dual port DDR CMOS no swap.
     *      Force TX on one port, RX on the other. */
    switch (_client_params->get_digital_interface_mode()) {
    case AD9361_DDR_FDD_LVCMOS: {
        _io_iface->poke8(0x010, 0xc8); // Swap I&Q on Tx, Swap I&Q on Rx, Toggle frame sync mode
        _io_iface->poke8(0x011, 0x00);
        _io_iface->poke8(0x012, 0x02);
    } break;

    case AD9361_DDR_FDD_LVDS: {
        _io_iface->poke8(0x010, 0xcc); // Swap I&Q on Tx, Swap I&Q on Rx, Toggle frame sync mode, 2R2T timing.
        _io_iface->poke8(0x011, 0x00);
        _io_iface->poke8(0x012, 0x10);

        //LVDS Specific
        _io_iface->poke8(0x03C, 0x23);
        _io_iface->poke8(0x03D, 0xFF);
        _io_iface->poke8(0x03E, 0x0F);
    } break;

    default:
        throw uhd::runtime_error("[ad9361_device_t] NOT IMPLEMENTED");
    }

    /* Data delay for TX and RX data clocks */
    digital_interface_delays_t timing =
            _client_params->get_digital_interface_timing();
    uint8_t rx_delays = ((timing.rx_clk_delay & 0xF) << 4)
            | (timing.rx_data_delay & 0xF);
    uint8_t tx_delays = ((timing.tx_clk_delay & 0xF) << 4)
            | (timing.tx_data_delay & 0xF);
    _io_iface->poke8(0x006, rx_delays);
    _io_iface->poke8(0x007, tx_delays);

    /* Setup AuxDAC */
    _io_iface->poke8(0x018, 0x00); // AuxDAC1 Word[9:2]
    _io_iface->poke8(0x019, 0x00); // AuxDAC2 Word[9:2]
    _io_iface->poke8(0x01A, 0x00); // AuxDAC1 Config and Word[1:0]
    _io_iface->poke8(0x01B, 0x00); // AuxDAC2 Config and Word[1:0]
    _io_iface->poke8(0x023, 0xFF); // AuxDAC Manaul/Auto Control
    _io_iface->poke8(0x026, 0x00); // AuxDAC Manual Select Bit/GPO Manual Select
    _io_iface->poke8(0x030, 0x00); // AuxDAC1 Rx Delay
    _io_iface->poke8(0x031, 0x00); // AuxDAC1 Tx Delay
    _io_iface->poke8(0x032, 0x00); // AuxDAC2 Rx Delay
    _io_iface->poke8(0x033, 0x00); // AuxDAC2 Tx Delay

    /* LNA bypass polarity inversion
     *     According to the register map, we should invert the bypass path to
     *     match LNA phase. Extensive testing, however, shows otherwise and that
     *     to align bypass and LNA phases, the bypass inversion switch should be
     *     turned off.
     */
    _io_iface->poke8(0x022, 0x0A);

    /* Setup AuxADC */
    _io_iface->poke8(0x00B, 0x00); // Temp Sensor Setup (Offset)
    _io_iface->poke8(0x00C, 0x00); // Temp Sensor Setup (Temp Window)
    _io_iface->poke8(0x00D, 0x00); // Temp Sensor Setup (Manual  Measure)
    _io_iface->poke8(0x00F, 0x04); // Temp Sensor Setup (Decimation)
    _io_iface->poke8(0x01C, 0x10); // AuxADC Setup (Clock Div)
    _io_iface->poke8(0x01D, 0x01); // AuxADC Setup (Decimation/Enable)

    /* Setup control outputs. */
    _io_iface->poke8(0x035, 0x01);
    _io_iface->poke8(0x036, 0xFF);

    /* Setup GPO */
    _io_iface->poke8(0x03a, 0x27); //set delay register
    _io_iface->poke8(0x020, 0x00); // GPO Auto Enable Setup in RX and TX
    _io_iface->poke8(0x027, 0x03); // GPO Manual and GPO auto value in ALERT
    _io_iface->poke8(0x028, 0x00); // GPO_0 RX Delay
    _io_iface->poke8(0x029, 0x00); // GPO_1 RX Delay
    _io_iface->poke8(0x02A, 0x00); // GPO_2 RX Delay
    _io_iface->poke8(0x02B, 0x00); // GPO_3 RX Delay
    _io_iface->poke8(0x02C, 0x00); // GPO_0 TX Delay
    _io_iface->poke8(0x02D, 0x00); // GPO_1 TX Delay
    _io_iface->poke8(0x02E, 0x00); // GPO_2 TX Delay
    _io_iface->poke8(0x02F, 0x00); // GPO_3 TX Delay

    _io_iface->poke8(0x261, 0x00); // RX LO power
    _io_iface->poke8(0x2a1, 0x00); // TX LO power
    _io_iface->poke8(0x248, 0x0b); // en RX VCO LDO
    _io_iface->poke8(0x288, 0x0b); // en TX VCO LDO
    _io_iface->poke8(0x246, 0x02); // pd RX cal Tcf
    _io_iface->poke8(0x286, 0x02); // pd TX cal Tcf
    _io_iface->poke8(0x249, 0x8e); // rx vco cal length
    _io_iface->poke8(0x289, 0x8e); // rx vco cal length
    _io_iface->poke8(0x23b, 0x80); // set RX MSB?, FIXME 0x89 magic cp
    _io_iface->poke8(0x27b, 0x80); // "" TX //FIXME 0x88 see above
    _io_iface->poke8(0x243, 0x0d); // set rx prescaler bias
    _io_iface->poke8(0x283, 0x0d); // "" TX

    _io_iface->poke8(0x23d, 0x00); // Clear half VCO cal clock setting
    _io_iface->poke8(0x27d, 0x00); // Clear half VCO cal clock setting

    /* The order of the following process is EXTREMELY important. If the
     * below functions are modified at all, device initialization and
     * calibration might be broken in the process! */

    _io_iface->poke8(0x015, 0x04); // dual synth mode, synth en ctrl en
    _io_iface->poke8(0x014, 0x05); // use SPI for TXNRX ctrl, to ALERT, TX on
    _io_iface->poke8(0x013, 0x01); // enable ENSM
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    _calibrate_synth_charge_pumps();

    _tune_helper(RX, _rx_freq);
    _tune_helper(TX, _tx_freq);

    _program_mixer_gm_subtable();
    _program_gain_table();
    _setup_gain_control(false);

    set_bw_filter(RX, _baseband_bw);
    set_bw_filter(TX, _baseband_bw);

    _setup_adc();

    _calibrate_baseband_dc_offset();
    _calibrate_rf_dc_offset();
    _calibrate_rx_quadrature();

    /*
     * Rx BB DC and IQ tracking are both disabled by calibration at this
     * point. Only issue commands if tracking needs to be turned on.
     */
    if (_use_dc_offset_tracking)
        _configure_bb_dc_tracking();
    if (_use_iq_balance_tracking)
        _configure_rx_iq_tracking();

    _last_rx_cal_freq = _rx_freq;
    _last_tx_cal_freq = _tx_freq;

    // cals done, set PPORT config
    switch (_client_params->get_digital_interface_mode()) {
    case AD9361_DDR_FDD_LVCMOS: {
        _io_iface->poke8(0x012, 0x02);
    } break;

    case AD9361_DDR_FDD_LVDS: {
        _io_iface->poke8(0x012, 0x10);
    } break;

    default:
        throw uhd::runtime_error("[ad9361_device_t] NOT IMPLEMENTED");
    }

    _io_iface->poke8(0x013, 0x01); // Set ENSM FDD bit
    _io_iface->poke8(0x015, 0x04); // dual synth mode, synth en ctrl en

    /* Default TX attentuation to 10dB on both TX1 and TX2 */
    _io_iface->poke8(0x073, 0x00);
    _io_iface->poke8(0x074, 0x00);
    _io_iface->poke8(0x075, 0x00);
    _io_iface->poke8(0x076, 0x00);

    /* Setup RSSI Measurements */
    _io_iface->poke8(0x150, 0x0E); // RSSI Measurement Duration 0, 1
    _io_iface->poke8(0x151, 0x00); // RSSI Measurement Duration 2, 3
    _io_iface->poke8(0x152, 0xFF); // RSSI Weighted Multiplier 0
    _io_iface->poke8(0x153, 0x00); // RSSI Weighted Multiplier 1
    _io_iface->poke8(0x154, 0x00); // RSSI Weighted Multiplier 2
    _io_iface->poke8(0x155, 0x00); // RSSI Weighted Multiplier 3
    _io_iface->poke8(0x156, 0x00); // RSSI Delay
    _io_iface->poke8(0x157, 0x00); // RSSI Wait
    _io_iface->poke8(0x158, 0x0D); // RSSI Mode Select
    _io_iface->poke8(0x15C, 0x67); // Power Measurement Duration

    /* Turn on the default RX & TX chains. */
    set_active_chains(true, false, false, false);

    /* Set TXers & RXers on (only works in FDD mode) */
    _io_iface->poke8(0x014, 0x21);
}

void ad9361_device_t::set_io_iface(ad9361_io::sptr io_iface)
{
    _io_iface = io_iface;
}

/* This function sets the RX / TX rate between AD9361 and the FPGA, and
 * thus determines the interpolation / decimation required in the FPGA to
 * achieve the user's requested rate.
 *
 * This is the only clock setting function that is exposed to the outside. */
double ad9361_device_t::set_clock_rate(const double req_rate)
{
    boost::lock_guard<boost::recursive_mutex> lock(_mutex);

    if (req_rate > 61.44e6) {
        throw uhd::runtime_error("[ad9361_device_t] Requested master clock rate outside range");
    }

    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::set_clock_rate] req_rate=%.10f\n") % req_rate;

    /* UHD has a habit of requesting the same rate like four times when it
     * starts up. This prevents that, and any bugs in user code that request
     * the same rate over and over. */
    if (freq_is_nearly_equal(req_rate, _req_clock_rate)) {
        // We return _baseband_bw, because that's closest to the
        // actual value we're currently running.
        return _baseband_bw;
    }

    /* We must be in the SLEEP / WAIT state to do this. If we aren't already
     * there, transition the ENSM to State 0. */
    uint8_t current_state = _io_iface->peek8(0x017) & 0x0F;
    switch (current_state) {
    case 0x05:
        /* We are in the ALERT state. */
        _io_iface->poke8(0x014, 0x21);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        _io_iface->poke8(0x014, 0x00);
        break;

    case 0x0A:
        /* We are in the FDD state. */
        _io_iface->poke8(0x014, 0x00);
        break;

    default:
        throw uhd::runtime_error("[ad9361_device_t] [set_clock_rate:1] AD9361 in unknown state");
        break;
    };

    /* Store the current chain / antenna selections so that we can restore
     * them at the end of this routine; all chains will be enabled from
     * within setup_rates for calibration purposes. */
    uint8_t orig_tx_chains = _regs.txfilt & 0xC0;
    uint8_t orig_rx_chains = _regs.rxfilt & 0xC0;

    /* Call into the clock configuration / settings function. This is where
     * all the hard work gets done. */
    double rate = _setup_rates(req_rate);

    UHD_LOGGER_TRACE("AD936X")<< boost::format("[ad9361_device_t::set_clock_rate] rate=%.10f\n") % rate;

    /* Transition to the ALERT state and calibrate everything. */
    _io_iface->poke8(0x015, 0x04); //dual synth mode, synth en ctrl en
    _io_iface->poke8(0x014, 0x05); //use SPI for TXNRX ctrl, to ALERT, TX on
    _io_iface->poke8(0x013, 0x01); //enable ENSM
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    _calibrate_synth_charge_pumps();

    _tune_helper(RX, _rx_freq);
    _tune_helper(TX, _tx_freq);

    _program_mixer_gm_subtable();
    _program_gain_table();
    _setup_gain_control(false);
    _reprogram_gains();

    set_bw_filter(RX, _baseband_bw);
    set_bw_filter(TX, _baseband_bw);

    _setup_adc();

    _calibrate_baseband_dc_offset();
    _calibrate_rf_dc_offset();
    _calibrate_rx_quadrature();

    /*
     * Rx BB DC and IQ tracking are both disabled by calibration at this
     * point. Only issue commands if tracking needs to be turned on.
     */
    if (_use_dc_offset_tracking)
        _configure_bb_dc_tracking();
    if (_use_iq_balance_tracking)
        _configure_rx_iq_tracking();

    _last_rx_cal_freq = _rx_freq;
    _last_tx_cal_freq = _tx_freq;

    // cals done, set PPORT config
    switch (_client_params->get_digital_interface_mode()) {
        case AD9361_DDR_FDD_LVCMOS: {
            _io_iface->poke8(0x012, 0x02);
        }break;

        case AD9361_DDR_FDD_LVDS: {
            _io_iface->poke8(0x012, 0x10);
        }break;

        default:
        throw uhd::runtime_error("[ad9361_device_t] NOT IMPLEMENTED");
    }
    _io_iface->poke8(0x013, 0x01); // Set ENSM FDD bit
    _io_iface->poke8(0x015, 0x04); // dual synth mode, synth en ctrl en

    /* End the function in the same state as the entry state. */
    switch (current_state) {
    case 0x05:
        /* We are already in ALERT. */
        break;

    case 0x0A:
        /* Transition back to FDD, and restore the original antenna
         * / chain selections. */
        _regs.txfilt = (_regs.txfilt & 0x3F) | orig_tx_chains;
        _regs.rxfilt = (_regs.rxfilt & 0x3F) | orig_rx_chains;

        _io_iface->poke8(0x002, _regs.txfilt);
        _io_iface->poke8(0x003, _regs.rxfilt);
        _io_iface->poke8(0x014, 0x21);
        break;

    default:
        throw uhd::runtime_error("[ad9361_device_t] [set_clock_rate:2] AD9361 in unknown state");
        break;
    };

    return rate;
}


/* Set which of the four TX / RX chains provided by AD9361 are active.
 *
 * AD9361 provides two sets of chains, Side A and Side B. Each side
 * provides one TX antenna, and one RX antenna. The B200 maintains the USRP
 * standard of providing one antenna connection that is both TX & RX, and
 * one that is RX-only - for each chain. Thus, the possible antenna and
 * chain selections are:
 *
 *  B200 Antenna    AD9361 Side       AD9361 Chain
 *  -------------------------------------------------------------------
 *  TX / RX1        Side A              TX1 (when switched to TX)
 *  TX / RX1        Side A              RX1 (when switched to RX)
 *  RX1             Side A              RX1
 *
 *  TX / RX2        Side B              TX2 (when switched to TX)
 *  TX / RX2        Side B              RX2 (when switched to RX)
 *  RX2             Side B              RX2
 */
void ad9361_device_t::set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2)
{
    boost::lock_guard<boost::recursive_mutex> lock(_mutex);

    /* Clear out the current active chain settings. */
    _regs.txfilt = _regs.txfilt & 0x3F;
    _regs.rxfilt = _regs.rxfilt & 0x3F;

    /* Turn on the different chains based on the passed parameters. */
    if (tx1) {
        _regs.txfilt = _regs.txfilt | 0x40;
    }
    if (tx2) {
        _regs.txfilt = _regs.txfilt | 0x80;
    }
    if (rx1) {
        _regs.rxfilt = _regs.rxfilt | 0x40;
    }
    if (rx2) {
        _regs.rxfilt = _regs.rxfilt | 0x80;
    }

    /* Check for FDD state */
    uint8_t set_back_to_fdd = 0;
    uint8_t ensm_state = _io_iface->peek8(0x017) & 0x0F;
    if (ensm_state == 0xA)   // FDD
            {
        /* Put into ALERT state (via the FDD flush state). */
        _io_iface->poke8(0x014, 0x01);
        set_back_to_fdd = 1;
    }

    /* Wait for FDD flush state to complete (if necessary) */
    while (ensm_state == 0xA || ensm_state == 0xB)
        ensm_state = _io_iface->peek8(0x017) & 0x0F;

    /* Turn on / off the chains. */
    _io_iface->poke8(0x002, _regs.txfilt);
    _io_iface->poke8(0x003, _regs.rxfilt);

    /*
     * Last unconditional Tx calibration point. Any later Tx calibration will
     * require user intervention (currently triggered by tuning difference that
     * is > 100 MHz). Late calibration provides better performance.
     */
    if (tx1 | tx2)
        _calibrate_tx_quadrature();

    /* Put back into FDD state if necessary */
    if (set_back_to_fdd)
        _io_iface->poke8(0x014, 0x21);
}

/* Setup Timing mode depending on active channels.
 *
 * LVDS interface can have two timing modes - 1R1T and 2R2T
 */
void ad9361_device_t::set_timing_mode(const ad9361_device_t::timing_mode_t timing_mode)
{
    switch (_client_params->get_digital_interface_mode()) {
    case AD9361_DDR_FDD_LVCMOS: {
        switch(timing_mode) {
        case TIMING_MODE_1R1T: {
            _io_iface->poke8(0x010, 0xc8); // Swap I&Q on Tx, Swap I&Q on Rx, Toggle frame sync mode
            break;
        }
        case TIMING_MODE_2R2T: {
            throw uhd::runtime_error("[ad9361_device_t] [set_timing_mode] 2R2T timing mode not supported for CMOS");
            break;
        }
        default:
        UHD_THROW_INVALID_CODE_PATH();
        }
    break;
    }
    case AD9361_DDR_FDD_LVDS: {
        switch(timing_mode) {
        case TIMING_MODE_1R1T: {
            _io_iface->poke8(0x010, 0xc8); // Swap I&Q on Tx, Swap I&Q on Rx, Toggle frame sync mode, 1R1T timing.
            break;
        }
        case TIMING_MODE_2R2T: {
            _io_iface->poke8(0x010, 0xcc); // Swap I&Q on Tx, Swap I&Q on Rx, Toggle frame sync mode, 2R2T timing.
            break;
        }
        default:
        UHD_THROW_INVALID_CODE_PATH();
        }
    break;
    }
    default:
        throw uhd::runtime_error("[ad9361_device_t] NOT IMPLEMENTED");
    }
}

/* Tune the RX or TX frequency.
 *
 * This is the publicly-accessible tune function. It makes sure the tune
 * isn't a redundant request, and if not, passes it on to the class's
 * internal tune function.
 *
 * After tuning, it runs any appropriate calibrations. */
double ad9361_device_t::tune(direction_t direction, const double value)
{
    boost::lock_guard<boost::recursive_mutex> lock(_mutex);
    double last_cal_freq;

    if (direction == RX) {
        if (freq_is_nearly_equal(value, _req_rx_freq)) {
            return _rx_freq;
        }
        last_cal_freq = _last_rx_cal_freq;
    } else if (direction == TX) {
        if (freq_is_nearly_equal(value, _req_tx_freq)) {
            return _tx_freq;
        }
        last_cal_freq = _last_tx_cal_freq;
    } else {
        throw uhd::runtime_error("[ad9361_device_t] [tune] INVALID_CODE_PATH");
    }

    /* If we aren't already in the ALERT state, we will need to return to
     * the FDD state after tuning. */
    int not_in_alert = 0;
    if ((_io_iface->peek8(0x017) & 0x0F) != 5) {
        /* Force the device into the ALERT state. */
        not_in_alert = 1;
        _io_iface->poke8(0x014, 0x01);
    }

    /* Tune the RF VCO! */
    double tune_freq = _tune_helper(direction, value);

    /* Run any necessary calibrations / setups */
    if (direction == RX) {
        _program_gain_table();
    }

    /* Update the gain settings. */
    _reprogram_gains();

    /*
     * Only run the following calibrations if we are more than 100MHz away
     * from the previous Tx or Rx calibration point. Leave out single shot
     * Rx quadrature unless Rx quad-cal is disabled.
     */
    if (std::abs(last_cal_freq - tune_freq) > AD9361_CAL_VALID_WINDOW) {
        /* Run the calibration algorithms. */
        if (direction == RX) {
            _calibrate_rf_dc_offset();
            if (!_use_iq_balance_tracking)
                _calibrate_rx_quadrature();
            if (_use_dc_offset_tracking)
                _configure_bb_dc_tracking();

            _last_rx_cal_freq = tune_freq;
        } else {
            _calibrate_tx_quadrature();
            _last_tx_cal_freq = tune_freq;
        }

        /* Rx IQ tracking can be disabled on Rx or Tx re-calibration */
        if (_use_iq_balance_tracking)
            _configure_rx_iq_tracking();
    }

    /* If we were in the FDD state, return it now. */
    if (not_in_alert) {
        _io_iface->poke8(0x014, 0x21);
    }

    return tune_freq;
}

/* Get the current RX or TX frequency. */
double ad9361_device_t::get_freq(direction_t direction)
{
    boost::lock_guard<boost::recursive_mutex> lock(_mutex);

    if (direction == RX)
        return _rx_freq;
    else
        return _tx_freq;
}

/* Set the gain of RX1, RX2, TX1, or TX2.
 *
 * Note that the 'value' passed to this function is the gain index
 * for RX. Also note that the RX chains are done in terms of gain, and
 * the TX chains  are done in terms of attenuation. */
double ad9361_device_t::set_gain(direction_t direction, chain_t chain, const double value)
{
    boost::lock_guard<boost::recursive_mutex> lock(_mutex);

    if (direction == RX) {

        int gain_index = static_cast<int>(value);

        /* Clip the gain values to the proper min/max gain values. */
        if (gain_index > 76)
            gain_index = 76;
        if (gain_index < 0)
            gain_index = 0;

        if (chain == CHAIN_1) {
            _rx1_gain = value;
            _io_iface->poke8(0x109, gain_index);
        } else {
            _rx2_gain = value;
            _io_iface->poke8(0x10c, gain_index);
        }

        return gain_index;
    } else {
        /* Setting the below bits causes a change in the TX attenuation word
         * to immediately take effect. */
        _io_iface->poke8(0x077, 0x40);
        _io_iface->poke8(0x07c, 0x40);

        /* Each gain step is -0.25dB. Calculate the attenuation necessary
         * for the requested gain, convert it into gain steps, then write
         * the attenuation word. Max gain (so zero attenuation) is 89.75.
         * Ugly values will be written to the attenuation registers if
         * "value" is out of bounds, so range checking must be performed
         * outside this function.
         */
        double atten = AD9361_MAX_GAIN - value;
        uint32_t attenreg = uint32_t(atten * 4);
        if (chain == CHAIN_1) {
            _tx1_gain = value;
            _io_iface->poke8(0x073, attenreg & 0xFF);
            _io_iface->poke8(0x074, (attenreg >> 8) & 0x01);
        } else {
            _tx2_gain = value;
            _io_iface->poke8(0x075, attenreg & 0xFF);
            _io_iface->poke8(0x076, (attenreg >> 8) & 0x01);
        }
        return AD9361_MAX_GAIN - ((double) (attenreg) / 4);
    }
}

void ad9361_device_t::output_test_tone()  // On RF side!
{
    boost::lock_guard<boost::recursive_mutex> lock(_mutex);
    /* Output a 480 kHz tone at 800 MHz */
    _io_iface->poke8(0x3F4, 0x0B);
    _io_iface->poke8(0x3FC, 0xFF);
    _io_iface->poke8(0x3FD, 0xFF);
    _io_iface->poke8(0x3FE, 0x3F);
}

void ad9361_device_t::digital_test_tone(bool enb) // Digital output
{
    boost::lock_guard<boost::recursive_mutex> lock(_mutex);
    _io_iface->poke8(0x3F4, 0x02 | (enb ? 0x01 : 0x00));
}

void ad9361_device_t::data_port_loopback(const bool loopback_enabled)
{
    boost::lock_guard<boost::recursive_mutex> lock(_mutex);
    _io_iface->poke8(0x3F5, (loopback_enabled ? 0x01 : 0x00));
}

/* Read back the internal RSSI measurement data. The result is in dB
 * but not in absolute units. If absolute units are required
 * a bench calibration should be done.
 * -0.25dB / bit 9bit resolution.*/
double ad9361_device_t::get_rssi(chain_t chain)
{
    uint32_t reg_rssi = 0;
    uint8_t lsb_bit_pos = 0;
    if (chain == CHAIN_1) {
        reg_rssi = 0x1A7;
        lsb_bit_pos = 0;
    }else {
        reg_rssi = 0x1A9;
        lsb_bit_pos = 1;
    }
    uint8_t msbs = _io_iface->peek8(reg_rssi);
    uint8_t lsb = ((_io_iface->peek8(0x1AB)) >> lsb_bit_pos) & 0x01;
    uint16_t val = ((msbs << 1) | lsb);
    double rssi = (-0.25f * ((double)val)); //-0.25dB/lsb (See Gain Control Users Guide p. 25)
    return rssi;
}

/*
 * Returns the reading of the internal temperature sensor.
 * One point calibration of the sensor was done according to datasheet
 * leading to the given default constant correction factor.
 */
double ad9361_device_t::_get_temperature(const double cal_offset, const double timeout)
{
    //set 0x01D[0] to 1 to disable AuxADC GPIO reading
    uint8_t tmp = 0;
    tmp = _io_iface->peek8(0x01D);
    _io_iface->poke8(0x01D, (tmp | 0x01));
    _io_iface->poke8(0x00B, 0); //set offset to 0

    _io_iface->poke8(0x00C, 0x01); //start reading, clears bit 0x00C[1]
    auto end_time =
        std::chrono::steady_clock::now()
        + std::chrono::milliseconds(int64_t(timeout * 1000));
    //wait for valid data (toggle of bit 1 in 0x00C)
    while(((_io_iface->peek8(0x00C) >> 1) & 0x01) == 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        if (std::chrono::steady_clock::now() > end_time) {
            throw uhd::runtime_error(
                "[ad9361_device_t] timeout while reading temperature");
        }
    }
    _io_iface->poke8(0x00C, 0x00); //clear read flag

    uint8_t temp = _io_iface->peek8(0x00E); //read temperature.
    double tmp_temp = temp/1.140f; //according to ADI driver
    tmp_temp = tmp_temp + cal_offset; //Constant offset acquired by one point calibration.

    return tmp_temp;
}

double ad9361_device_t::get_average_temperature(const double cal_offset, const size_t num_samples)
{
    double d_temp = 0;
    for(size_t i = 0; i < num_samples; i++) {
        double tmp_temp = _get_temperature(cal_offset);
        d_temp += (tmp_temp/num_samples);
    }
    return d_temp;
}

/*
 * Enable/Disable DC offset tracking
 *
 * Only disable BB tracking while leaving static RF and BB DC calibrations enabled.
 * According to correspondance from ADI, turning off Rx BB DC tracking clears the
 * correction words so we don't need to be concerned with leaving the calibration
 * in a bad state upon disabling. Testing also confirms this behavior.
 *
 * Note that Rx IQ tracking does not show similar state clearing behavior when
 * disabled.
 */
void ad9361_device_t::set_dc_offset_auto(direction_t direction, const bool on)
{
    if (direction == RX) {
        _use_dc_offset_tracking = on;
        _configure_bb_dc_tracking();
    } else {
        throw uhd::runtime_error("[ad9361_device_t] [set_dc_offset_auto] Tx DC tracking not supported");
    }
}

/*
 * Enable/Disable IQ balance tracking
 *
 * Run static Rx quadrature calibration after disabling quadrature tracking.
 * This avoids the situation where a user might disable tracking when the loop
 * is in a confused state (e.g. at or near saturation). Otherwise, the
 * calibration setting could be forced to and left in a bad state.
 */
void ad9361_device_t::set_iq_balance_auto(direction_t direction, const bool on)
{
    if (direction == RX) {
        _use_iq_balance_tracking = on;
        _configure_rx_iq_tracking();
        if (!on) {
            _io_iface->poke8(0x014, 0x05); // ALERT mode
            _calibrate_rx_quadrature();
            _io_iface->poke8(0x014, 0x21); // FDD mode
        }
    } else {
        throw uhd::runtime_error("[ad9361_device_t] [set_iq_balance_auto] Tx IQ tracking not supported");
    }
}

/* Sets the RX gain mode to be used.
 * If a transition from an AGC to an non AGC mode occurs (or vice versa)
 * the gain configuration will be reloaded. */
void ad9361_device_t::_setup_agc(chain_t chain, gain_mode_t gain_mode)
{
    uint8_t gain_mode_reg = 0;
    uint8_t gain_mode_prev = 0;
    uint8_t gain_mode_bits_pos = 0;

    gain_mode_reg = _io_iface->peek8(0x0FA);
    gain_mode_prev = (gain_mode_reg & 0x0F);

    if (chain == CHAIN_1) {
        gain_mode_bits_pos = 0;
    } else if (chain == CHAIN_2) {
        gain_mode_bits_pos = 2;
    } else
    {
        throw uhd::runtime_error("[ad9361_device_t] Wrong value for chain");
    }

    gain_mode_reg = (gain_mode_reg & (~(0x03<<gain_mode_bits_pos))); //clear mode bits
    switch (gain_mode) {
        case GAIN_MODE_MANUAL:
            //leave bits cleared
            break;
        case GAIN_MODE_SLOW_AGC:
            gain_mode_reg = (gain_mode_reg | (0x02<<gain_mode_bits_pos));
            break;
        case GAIN_MODE_FAST_AGC:
            gain_mode_reg = (gain_mode_reg | (0x01<<gain_mode_bits_pos));
            break;
        default:
            throw uhd::runtime_error("[ad9361_device_t] Gain mode does not exist");
    }
    _io_iface->poke8(0x0FA, gain_mode_reg);
    uint8_t gain_mode_status = _io_iface->peek8(0x0FA);
    gain_mode_status = (gain_mode_status & 0x0F);
    /*Check if gain mode configuration needs to be reprogrammed*/
    if (((gain_mode_prev == 0) && (gain_mode_status != 0)) || ((gain_mode_prev != 0) && (gain_mode_status == 0))) {
        if (gain_mode_status == 0) {
            /*load manual mode config*/
            _setup_gain_control(false);
        } else {
            /*load agc mode config*/
            _setup_gain_control(true);
        }
    }
}

void ad9361_device_t::set_agc(chain_t chain, bool enable)
{
    if(chain == CHAIN_1) {
        _rx1_agc_enable = enable;
        if(enable) {
            _setup_agc(chain, _rx1_agc_mode);
        } else {
            _setup_agc(chain, GAIN_MODE_MANUAL);
        }
    } else if (chain == CHAIN_2){
        _rx2_agc_enable = enable;
        if(enable) {
            _setup_agc(chain, _rx2_agc_mode);
        } else {
            _setup_agc(chain, GAIN_MODE_MANUAL);
        }
    } else
    {
        throw uhd::runtime_error("[ad9361_device_t] Wrong value for chain");
    }
}

void ad9361_device_t::set_agc_mode(chain_t chain, gain_mode_t gain_mode)
{
    if(chain == CHAIN_1) {
        _rx1_agc_mode = gain_mode;
        if(_rx1_agc_enable) {
            _setup_agc(chain, _rx1_agc_mode);
        }
    } else if(chain == CHAIN_2){
        _rx2_agc_mode = gain_mode;
        if(_rx2_agc_enable) {
            _setup_agc(chain, _rx2_agc_mode);
        }
    } else
    {
        throw uhd::runtime_error("[ad9361_device_t] Wrong value for chain");
    }
}

std::vector<std::string> ad9361_device_t::get_filter_names(direction_t direction)
{
    std::vector<std::string> ret;
    if(direction == RX) {
        for(std::map<std::string, filter_query_helper>::iterator it = _rx_filters.begin(); it != _rx_filters.end(); ++it) {
            ret.push_back(it->first);
        }
    } else if (direction == TX)
    {
        for(std::map<std::string, filter_query_helper>::iterator it = _tx_filters.begin(); it != _tx_filters.end(); ++it) {
            ret.push_back(it->first);
        }
    }
    return ret;
}

filter_info_base::sptr ad9361_device_t::get_filter(direction_t direction, chain_t chain, const std::string &name)
{
    if(direction == RX) {
        if (not _rx_filters[name].get)
        {
            throw uhd::runtime_error("ad9361_device_t::get_filter this filter can not be read.");
        }
        return _rx_filters[name].get(direction, chain);
    } else if (direction == TX) {
        if (not _tx_filters[name].get)
        {
            throw uhd::runtime_error("ad9361_device_t::get_filter this filter can not be read.");
        }
        return _tx_filters[name].get(direction, chain);
    }

    throw uhd::runtime_error("ad9361_device_t::get_filter wrong direction parameter.");
}

void ad9361_device_t::set_filter(direction_t direction, chain_t chain, const std::string &name, filter_info_base::sptr filter)
{

    if(direction == RX) {
        if(not _rx_filters[name].set)
        {
            throw uhd::runtime_error("ad9361_device_t::set_filter this filter can not be written.");
        }
        _rx_filters[name].set(direction, chain, filter);
    } else if (direction == TX) {
        if(not _tx_filters[name].set)
        {
            throw uhd::runtime_error("ad9361_device_t::set_filter this filter can not be written.");
        }
        _tx_filters[name].set(direction, chain, filter);
    }

}

double ad9361_device_t::set_bw_filter(direction_t direction, const double rf_bw)
{
    //both low pass filters are programmed to the same bw. However, their cutoffs will differ.
    //Together they should create the requested bb bw.
    //Select rf_bw if it is between AD9361_MIN_BW & AD9361_MAX_BW.
    const double clipped_bw = std::min(std::max(rf_bw, AD9361_MIN_BW), AD9361_MAX_BW);
    if(direction == RX)
    {
        _rx_bb_lp_bw = _calibrate_baseband_rx_analog_filter(clipped_bw); //returns bb bw
        _rx_tia_lp_bw = _calibrate_rx_TIAs(clipped_bw);
        _rx_analog_bw = clipped_bw;
    } else {
        _tx_bb_lp_bw = _calibrate_baseband_tx_analog_filter(clipped_bw); //returns bb bw
        _tx_sec_lp_bw = _calibrate_secondary_tx_filter(clipped_bw);
        _tx_analog_bw = clipped_bw;
    }

    return (clipped_bw);
}

void ad9361_device_t::_set_fir_taps(direction_t direction, chain_t chain, const std::vector<int16_t>& taps)
{
    size_t num_taps = taps.size();
    size_t num_taps_avail = _get_num_fir_taps(direction);
    if(num_taps == num_taps_avail)
    {
        boost::scoped_array<uint16_t> coeffs(new uint16_t[num_taps_avail]);
        for (size_t i = 0; i < num_taps_avail; i++)
        {
            coeffs[i] = uint16_t(taps[i]);
        }
        _program_fir_filter(direction, chain, num_taps_avail, coeffs.get());
    } else if(num_taps < num_taps_avail){
        throw uhd::runtime_error("ad9361_device_t::_set_fir_taps not enough coefficients.");
    } else {
        throw uhd::runtime_error("ad9361_device_t::_set_fir_taps too many coefficients.");
    }
}

size_t ad9361_device_t::_get_num_fir_taps(direction_t direction)
{
    uint8_t num = 0;
    if(direction == RX)
        num = _io_iface->peek8(0x0F5);
    else
        num = _io_iface->peek8(0x065);
    num = ((num >> 5) & 0x07);
    return ((num + 1) * 16);
}

size_t ad9361_device_t::_get_fir_dec_int(direction_t direction)
{
    uint8_t dec_int = 0;
    if(direction == RX)
        dec_int = _io_iface->peek8(0x003);
    else
        dec_int = _io_iface->peek8(0x002);
    /*
     * 0 = dec/int by 1 and bypass filter
     * 1 = dec/int by 1
     * 2 = dec/int by 2
     * 3 = dec/int by 4 */
    dec_int = (dec_int & 0x03);
    if(dec_int == 3)
    {
        return 4;
    }
    return dec_int;
}

std::vector<int16_t> ad9361_device_t::_get_fir_taps(direction_t direction, chain_t chain)
{
    int base;
    size_t num_taps = _get_num_fir_taps(direction);
    uint8_t config;
    uint8_t reg_numtaps = (((num_taps / 16) - 1) & 0x07) << 5;
    config = reg_numtaps | 0x02; //start the programming clock

    if(chain == CHAIN_1)
    {
        config = config | (1 << 3);
    } else if (chain == CHAIN_2){
        config = config | (1 << 4);
    } else {
        throw uhd::runtime_error("[ad9361_device_t] Can not read both chains synchronously");
    }

    if(direction == RX)
    {
        base = 0xF0;
    } else {
        base = 0x60;
    }

    _io_iface->poke8(base+5,config);

    std::vector<int16_t> taps;
    uint8_t lower_val;
    uint8_t higher_val;
    uint16_t coeff;
    for(size_t i = 0;i < num_taps;i++)
    {
        _io_iface->poke8(base,0x00+i);
        lower_val = _io_iface->peek8(base+3);
        higher_val = _io_iface->peek8(base+4);
        coeff = ((higher_val << 8) | lower_val);
        taps.push_back(int16_t(coeff));
    }

    config = (config & (~(1 << 1))); //disable filter clock
    _io_iface->poke8(base+5,config);
    return taps;
}

/*
 * Returns either RX TIA LPF or TX Secondary LPF
 * depending on the direction.
 * See UG570 for details on used scaling factors. */
filter_info_base::sptr ad9361_device_t::_get_filter_lp_tia_sec(direction_t direction)
{
    double cutoff = 0;

    if(direction == RX)
    {
       cutoff = 2.5 * _rx_tia_lp_bw;
    } else {
       cutoff = 5 * _tx_sec_lp_bw;
    }

    filter_info_base::sptr lp(new analog_filter_lp(filter_info_base::ANALOG_LOW_PASS, false, 0, "single-pole", cutoff, 20));
    return  lp;
}

/*
 * Returns RX/TX BB LPF.
 * See UG570 for details on used scaling factors. */
filter_info_base::sptr ad9361_device_t::_get_filter_lp_bb(direction_t direction)
{
    double cutoff = 0;
    if(direction == RX)
    {
        cutoff = 1.4 * _rx_bb_lp_bw;
    } else {
        cutoff = 1.6 * _tx_bb_lp_bw;
    }

    filter_info_base::sptr bb_lp(new analog_filter_lp(filter_info_base::ANALOG_LOW_PASS, false, 1, "third-order Butterworth", cutoff, 60));
    return  bb_lp;
}

/*
 * For RX direction the DEC3 is returned.
 * For TX direction the INT3 is returned. */
filter_info_base::sptr ad9361_device_t::_get_filter_dec_int_3(direction_t direction)
{
    uint8_t enable = 0;
    double rate = _adcclock_freq;
    double full_scale;
    size_t dec = 0;
    size_t interpol = 0;
    filter_info_base::filter_type type = filter_info_base::DIGITAL_I16;
    std::string name;
    int16_t taps_array_rx[] = {55, 83, 0, -393, -580, 0, 1914, 4041, 5120, 4041, 1914, 0, -580, -393, 0, 83, 55};
    int16_t taps_array_tx[] = {36, -19, 0, -156, -12, 0, 479, 233, 0, -1215, -993, 0, 3569, 6277, 8192, 6277, 3569, 0, -993, -1215, 0, 223, 479, 0, -12, -156, 0, -19, 36};
    std::vector<int16_t> taps;

    filter_info_base::sptr ret;

    if(direction == RX)
    {
        full_scale = 16384;
        dec = 3;
        interpol = 1;

        enable = _io_iface->peek8(0x003);
        enable = ((enable >> 4) & 0x03);
        taps.assign(taps_array_rx, taps_array_rx + sizeof(taps_array_rx) / sizeof(int16_t) );

    } else {
        full_scale = 8192;
        dec = 1;
        interpol = 3;

        uint8_t use_dac_clk_div = _io_iface->peek8(0x00A);
        use_dac_clk_div = ((use_dac_clk_div >> 3) & 0x01);
        if(use_dac_clk_div == 1)
        {
            rate = rate / 2;
        }

        enable = _io_iface->peek8(0x002);
        enable = ((enable >> 4) & 0x03);
        if(enable == 2) //0 => int. by 1, 1 => int. by 2 (HB3), 2 => int. by 3
        {
            rate /= 3;
        }

        taps.assign(taps_array_tx, taps_array_tx + sizeof(taps_array_tx) / sizeof(int16_t) );
    }

    ret = filter_info_base::sptr(new digital_filter_base<int16_t>(type, (enable != 2) ? true : false, 2, rate, interpol, dec, full_scale, taps.size(), taps));
    return  ret;
}

filter_info_base::sptr ad9361_device_t::_get_filter_hb_3(direction_t direction)
{
    uint8_t enable = 0;
    double rate = _adcclock_freq;
    double full_scale = 0;
    size_t dec = 1;
    size_t interpol = 1;
    filter_info_base::filter_type type = filter_info_base::DIGITAL_I16;
    int16_t taps_array_rx[] = {1, 4, 6, 4, 1};
    int16_t taps_array_tx[] = {1, 2, 1};
    std::vector<int16_t> taps;

    if(direction == RX)
    {
        full_scale = 16;
        dec = 2;

        enable = _io_iface->peek8(0x003);
        enable = ((enable >> 4) & 0x03);
        taps.assign(taps_array_rx, taps_array_rx + sizeof(taps_array_rx) / sizeof(int16_t) );
    } else {
        full_scale = 2;
        interpol = 2;

        uint8_t use_dac_clk_div = _io_iface->peek8(0x00A);
        use_dac_clk_div = ((use_dac_clk_div >> 3) & 0x01);
        if(use_dac_clk_div == 1)
        {
            rate = rate / 2;
        }

        enable = _io_iface->peek8(0x002);
        enable = ((enable >> 4) & 0x03);
        if(enable == 1)
        {
            rate /= 2;
        }
        taps.assign(taps_array_tx, taps_array_tx + sizeof(taps_array_tx) / sizeof(int16_t) );
    }

    filter_info_base::sptr hb = filter_info_base::sptr(new digital_filter_base<int16_t>(type, (enable != 1) ? true : false, 2, rate, interpol, dec, full_scale, taps.size(), taps));
    return  hb;
}

filter_info_base::sptr ad9361_device_t::_get_filter_hb_2(direction_t direction)
{
    uint8_t enable = 0;
    double rate = _adcclock_freq;
    double full_scale = 0;
    size_t dec = 1;
    size_t interpol = 1;
    filter_info_base::filter_type type = filter_info_base::DIGITAL_I16;
    int16_t taps_array[] = {-9, 0, 73, 128, 73, 0, -9};
    std::vector<int16_t> taps(taps_array, taps_array + sizeof(taps_array) / sizeof(int16_t) );

    digital_filter_base<int16_t>::sptr hb_3 = boost::dynamic_pointer_cast<digital_filter_base<int16_t> >(_get_filter_hb_3(direction));
    digital_filter_base<int16_t>::sptr dec_int_3 = boost::dynamic_pointer_cast<digital_filter_base<int16_t> >(_get_filter_dec_int_3(direction));

    if(direction == RX)
    {
        full_scale = 256;
        dec = 2;
        enable = _io_iface->peek8(0x003);
    } else {
        full_scale = 128;
        interpol = 2;
        enable = _io_iface->peek8(0x002);
    }

    enable = ((enable >> 3) & 0x01);

    if(!(hb_3->is_bypassed()))
    {
        if(direction == RX)
        {
            rate = hb_3->get_output_rate();
        }else if (direction == TX) {
            rate = hb_3->get_input_rate();
            if(enable)
            {
                rate /= 2;
            }
        }
    } else { //else dec3/int3 or none of them is used.
        if(direction == RX)
        {
            rate = dec_int_3->get_output_rate();
        }else if (direction == TX) {
            rate = dec_int_3->get_input_rate();
            if(enable)
            {
                rate /= 2;
            }
        }
    }

    filter_info_base::sptr hb(new digital_filter_base<int16_t>(type, (enable == 0) ? true : false, 3, rate, interpol, dec, full_scale, taps.size(), taps));
    return  hb;
}

filter_info_base::sptr ad9361_device_t::_get_filter_hb_1(direction_t direction)
{
    uint8_t enable = 0;
    double rate = 0;
    double full_scale = 0;
    size_t dec = 1;
    size_t interpol = 1;
    filter_info_base::filter_type type = filter_info_base::DIGITAL_I16;

    std::vector<int16_t> taps;
    int16_t taps_rx_array[] = {-8, 0, 42, 0, -147, 0, 619, 1013, 619, 0, -147, 0, 42, 0, -8};
    int16_t taps_tx_array[] = {-53, 0, 313, 0, -1155, 0, 4989, 8192, 4989, 0, -1155, 0, 313, 0, -53};

    digital_filter_base<int16_t>::sptr hb_2 = boost::dynamic_pointer_cast<digital_filter_base<int16_t> >(_get_filter_hb_2(direction));

    if(direction == RX)
    {
        full_scale = 2048;
        dec = 2;
        enable = _io_iface->peek8(0x003);
        enable = ((enable >> 2) & 0x01);
        rate = hb_2->get_output_rate();
        taps.assign(taps_rx_array, taps_rx_array + sizeof(taps_rx_array) / sizeof(int16_t) );
    } else if (direction == TX) {
        full_scale = 8192;
        interpol = 2;
        enable = _io_iface->peek8(0x002);
        enable = ((enable >> 2) & 0x01);
        rate = hb_2->get_input_rate();
        if(enable)
        {
            rate /= 2;
        }
        taps.assign(taps_tx_array, taps_tx_array + sizeof(taps_tx_array) / sizeof(int16_t) );
    }

    filter_info_base::sptr hb(new digital_filter_base<int16_t>(type, (enable == 0) ? true : false, 4, rate, interpol, dec, full_scale, taps.size(), taps));
    return  hb;
}

filter_info_base::sptr ad9361_device_t::_get_filter_fir(direction_t direction, chain_t chain)
{
    double rate = 0;
    size_t dec = 1;
    size_t interpol = 1;
    size_t max_num_taps = 128;
    uint8_t enable = 1;

    digital_filter_base<int16_t>::sptr hb_1 = boost::dynamic_pointer_cast<digital_filter_base<int16_t> >(_get_filter_hb_1(direction));

    if(direction == RX)
    {
        dec = _get_fir_dec_int(direction);
        if(dec == 0)
        {
            enable = 0;
            dec = 1;
        }
        interpol = 1;
        rate = hb_1->get_output_rate();
    }else if (direction == TX) {
        interpol = _get_fir_dec_int(direction);
        if(interpol == 0)
        {
            enable = 0;
            interpol = 1;
        }
        dec = 1;
        rate = hb_1->get_input_rate();
        if(enable)
        {
            rate /= interpol;
        }
    }
    max_num_taps = _get_num_fir_taps(direction);

    filter_info_base::sptr fir(new digital_filter_fir<int16_t>(filter_info_base::DIGITAL_FIR_I16, (enable == 0) ? true : false, 5, rate, interpol, dec, 32767, max_num_taps, _get_fir_taps(direction, chain)));

    return fir;
}

void ad9361_device_t::_set_filter_fir(direction_t direction, chain_t channel, filter_info_base::sptr filter)
{
    digital_filter_fir<int16_t>::sptr fir = boost::dynamic_pointer_cast<digital_filter_fir<int16_t> >(filter);
    //only write taps. Ignore everything else for now
    _set_fir_taps(direction, channel, fir->get_taps());
}

/*
 * If BW of one of the analog filters gets overwritten manually,
 * _tx_analog_bw and _rx_analog_bw are not valid any more!
 * For useful data in those variables set_bw_filter method should be used
 */
void ad9361_device_t::_set_filter_lp_bb(direction_t direction, filter_info_base::sptr filter)
{
    analog_filter_lp::sptr lpf = boost::dynamic_pointer_cast<analog_filter_lp>(filter);
    double bw = lpf->get_cutoff();
    if(direction == RX)
    {
        //remember: this function takes rf bw as its input and calibrated to 1.4 x the given value
        _rx_bb_lp_bw = _calibrate_baseband_rx_analog_filter(2 * bw / 1.4); //returns bb bw

    } else {
        //remember: this function takes rf bw as its input and calibrates to 1.6 x the given value
        _tx_bb_lp_bw = _calibrate_baseband_tx_analog_filter(2 * bw / 1.6);
    }
}

void ad9361_device_t::_set_filter_lp_tia_sec(direction_t direction, filter_info_base::sptr filter)
{
    analog_filter_lp::sptr lpf = boost::dynamic_pointer_cast<analog_filter_lp>(filter);
    double bw = lpf->get_cutoff();
    if(direction == RX)
    {
        //remember: this function takes rf bw as its input and calibrated to 2.5 x the given value
        _rx_tia_lp_bw = _calibrate_rx_TIAs(2 * bw / 2.5); //returns bb bw

    } else {
        //remember: this function takes rf bw as its input and calibrates to 5 x the given value
        _tx_sec_lp_bw = _calibrate_secondary_tx_filter(2 * bw / 5);
    }
}

}}
