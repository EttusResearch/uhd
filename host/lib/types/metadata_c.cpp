//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/metadata.h>

#include <uhd/types/time_spec.hpp>

#include <string.h>

/*
 * RX metadata
 */

uhd_error uhd_rx_metadata_make(
    uhd_rx_metadata_handle* handle
){
    UHD_SAFE_C(
        *handle = new uhd_rx_metadata_t;
    )
}

uhd_error uhd_rx_metadata_free(
    uhd_rx_metadata_handle* handle
){
    UHD_SAFE_C(
        delete *handle;
        *handle = NULL;
    )
}

uhd_error uhd_rx_metadata_has_time_spec(
    uhd_rx_metadata_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
         *result_out = h->rx_metadata_cpp.has_time_spec;
    )
}

uhd_error uhd_rx_metadata_time_spec(
    uhd_rx_metadata_handle h,
    time_t *full_secs_out,
    double *frac_secs_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::time_spec_t time_spec_cpp = h->rx_metadata_cpp.time_spec;
        *full_secs_out = time_spec_cpp.get_full_secs();
        *frac_secs_out = time_spec_cpp.get_frac_secs();
    )
}

uhd_error uhd_rx_metadata_more_fragments(
    uhd_rx_metadata_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *result_out = h->rx_metadata_cpp.more_fragments;
    )
}

uhd_error uhd_rx_metadata_fragment_offset(
    uhd_rx_metadata_handle h,
    size_t *fragment_offset_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *fragment_offset_out = h->rx_metadata_cpp.fragment_offset;
    )
}

uhd_error uhd_rx_metadata_start_of_burst(
    uhd_rx_metadata_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *result_out = h->rx_metadata_cpp.start_of_burst;
    )
}

uhd_error uhd_rx_metadata_end_of_burst(
    uhd_rx_metadata_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *result_out = h->rx_metadata_cpp.end_of_burst;
    )
}

uhd_error uhd_rx_metadata_out_of_sequence(
    uhd_rx_metadata_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *result_out = h->rx_metadata_cpp.out_of_sequence;
    )
}

uhd_error uhd_rx_metadata_to_pp_string(
    uhd_rx_metadata_handle h,
    char* pp_string_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string pp_string_cpp = h->rx_metadata_cpp.to_pp_string();
        memset(pp_string_out, '\0', strbuffer_len);
        strncpy(pp_string_out, pp_string_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_rx_metadata_error_code(
    uhd_rx_metadata_handle h,
    uhd_rx_metadata_error_code_t *error_code_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *error_code_out = uhd_rx_metadata_error_code_t(h->rx_metadata_cpp.error_code);
    )
}

uhd_error uhd_rx_metadata_strerror(
    uhd_rx_metadata_handle h,
    char* strerror_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string strerror_cpp = h->rx_metadata_cpp.strerror();
        memset(strerror_out, '\0', strbuffer_len);
        strncpy(strerror_out, strerror_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_rx_metadata_last_error(
    uhd_rx_metadata_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}

/*
 * TX metadata
 */

uhd_error uhd_tx_metadata_make(
    uhd_tx_metadata_handle* handle,
    bool has_time_spec,
    time_t full_secs,
    double frac_secs,
    bool start_of_burst,
    bool end_of_burst
){
    UHD_SAFE_C(
        *handle = new uhd_tx_metadata_t;
        (*handle)->tx_metadata_cpp.has_time_spec = has_time_spec;
        if(has_time_spec){
            (*handle)->tx_metadata_cpp.time_spec = uhd::time_spec_t(full_secs, frac_secs);
        }
        (*handle)->tx_metadata_cpp.start_of_burst = start_of_burst;
        (*handle)->tx_metadata_cpp.end_of_burst = end_of_burst;
    )
}

uhd_error uhd_tx_metadata_free(
    uhd_tx_metadata_handle* handle
){
    UHD_SAFE_C(
        delete *handle;
        *handle = NULL;
    )
}

uhd_error uhd_tx_metadata_has_time_spec(
    uhd_tx_metadata_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
         *result_out = h->tx_metadata_cpp.has_time_spec;
    )
}

uhd_error uhd_tx_metadata_time_spec(
    uhd_tx_metadata_handle h,
    time_t *full_secs_out,
    double *frac_secs_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::time_spec_t time_spec_cpp = h->tx_metadata_cpp.time_spec;
        *full_secs_out = time_spec_cpp.get_full_secs();
        *frac_secs_out = time_spec_cpp.get_frac_secs();
    )
}

uhd_error uhd_tx_metadata_start_of_burst(
    uhd_tx_metadata_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *result_out = h->tx_metadata_cpp.start_of_burst;
    )
}

uhd_error uhd_tx_metadata_end_of_burst(
    uhd_tx_metadata_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *result_out = h->tx_metadata_cpp.end_of_burst;
    )
}

uhd_error uhd_tx_metadata_last_error(
    uhd_tx_metadata_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}

/*
 * Async metadata
 */

uhd_error uhd_async_metadata_make(
    uhd_async_metadata_handle* handle
){
    UHD_SAFE_C(
        *handle = new uhd_async_metadata_t;
    )
}

uhd_error uhd_async_metadata_free(
    uhd_async_metadata_handle* handle
){
    UHD_SAFE_C(
        delete *handle;
        *handle = NULL;
    )
}

uhd_error uhd_async_metadata_channel(
    uhd_async_metadata_handle h,
    size_t *channel_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *channel_out = h->async_metadata_cpp.channel;
    )
}

uhd_error uhd_async_metadata_has_time_spec(
    uhd_async_metadata_handle h,
    bool *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
         *result_out = h->async_metadata_cpp.has_time_spec;
    )
}

uhd_error uhd_async_metadata_time_spec(
    uhd_async_metadata_handle h,
    time_t *full_secs_out,
    double *frac_secs_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd::time_spec_t time_spec_cpp = h->async_metadata_cpp.time_spec;
        *full_secs_out = time_spec_cpp.get_full_secs();
        *frac_secs_out = time_spec_cpp.get_frac_secs();
    )
}

uhd_error uhd_async_metadata_event_code(
    uhd_async_metadata_handle h,
    uhd_async_metadata_event_code_t *event_code_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *event_code_out = uhd_async_metadata_event_code_t(h->async_metadata_cpp.event_code);
    )
}

uhd_error uhd_async_metadata_user_payload(
    uhd_async_metadata_handle h,
    uint32_t user_payload_out[4]
){
    UHD_SAFE_C_SAVE_ERROR(h,
        memcpy(user_payload_out, h->async_metadata_cpp.user_payload, 4*sizeof(uint32_t));
    )
}

uhd_error uhd_async_metadata_last_error(
    uhd_async_metadata_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}
