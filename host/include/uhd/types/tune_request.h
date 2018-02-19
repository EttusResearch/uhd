//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_TUNE_REQUEST_H
#define INCLUDED_UHD_TYPES_TUNE_REQUEST_H

#include <uhd/config.h>

#include <stdlib.h>

//! Policy options for tunable elements in the RF chain.
typedef enum {
    //! Do not set this argument, use current setting.
    UHD_TUNE_REQUEST_POLICY_NONE   = 78,
    //! Automatically determine the argument's value.
    UHD_TUNE_REQUEST_POLICY_AUTO   = 65,
    //! Use the argument's value for the setting.
    UHD_TUNE_REQUEST_POLICY_MANUAL = 77
} uhd_tune_request_policy_t;

//! Instructs implementation how to tune the RF chain
/*!
 * See uhd::tune_request_t for more details.
 */
typedef struct {
    //! Target frequency for RF chain in Hz
    double target_freq;
    //! RF frequency policy
    uhd_tune_request_policy_t rf_freq_policy;
    //! RF frequency in Hz
    double rf_freq;
    //! DSP frequency policy
    uhd_tune_request_policy_t dsp_freq_policy;
    //! DSP frequency in Hz
    double dsp_freq;
    //! Key-value pairs delimited by commas
    char* args;
} uhd_tune_request_t;

#ifdef __cplusplus
#include <uhd/types/tune_request.hpp>

UHD_API uhd::tune_request_t uhd_tune_request_c_to_cpp(uhd_tune_request_t *tune_request_c);

#endif

#endif /* INCLUDED_UHD_TYPES_TUNE_REQUEST_H */
