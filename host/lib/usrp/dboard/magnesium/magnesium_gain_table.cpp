//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "magnesium_gain_table.hpp"
#include "magnesium_constants.hpp"
#include <uhd/exception.hpp>
#include <algorithm>
#include <map>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace magnesium;

namespace {
    typedef magnesium_radio_ctrl_impl::rx_band rx_band;
    typedef magnesium_radio_ctrl_impl::tx_band tx_band;

    const size_t TX_LOWBAND = 0;
    const size_t TX_HIGHBAND = 1;
    const size_t RX_LOWBAND = 0;
    const size_t RX_MIDBAND = 1;
    const size_t RX_HIGHBAND = 2;

    size_t map_tx_band(const tx_band band)
    {
        if (band == tx_band::LOWBAND) {
            return TX_LOWBAND;
        }
        return TX_HIGHBAND;
    }

    size_t map_rx_band(const rx_band band)
    {
        if (band == rx_band::LOWBAND) {
            return RX_LOWBAND;
        }
        if (band == rx_band::BAND0 or
                band == rx_band::BAND1 or
                band == rx_band::BAND2 or
                band == rx_band::BAND3) {
            return RX_MIDBAND;
        }
        return RX_HIGHBAND;
    }

    //! Maps gain index -> gain_tuple_t
    //
    // Note: This is an int, for easier lookups. We're basically hardcoding the
    // knowledge that the gain map has a 1 dB granularity.
    using gain_tuple_map_t = std::map<int, gain_tuple_t>;

    //! Maps band -> gain_tuple_map_t
    using gain_tables_t = std::map<size_t, gain_tuple_map_t>;

    /*! RX gain tables
     */
    const gain_tables_t rx_gain_tables = {
        {RX_LOWBAND, {
                // Gain, DSA att, Myk att, bypass
                {0,    {30, 30, true}},
                {1,    {30, 29, true}},
                {2,    {30, 28, true}},
                {3,    {30, 27, true}},
                {4,    {30, 26, true}},
                {5,    {30, 25, true}},
                {6,    {29, 25, true}},
                {7,    {28, 25, true}},
                {8,    {27, 25, true}},
                {9,    {26, 25, true}},
                {10,   {25, 25, true}},
                {11,   {25, 24, true}},
                {12,   {25, 23, true}},
                {13,   {25, 22, true}},
                {14,   {25, 21, true}},
                {15,   {25, 20, true}},
                {16,   {24, 20, true}},
                {17,   {23, 20, true}},
                {18,   {22, 20, true}},
                {19,   {21, 20, true}},
                {20,   {20, 20, true}},
                {21,   {20, 19, true}},
                {22,   {20, 18, true}},
                {23,   {20, 17, true}},
                {24,   {20, 16, true}},
                {25,   {20, 15, true}},
                {26,   {19, 15, true}},
                {27,   {18, 15, true}},
                {28,   {17, 15, true}},
                {29,   {16, 15, true}},
                {30,   {15, 15, true}},
                {31,   {14, 15, true}},
                {32,   {13, 15, true}},
                {33,   {12, 15, true}},
                {34,   {11, 15, true}},
                {35,   {10, 15, true}},
                {36,   {10, 14, true}},
                {37,   {10, 13, true}},
                {38,   {10, 12, true}},
                {39,   {10, 11, true}},
                {40,   {10, 10, true}},
                {41,   {9, 10, true}},
                {42,   {8, 10, true}},
                {43,   {7, 10, true}},
                {44,   {6, 10, true}},
                {45,   {5, 10, true}},
                {46,   {4, 10, true}},
                {47,   {3, 10, true}},
                {48,   {2, 10, true}},
                {49,   {1, 10, true}},
                {50,   {15, 15, false}},
                {51,   {14, 15, false}},
                {52,   {13, 15, false}},
                {53,   {12, 15, false}},
                {54,   {11, 15, false}},
                {55,   {10, 15, false}},
                {56,   {10, 14, false}},
                {57,   {10, 13, false}},
                {58,   {10, 12, false}},
                {59,   {10, 11, false}},
                {60,   {10, 10, false}},
                {61,   {9, 10, false}},
                {62,   {8, 10, false}},
                {63,   {7, 10, false}},
                {64,   {6, 10, false}},
                {65,   {5, 10, false}},
                {66,   {4, 10, false}},
                {67,   {3, 10, false}},
                {68,   {2, 10, false}},
                {69,   {1, 10, false}},
                {70,   {0, 10, false}},
                {71,   {0, 9, false}},
                {72,   {0, 8, false}},
                {73,   {0, 7, false}},
                {74,   {0, 6, false}},
                {75,   {0, 5, false}}
        }},
        {RX_MIDBAND, { // Valid for bands 0, 1, 2, 3
                {0, {30, 30, true}},
                {1, {30, 29, true}},
                {2, {30, 28, true}},
                {3, {30, 27, true}},
                {4, {30, 26, true}},
                {5, {30, 25, true}},
                {6, {30, 24, true}},
                {7, {30, 23, true}},
                {8, {30, 22, true}},
                {9, {30, 21, true}},
                {10, {30, 20, true}},
                {11, {30, 19, true}},
                {12, {30, 18, true}},
                {13, {30, 17, true}},
                {14, {30, 16, true}},
                {15, {30, 15, true}},
                {16, {29, 15, true}},
                {17, {28, 15, true}},
                {18, {27, 15, true}},
                {19, {26, 15, true}},
                {20, {25, 15, true}},
                {21, {24, 15, true}},
                {22, {23, 15, true}},
                {23, {22, 15, true}},
                {24, {21, 15, true}},
                {25, {20, 15, true}},
                {26, {19, 15, true}},
                {27, {18, 15, true}},
                {28, {17, 15, true}},
                {29, {16, 15, true}},
                {30, {15, 15, true}},
                {31, {14, 15, true}},
                {32, {13, 15, true}},
                {33, {12, 15, true}},
                {34, {11, 15, true}},
                {35, {10, 15, true}},
                {36, {9, 15, true}},
                {37, {8, 15, true}},
                {38, {7, 15, true}},
                {39, {6, 15, true}},
                {40, {5, 15, true}},
                {41, {5, 14, true}},
                {42, {5, 13, true}},
                {43, {5, 12, true}},
                {44, {5, 11, true}},
                {45, {5, 10, true}},
                {46, {4, 10, true}},
                {47, {3, 10, true}},
                {48, {2, 10, true}},
                {49, {1, 10, true}},
                {50, {15, 15, false}},
                {51, {15, 14, false}},
                {52, {15, 13, false}},
                {53, {15, 12, false}},
                {54, {15, 11, false}},
                {55, {15, 10, false}},
                {56, {15, 9, false}},
                {57, {15, 8, false}},
                {58, {15, 7, false}},
                {59, {15, 6, false}},
                {60, {15, 5, false}},
                {61, {14, 5, false}},
                {62, {13, 5, false}},
                {63, {12, 5, false}},
                {64, {11, 5, false}},
                {65, {10, 5, false}},
                {66, {10, 4, false}},
                {67, {10, 3, false}},
                {68, {10, 2, false}},
                {69, {10, 1, false}},
                {70, {10, 0, false}},
                {71, {9, 0, false}},
                {72, {8, 0, false}},
                {73, {7, 0, false}},
                {74, {6, 0, false}},
                {75, {5, 0, false}}
        }},
        {RX_HIGHBAND, { // Valid for bands 4, 5, 6
                {0, {30, 30, true}},
                {1, {30, 29, true}},
                {2, {30, 28, true}},
                {3, {30, 27, true}},
                {4, {30, 26, true}},
                {5, {30, 25, true}},
                {6, {30, 24, true}},
                {7, {30, 23, true}},
                {8, {30, 22, true}},
                {9, {30, 21, true}},
                {10, {30, 20, true}},
                {11, {30, 19, true}},
                {12, {30, 18, true}},
                {13, {30, 17, true}},
                {14, {30, 16, true}},
                {15, {30, 15, true}},
                {16, {29, 15, true}},
                {17, {28, 15, true}},
                {18, {27, 15, true}},
                {19, {26, 15, true}},
                {20, {25, 15, true}},
                {21, {24, 15, true}},
                {22, {23, 15, true}},
                {23, {22, 15, true}},
                {24, {21, 15, true}},
                {25, {20, 15, true}},
                {26, {19, 15, true}},
                {27, {18, 15, true}},
                {28, {17, 15, true}},
                {29, {16, 15, true}},
                {30, {15, 15, true}},
                {31, {15, 14, true}},
                {32, {15, 13, true}},
                {33, {15, 12, true}},
                {34, {15, 11, true}},
                {35, {15, 10, true}},
                {36, {14, 10, true}},
                {37, {13, 10, true}},
                {38, {12, 10, true}},
                {39, {11, 10, true}},
                {40, {10, 10, true}},
                {41, {9, 10, true}},
                {42, {8, 10, true}},
                {43, {7, 10, true}},
                {44, {6, 10, true}},
                {45, {5, 10, true}},
                {46, {4, 10, true}},
                {47, {3, 10, true}},
                {48, {2, 10, true}},
                {49, {1, 10, true}},
                {50, {15, 15, false}},
                {51, {15, 14, false}},
                {52, {15, 13, false}},
                {53, {15, 12, false}},
                {54, {15, 11, false}},
                {55, {15, 10, false}},
                {56, {14, 10, false}},
                {57, {13, 10, false}},
                {58, {12, 10, false}},
                {59, {11, 10, false}},
                {60, {10, 10, false}},
                {61, {10, 9, false}},
                {62, {10, 8, false}},
                {63, {10, 7, false}},
                {64, {10, 6, false}},
                {65, {10, 5, false}},
                {66, {9, 5, false}},
                {67, {8, 5, false}},
                {68, {7, 5, false}},
                {69, {6, 5, false}},
                {70, {5, 5, false}},
                {71, {5, 4, false}},
                {72, {5, 3, false}},
                {73, {5, 2, false}},
                {74, {5, 1, false}},
                {75, {5, 0, false}}
            },
        }
    }; /* rx_gain_tables */

    const gain_tables_t tx_gain_tables = {
        {TX_LOWBAND, {
                // Gain, DSA att, Myk att, bypass
                {0, {30, 20, true}},
                {1, {29, 20, true}},
                {2, {28, 20, true}},
                {3, {27, 20, true}},
                {4, {26, 20, true}},
                {5, {25, 20, true}},
                {6, {24, 20, true}},
                {7, {23, 20, true}},
                {8, {22, 20, true}},
                {9, {21, 20, true}},
                {10, {20, 20, true}},
                {11, {19, 20, true}},
                {12, {18, 20, true}},
                {13, {17, 20, true}},
                {14, {16, 20, true}},
                {15, {15, 20, true}},
                {16, {14, 20, true}},
                {17, {13, 20, true}},
                {18, {12, 20, true}},
                {19, {11, 20, true}},
                {20, {10, 20, true}},
                {21, {9, 20, true}},
                {22, {8, 20, true}},
                {23, {7, 20, true}},
                {24, {6, 20, true}},
                {25, {5, 20, true}},
                {26, {4, 20, true}},
                {27, {3, 20, true}},
                {28, {2, 20, true}},
                {29, {1, 20, true}},
                {30, {0, 20, true}},
                {31, {0, 19, true}},
                {32, {0, 18, true}},
                {33, {0, 17, true}},
                {34, {0, 16, true}},
                {35, {0, 15, true}},
                {36, {0, 14, true}},
                {37, {0, 13, true}},
                {38, {0, 12, true}},
                {39, {0, 11, true}},
                {40, {10, 15, false}},
                {41, {9, 15, false}},
                {42, {8, 15, false}},
                {43, {7, 15, false}},
                {44, {6, 15, false}},
                {45, {5, 15, false}},
                {46, {4, 15, false}},
                {47, {3, 15, false}},
                {48, {2, 15, false}},
                {49, {1, 15, false}},
                {50, {0, 15, false}},
                {51, {0, 14, false}},
                {52, {0, 13, false}},
                {53, {0, 12, false}},
                {54, {0, 11, false}},
                {55, {0, 10, false}},
                {56, {0, 9, false}},
                {57, {0, 8, false}},
                {58, {0, 7, false}},
                {59, {0, 6, false}},
                {60, {0, 5, false}},
                {61, {0, 4, false}},
                {62, {0, 3, false}},
                {63, {0, 2, false}},
                {64, {0, 1, false}},
                {65, {0, 0, false}}
        }},
        {TX_HIGHBAND, { // Valid for bands 1, 2, 3, 4
                {0, {30, 20, true}},
                {1, {29, 20, true}},
                {2, {28, 20, true}},
                {3, {27, 20, true}},
                {4, {26, 20, true}},
                {5, {25, 20, true}},
                {6, {24, 20, true}},
                {7, {23, 20, true}},
                {8, {22, 20, true}},
                {9, {21, 20, true}},
                {10, {20, 20, true}},
                {11, {19, 20, true}},
                {12, {18, 20, true}},
                {13, {17, 20, true}},
                {14, {16, 20, true}},
                {15, {15, 20, true}},
                {16, {14, 20, true}},
                {17, {13, 20, true}},
                {18, {12, 20, true}},
                {19, {11, 20, true}},
                {20, {10, 20, true}},
                {21, {9, 20, true}},
                {22, {8, 20, true}},
                {23, {7, 20, true}},
                {24, {6, 20, true}},
                {25, {5, 20, true}},
                {26, {4, 20, true}},
                {27, {3, 20, true}},
                {28, {2, 20, true}},
                {29, {1, 20, true}},
                {30, {0, 20, true}},
                {31, {0, 19, true}},
                {32, {0, 18, true}},
                {33, {0, 17, true}},
                {34, {0, 16, true}},
                {35, {5, 20, false}},
                {36, {4, 20, false}},
                {37, {3, 20, false}},
                {38, {2, 20, false}},
                {39, {1, 20, false}},
                {40, {0, 20, false}},
                {41, {0, 19, false}},
                {42, {0, 18, false}},
                {43, {0, 17, false}},
                {44, {0, 16, false}},
                {45, {0, 15, false}},
                {46, {0, 14, false}},
                {47, {0, 13, false}},
                {48, {0, 12, false}},
                {49, {0, 11, false}},
                {50, {0, 10, false}},
                {51, {0, 9, false}},
                {52, {0, 8, false}},
                {53, {0, 7, false}},
                {54, {0, 6, false}},
                {55, {0, 5, false}},
                {56, {0, 4, false}},
                {57, {0, 3, false}},
                {58, {0, 2, false}},
                {59, {0, 1, false}},
                {60, {0, 0, false}},
                // Rest is fake to keep same gain range as low band
                {61, {0, 0, false}},
                {62, {0, 0, false}},
                {63, {0, 0, false}},
                {64, {0, 0, false}},
                {65, {0, 0, false}}
        }}
    }; /* tx_gain_tables */

    gain_tuple_t fine_tune_ad9371_att(
            const gain_tuple_t gain_tuple,
            const double gain_index
    ) {
        // Here, we hardcode the half-dB steps. We soak up all half-dB
        // steps by twiddling the AD9371 attenuation, but we need to make
        // sure we don't make it negative.
        if (gain_index - int(gain_index) >= .5) {
            gain_tuple_t gt2 = gain_tuple;
            gt2.ad9371_att = std::max(0.0, gain_tuple.ad9371_att - .5);
            return gt2;
        }
        return gain_tuple;
    }

} /* namespace ANON */


gain_tuple_t magnesium::get_rx_gain_tuple(
    const double gain_index,
    const magnesium_radio_ctrl_impl::rx_band band
) {
    UHD_ASSERT_THROW(
        gain_index <= ALL_RX_MAX_GAIN and gain_index >= ALL_RX_MIN_GAIN
    );
    auto &gain_table = rx_gain_tables.at(map_rx_band(band));
    const int gain_index_truncd = int(gain_index);
    return fine_tune_ad9371_att(
        gain_table.at(gain_index_truncd),
        gain_index
    );
}

gain_tuple_t magnesium::get_tx_gain_tuple(
    const double gain_index,
    const magnesium_radio_ctrl_impl::tx_band band
) {
    UHD_ASSERT_THROW(
        gain_index <= ALL_TX_MAX_GAIN and gain_index >= ALL_TX_MIN_GAIN
    );
    auto &gain_table = tx_gain_tables.at(map_tx_band(band));
    const int gain_index_truncd = int(gain_index);
    return fine_tune_ad9371_att(
        gain_table.at(gain_index_truncd),
        gain_index
    );
}

