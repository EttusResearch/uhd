//
// Copyright 2013 Ettus Research LLC
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

#ifndef INCLUDED_AD9361_TRANSACTION_H
#define INCLUDED_AD9361_TRANSACTION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//various constants
#define AD9361_TRANSACTION_VERSION 0x4
#define AD9361_TRANSACTION_MAX_ERROR_MSG 40

//action types
#define AD9361_ACTION_ECHO 0
#define AD9361_ACTION_INIT 1
#define AD9361_ACTION_SET_RX1_GAIN 2
#define AD9361_ACTION_SET_TX1_GAIN 3
#define AD9361_ACTION_SET_RX2_GAIN 4
#define AD9361_ACTION_SET_TX2_GAIN 5
#define AD9361_ACTION_SET_RX_FREQ 6
#define AD9361_ACTION_SET_TX_FREQ 7
#define AD9361_ACTION_SET_CODEC_LOOP 8
#define AD9361_ACTION_SET_CLOCK_RATE 9
#define AD9361_ACTION_SET_ACTIVE_CHAINS 10

typedef union
{
    double d;
    uint32_t x[2];
} ad9361_double_union_t;

static inline void ad9361_double_pack(const double input, uint32_t output[2])
{
    ad9361_double_union_t p = {};
    p.d = input;
    output[0] = p.x[0];
    output[1] = p.x[1];
}

static inline double ad9361_double_unpack(const uint32_t input[2])
{
    ad9361_double_union_t p = {};
    p.x[0] = input[0];
    p.x[1] = input[1];
    return p.d;
}

typedef struct
{
    //version is expected to be AD9361_TRANSACTION_VERSION
    //check otherwise for compatibility
    uint32_t version;

    //sequence number - increment every call for sanity
    uint32_t sequence;

    //action tells us what to do, see AD9361_ACTION_*
    uint32_t action;

    union
    {
        //enable mask for chains
        uint32_t enable_mask;

        //true to enable codec internal loopback
        uint32_t codec_loop;

        //freq holds request LO freq and result from tune
        uint32_t freq[2];

        //gain holds request gain and result from action
        uint32_t gain[2];

        //rate holds request clock rate and result from action
        uint32_t rate[2];

    } value;

    //error message comes back as a reply -
    //set to null string for no error \0
    char error_msg[];

} ad9361_transaction_t;


#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_AD9361_TRANSACTION_H */
