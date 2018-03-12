//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/subdev_spec.h>

#include <string.h>

uhd_error uhd_subdev_spec_pair_free(
    uhd_subdev_spec_pair_t *subdev_spec_pair
){
    UHD_SAFE_C(
        if(subdev_spec_pair->db_name){
            free(subdev_spec_pair->db_name);
            subdev_spec_pair->db_name = NULL;
        }
        if(subdev_spec_pair->sd_name){
            free(subdev_spec_pair->sd_name);
            subdev_spec_pair->sd_name = NULL;
        }
    )
}

uhd_error uhd_subdev_spec_pairs_equal(
    const uhd_subdev_spec_pair_t* first,
    const uhd_subdev_spec_pair_t* second,
    bool *result_out
){
    UHD_SAFE_C(
        *result_out = (uhd_subdev_spec_pair_c_to_cpp(first) ==
                       uhd_subdev_spec_pair_c_to_cpp(second));
    )
}

uhd_error uhd_subdev_spec_make(
    uhd_subdev_spec_handle* h,
    const char* markup
){
    UHD_SAFE_C(
        (*h) = new uhd_subdev_spec_t;
        std::string markup_cpp(markup);
        if(!markup_cpp.empty()){
            (*h)->subdev_spec_cpp = uhd::usrp::subdev_spec_t(markup_cpp);
        }
    )
}

uhd_error uhd_subdev_spec_free(
    uhd_subdev_spec_handle* h
){
    UHD_SAFE_C(
        delete (*h);
        (*h) = NULL;
    )
}

uhd_error uhd_subdev_spec_size(
    uhd_subdev_spec_handle h,
    size_t *size_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *size_out = h->subdev_spec_cpp.size();
    )
}

uhd_error uhd_subdev_spec_push_back(
    uhd_subdev_spec_handle h,
    const char* markup
){
    UHD_SAFE_C_SAVE_ERROR(h,
        h->subdev_spec_cpp.push_back(uhd::usrp::subdev_spec_pair_t(markup));
    )
}

uhd_error uhd_subdev_spec_at(
    uhd_subdev_spec_handle h,
    size_t num,
    uhd_subdev_spec_pair_t *subdev_spec_pair_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        uhd_subdev_spec_pair_cpp_to_c(
            h->subdev_spec_cpp.at(num),
            subdev_spec_pair_out
        );
    )
}

uhd_error uhd_subdev_spec_to_pp_string(
    uhd_subdev_spec_handle h,
    char* pp_string_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string pp_string_cpp = h->subdev_spec_cpp.to_pp_string();
        memset(pp_string_out, '\0', strbuffer_len);
        strncpy(pp_string_out, pp_string_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_subdev_spec_to_string(
    uhd_subdev_spec_handle h,
    char* string_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string string_cpp = h->subdev_spec_cpp.to_string();
        memset(string_out, '\0', strbuffer_len);
        strncpy(string_out, string_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_subdev_spec_last_error(
    uhd_subdev_spec_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}

uhd::usrp::subdev_spec_pair_t uhd_subdev_spec_pair_c_to_cpp(
    const uhd_subdev_spec_pair_t *subdev_spec_pair_c
){
    return uhd::usrp::subdev_spec_pair_t(subdev_spec_pair_c->db_name,
                                         subdev_spec_pair_c->sd_name);
}

void uhd_subdev_spec_pair_cpp_to_c(
    const uhd::usrp::subdev_spec_pair_t &subdev_spec_pair_cpp,
    uhd_subdev_spec_pair_t *subdev_spec_pair_c
){
    subdev_spec_pair_c->db_name = strdup(subdev_spec_pair_cpp.db_name.c_str());
    subdev_spec_pair_c->sd_name = strdup(subdev_spec_pair_cpp.sd_name.c_str());
}
