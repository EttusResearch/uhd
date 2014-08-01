//
// Copyright 2014 Ettus Research LLC
//

#ifndef INCLUDED_AD9361_TRANSACTION_H
#define INCLUDED_AD9361_TRANSACTION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//various constants
#define AD9361_TRANSACTION_VERSION       0x5
#define AD9361_DISPATCH_PACKET_SIZE      64

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

typedef struct
{
    //version is expected to be AD9361_TRANSACTION_VERSION
    //check otherwise for compatibility
    uint32_t version;

    //sequence number - increment every call for sanity
    uint32_t sequence;

    //location info for the ad9361 chip class
    uint64_t handle;

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

#define AD9361_TRANSACTION_MAX_ERROR_MSG (AD9361_DISPATCH_PACKET_SIZE - (sizeof(ad9361_transaction_t)-4)-1)	// -4 for 'error_msg' alignment padding, -1 for terminating \0

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_AD9361_TRANSACTION_H */
