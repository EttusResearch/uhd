//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/ranges.h>

#include <string.h>

/*
 * uhd::range_t
 */
uhd_error uhd_range_to_pp_string(
    const uhd_range_t *range,
    char* pp_string_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        uhd::range_t range_cpp = uhd_range_c_to_cpp(range);
        std::string pp_string_cpp = range_cpp.to_pp_string();

        memset(pp_string_out, '\0', strbuffer_len);
        strncpy(pp_string_out, pp_string_cpp.c_str(), strbuffer_len);
    )
}

uhd::range_t uhd_range_c_to_cpp(
    const uhd_range_t *range_c
){
    return uhd::range_t(range_c->start, range_c->stop, range_c->step);
}

void uhd_range_cpp_to_c(
    const uhd::range_t &range_cpp,
    uhd_range_t *range_c
){
    range_c->start = range_cpp.start();
    range_c->stop = range_cpp.stop();
    range_c->step = range_cpp.step();
}

/*
 * uhd::meta_range_t
 */
uhd_error uhd_meta_range_make(
    uhd_meta_range_handle* h
){
    UHD_SAFE_C(
        (*h) = new uhd_meta_range_t;
    )
}

uhd_error uhd_meta_range_free(
    uhd_meta_range_handle* h
){
    UHD_SAFE_C(
        delete (*h);
        (*h) = NULL;
    )
}

uhd_error uhd_meta_range_start(
    uhd_meta_range_handle h,
    double *start_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *start_out = h->meta_range_cpp.start();
    )
}

uhd_error uhd_meta_range_stop(
    uhd_meta_range_handle h,
    double *stop_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *stop_out = h->meta_range_cpp.stop();
    )
}

uhd_error uhd_meta_range_step(
    uhd_meta_range_handle h,
    double *step_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *step_out = h->meta_range_cpp.step();
    )
}

uhd_error uhd_meta_range_clip(
    uhd_meta_range_handle h,
    double value,
    bool clip_step,
    double *result_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *result_out = h->meta_range_cpp.clip(value, clip_step);
    )
}

uhd_error uhd_meta_range_size(
    uhd_meta_range_handle h,
    size_t *size_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *size_out = h->meta_range_cpp.size();
    )
}

uhd_error uhd_meta_range_push_back(
    uhd_meta_range_handle h,
    const uhd_range_t *range
){
    UHD_SAFE_C_SAVE_ERROR(h,
        h->meta_range_cpp.push_back(uhd_range_c_to_cpp(range));
    )
}

uhd_error uhd_meta_range_at(
    uhd_meta_range_handle h,
    size_t num,
    uhd_range_t *range_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd_range_cpp_to_c(h->meta_range_cpp.at(num),
                           range_out);
    )
}

uhd_error uhd_meta_range_to_pp_string(
    uhd_meta_range_handle h,
    char* pp_string_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string pp_string_cpp = h->meta_range_cpp.to_pp_string();
        memset(pp_string_out, '\0', strbuffer_len);
        strncpy(pp_string_out, pp_string_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_meta_range_last_error(
    uhd_meta_range_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}
