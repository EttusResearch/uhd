//
// Copyright 2010,2012 Ettus Research LLC
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

// Common IO Pins
#define REFCLOCK_DIV_MASK    ((1 << 8)|(1 << 9)|(1 << 10))    // Three GPIO lines to CPLD for Clock Divisor Selection
#define REFCLOCK_DIV8        ((1 << 8)|(1 << 9)|(1 << 10))    // GPIO to set clock div8 mode
#define REFCLOCK_DIV7        ((0 << 8)|(1 << 9)|(1 << 10))    // GPIO to set clock div7 mode
#define REFCLOCK_DIV6        ((1 << 8)|(0 << 9)|(1 << 10))    // GPIO to set clock div6 mode
#define REFCLOCK_DIV5        ((0 << 8)|(0 << 9)|(1 << 10))    // GPIO to set clock div5 mode
#define REFCLOCK_DIV4        ((1 << 8)|(1 <<9))               // GPIO to set clock div4 mode
#define REFCLOCK_DIV3        (1 <<9)                          // GPIO to set clock div3 mode
#define REFCLOCK_DIV2        (1 <<8)                          // GPIO to set clock div2 mode
#define REFCLOCK_DIV1        ((0 << 8)|(0 << 9)|(0 << 10))    // GPIO to set clock div1 mode

// RX1 IO Pins
#define RX1_MASTERSYNC  (1 << 3)                // MASTERSYNC Signal for Slave Tuner Coordination
#define RX1_FREEZE      (1 << 2)                // FREEZE Signal for Slave Tuner Coordination
#define RX1_IRQ         (1 << 1)                // IRQ Signals TDA18272HNM State Machine Completion
#define RX1_VSYNC       (1 << 0)                // VSYNC Pulse for AGC Holdoff

// RX2 IO Pins
#define RX2_MASTERSYNC  (1 << 7)                // MASTERSYNC Signal for Slave Tuner Coordination
#define RX2_FREEZE      (1 << 6)                // FREEZE Signal for Slave Tuner Coordination
#define RX2_IRQ         (1 << 5)                // IRQ Signals TDA18272HNM State Machine Completion
#define RX2_VSYNC       (1 << 4)                // VSYNC Pulse for AGC Holdoff

// Pin functions
#define RX1_OUTPUT_MASK (0)
#define RX1_INPUT_MASK  (RX1_VSYNC|RX1_MASTERSYNC|RX1_FREEZE|RX1_IRQ)

#define RX2_OUTPUT_MASK (0)
#define RX2_INPUT_MASK  (RX2_VSYNC|RX2_MASTERSYNC|RX2_FREEZE|RX2_IRQ)

#define OUTPUT_MASK     (RX1_OUTPUT_MASK|RX2_OUTPUT_MASK|REFCLOCK_DIV_MASK)
#define INPUT_MASK      (RX1_INPUT_MASK|RX2_INPUT_MASK)


#include "tda18272hnm_regs.hpp"
#include <uhd/utils/static.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/array.hpp>
#include <boost/math/special_functions/round.hpp>
#include <utility>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The TVRX2 types
 **********************************************************************/
struct tvrx2_tda18272_rfcal_result_t {
    boost::int8_t    delta_c;
    boost::int8_t    c_offset;
    tvrx2_tda18272_rfcal_result_t(void): delta_c(0), c_offset(0){}
};

struct tvrx2_tda18272_rfcal_coeffs_t  {
    boost::uint8_t   cal_number;
    boost::int32_t   RF_A1;
    boost::int32_t   RF_B1;
    tvrx2_tda18272_rfcal_coeffs_t(void): cal_number(0), RF_A1(0), RF_B1(0) {}
    tvrx2_tda18272_rfcal_coeffs_t(boost::uint32_t num): RF_A1(0), RF_B1(0) { cal_number = num; }
};

struct tvrx2_tda18272_cal_map_t {
    boost::array<boost::uint32_t, 4>  cal_freq;
    boost::array<boost::uint8_t, 4>   c_offset;
    tvrx2_tda18272_cal_map_t(boost::array<boost::uint32_t, 4> freqs, boost::array<boost::uint8_t, 4> offsets)
        { cal_freq = freqs; c_offset = offsets; }
};

struct tvrx2_tda18272_freq_map_t {
    boost::uint32_t  rf_max;
    boost::uint8_t   c_prog;
    boost::uint8_t   gain_taper;
    boost::uint8_t   rf_band;
    tvrx2_tda18272_freq_map_t( boost::uint32_t max, boost::uint8_t c, boost::uint8_t taper, boost::uint8_t band)
        { rf_max = max; c_prog = c; gain_taper = taper; rf_band = band; }
};

/***********************************************************************
 * The TVRX2 constants
 **********************************************************************/

static const boost::array<freq_range_t, 4> tvrx2_tda18272_rf_bands = list_of
    ( freq_range_t(  44.056e6, 144.408e6) )
    ( freq_range_t( 145.432e6, 361.496e6) )
    ( freq_range_t( 365.592e6, 618.520e6) )
    ( freq_range_t( 619.544e6, 865.304e6) )
;

#define TVRX2_TDA18272_FREQ_MAP_ENTRIES (565)

static const uhd::dict<boost::uint32_t, tvrx2_tda18272_cal_map_t> tvrx2_tda18272_cal_map = map_list_of
    (  0, tvrx2_tda18272_cal_map_t( list_of( 44032000)( 48128000)( 52224000)( 56320000), list_of(15)( 0)(10)(17) ) )
    (  1, tvrx2_tda18272_cal_map_t( list_of( 84992000)( 89088000)( 93184000)( 97280000), list_of( 1)( 0)(-2)( 3) ) )
    (  2, tvrx2_tda18272_cal_map_t( list_of(106496000)(111616000)(115712000)(123904000), list_of( 0)(-1)( 1)( 2) ) )
    (  3, tvrx2_tda18272_cal_map_t( list_of(161792000)(165888000)(169984000)(174080000), list_of( 3)( 0)( 1)( 2) ) )
    (  4, tvrx2_tda18272_cal_map_t( list_of(224256000)(228352000)(232448000)(235520000), list_of( 3)( 0)( 1)( 2) ) )
    (  5, tvrx2_tda18272_cal_map_t( list_of(301056000)(312320000)(322560000)(335872000), list_of( 0)(-1)( 1)( 2) ) )
    (  6, tvrx2_tda18272_cal_map_t( list_of(389120000)(393216000)(397312000)(401408000), list_of(-2)( 0)(-1)( 1) ) )
    (  7, tvrx2_tda18272_cal_map_t( list_of(455680000)(460800000)(465920000)(471040000), list_of( 0)(-2)(-3)( 1) ) )
    (  8, tvrx2_tda18272_cal_map_t( list_of(555008000)(563200000)(570368000)(577536000), list_of(-1)( 0)(-3)(-2) ) )
    (  9, tvrx2_tda18272_cal_map_t( list_of(647168000)(652288000)(658432000)(662528000), list_of(-6)(-3)( 0)(-5) ) )
    ( 10, tvrx2_tda18272_cal_map_t( list_of(748544000)(755712000)(762880000)(770048000), list_of(-6)(-3)( 0)(-5) ) )
    ( 11, tvrx2_tda18272_cal_map_t( list_of(792576000)(801792000)(809984000)(818176000), list_of(-5)(-2)( 0)(-4) ) )
;

static const std::vector<tvrx2_tda18272_freq_map_t> tvrx2_tda18272_freq_map = list_of
    ( tvrx2_tda18272_freq_map_t( 39936000, 0xFF, 0x17, 0) )
    ( tvrx2_tda18272_freq_map_t( 40960000, 0xFD, 0x17, 0) )
    ( tvrx2_tda18272_freq_map_t( 41984000, 0xF1, 0x15, 0) )
    ( tvrx2_tda18272_freq_map_t( 43008000, 0xE5, 0x13, 0) )
    ( tvrx2_tda18272_freq_map_t( 44032000, 0xDB, 0x13, 0) )
    ( tvrx2_tda18272_freq_map_t( 45056000, 0xD1, 0x12, 0) )
    ( tvrx2_tda18272_freq_map_t( 46080000, 0xC7, 0x10, 0) )
    ( tvrx2_tda18272_freq_map_t( 47104000, 0xBE, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 48128000, 0xB5, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 49152000, 0xAD, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 50176000, 0xA6, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 51200000, 0x9F, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 52224000, 0x98, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 53248000, 0x91, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 54272000, 0x8B, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 55296000, 0x86, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 56320000, 0x80, 0x0F, 0) )
    ( tvrx2_tda18272_freq_map_t( 57344000, 0x7B, 0x0E, 0) )
    ( tvrx2_tda18272_freq_map_t( 58368000, 0x76, 0x0E, 0) )
    ( tvrx2_tda18272_freq_map_t( 59392000, 0x72, 0x0D, 0) )
    ( tvrx2_tda18272_freq_map_t( 60416000, 0x6D, 0x0D, 0) )
    ( tvrx2_tda18272_freq_map_t( 61440000, 0x69, 0x0C, 0) )
    ( tvrx2_tda18272_freq_map_t( 62464000, 0x65, 0x0C, 0) )
    ( tvrx2_tda18272_freq_map_t( 63488000, 0x61, 0x0B, 0) )
    ( tvrx2_tda18272_freq_map_t( 64512000, 0x5E, 0x0B, 0) )
    ( tvrx2_tda18272_freq_map_t( 64512000, 0x5A, 0x0B, 0) )
    ( tvrx2_tda18272_freq_map_t( 65536000, 0x57, 0x0A, 0) )
    ( tvrx2_tda18272_freq_map_t( 66560000, 0x54, 0x0A, 0) )
    ( tvrx2_tda18272_freq_map_t( 67584000, 0x51, 0x09, 0) )
    ( tvrx2_tda18272_freq_map_t( 68608000, 0x4E, 0x09, 0) )
    ( tvrx2_tda18272_freq_map_t( 69632000, 0x4B, 0x09, 0) )
    ( tvrx2_tda18272_freq_map_t( 70656000, 0x49, 0x08, 0) )
    ( tvrx2_tda18272_freq_map_t( 71680000, 0x46, 0x08, 0) )
    ( tvrx2_tda18272_freq_map_t( 72704000, 0x44, 0x08, 0) )
    ( tvrx2_tda18272_freq_map_t( 73728000, 0x41, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 74752000, 0x3F, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 75776000, 0x3D, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 76800000, 0x3B, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 77824000, 0x39, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 78848000, 0x37, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 79872000, 0x35, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 80896000, 0x33, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 81920000, 0x32, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 82944000, 0x30, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 83968000, 0x2F, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 84992000, 0x2D, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 86016000, 0x2C, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 87040000, 0x2A, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t( 88064000, 0x29, 0x06, 0) )
    ( tvrx2_tda18272_freq_map_t( 89088000, 0x27, 0x06, 0) )
    ( tvrx2_tda18272_freq_map_t( 90112000, 0x26, 0x06, 0) )
    ( tvrx2_tda18272_freq_map_t( 91136000, 0x25, 0x06, 0) )
    ( tvrx2_tda18272_freq_map_t( 92160000, 0x24, 0x06, 0) )
    ( tvrx2_tda18272_freq_map_t( 93184000, 0x22, 0x05, 0) )
    ( tvrx2_tda18272_freq_map_t( 94208000, 0x21, 0x05, 0) )
    ( tvrx2_tda18272_freq_map_t( 95232000, 0x20, 0x05, 0) )
    ( tvrx2_tda18272_freq_map_t( 96256000, 0x1F, 0x05, 0) )
    ( tvrx2_tda18272_freq_map_t( 97280000, 0x1E, 0x05, 0) )
    ( tvrx2_tda18272_freq_map_t( 98304000, 0x1D, 0x05, 0) )
    ( tvrx2_tda18272_freq_map_t( 99328000, 0x1C, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(100352000, 0x1B, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(101376000, 0x1A, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(103424000, 0x19, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(104448000, 0x18, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(105472000, 0x17, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(106496000, 0x16, 0x03, 0) )
    ( tvrx2_tda18272_freq_map_t(106496000, 0x15, 0x03, 0) )
    ( tvrx2_tda18272_freq_map_t(108544000, 0x14, 0x03, 0) )
    ( tvrx2_tda18272_freq_map_t(109568000, 0x13, 0x03, 0) )
    ( tvrx2_tda18272_freq_map_t(111616000, 0x12, 0x03, 0) )
    ( tvrx2_tda18272_freq_map_t(112640000, 0x11, 0x03, 0) )
    ( tvrx2_tda18272_freq_map_t(113664000, 0x11, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t(114688000, 0x10, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t(115712000, 0x0F, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t(117760000, 0x0E, 0x07, 0) )
    ( tvrx2_tda18272_freq_map_t(119808000, 0x0D, 0x06, 0) )
    ( tvrx2_tda18272_freq_map_t(121856000, 0x0C, 0x06, 0) )
    ( tvrx2_tda18272_freq_map_t(123904000, 0x0B, 0x06, 0) )
    ( tvrx2_tda18272_freq_map_t(125952000, 0x0A, 0x05, 0) )
    ( tvrx2_tda18272_freq_map_t(128000000, 0x09, 0x05, 0) )
    ( tvrx2_tda18272_freq_map_t(130048000, 0x08, 0x05, 0) )
    ( tvrx2_tda18272_freq_map_t(133120000, 0x07, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(135168000, 0x06, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(138240000, 0x05, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(141312000, 0x04, 0x04, 0) )
    ( tvrx2_tda18272_freq_map_t(144384000, 0x03, 0x03, 0) )
    ( tvrx2_tda18272_freq_map_t(145408000, 0xE0, 0x3F, 1) )
    ( tvrx2_tda18272_freq_map_t(147456000, 0xDC, 0x37, 1) )
    ( tvrx2_tda18272_freq_map_t(148480000, 0xD9, 0x32, 1) )
    ( tvrx2_tda18272_freq_map_t(149504000, 0xD6, 0x2F, 1) )
    ( tvrx2_tda18272_freq_map_t(149504000, 0xD2, 0x2F, 1) )
    ( tvrx2_tda18272_freq_map_t(150528000, 0xCF, 0x2F, 1) )
    ( tvrx2_tda18272_freq_map_t(151552000, 0xCC, 0x2B, 1) )
    ( tvrx2_tda18272_freq_map_t(152576000, 0xC9, 0x27, 1) )
    ( tvrx2_tda18272_freq_map_t(153600000, 0xC5, 0x27, 1) )
    ( tvrx2_tda18272_freq_map_t(154624000, 0xC2, 0x25, 1) )
    ( tvrx2_tda18272_freq_map_t(155648000, 0xBF, 0x23, 1) )
    ( tvrx2_tda18272_freq_map_t(156672000, 0xBD, 0x20, 1) )
    ( tvrx2_tda18272_freq_map_t(157696000, 0xBA, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(158720000, 0xB7, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(159744000, 0xB4, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(160768000, 0xB1, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(161792000, 0xAF, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(162816000, 0xAC, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(163840000, 0xAA, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(164864000, 0xA7, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(165888000, 0xA5, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(166912000, 0xA2, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(167936000, 0xA0, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(168960000, 0x9D, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(169984000, 0x9B, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(171008000, 0x99, 0x1F, 1) )
    ( tvrx2_tda18272_freq_map_t(172032000, 0x97, 0x1E, 1) )
    ( tvrx2_tda18272_freq_map_t(173056000, 0x95, 0x1D, 1) )
    ( tvrx2_tda18272_freq_map_t(174080000, 0x92, 0x1C, 1) )
    ( tvrx2_tda18272_freq_map_t(175104000, 0x90, 0x1B, 1) )
    ( tvrx2_tda18272_freq_map_t(176128000, 0x8E, 0x1A, 1) )
    ( tvrx2_tda18272_freq_map_t(177152000, 0x8C, 0x19, 1) )
    ( tvrx2_tda18272_freq_map_t(178176000, 0x8A, 0x18, 1) )
    ( tvrx2_tda18272_freq_map_t(179200000, 0x88, 0x17, 1) )
    ( tvrx2_tda18272_freq_map_t(180224000, 0x86, 0x17, 1) )
    ( tvrx2_tda18272_freq_map_t(181248000, 0x84, 0x17, 1) )
    ( tvrx2_tda18272_freq_map_t(182272000, 0x82, 0x17, 1) )
    ( tvrx2_tda18272_freq_map_t(183296000, 0x81, 0x17, 1) )
    ( tvrx2_tda18272_freq_map_t(184320000, 0x7F, 0x17, 1) )
    ( tvrx2_tda18272_freq_map_t(185344000, 0x7D, 0x16, 1) )
    ( tvrx2_tda18272_freq_map_t(186368000, 0x7B, 0x15, 1) )
    ( tvrx2_tda18272_freq_map_t(187392000, 0x7A, 0x14, 1) )
    ( tvrx2_tda18272_freq_map_t(188416000, 0x78, 0x14, 1) )
    ( tvrx2_tda18272_freq_map_t(189440000, 0x76, 0x13, 1) )
    ( tvrx2_tda18272_freq_map_t(190464000, 0x75, 0x13, 1) )
    ( tvrx2_tda18272_freq_map_t(191488000, 0x73, 0x13, 1) )
    ( tvrx2_tda18272_freq_map_t(192512000, 0x71, 0x12, 1) )
    ( tvrx2_tda18272_freq_map_t(192512000, 0x70, 0x11, 1) )
    ( tvrx2_tda18272_freq_map_t(193536000, 0x6E, 0x11, 1) )
    ( tvrx2_tda18272_freq_map_t(194560000, 0x6D, 0x10, 1) )
    ( tvrx2_tda18272_freq_map_t(195584000, 0x6B, 0x10, 1) )
    ( tvrx2_tda18272_freq_map_t(196608000, 0x6A, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(197632000, 0x68, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(198656000, 0x67, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(199680000, 0x65, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(200704000, 0x64, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(201728000, 0x63, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(202752000, 0x61, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(203776000, 0x60, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(204800000, 0x5F, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(205824000, 0x5D, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(206848000, 0x5C, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(207872000, 0x5B, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(208896000, 0x5A, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(209920000, 0x58, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(210944000, 0x57, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(211968000, 0x56, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(212992000, 0x55, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(214016000, 0x54, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(215040000, 0x53, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(216064000, 0x52, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(217088000, 0x50, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(218112000, 0x4F, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(219136000, 0x4E, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(220160000, 0x4D, 0x0E, 1) )
    ( tvrx2_tda18272_freq_map_t(221184000, 0x4C, 0x0E, 1) )
    ( tvrx2_tda18272_freq_map_t(222208000, 0x4B, 0x0E, 1) )
    ( tvrx2_tda18272_freq_map_t(223232000, 0x4A, 0x0E, 1) )
    ( tvrx2_tda18272_freq_map_t(224256000, 0x49, 0x0D, 1) )
    ( tvrx2_tda18272_freq_map_t(225280000, 0x48, 0x0D, 1) )
    ( tvrx2_tda18272_freq_map_t(226304000, 0x47, 0x0D, 1) )
    ( tvrx2_tda18272_freq_map_t(227328000, 0x46, 0x0D, 1) )
    ( tvrx2_tda18272_freq_map_t(228352000, 0x45, 0x0C, 1) )
    ( tvrx2_tda18272_freq_map_t(229376000, 0x44, 0x0C, 1) )
    ( tvrx2_tda18272_freq_map_t(230400000, 0x43, 0x0C, 1) )
    ( tvrx2_tda18272_freq_map_t(231424000, 0x42, 0x0C, 1) )
    ( tvrx2_tda18272_freq_map_t(232448000, 0x42, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(233472000, 0x41, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(234496000, 0x40, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(234496000, 0x3F, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(235520000, 0x3E, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(236544000, 0x3D, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(237568000, 0x3C, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(239616000, 0x3B, 0x0A, 1) )
    ( tvrx2_tda18272_freq_map_t(240640000, 0x3A, 0x0A, 1) )
    ( tvrx2_tda18272_freq_map_t(241664000, 0x39, 0x0A, 1) )
    ( tvrx2_tda18272_freq_map_t(242688000, 0x38, 0x0A, 1) )
    ( tvrx2_tda18272_freq_map_t(244736000, 0x37, 0x09, 1) )
    ( tvrx2_tda18272_freq_map_t(245760000, 0x36, 0x09, 1) )
    ( tvrx2_tda18272_freq_map_t(246784000, 0x35, 0x09, 1) )
    ( tvrx2_tda18272_freq_map_t(248832000, 0x34, 0x09, 1) )
    ( tvrx2_tda18272_freq_map_t(249856000, 0x33, 0x09, 1) )
    ( tvrx2_tda18272_freq_map_t(250880000, 0x32, 0x08, 1) )
    ( tvrx2_tda18272_freq_map_t(252928000, 0x31, 0x08, 1) )
    ( tvrx2_tda18272_freq_map_t(253952000, 0x30, 0x08, 1) )
    ( tvrx2_tda18272_freq_map_t(256000000, 0x2F, 0x08, 1) )
    ( tvrx2_tda18272_freq_map_t(257024000, 0x2E, 0x08, 1) )
    ( tvrx2_tda18272_freq_map_t(259072000, 0x2D, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(260096000, 0x2C, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(262144000, 0x2B, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(264192000, 0x2A, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(265216000, 0x29, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(267264000, 0x28, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(269312000, 0x27, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(270336000, 0x26, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(272384000, 0x25, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(274432000, 0x24, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(276480000, 0x23, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(277504000, 0x22, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(279552000, 0x21, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(281600000, 0x20, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(283648000, 0x1F, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(285696000, 0x1E, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(287744000, 0x1D, 0x0F, 1) )
    ( tvrx2_tda18272_freq_map_t(289792000, 0x1C, 0x0E, 1) )
    ( tvrx2_tda18272_freq_map_t(291840000, 0x1B, 0x0E, 1) )
    ( tvrx2_tda18272_freq_map_t(293888000, 0x1A, 0x0D, 1) )
    ( tvrx2_tda18272_freq_map_t(296960000, 0x19, 0x0D, 1) )
    ( tvrx2_tda18272_freq_map_t(299008000, 0x18, 0x0C, 1) )
    ( tvrx2_tda18272_freq_map_t(301056000, 0x17, 0x0C, 1) )
    ( tvrx2_tda18272_freq_map_t(304128000, 0x16, 0x0C, 1) )
    ( tvrx2_tda18272_freq_map_t(306176000, 0x15, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(309248000, 0x14, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(312320000, 0x13, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(314368000, 0x12, 0x0B, 1) )
    ( tvrx2_tda18272_freq_map_t(317440000, 0x11, 0x0A, 1) )
    ( tvrx2_tda18272_freq_map_t(320512000, 0x10, 0x0A, 1) )
    ( tvrx2_tda18272_freq_map_t(322560000, 0x0F, 0x0A, 1) )
    ( tvrx2_tda18272_freq_map_t(325632000, 0x0E, 0x09, 1) )
    ( tvrx2_tda18272_freq_map_t(328704000, 0x0D, 0x09, 1) )
    ( tvrx2_tda18272_freq_map_t(331776000, 0x0C, 0x08, 1) )
    ( tvrx2_tda18272_freq_map_t(335872000, 0x0B, 0x08, 1) )
    ( tvrx2_tda18272_freq_map_t(338944000, 0x0A, 0x08, 1) )
    ( tvrx2_tda18272_freq_map_t(343040000, 0x09, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(346112000, 0x08, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(350208000, 0x07, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(354304000, 0x06, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(358400000, 0x05, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(362496000, 0x04, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(365568000, 0x04, 0x07, 1) )
    ( tvrx2_tda18272_freq_map_t(367616000, 0xDA, 0x2A, 2) )
    ( tvrx2_tda18272_freq_map_t(367616000, 0xD9, 0x27, 2) )
    ( tvrx2_tda18272_freq_map_t(368640000, 0xD8, 0x27, 2) )
    ( tvrx2_tda18272_freq_map_t(369664000, 0xD6, 0x27, 2) )
    ( tvrx2_tda18272_freq_map_t(370688000, 0xD5, 0x27, 2) )
    ( tvrx2_tda18272_freq_map_t(371712000, 0xD3, 0x25, 2) )
    ( tvrx2_tda18272_freq_map_t(372736000, 0xD2, 0x23, 2) )
    ( tvrx2_tda18272_freq_map_t(373760000, 0xD0, 0x23, 2) )
    ( tvrx2_tda18272_freq_map_t(374784000, 0xCF, 0x21, 2) )
    ( tvrx2_tda18272_freq_map_t(375808000, 0xCD, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(376832000, 0xCC, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(377856000, 0xCA, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(378880000, 0xC9, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(379904000, 0xC7, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(380928000, 0xC6, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(381952000, 0xC4, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(382976000, 0xC3, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(384000000, 0xC1, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(385024000, 0xC0, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(386048000, 0xBF, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(387072000, 0xBD, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(388096000, 0xBC, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(389120000, 0xBB, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(390144000, 0xB9, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(391168000, 0xB8, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(392192000, 0xB7, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(393216000, 0xB5, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(394240000, 0xB4, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(395264000, 0xB3, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(396288000, 0xB1, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(397312000, 0xB0, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(398336000, 0xAF, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(399360000, 0xAD, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(400384000, 0xAC, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(401408000, 0xAB, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(402432000, 0xAA, 0x1F, 2) )
    ( tvrx2_tda18272_freq_map_t(403456000, 0xA8, 0x1E, 2) )
    ( tvrx2_tda18272_freq_map_t(404480000, 0xA7, 0x1D, 2) )
    ( tvrx2_tda18272_freq_map_t(405504000, 0xA6, 0x1D, 2) )
    ( tvrx2_tda18272_freq_map_t(405504000, 0xA5, 0x1C, 2) )
    ( tvrx2_tda18272_freq_map_t(406528000, 0xA3, 0x1C, 2) )
    ( tvrx2_tda18272_freq_map_t(407552000, 0xA2, 0x1B, 2) )
    ( tvrx2_tda18272_freq_map_t(408576000, 0xA1, 0x1B, 2) )
    ( tvrx2_tda18272_freq_map_t(409600000, 0xA0, 0x1B, 2) )
    ( tvrx2_tda18272_freq_map_t(410624000, 0x9F, 0x1A, 2) )
    ( tvrx2_tda18272_freq_map_t(411648000, 0x9D, 0x1A, 2) )
    ( tvrx2_tda18272_freq_map_t(412672000, 0x9C, 0x19, 2) )
    ( tvrx2_tda18272_freq_map_t(413696000, 0x9B, 0x18, 2) )
    ( tvrx2_tda18272_freq_map_t(414720000, 0x9A, 0x18, 2) )
    ( tvrx2_tda18272_freq_map_t(415744000, 0x99, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(416768000, 0x98, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(417792000, 0x97, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(418816000, 0x95, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(419840000, 0x94, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(420864000, 0x93, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(421888000, 0x92, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(422912000, 0x91, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(423936000, 0x90, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(424960000, 0x8F, 0x17, 2) )
    ( tvrx2_tda18272_freq_map_t(425984000, 0x8E, 0x16, 2) )
    ( tvrx2_tda18272_freq_map_t(427008000, 0x8D, 0x16, 2) )
    ( tvrx2_tda18272_freq_map_t(428032000, 0x8C, 0x15, 2) )
    ( tvrx2_tda18272_freq_map_t(429056000, 0x8B, 0x15, 2) )
    ( tvrx2_tda18272_freq_map_t(430080000, 0x8A, 0x15, 2) )
    ( tvrx2_tda18272_freq_map_t(431104000, 0x88, 0x14, 2) )
    ( tvrx2_tda18272_freq_map_t(432128000, 0x87, 0x14, 2) )
    ( tvrx2_tda18272_freq_map_t(433152000, 0x86, 0x14, 2) )
    ( tvrx2_tda18272_freq_map_t(434176000, 0x85, 0x13, 2) )
    ( tvrx2_tda18272_freq_map_t(435200000, 0x84, 0x13, 2) )
    ( tvrx2_tda18272_freq_map_t(436224000, 0x83, 0x13, 2) )
    ( tvrx2_tda18272_freq_map_t(437248000, 0x82, 0x13, 2) )
    ( tvrx2_tda18272_freq_map_t(438272000, 0x81, 0x13, 2) )
    ( tvrx2_tda18272_freq_map_t(439296000, 0x80, 0x12, 2) )
    ( tvrx2_tda18272_freq_map_t(440320000, 0x7F, 0x12, 2) )
    ( tvrx2_tda18272_freq_map_t(441344000, 0x7E, 0x12, 2) )
    ( tvrx2_tda18272_freq_map_t(442368000, 0x7D, 0x11, 2) )
    ( tvrx2_tda18272_freq_map_t(444416000, 0x7C, 0x11, 2) )
    ( tvrx2_tda18272_freq_map_t(445440000, 0x7B, 0x10, 2) )
    ( tvrx2_tda18272_freq_map_t(446464000, 0x7A, 0x10, 2) )
    ( tvrx2_tda18272_freq_map_t(447488000, 0x79, 0x10, 2) )
    ( tvrx2_tda18272_freq_map_t(448512000, 0x78, 0x10, 2) )
    ( tvrx2_tda18272_freq_map_t(448512000, 0x77, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(449536000, 0x76, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(450560000, 0x75, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(451584000, 0x74, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(452608000, 0x73, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(453632000, 0x72, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(454656000, 0x71, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(455680000, 0x70, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(457728000, 0x6F, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(458752000, 0x6E, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(459776000, 0x6D, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(460800000, 0x6C, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(461824000, 0x6B, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(462848000, 0x6A, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(464896000, 0x69, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(465920000, 0x68, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(466944000, 0x67, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(467968000, 0x66, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(468992000, 0x65, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(471040000, 0x64, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(472064000, 0x63, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(473088000, 0x62, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(474112000, 0x61, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(476160000, 0x60, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(477184000, 0x5F, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(478208000, 0x5E, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(479232000, 0x5D, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(481280000, 0x5C, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(482304000, 0x5B, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(483328000, 0x5A, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(485376000, 0x59, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(486400000, 0x58, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(487424000, 0x57, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(489472000, 0x56, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(490496000, 0x55, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(490496000, 0x54, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(492544000, 0x53, 0x0E, 2) )
    ( tvrx2_tda18272_freq_map_t(493568000, 0x52, 0x0E, 2) )
    ( tvrx2_tda18272_freq_map_t(495616000, 0x51, 0x0E, 2) )
    ( tvrx2_tda18272_freq_map_t(496640000, 0x50, 0x0E, 2) )
    ( tvrx2_tda18272_freq_map_t(497664000, 0x4F, 0x0E, 2) )
    ( tvrx2_tda18272_freq_map_t(499712000, 0x4E, 0x0D, 2) )
    ( tvrx2_tda18272_freq_map_t(500736000, 0x4D, 0x0D, 2) )
    ( tvrx2_tda18272_freq_map_t(502784000, 0x4C, 0x0D, 2) )
    ( tvrx2_tda18272_freq_map_t(503808000, 0x4B, 0x0D, 2) )
    ( tvrx2_tda18272_freq_map_t(505856000, 0x4A, 0x0C, 2) )
    ( tvrx2_tda18272_freq_map_t(506880000, 0x49, 0x0C, 2) )
    ( tvrx2_tda18272_freq_map_t(508928000, 0x48, 0x0C, 2) )
    ( tvrx2_tda18272_freq_map_t(509952000, 0x47, 0x0C, 2) )
    ( tvrx2_tda18272_freq_map_t(512000000, 0x46, 0x0C, 2) )
    ( tvrx2_tda18272_freq_map_t(513024000, 0x45, 0x0B, 2) )
    ( tvrx2_tda18272_freq_map_t(515072000, 0x44, 0x0B, 2) )
    ( tvrx2_tda18272_freq_map_t(517120000, 0x43, 0x0B, 2) )
    ( tvrx2_tda18272_freq_map_t(518144000, 0x42, 0x0B, 2) )
    ( tvrx2_tda18272_freq_map_t(520192000, 0x41, 0x0B, 2) )
    ( tvrx2_tda18272_freq_map_t(521216000, 0x40, 0x0B, 2) )
    ( tvrx2_tda18272_freq_map_t(523264000, 0x3F, 0x0B, 2) )
    ( tvrx2_tda18272_freq_map_t(525312000, 0x3E, 0x0B, 2) )
    ( tvrx2_tda18272_freq_map_t(526336000, 0x3D, 0x0B, 2) )
    ( tvrx2_tda18272_freq_map_t(528384000, 0x3C, 0x0A, 2) )
    ( tvrx2_tda18272_freq_map_t(530432000, 0x3B, 0x0A, 2) )
    ( tvrx2_tda18272_freq_map_t(531456000, 0x3A, 0x0A, 2) )
    ( tvrx2_tda18272_freq_map_t(533504000, 0x39, 0x0A, 2) )
    ( tvrx2_tda18272_freq_map_t(534528000, 0x38, 0x0A, 2) )
    ( tvrx2_tda18272_freq_map_t(536576000, 0x37, 0x0A, 2) )
    ( tvrx2_tda18272_freq_map_t(537600000, 0x36, 0x09, 2) )
    ( tvrx2_tda18272_freq_map_t(539648000, 0x35, 0x09, 2) )
    ( tvrx2_tda18272_freq_map_t(541696000, 0x34, 0x09, 2) )
    ( tvrx2_tda18272_freq_map_t(543744000, 0x33, 0x09, 2) )
    ( tvrx2_tda18272_freq_map_t(544768000, 0x32, 0x09, 2) )
    ( tvrx2_tda18272_freq_map_t(546816000, 0x31, 0x09, 2) )
    ( tvrx2_tda18272_freq_map_t(548864000, 0x30, 0x08, 2) )
    ( tvrx2_tda18272_freq_map_t(550912000, 0x2F, 0x08, 2) )
    ( tvrx2_tda18272_freq_map_t(552960000, 0x2E, 0x08, 2) )
    ( tvrx2_tda18272_freq_map_t(555008000, 0x2D, 0x08, 2) )
    ( tvrx2_tda18272_freq_map_t(557056000, 0x2C, 0x08, 2) )
    ( tvrx2_tda18272_freq_map_t(559104000, 0x2B, 0x08, 2) )
    ( tvrx2_tda18272_freq_map_t(561152000, 0x2A, 0x07, 2) )
    ( tvrx2_tda18272_freq_map_t(563200000, 0x29, 0x07, 2) )
    ( tvrx2_tda18272_freq_map_t(565248000, 0x28, 0x07, 2) )
    ( tvrx2_tda18272_freq_map_t(567296000, 0x27, 0x07, 2) )
    ( tvrx2_tda18272_freq_map_t(569344000, 0x26, 0x07, 2) )
    ( tvrx2_tda18272_freq_map_t(570368000, 0x26, 0x07, 2) )
    ( tvrx2_tda18272_freq_map_t(571392000, 0x25, 0x07, 2) )
    ( tvrx2_tda18272_freq_map_t(573440000, 0x24, 0x07, 2) )
    ( tvrx2_tda18272_freq_map_t(575488000, 0x23, 0x07, 2) )
    ( tvrx2_tda18272_freq_map_t(577536000, 0x22, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(578560000, 0x21, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(580608000, 0x20, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(583680000, 0x1F, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(585728000, 0x1E, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(587776000, 0x1D, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(589824000, 0x1C, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(592896000, 0x1B, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(594944000, 0x1A, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(596992000, 0x19, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(600064000, 0x18, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(602112000, 0x17, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(604160000, 0x16, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(607232000, 0x15, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(609280000, 0x14, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(612352000, 0x13, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(615424000, 0x12, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(617472000, 0x11, 0x0F, 2) )
    ( tvrx2_tda18272_freq_map_t(619520000, 0x10, 0x0E, 2) )
    ( tvrx2_tda18272_freq_map_t(621568000, 0x0F, 0x0E, 2) )
    ( tvrx2_tda18272_freq_map_t(623616000, 0x0F, 0x0E, 2) )
    ( tvrx2_tda18272_freq_map_t(624640000, 0xA3, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(625664000, 0xA2, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(626688000, 0xA1, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(627712000, 0xA0, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(628736000, 0x9F, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(630784000, 0x9E, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(631808000, 0x9D, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(632832000, 0x9C, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(633856000, 0x9B, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(635904000, 0x9A, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(636928000, 0x99, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(637952000, 0x98, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(638976000, 0x97, 0x1F, 3) )
    ( tvrx2_tda18272_freq_map_t(641024000, 0x96, 0x1E, 3) )
    ( tvrx2_tda18272_freq_map_t(642048000, 0x95, 0x1E, 3) )
    ( tvrx2_tda18272_freq_map_t(643072000, 0x94, 0x1E, 3) )
    ( tvrx2_tda18272_freq_map_t(644096000, 0x93, 0x1D, 3) )
    ( tvrx2_tda18272_freq_map_t(646144000, 0x92, 0x1D, 3) )
    ( tvrx2_tda18272_freq_map_t(647168000, 0x91, 0x1C, 3) )
    ( tvrx2_tda18272_freq_map_t(648192000, 0x90, 0x1C, 3) )
    ( tvrx2_tda18272_freq_map_t(650240000, 0x8F, 0x1B, 3) )
    ( tvrx2_tda18272_freq_map_t(651264000, 0x8E, 0x1B, 3) )
    ( tvrx2_tda18272_freq_map_t(652288000, 0x8D, 0x1B, 3) )
    ( tvrx2_tda18272_freq_map_t(654336000, 0x8C, 0x1B, 3) )
    ( tvrx2_tda18272_freq_map_t(655360000, 0x8B, 0x1B, 3) )
    ( tvrx2_tda18272_freq_map_t(656384000, 0x8A, 0x1B, 3) )
    ( tvrx2_tda18272_freq_map_t(658432000, 0x89, 0x1A, 3) )
    ( tvrx2_tda18272_freq_map_t(659456000, 0x88, 0x1A, 3) )
    ( tvrx2_tda18272_freq_map_t(660480000, 0x87, 0x1A, 3) )
    ( tvrx2_tda18272_freq_map_t(661504000, 0x86, 0x19, 3) )
    ( tvrx2_tda18272_freq_map_t(662528000, 0x85, 0x19, 3) )
    ( tvrx2_tda18272_freq_map_t(664576000, 0x84, 0x18, 3) )
    ( tvrx2_tda18272_freq_map_t(665600000, 0x83, 0x18, 3) )
    ( tvrx2_tda18272_freq_map_t(666624000, 0x82, 0x18, 3) )
    ( tvrx2_tda18272_freq_map_t(668672000, 0x81, 0x18, 3) )
    ( tvrx2_tda18272_freq_map_t(669696000, 0x80, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(671744000, 0x7F, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(672768000, 0x7E, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(674816000, 0x7D, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(675840000, 0x7C, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(676864000, 0x7B, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(678912000, 0x7A, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(679936000, 0x79, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(681984000, 0x78, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(683008000, 0x77, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(685056000, 0x76, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(686080000, 0x75, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(688128000, 0x74, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(689152000, 0x73, 0x17, 3) )
    ( tvrx2_tda18272_freq_map_t(691200000, 0x72, 0x16, 3) )
    ( tvrx2_tda18272_freq_map_t(693248000, 0x71, 0x16, 3) )
    ( tvrx2_tda18272_freq_map_t(694272000, 0x70, 0x16, 3) )
    ( tvrx2_tda18272_freq_map_t(696320000, 0x6F, 0x15, 3) )
    ( tvrx2_tda18272_freq_map_t(697344000, 0x6E, 0x15, 3) )
    ( tvrx2_tda18272_freq_map_t(699392000, 0x6D, 0x15, 3) )
    ( tvrx2_tda18272_freq_map_t(700416000, 0x6C, 0x15, 3) )
    ( tvrx2_tda18272_freq_map_t(702464000, 0x6B, 0x14, 3) )
    ( tvrx2_tda18272_freq_map_t(704512000, 0x6A, 0x14, 3) )
    ( tvrx2_tda18272_freq_map_t(704512000, 0x69, 0x14, 3) )
    ( tvrx2_tda18272_freq_map_t(706560000, 0x68, 0x14, 3) )
    ( tvrx2_tda18272_freq_map_t(707584000, 0x67, 0x13, 3) )
    ( tvrx2_tda18272_freq_map_t(709632000, 0x66, 0x13, 3) )
    ( tvrx2_tda18272_freq_map_t(711680000, 0x65, 0x13, 3) )
    ( tvrx2_tda18272_freq_map_t(712704000, 0x64, 0x13, 3) )
    ( tvrx2_tda18272_freq_map_t(714752000, 0x63, 0x13, 3) )
    ( tvrx2_tda18272_freq_map_t(716800000, 0x62, 0x13, 3) )
    ( tvrx2_tda18272_freq_map_t(717824000, 0x61, 0x13, 3) )
    ( tvrx2_tda18272_freq_map_t(719872000, 0x60, 0x13, 3) )
    ( tvrx2_tda18272_freq_map_t(721920000, 0x5F, 0x12, 3) )
    ( tvrx2_tda18272_freq_map_t(723968000, 0x5E, 0x12, 3) )
    ( tvrx2_tda18272_freq_map_t(724992000, 0x5D, 0x12, 3) )
    ( tvrx2_tda18272_freq_map_t(727040000, 0x5C, 0x12, 3) )
    ( tvrx2_tda18272_freq_map_t(729088000, 0x5B, 0x11, 3) )
    ( tvrx2_tda18272_freq_map_t(731136000, 0x5A, 0x11, 3) )
    ( tvrx2_tda18272_freq_map_t(732160000, 0x59, 0x11, 3) )
    ( tvrx2_tda18272_freq_map_t(734208000, 0x58, 0x11, 3) )
    ( tvrx2_tda18272_freq_map_t(736256000, 0x57, 0x10, 3) )
    ( tvrx2_tda18272_freq_map_t(738304000, 0x56, 0x10, 3) )
    ( tvrx2_tda18272_freq_map_t(740352000, 0x55, 0x10, 3) )
    ( tvrx2_tda18272_freq_map_t(741376000, 0x54, 0x10, 3) )
    ( tvrx2_tda18272_freq_map_t(743424000, 0x53, 0x10, 3) )
    ( tvrx2_tda18272_freq_map_t(745472000, 0x52, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(746496000, 0x51, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(748544000, 0x50, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(750592000, 0x4F, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(752640000, 0x4E, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(753664000, 0x4D, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(755712000, 0x4C, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(757760000, 0x4B, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(759808000, 0x4A, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(761856000, 0x49, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(762880000, 0x49, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(763904000, 0x48, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(765952000, 0x47, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(768000000, 0x46, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(770048000, 0x45, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(772096000, 0x44, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(774144000, 0x43, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(776192000, 0x42, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(778240000, 0x41, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(780288000, 0x40, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(783360000, 0x3F, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(785408000, 0x3E, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(787456000, 0x3D, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(789504000, 0x3C, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(790528000, 0x3B, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(792576000, 0x3A, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(794624000, 0x39, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(797696000, 0x38, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(799744000, 0x37, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(801792000, 0x36, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(803840000, 0x35, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(806912000, 0x34, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(808960000, 0x33, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(809984000, 0x33, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(811008000, 0x32, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(813056000, 0x31, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(816128000, 0x30, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(818176000, 0x2F, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(820224000, 0x2E, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(823296000, 0x2D, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(825344000, 0x2C, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(828416000, 0x2B, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(830464000, 0x2A, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(832512000, 0x29, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(834560000, 0x28, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(836608000, 0x27, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(839680000, 0x26, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(841728000, 0x25, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(844800000, 0x24, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(847872000, 0x23, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(849920000, 0x22, 0x0F, 3) )
    ( tvrx2_tda18272_freq_map_t(852992000, 0x21, 0x0E, 3) )
    ( tvrx2_tda18272_freq_map_t(855040000, 0x20, 0x0E, 3) )
    ( tvrx2_tda18272_freq_map_t(858112000, 0x1F, 0x0E, 3) )
    ( tvrx2_tda18272_freq_map_t(861184000, 0x1E, 0x0E, 3) )
    ( tvrx2_tda18272_freq_map_t(863232000, 0x1D, 0x0E, 3) )
    ( tvrx2_tda18272_freq_map_t(866304000, 0x1C, 0x0E, 3) )
    ( tvrx2_tda18272_freq_map_t(900096000, 0x10, 0x0C, 3) )
    ( tvrx2_tda18272_freq_map_t(929792000, 0x07, 0x0B, 3) )
    ( tvrx2_tda18272_freq_map_t(969728000, 0x00, 0x0A, 3) )
;

static const freq_range_t tvrx2_freq_range(42e6, 870e6);

static const freq_range_t tvrx2_bandwidth_range = list_of
    (range_t(1.7e6))
    (range_t(6.0e6))
    (range_t(7.0e6))
    (range_t(8.0e6))
    (range_t(10.0e6))
;

static const uhd::dict<std::string, std::string> tvrx2_sd_name_to_antennas = map_list_of
    ("RX1", "J100")
    ("RX2", "J140")
;

static const uhd::dict<std::string, std::string> tvrx2_sd_name_to_conn = map_list_of
    ("RX1",  "Q")
    ("RX2",  "I")
;

static const uhd::dict<std::string, boost::uint8_t> tvrx2_sd_name_to_i2c_addr = map_list_of
    ("RX1", 0x63)
    ("RX2", 0x60)
;

static const uhd::dict<std::string, boost::uint8_t> tvrx2_sd_name_to_irq_io = map_list_of
    ("RX1", (RX1_IRQ))
    ("RX2", (RX2_IRQ))
;

static const uhd::dict<std::string, dboard_iface::aux_dac_t> tvrx2_sd_name_to_dac = map_list_of
    ("RX1", dboard_iface::AUX_DAC_A)
    ("RX2", dboard_iface::AUX_DAC_B)
;

static const uhd::dict<std::string, gain_range_t> tvrx2_gain_ranges = map_list_of
//    ("LNA", gain_range_t(-12, 15, 3))
//    ("RF_FILTER", gain_range_t(-11, -2, 3))
//    ("IR_MIXER", gain_range_t(2, 14, 3))
//    ("LPF", gain_range_t(0, 9, 3))
    ("IF", gain_range_t(0, 30, 0.5))
;

/***********************************************************************
 * The TVRX2 dboard class
 **********************************************************************/
class tvrx2 : public rx_dboard_base{
public:
    tvrx2(ctor_args_t args);
    ~tvrx2(void);

private:
    double _freq_scalar;
    double _lo_freq;
    double _if_freq;
    double _bandwidth;
    uhd::dict<std::string, double> _gains;
    tda18272hnm_regs_t _tda18272hnm_regs;
    uhd::dict<boost::uint32_t, tvrx2_tda18272_rfcal_result_t> _rfcal_results;
    uhd::dict<boost::uint32_t, tvrx2_tda18272_rfcal_coeffs_t> _rfcal_coeffs;

    bool _enabled;

    bool set_enabled(bool);

    double set_lo_freq(double target_freq);
    double set_gain(double gain, const std::string &name);
    double set_bandwidth(double bandwidth);

    void set_scaled_rf_freq(double rf_freq);
    double get_scaled_rf_freq(void);
    void set_scaled_if_freq(double if_freq);
    double get_scaled_if_freq(void);
    void send_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg);
    void read_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg);

    freq_range_t get_tda18272_rfcal_result_freq_range(boost::uint32_t result);
    void tvrx2_tda18272_init_rfcal(void);
    void tvrx2_tda18272_tune_rf_filter(boost::uint32_t uRF);
    void soft_calibration(void);
    void transition_0(void);
    void transition_1(void);
    void transition_2(int rf_freq);
    void transition_3(void);
    void transition_4(int rf_freq);
    void wait_irq(void);
    void test_rf_filter_robustness(void);

/***********************************************************************
 * The TVRX2 class helper functions
 **********************************************************************/
    /*!
     * Is the IRQ set or cleared?
     * \return true for set
     */
    bool get_irq(void){
        read_reg(0x8, 0x8);

        //return irq status
        bool irq = _tda18272hnm_regs.irq_status == tda18272hnm_regs_t::IRQ_STATUS_SET;

        UHD_LOGV(often) << boost::format(
            "TVRX2 (%s): IRQ %d"
        ) % (get_subdev_name()) % irq << std::endl;

        return irq;
    }

    /*!
     * In Power-On Reset State?
     *      Check POR logic for reset state (causes POR to clear)
     * \return true for reset
     */
    bool get_power_reset(void){
        read_reg(0x5, 0x5);

        //return POR state
        bool por = _tda18272hnm_regs.por == tda18272hnm_regs_t::POR_RESET;

        UHD_LOGV(often) << boost::format(
            "TVRX2 (%s): POR %d"
        ) % (get_subdev_name()) % int(_tda18272hnm_regs.por) << std::endl;

        return por;
    }

    /*!
     * Get the lock detect status of the LO.
     * \return sensor for locked
     */
    sensor_value_t get_locked(void){
        read_reg(0x5, 0x5);

        //return lock detect
        bool locked = _tda18272hnm_regs.lo_lock == tda18272hnm_regs_t::LO_LOCK_LOCKED;

        UHD_LOGV(often) << boost::format(
            "TVRX2 (%s): locked %d"
        ) % (get_subdev_name()) % locked << std::endl;

        return sensor_value_t("LO", locked, "locked", "unlocked");
    }

    /*!
     * Read the RSSI from the registers
     * Read the RSSI from the aux adc
     * \return the rssi sensor in dB(m?) FIXME
     */
    sensor_value_t get_rssi(void){
        //Launch RSSI calculation with MSM statemachine
        _tda18272hnm_regs.set_reg(0x19, 0x80); //set MSM_byte_1 for rssi calculation
        _tda18272hnm_regs.set_reg(0x1A, 0x01); //set MSM_byte_2 for launching rssi calculation

        send_reg(0x19, 0x1A);

        wait_irq();

        //read rssi in dBuV
        read_reg(0x7, 0x7);

        //calculate the rssi from the voltage
        double rssi_dBuV = 40.0 + double(((110.0 - 40.0)/128.0) * _tda18272hnm_regs.get_reg(0x7));
        double rssi =  rssi_dBuV - 107.0; //convert to dBm in 50ohm environment ( -108.8 if 75ohm ) FIXME

        return sensor_value_t("RSSI", rssi, "dBm");
    }

    /*!
     * Read the Temperature from the registers
     * \return the temp in degC
     */
    sensor_value_t get_temp(void){
        //Enable Temperature reading
        _tda18272hnm_regs.tm_on = tda18272hnm_regs_t::TM_ON_SENSOR_ON;
        send_reg(0x4, 0x4);

        //read temp in degC
        read_reg(0x3, 0x3);

        UHD_LOGV(often) << boost::format(
            "TVRX2 (%s): Temperature %f C"
        ) % (get_subdev_name()) % (double(_tda18272hnm_regs.tm_d)) << std::endl;

        //Disable Temperature reading
        _tda18272hnm_regs.tm_on = tda18272hnm_regs_t::TM_ON_SENSOR_OFF;
        send_reg(0x4, 0x4);

        return sensor_value_t("TEMP", double(_tda18272hnm_regs.tm_d), "degC");
    }
};

/***********************************************************************
 * Register the TVRX2 dboard
 **********************************************************************/
static dboard_base::sptr make_tvrx2(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new tvrx2(args));
}

UHD_STATIC_BLOCK(reg_tvrx2_dboard){
    //register the factory function for the rx dbid
    dboard_manager::register_dboard(0x0046, &make_tvrx2, "TVRX2", tvrx2_sd_name_to_conn.keys());
}

/***********************************************************************
 * Structors
 **********************************************************************/
tvrx2::tvrx2(ctor_args_t args) : rx_dboard_base(args){
    //FIXME for USRP1, we can only support one TVRX2 installed

    _rfcal_results = map_list_of
        (  0, tvrx2_tda18272_rfcal_result_t() )
        (  1, tvrx2_tda18272_rfcal_result_t() )
        (  2, tvrx2_tda18272_rfcal_result_t() )
        (  3, tvrx2_tda18272_rfcal_result_t() )
        (  4, tvrx2_tda18272_rfcal_result_t() )
        (  5, tvrx2_tda18272_rfcal_result_t() )
        (  6, tvrx2_tda18272_rfcal_result_t() )
        (  7, tvrx2_tda18272_rfcal_result_t() )
        (  8, tvrx2_tda18272_rfcal_result_t() )
        (  9, tvrx2_tda18272_rfcal_result_t() )
        ( 10, tvrx2_tda18272_rfcal_result_t() )
        ( 11, tvrx2_tda18272_rfcal_result_t() )
    ;

    _rfcal_coeffs = map_list_of
        ( 0, tvrx2_tda18272_rfcal_coeffs_t(0) )
        ( 1, tvrx2_tda18272_rfcal_coeffs_t(1) )
        ( 2, tvrx2_tda18272_rfcal_coeffs_t(3) )
        ( 3, tvrx2_tda18272_rfcal_coeffs_t(4) )
        ( 4, tvrx2_tda18272_rfcal_coeffs_t(6) )
        ( 5, tvrx2_tda18272_rfcal_coeffs_t(7) )
        ( 6, tvrx2_tda18272_rfcal_coeffs_t(9) )
        ( 7, tvrx2_tda18272_rfcal_coeffs_t(10) )
    ;

    //set defaults for LO, gains, and filter bandwidth
    _bandwidth = 10e6;

    _if_freq = 12.5e6;

    _enabled = false;

    //send initial register settings
    //this->read_reg(0x0, 0x43);
    //this->send_reg(0x0, 0x43);

    //send magic xtal_cal_dac setting
    send_reg(0x65, 0x65);

    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<std::string>("name")
        .set("TVRX2");
    this->get_rx_subtree()->create<sensor_value_t>("sensors/lo_locked")
        .publish(boost::bind(&tvrx2::get_locked, this));
    this->get_rx_subtree()->create<sensor_value_t>("sensors/rssi")
        .publish(boost::bind(&tvrx2::get_rssi, this));
    this->get_rx_subtree()->create<sensor_value_t>("sensors/temperature")
        .publish(boost::bind(&tvrx2::get_temp, this));
    BOOST_FOREACH(const std::string &name, tvrx2_gain_ranges.keys()){
        this->get_rx_subtree()->create<double>("gains/"+name+"/value")
            .coerce(boost::bind(&tvrx2::set_gain, this, _1, name));
        this->get_rx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(tvrx2_gain_ranges[name]);
    }
    this->get_rx_subtree()->create<double>("freq/value")
        .coerce(boost::bind(&tvrx2::set_lo_freq, this, _1));
    this->get_rx_subtree()->create<meta_range_t>("freq/range")
        .set(tvrx2_freq_range);
    this->get_rx_subtree()->create<std::string>("antenna/value")
        .set(tvrx2_sd_name_to_antennas[get_subdev_name()]);
    this->get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(list_of(tvrx2_sd_name_to_antennas[get_subdev_name()]));
    this->get_rx_subtree()->create<std::string>("connection")
        .set(tvrx2_sd_name_to_conn[get_subdev_name()]);
    this->get_rx_subtree()->create<bool>("enabled")
        .coerce(boost::bind(&tvrx2::set_enabled, this, _1))
        .set(_enabled);
    this->get_rx_subtree()->create<bool>("use_lo_offset")
        .set(false);
    this->get_rx_subtree()->create<double>("bandwidth/value")
        .coerce(boost::bind(&tvrx2::set_bandwidth, this, _1))
        .set(_bandwidth);
    this->get_rx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(tvrx2_bandwidth_range);

    //set the gpio directions and atr controls (identically)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, 0); // All unused in atr
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, OUTPUT_MASK); // Set outputs

    //configure ref_clock
    double ref_clock = this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX);

    if (ref_clock == 64.0e6) {
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, REFCLOCK_DIV4);

        UHD_LOGV(often) << boost::format(
            "TVRX2 (%s): Dividing Refclock by 4"
        ) % (get_subdev_name()) << std::endl;

        _freq_scalar = (4*16.0e6)/(this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX));
    } else if (ref_clock == 100e6) {
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, REFCLOCK_DIV6);

        UHD_LOGV(often) << boost::format(
            "TVRX2 (%s): Dividing Refclock by 6"
        ) % (get_subdev_name()) << std::endl;

        _freq_scalar = (6*16.0e6)/this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX);
    } else {
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, REFCLOCK_DIV6);
        UHD_MSG(warning) << boost::format("Unsupported ref_clock %0.2f, valid options 64e6 and 100e6") % ref_clock << std::endl;
        _freq_scalar = 1.0;
    }

    //enable only the clocks we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    UHD_LOGV(often) << boost::format(
        "TVRX2 (%s): Refclock %f Hz, scalar = %f"
    ) % (get_subdev_name()) % (this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX)) % _freq_scalar << std::endl;

    _tda18272hnm_regs.irq_polarity = tda18272hnm_regs_t::IRQ_POLARITY_RAISED_VCC;
    _tda18272hnm_regs.irq_clear = tda18272hnm_regs_t::IRQ_CLEAR_TRUE;
    send_reg(0x37, 0x37);
    send_reg(0xA, 0xA);

    send_reg(0x31, 0x31); //N_CP_Current
    send_reg(0x36, 0x36); //RSSI_Clock
    send_reg(0x24, 0x25); //AGC1_Do_step
    send_reg(0x2C, 0x2C); //AGC1_Do_step
    send_reg(0x2E, 0x2E); //AGC2_Do_step
    send_reg(0x0E, 0x0E); //AGCs_Up_step_assym
    send_reg(0x11, 0x11); //AGCs_Do_step_assym

    //intialize i2c
    //soft_calibration();
    //tvrx2_tda18272_init_rfcal();
    transition_0();
}

bool tvrx2::set_enabled(bool enable){
    if (enable == _enabled) return _enabled;

    if (enable and not _enabled){
        //setup tuner parameters
        transition_1();

        transition_2(int(tvrx2_freq_range.start()));

        test_rf_filter_robustness();

        BOOST_FOREACH(const std::string &name, tvrx2_gain_ranges.keys()){
            this->get_rx_subtree()->access<double>("gains/"+name+"/value")
                .set(tvrx2_gain_ranges[name].start());
        }

        this->get_rx_subtree()->access<double>("bandwidth/value")
            .set(_bandwidth); // default bandwidth from datasheet

        //transition_2 equivalent
        this->get_rx_subtree()->access<double>("freq/value")
            .set(tvrx2_freq_range.start());

        //enter standby mode
        transition_3();
        _enabled = true;

    } else {
        //enter standby mode
        transition_3();
        _enabled = false;
    }

    return _enabled;
}

tvrx2::~tvrx2(void){
    UHD_LOGV(often) << boost::format(
        "TVRX2 (%s): Called Destructor"
    ) % (get_subdev_name()) << std::endl;
    UHD_SAFE_CALL(if (_enabled) set_enabled(false);)
}


/***********************************************************************
 * TDA18272 Register IO Functions
 **********************************************************************/
void tvrx2::set_scaled_rf_freq(double rf_freq){
    _tda18272hnm_regs.set_rf_freq(_freq_scalar*rf_freq/1e3);
}

double tvrx2::get_scaled_rf_freq(void){
    return _tda18272hnm_regs.get_rf_freq()*1e3/_freq_scalar;
}

void tvrx2::set_scaled_if_freq(double if_freq){
    _tda18272hnm_regs.if_freq = int(_freq_scalar*if_freq/(50e3)); //max 12.8MHz??
}

double tvrx2::get_scaled_if_freq(void){
    return _tda18272hnm_regs.if_freq*50e3/_freq_scalar;
}

void tvrx2::send_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg){
    start_reg = boost::uint8_t(uhd::clip(int(start_reg), 0x0, 0x43));
    stop_reg = boost::uint8_t(uhd::clip(int(stop_reg), 0x0, 0x43));

    for(boost::uint8_t start_addr=start_reg; start_addr <= stop_reg; start_addr += sizeof(boost::uint32_t) - 1){
        int num_bytes = int(stop_reg - start_addr + 1) > int(sizeof(boost::uint32_t)) - 1 ? sizeof(boost::uint32_t) - 1 : stop_reg - start_addr + 1;

        //create buffer for register data (+1 for start address)
        byte_vector_t regs_vector(num_bytes + 1);

        //first byte is the address of first register
        regs_vector[0] = start_addr;

        //get the register data
        for(int i=0; i<num_bytes; i++){
            regs_vector[1+i] = _tda18272hnm_regs.get_reg(start_addr+i);
            UHD_LOGV(often) << boost::format(
                "TVRX2 (%s, 0x%02x): send reg 0x%02x, value 0x%04x, start_addr = 0x%04x, num_bytes %d"
            ) % (get_subdev_name()) % int(tvrx2_sd_name_to_i2c_addr[get_subdev_name()]) % int(start_addr+i) % int(regs_vector[1+i]) % int(start_addr) % num_bytes << std::endl;
        }

        //send the data
        this->get_iface()->write_i2c(
            tvrx2_sd_name_to_i2c_addr[get_subdev_name()], regs_vector
        );
    }
}

void tvrx2::read_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg){
    static const boost::uint8_t status_addr = 0x0;
    start_reg = boost::uint8_t(uhd::clip(int(start_reg), 0x0, 0x43));
    stop_reg = boost::uint8_t(uhd::clip(int(stop_reg), 0x0, 0x43));

    for(boost::uint8_t start_addr=start_reg; start_addr <= stop_reg; start_addr += sizeof(boost::uint32_t)){
        int num_bytes = int(stop_reg - start_addr + 1) > int(sizeof(boost::uint32_t)) ? sizeof(boost::uint32_t) : stop_reg - start_addr + 1;

        //create buffer for starting address
        byte_vector_t start_address_vector(1);

        //first byte is the address of first register
        start_address_vector[0] = start_addr;

        //send the start address
        this->get_iface()->write_i2c(
            tvrx2_sd_name_to_i2c_addr[get_subdev_name()], start_address_vector
        );

        //create buffer for register data
        byte_vector_t regs_vector(num_bytes);

        //read from i2c
        regs_vector = this->get_iface()->read_i2c(
            tvrx2_sd_name_to_i2c_addr[get_subdev_name()], num_bytes
        );

        for(boost::uint8_t i=0; i < num_bytes; i++){
            if (i + start_addr >= status_addr){
                _tda18272hnm_regs.set_reg(i + start_addr, regs_vector[i]);
            }
            UHD_LOGV(often) << boost::format(
                "TVRX2 (%s, 0x%02x): read reg 0x%02x, value 0x%04x, start_addr = 0x%04x, num_bytes %d"
            ) % (get_subdev_name()) % int(tvrx2_sd_name_to_i2c_addr[get_subdev_name()]) % int(start_addr+i) % int(regs_vector[i]) % int(start_addr) % num_bytes << std::endl;
        }
    }
}


/***********************************************************************
 * TDA18272 Calibration Functions
 **********************************************************************/
freq_range_t tvrx2::get_tda18272_rfcal_result_freq_range(boost::uint32_t result)
{

    uhd::dict<boost::uint32_t, freq_range_t> result_to_cal_freq_ranges_map = map_list_of
        ( 0, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[0].cal_freq[_tda18272hnm_regs.rfcal_freq0] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[1].cal_freq[_tda18272hnm_regs.rfcal_freq1] * _freq_scalar
                 ) )
        ( 1, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[1].cal_freq[_tda18272hnm_regs.rfcal_freq1] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[2].cal_freq[_tda18272hnm_regs.rfcal_freq2] * _freq_scalar
                 ) )
        ( 2, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[2].cal_freq[_tda18272hnm_regs.rfcal_freq2] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[3].cal_freq[_tda18272hnm_regs.rfcal_freq3] * _freq_scalar
                 ) )
        ( 3, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[3].cal_freq[_tda18272hnm_regs.rfcal_freq3] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[4].cal_freq[_tda18272hnm_regs.rfcal_freq4] * _freq_scalar
                 ) )
        ( 4, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[4].cal_freq[_tda18272hnm_regs.rfcal_freq4] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[5].cal_freq[_tda18272hnm_regs.rfcal_freq5] * _freq_scalar
                 ) )
        ( 5, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[5].cal_freq[_tda18272hnm_regs.rfcal_freq5] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[6].cal_freq[_tda18272hnm_regs.rfcal_freq6] * _freq_scalar
                 ) )
        ( 6, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[6].cal_freq[_tda18272hnm_regs.rfcal_freq6] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[7].cal_freq[_tda18272hnm_regs.rfcal_freq7] * _freq_scalar
                 ) )
        ( 7, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[7].cal_freq[_tda18272hnm_regs.rfcal_freq7] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[8].cal_freq[_tda18272hnm_regs.rfcal_freq8] * _freq_scalar
                 ) )
        ( 8, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[8].cal_freq[_tda18272hnm_regs.rfcal_freq8] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[9].cal_freq[_tda18272hnm_regs.rfcal_freq9] * _freq_scalar
                 ) )
        ( 9, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[9].cal_freq[_tda18272hnm_regs.rfcal_freq9] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[10].cal_freq[_tda18272hnm_regs.rfcal_freq10] * _freq_scalar
                 ) )
        (10, freq_range_t(
                 (double) tvrx2_tda18272_cal_map[10].cal_freq[_tda18272hnm_regs.rfcal_freq10] * _freq_scalar,
                 (double) tvrx2_tda18272_cal_map[11].cal_freq[_tda18272hnm_regs.rfcal_freq11] * _freq_scalar
                 ) )
    ;

    if (result < 11)
        return result_to_cal_freq_ranges_map[result]; 
    else
        return freq_range_t(0.0, 0.0);
}


/*
 * Initialize the RF Filter calibration maps after hardware init
 */
void tvrx2::tvrx2_tda18272_init_rfcal(void)
{

    /* read byte 0x38-0x43 */
    read_reg(0x38, 0x43);

    uhd::dict<boost::uint32_t, boost::uint8_t> result_to_cal_regs = map_list_of
        ( 0, _tda18272hnm_regs.rfcal_log_1)
        ( 1, _tda18272hnm_regs.rfcal_log_2)
        ( 2, _tda18272hnm_regs.rfcal_log_3)
        ( 3, _tda18272hnm_regs.rfcal_log_4)
        ( 4, _tda18272hnm_regs.rfcal_log_5)
        ( 5, _tda18272hnm_regs.rfcal_log_6)
        ( 6, _tda18272hnm_regs.rfcal_log_7)
        ( 7, _tda18272hnm_regs.rfcal_log_8)
        ( 8, _tda18272hnm_regs.rfcal_log_9)
        ( 9, _tda18272hnm_regs.rfcal_log_10)
        (10, _tda18272hnm_regs.rfcal_log_11)
        (11, _tda18272hnm_regs.rfcal_log_12)
    ;


    // Loop through rfcal_log_* registers, initialize _rfcal_results
    BOOST_FOREACH(const boost::uint32_t &result, result_to_cal_regs.keys())
        _rfcal_results[result].delta_c = result_to_cal_regs[result] > 63 ? result_to_cal_regs[result] - 128 : result_to_cal_regs[result];

    /* read byte 0x26-0x2B */
    read_reg(0x26, 0x2B);

    // Loop through rfcal_byte_* registers, initialize _rfcal_coeffs
    BOOST_FOREACH(const boost::uint32_t &subband, _rfcal_coeffs.keys())
    {
        freq_range_t subband_freqs;

        boost::uint32_t result = _rfcal_coeffs[subband].cal_number;

        subband_freqs = get_tda18272_rfcal_result_freq_range(result);

        _rfcal_coeffs[subband].RF_B1 = _rfcal_results[result].delta_c + tvrx2_tda18272_cal_map[result].c_offset[_rfcal_results[result].c_offset];

        boost::uint32_t quotient = (((_rfcal_results[result+1].delta_c + tvrx2_tda18272_cal_map[result+1].c_offset[_rfcal_results[result].c_offset])
                                        - (_rfcal_results[result].delta_c + tvrx2_tda18272_cal_map[result].c_offset[_rfcal_results[result].c_offset])) * 1000000);

        boost::uint32_t divisor = ((boost::int32_t)(subband_freqs.stop() - subband_freqs.start())/1000);

        _rfcal_coeffs[subband].RF_A1 = quotient / divisor;

    }

}

/*
 * Apply calibration coefficients to RF Filter tuning
 */
void tvrx2::tvrx2_tda18272_tune_rf_filter(boost::uint32_t uRF)
{
    boost::uint32_t                  uCounter = 0;
    boost::uint8_t                   cal_result = 0;
    boost::uint32_t                  uRFCal0 = 0;
    boost::uint32_t                  uRFCal1 = 0;
    boost::uint8_t                   subband = 0;
    boost::int32_t                   cProg = 0;
    boost::uint8_t                   gain_taper = 0;
    boost::uint8_t                   RFBand = 0;
    boost::int32_t                   RF_A1 = 0;
    boost::int32_t                   RF_B1 = 0;
    freq_range_t                     subband_freqs;

    /* read byte 0x26-0x2B */
    read_reg(0x26, 0x2B);

    subband_freqs = get_tda18272_rfcal_result_freq_range(1);
    uRFCal0 = subband_freqs.start();
    subband_freqs = get_tda18272_rfcal_result_freq_range(4);
    uRFCal1 = subband_freqs.start();

    if(uRF < uRFCal0)
        subband = 0;
    else if(uRF < 145700000)
        subband = 1;
    else if(uRF < uRFCal1)
        subband = 2;
    else if(uRF < 367400000)
        subband = 3;
    else
    {
        subband_freqs = get_tda18272_rfcal_result_freq_range(7);
        uRFCal0 = subband_freqs.start();
        subband_freqs = get_tda18272_rfcal_result_freq_range(10);
        uRFCal1 = subband_freqs.start();

        if(uRF < uRFCal0)
            subband = 4;
        else if(uRF < 625000000)
            subband = 5;
        else if(uRF < uRFCal1)
            subband = 6;
        else
            subband = 7;
    }

    cal_result = _rfcal_coeffs[subband].cal_number;
    subband_freqs = get_tda18272_rfcal_result_freq_range(cal_result);
    uRFCal0 = subband_freqs.start();

    RF_A1 = _rfcal_coeffs[subband].RF_A1;
    RF_B1 = _rfcal_coeffs[subband].RF_B1;

    uCounter = 0;
    do uCounter ++;
    while (uRF >= tvrx2_tda18272_freq_map[uCounter].rf_max && uCounter < TVRX2_TDA18272_FREQ_MAP_ENTRIES);

    cProg = tvrx2_tda18272_freq_map[uCounter - 1].c_prog;
    gain_taper = tvrx2_tda18272_freq_map[uCounter - 1].gain_taper;
    RFBand = tvrx2_tda18272_freq_map[uCounter - 1].rf_band;

    cProg = (boost::int32_t)(cProg + RF_B1 + (RF_A1*((boost::int32_t)(uRF - uRFCal0)/1000))/1000000);

    if(cProg>255)   cProg = 255;
    if(cProg<0)     cProg = 0;

    _tda18272hnm_regs.rf_filter_bypass = 1;
    _tda18272hnm_regs.rf_filter_cap = (boost::uint8_t) cProg;
    _tda18272hnm_regs.gain_taper = gain_taper;
    _tda18272hnm_regs.rf_filter_band = RFBand;

    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): Software Calibration:\n"
        "\tRF Filter Bypass = %d\n"
        "\tRF Filter Cap    = %d\n"
        "\tRF Filter Band   = %d\n"
        "\tGain Taper       = %d\n") 
        % (get_subdev_name())
        % int(_tda18272hnm_regs.rf_filter_bypass)
        % int(_tda18272hnm_regs.rf_filter_cap)
        % int(_tda18272hnm_regs.rf_filter_band) 
        % int(_tda18272hnm_regs.gain_taper)
        << std::endl;

    send_reg(0x2c, 0x2f);
}

void tvrx2::soft_calibration(void){
    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): Software Calibration: Initialize Tuner, Calibrate and Standby\n") % (get_subdev_name()) << std::endl;

    _tda18272hnm_regs.sm = tda18272hnm_regs_t::SM_NORMAL;
    _tda18272hnm_regs.sm_lna = tda18272hnm_regs_t::SM_LNA_ON;
    _tda18272hnm_regs.sm_pll = tda18272hnm_regs_t::SM_PLL_ON;

    send_reg(0x6, 0x6);
    read_reg(0x6, 0x6);

    read_reg(0x19, 0x1A);
    read_reg(0x26, 0x2B);

    _tda18272hnm_regs.rfcal_freq0  = 0x2;
    _tda18272hnm_regs.rfcal_freq1  = 0x2;
    _tda18272hnm_regs.rfcal_freq2  = 0x2;
    _tda18272hnm_regs.rfcal_freq3  = 0x2;
    _tda18272hnm_regs.rfcal_freq4  = 0x2;
    _tda18272hnm_regs.rfcal_freq5  = 0x2;
    _tda18272hnm_regs.rfcal_freq6  = 0x2;
    _tda18272hnm_regs.rfcal_freq7  = 0x2;
    _tda18272hnm_regs.rfcal_freq8  = 0x2;
    _tda18272hnm_regs.rfcal_freq9  = 0x2;
    _tda18272hnm_regs.rfcal_freq10 = 0x2;
    _tda18272hnm_regs.rfcal_freq11 = 0x2;

    send_reg(0x26, 0x2B);

    _tda18272hnm_regs.set_reg(0x19, 0x3B); //set MSM_byte_1 for calibration per datasheet
    _tda18272hnm_regs.set_reg(0x1A, 0x01); //set MSM_byte_2 for launching calibration

    send_reg(0x19, 0x1A);

    wait_irq();

    send_reg(0x1D, 0x1D); //Fmax_LO
    send_reg(0x0C, 0x0C); //LT_Enable
    send_reg(0x1B, 0x1B); //PSM_AGC1
    send_reg(0x0C, 0x0C); //AGC1_6_15dB

    //set spread spectrum for clock 
    //FIXME: NXP turns clock spread on and off 
    //   based on where clock spurs would be relative to RF frequency
    //   we should do this also
    _tda18272hnm_regs.digital_clock = tda18272hnm_regs_t::DIGITAL_CLOCK_SPREAD_OFF;
    if (get_subdev_name() == "RX1")
        //_tda18272hnm_regs.xtout = tda18272hnm_regs_t::XTOUT_NO;
        _tda18272hnm_regs.xtout = tda18272hnm_regs_t::XTOUT_16MHZ;
    else
        //_tda18272hnm_regs.xtout = tda18272hnm_regs_t::XTOUT_NO;
        _tda18272hnm_regs.xtout = tda18272hnm_regs_t::XTOUT_16MHZ;
    
    send_reg(0x14, 0x14);

    _tda18272hnm_regs.set_reg(0x36, 0x0E); //sets clock mode
    send_reg(0x36, 0x36);

    //go to standby mode
    _tda18272hnm_regs.sm = tda18272hnm_regs_t::SM_STANDBY;
    send_reg(0x6, 0x6);
}

void tvrx2::test_rf_filter_robustness(void){
    typedef uhd::dict<std::string, std::string> tvrx2_filter_ratings_t;
    typedef uhd::dict<std::string, double> tvrx2_filter_margins_t;

    tvrx2_filter_margins_t _filter_margins;
    tvrx2_filter_ratings_t _filter_ratings;

    read_reg(0x38, 0x43);

    uhd::dict<std::string, boost::uint8_t> filter_cal_regs = map_list_of
        ("VHFLow_0", 0x38)
        ("VHFLow_1", 0x3a)
        ("VHFHigh_0", 0x3b)
        ("VHFHigh_1", 0x3d)
        ("UHFLow_0", 0x3e)
        ("UHFLow_1", 0x40)
        ("UHFHigh_0", 0x41)
        ("UHFHigh_1", 0x43)
    ;

    BOOST_FOREACH(const std::string &name, filter_cal_regs.keys()){
        boost::uint8_t cal_result = _tda18272hnm_regs.get_reg(filter_cal_regs[name]);
        if (cal_result & 0x80) {
            _filter_ratings.set(name, "E");
            _filter_margins.set(name, 0.0);
        }
        else {
            double partial;

            if (name == "VHFLow_0")
                partial = 100 * (45 - 39.8225 * (1 + (0.31 * (cal_result < 64 ? cal_result : cal_result - 128)) / 1.0 / 100.0)) / 45.0;

            else if (name == "VHFLow_1")
                partial = 100 * (152.1828 * (1 + (1.53 * (cal_result < 64 ? cal_result : cal_result - 128)) / 1.0 / 100.0) - (144.896 - 6)) / (144.896 - 6); 

            else if (name == "VHFHigh_0")
                partial = 100 * ((144.896 + 6) - 135.4063 * (1 + (0.27 * (cal_result < 64 ? cal_result : cal_result - 128)) / 1.0 / 100.0)) / (144.896 + 6);

            else if (name == "VHFHigh_1")
                partial = 100 * (383.1455 * (1 + (0.91 * (cal_result < 64 ? cal_result : cal_result - 128)) / 1.0 / 100.0) - (367.104 - 8)) / (367.104 - 8);

            else if (name == "UHFLow_0")
                partial = 100 * ((367.104 + 8) - 342.6224 * (1 + (0.21 * (cal_result < 64 ? cal_result : cal_result - 128)) / 1.0 / 100.0)) / (367.104 + 8);

            else if (name == "UHFLow_1")
                partial = 100 * (662.5595 * (1 + (0.33 * (cal_result < 64 ? cal_result : cal_result - 128)) / 1.0 / 100.0) - (624.128 - 2)) / (624.128 - 2);

            else if (name == "UHFHigh_0")
                partial = 100 * ((624.128 + 2) - 508.2747 * (1 + (0.23 * (cal_result < 64 ? cal_result : cal_result - 128)) / 1.0 / 100.0)) / (624.128 + 2);

            else if (name == "UHFHigh_1")
                partial = 100 * (947.8913 * (1 + (0.3 * (cal_result < 64 ? cal_result : cal_result - 128)) / 1.0 / 100.0) - (866 - 14)) / (866 - 14);

            else
                UHD_THROW_INVALID_CODE_PATH();

            _filter_margins.set(name, 0.0024 * partial * partial * partial - 0.101 * partial * partial + 1.629 * partial + 1.8266);
            _filter_ratings.set(name, _filter_margins[name] >= 0.0 ? "H" : "L");
        }
    }

    std::stringstream robustness_message;
    robustness_message << boost::format("TVRX2 (%s): RF Filter Robustness Results:") % (get_subdev_name()) << std::endl;
    BOOST_FOREACH(const std::string &name, uhd::sorted(_filter_ratings.keys())){
        robustness_message << boost::format("\t%s:\tMargin = %0.2f,\tRobustness = %c") % name % (_filter_margins[name]) % (_filter_ratings[name]) << std::endl;
    }

    UHD_LOGV(often) << robustness_message.str();
}

/***********************************************************************
 * TDA18272 State Functions
 **********************************************************************/
void tvrx2::transition_0(void){
    //Transition 0: Initialize Tuner and place in standby
    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): Transistion 0: Initialize Tuner, Calibrate and Standby\n") % (get_subdev_name()) << std::endl;

    //Check for Power-On Reset, if reset, initialze tuner
    if (get_power_reset()) {
        _tda18272hnm_regs.sm = tda18272hnm_regs_t::SM_NORMAL;
        _tda18272hnm_regs.sm_lna = tda18272hnm_regs_t::SM_LNA_ON;
        _tda18272hnm_regs.sm_pll = tda18272hnm_regs_t::SM_PLL_ON;

        send_reg(0x6, 0x6);
        read_reg(0x6, 0x6);

        read_reg(0x19, 0x1A);

        _tda18272hnm_regs.set_reg(0x19, 0x3B); //set MSM_byte_1 for calibration per datasheet
        _tda18272hnm_regs.set_reg(0x1A, 0x01); //set MSM_byte_2 for launching calibration

        send_reg(0x19, 0x1A);

        wait_irq();
    }

    //send magic xtal_cal_dac setting
    send_reg(0x65, 0x65);

    send_reg(0x1D, 0x1D); //Fmax_LO
    send_reg(0x0C, 0x0C); //LT_Enable
    send_reg(0x1B, 0x1B); //PSM_AGC1
    send_reg(0x0C, 0x0C); //AGC1_6_15dB

    //set spread spectrum for clock 
    //FIXME: NXP turns clock spread on and off 
    //   based on where clock spurs would be relative to RF frequency
    //   we should do this also
    _tda18272hnm_regs.digital_clock = tda18272hnm_regs_t::DIGITAL_CLOCK_SPREAD_OFF;
    if (get_subdev_name() == "RX1")
        //_tda18272hnm_regs.xtout = tda18272hnm_regs_t::XTOUT_NO;
        _tda18272hnm_regs.xtout = tda18272hnm_regs_t::XTOUT_16MHZ;
    else
        //_tda18272hnm_regs.xtout = tda18272hnm_regs_t::XTOUT_NO;
        _tda18272hnm_regs.xtout = tda18272hnm_regs_t::XTOUT_16MHZ;
    
    send_reg(0x14, 0x14);

    _tda18272hnm_regs.set_reg(0x36, 0x0E); //sets clock mode
    send_reg(0x36, 0x36);

    //go to standby mode
    _tda18272hnm_regs.sm = tda18272hnm_regs_t::SM_STANDBY;
    send_reg(0x6, 0x6);
}

void tvrx2::transition_1(void){
    //Transition 1: Select TV Standard
    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): Transistion 1: Select TV Standard\n") % (get_subdev_name()) << std::endl;

    //send magic xtal_cal_dac setting
    send_reg(0x65, 0x65);

    //Choose IF Byte 1 Setting
    //_tda18272hnm_regs.if_hp_fc = tda18272hnm_regs_t::IF_HP_FC_0_4MHZ;
    //_tda18272hnm_regs.if_notch = tda18272hnm_regs_t::IF_NOTCH_OFF;
    //_tda18272hnm_regs.lp_fc_offset = tda18272hnm_regs_t::LP_FC_OFFSET_0_PERCENT;
    //_tda18272hnm_regs.lp_fc = tda18272hnm_regs_t::LP_FC_10_0MHZ;
    //send_reg(0x13, 0x13);

    //Choose IR Mixer Byte 2 Setting
    //_tda18272hnm_regs.hi_pass = tda18272hnm_regs_t::HI_PASS_DISABLE;
    //_tda18272hnm_regs.dc_notch = tda18272hnm_regs_t::DC_NOTCH_OFF;
    send_reg(0x23, 0x23);

    //Set AGC TOP Bytes
    send_reg(0x0C, 0x13);

    //Set PSM Byt1
    send_reg(0x1B, 0x1B);

    //Choose IF Frequency, setting is 50KHz steps
    set_scaled_if_freq(_if_freq);
    send_reg(0x15, 0x15);
}

void tvrx2::transition_2(int rf_freq){
    //Transition 2: Select RF Frequency after changing TV Standard
    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): Transistion 2: Select RF Frequency after changing TV Standard\n") % (get_subdev_name()) << std::endl;

    //send magic xtal_cal_dac setting
    send_reg(0x65, 0x65);

    //Wake up from Standby
    _tda18272hnm_regs.sm = tda18272hnm_regs_t::SM_NORMAL;
    _tda18272hnm_regs.sm_lna = tda18272hnm_regs_t::SM_LNA_ON;
    _tda18272hnm_regs.sm_pll = tda18272hnm_regs_t::SM_PLL_ON;
    
    send_reg(0x6, 0x6);
    
    //Set Clock Mode
    _tda18272hnm_regs.set_reg(0x36, 0x00);
    send_reg(0x36, 0x36);
    
    //Set desired RF Frequency
    set_scaled_rf_freq(rf_freq);
    send_reg(0x16, 0x18);
    
    //Lock PLL and tune RF Filters
    _tda18272hnm_regs.set_reg(0x19, 0x41); //set MSM_byte_1 for RF Filters Tuning, PLL Locking
    _tda18272hnm_regs.set_reg(0x1A, 0x01); //set MSM_byte_2 for launching calibration
    
    send_reg(0x19, 0x1A);
    
    wait_irq();

    tvrx2_tda18272_tune_rf_filter(rf_freq);

    ////LO Lock state in Reg 0x5 LSB
    //read_reg(0x6, 0x6);

}

void tvrx2::transition_3(void){
    //Transition 3: Standby Mode
    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): Transistion 3: Standby Mode\n") % (get_subdev_name()) << std::endl;

    //send magic xtal_cal_dac setting
    send_reg(0x65, 0x65);

    //Set clock mode
    _tda18272hnm_regs.set_reg(0x36, 0x0E);
    send_reg(0x36, 0x36);

    //go to standby mode
    _tda18272hnm_regs.sm = tda18272hnm_regs_t::SM_STANDBY;
    _tda18272hnm_regs.sm_lna = tda18272hnm_regs_t::SM_LNA_OFF;
    _tda18272hnm_regs.sm_pll = tda18272hnm_regs_t::SM_PLL_OFF;
    send_reg(0x6, 0x6);
}

void tvrx2::transition_4(int rf_freq){
    //Transition 4: Change RF Frequency without changing TV Standard
    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): Transistion 4: Change RF Frequency without changing TV Standard\n") % (get_subdev_name()) << std::endl;

    //send magic xtal_cal_dac setting
    send_reg(0x65, 0x65);

    //Set desired RF Frequency
    set_scaled_rf_freq(rf_freq);
    send_reg(0x16, 0x18);
    
    //Lock PLL and tune RF Filters
    _tda18272hnm_regs.set_reg(0x19, 0x41); //set MSM_byte_1 for RF Filters Tuning, PLL Locking
    _tda18272hnm_regs.set_reg(0x1A, 0x01); //set MSM_byte_2 for launching calibration
    
    send_reg(0x19, 0x1A);
    
    wait_irq();

    tvrx2_tda18272_tune_rf_filter(rf_freq);

    ////LO Lock state in Reg 0x5 LSB
    //read_reg(0x5, 0x6);

}

void tvrx2::wait_irq(void){
    int timeout = 20; //irq waiting timeout in milliseconds
    //int irq = (this->get_iface()->read_gpio(dboard_iface::UNIT_RX) & int(tvrx2_sd_name_to_irq_io[get_subdev_name()]));
    bool irq = get_irq();
    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): Waiting on IRQ, subdev = %d, mask = 0x%x, Status: 0x%x\n") % (get_subdev_name()) % get_subdev_name() % (int(tvrx2_sd_name_to_irq_io[get_subdev_name()])) % irq << std::endl;

    while (not irq and timeout > 0) {
        //irq = (this->get_iface()->read_gpio(dboard_iface::UNIT_RX) & tvrx2_sd_name_to_irq_io[get_subdev_name()]);
        irq = get_irq();
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        timeout -= 1;
    }

    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): IRQ Raised, subdev = %d, mask = 0x%x, Status: 0x%x, Timeout: %d\n") % (get_subdev_name()) % get_subdev_name() % (int(tvrx2_sd_name_to_irq_io[get_subdev_name()])) % irq % timeout << std::endl;

    read_reg(0xA, 0xB);
    //UHD_ASSERT_THROW(timeout > 0);
    if(timeout <= 0) UHD_MSG(warning) << boost::format(
        "\nTVRX2 (%s): Timeout waiting on IRQ\n") % (get_subdev_name()) << std::endl;

    _tda18272hnm_regs.irq_clear = tda18272hnm_regs_t::IRQ_CLEAR_TRUE;
    send_reg(0xA, 0xA);
    read_reg(0xA, 0xB);

    irq = (this->get_iface()->read_gpio(dboard_iface::UNIT_RX) & tvrx2_sd_name_to_irq_io[get_subdev_name()]);

    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): Cleared IRQ, subdev = %d, mask = 0x%x, Status: 0x%x\n") % (get_subdev_name()) % get_subdev_name() % (int(tvrx2_sd_name_to_irq_io[get_subdev_name()])) % irq << std::endl;
}



/***********************************************************************
 * Tuning
 **********************************************************************/
double tvrx2::set_lo_freq(double target_freq){
    //target_freq = std::clip(target_freq, tvrx2_freq_range.min, tvrx2_freq_range.max);

    read_reg(0x6, 0x6);

    if (_tda18272hnm_regs.sm == tda18272hnm_regs_t::SM_STANDBY) {
        transition_2(int(target_freq + _bandwidth/2 - get_scaled_if_freq()));
    } else {
        transition_4(int(target_freq + _bandwidth/2 - get_scaled_if_freq()));
    }
    read_reg(0x16, 0x18);

    //compute actual tuned frequency
    _lo_freq = get_scaled_rf_freq() + get_scaled_if_freq(); // - _bandwidth/2;

    //debug output of calculated variables
    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): LO Frequency\n"
        "\tRequested: \t%f\n"
        "\tComputed: \t%f\n"
        "\tReadback: \t%f\n"
        "\tIF Frequency: \t%f\n") % (get_subdev_name()) % target_freq % double(int(target_freq/1e3)*1e3) % get_scaled_rf_freq() % get_scaled_if_freq() << std::endl;

    get_locked();

    test_rf_filter_robustness();

    UHD_LOGV(often) << boost::format(
        "\nTVRX2 (%s): RSSI = %f dBm\n"
    ) % (get_subdev_name()) % (get_rssi().to_real()) << std::endl;

    return _lo_freq;
}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
/*
 * Convert the requested gain into a dac voltage
 */
static double gain_to_if_gain_dac(double &gain){
    //clip the input
    gain = tvrx2_gain_ranges["IF"].clip(gain);

    //voltage level constants
    static const double max_volts = double(1.7), min_volts = double(0.5);
    static const double slope = (max_volts-min_volts)/tvrx2_gain_ranges["IF"].stop();

    //calculate the voltage for the aux dac
    double dac_volts = gain*slope + min_volts;

    UHD_LOGV(often) << boost::format(
        "TVRX2 IF Gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts << std::endl;

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    return dac_volts;
}

double tvrx2::set_gain(double gain, const std::string &name){
    assert_has(tvrx2_gain_ranges.keys(), name, "tvrx2 gain name");

    if (name == "IF"){
        //write voltage to aux_dac
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, tvrx2_sd_name_to_dac[get_subdev_name()], gain_to_if_gain_dac(gain));
    }
    else UHD_THROW_INVALID_CODE_PATH();

    //shadow gain setting
    _gains[name] = gain;

    return gain;
}

/***********************************************************************
 * Bandwidth Handling
 **********************************************************************/
static tda18272hnm_regs_t::lp_fc_t bandwidth_to_lp_fc_reg(double &bandwidth){
    int reg = uhd::clip(boost::math::iround((bandwidth-5.0e6)/1.0e6), 0, 4);

    switch(reg){
    case 0:
        bandwidth = 1.7e6;
        return tda18272hnm_regs_t::LP_FC_1_7MHZ;
    case 1:
        bandwidth = 6e6;
        return tda18272hnm_regs_t::LP_FC_6_0MHZ;
    case 2:
        bandwidth = 7e6;
        return tda18272hnm_regs_t::LP_FC_7_0MHZ;
    case 3:
        bandwidth = 8e6;
        return tda18272hnm_regs_t::LP_FC_8_0MHZ;
    case 4:
        bandwidth = 10e6;
        return tda18272hnm_regs_t::LP_FC_10_0MHZ;
    }
    UHD_THROW_INVALID_CODE_PATH();
}

double tvrx2::set_bandwidth(double bandwidth){
    //clip the input
    bandwidth = tvrx2_bandwidth_range.clip(bandwidth);

    //compute low pass cutoff frequency setting
    _tda18272hnm_regs.lp_fc = bandwidth_to_lp_fc_reg(bandwidth);

    //shadow bandwidth setting
    _bandwidth = bandwidth;

    //update register
    send_reg(0x13, 0x13);

    UHD_LOGV(often) << boost::format(
        "TVRX2 (%s) Bandwidth (lp_fc): %f Hz, reg: %d"
    ) % (get_subdev_name()) % _bandwidth % (int(_tda18272hnm_regs.lp_fc)) << std::endl;

    return _bandwidth;
}
