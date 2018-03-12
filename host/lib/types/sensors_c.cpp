//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/sensors.h>
#include <stdexcept>
#include <string>
#include <string.h>

uhd_error uhd_sensor_value_make(
    uhd_sensor_value_handle* h
){
    try{
        *h = new uhd_sensor_value_t;
    }
    catch(...){
        return UHD_ERROR_UNKNOWN;
    }

    UHD_SAFE_C_SAVE_ERROR((*h),
        (*h)->sensor_value_cpp = new uhd::sensor_value_t("", false, "", "");
    )
}

uhd_error uhd_sensor_value_make_from_bool(
    uhd_sensor_value_handle* h,
    const char* name,
    bool value,
    const char* utrue,
    const char* ufalse
){
    try{
        *h = new uhd_sensor_value_t;
    }
    catch(...){
        return UHD_ERROR_UNKNOWN;
    }

    UHD_SAFE_C_SAVE_ERROR((*h),
        (*h)->sensor_value_cpp = new uhd::sensor_value_t(name,
                                                         value,
                                                         utrue,
                                                         ufalse);
    )
}

uhd_error uhd_sensor_value_make_from_int(
    uhd_sensor_value_handle* h,
    const char* name,
    int value,
    const char* unit,
    const char* formatter
){
    try{
        *h = new uhd_sensor_value_t;
    }
    catch(...){
        return UHD_ERROR_UNKNOWN;
    }

    UHD_SAFE_C_SAVE_ERROR((*h),
        std::string fmt(formatter);
        if(fmt.empty()){
            (*h)->sensor_value_cpp = new uhd::sensor_value_t(name,
                                                             value,
                                                             unit);
        }
        else{
            (*h)->sensor_value_cpp = new uhd::sensor_value_t(name,
                                                             value,
                                                             unit,
                                                             fmt);
        }
    )
} 

uhd_error uhd_sensor_value_make_from_realnum(
    uhd_sensor_value_handle* h,
    const char* name,
    double value,
    const char* unit,
    const char* formatter
){
    try{
        *h = new uhd_sensor_value_t;
    }
    catch(...){
        return UHD_ERROR_UNKNOWN;
    }

    UHD_SAFE_C_SAVE_ERROR((*h),
        std::string fmt(formatter);
        if(fmt.empty()){
            (*h)->sensor_value_cpp = new uhd::sensor_value_t(name,
                                                             value,
                                                             unit);
        }
        else{
            (*h)->sensor_value_cpp = new uhd::sensor_value_t(name,
                                                             value,
                                                             unit,
                                                             fmt);
        }
    )
}

uhd_error uhd_sensor_value_make_from_string(
    uhd_sensor_value_handle* h,
    const char* name,
    const char* value,
    const char* unit
){
    try{
        *h = new uhd_sensor_value_t;
    }
    catch(...){
        return UHD_ERROR_UNKNOWN;
    }

    UHD_SAFE_C_SAVE_ERROR((*h),
        (*h)->sensor_value_cpp = new uhd::sensor_value_t(name,
                                                         value,
                                                         unit);
    )
}

uhd_error uhd_sensor_value_free(
    uhd_sensor_value_handle *h
){
    UHD_SAFE_C(
        delete (*h)->sensor_value_cpp;
        delete *h;
        *h = NULL;
    )
}

uhd_error uhd_sensor_value_to_bool(
    uhd_sensor_value_handle h,
    bool *value_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *value_out = h->sensor_value_cpp->to_bool();
    )
}

uhd_error uhd_sensor_value_to_int(
    uhd_sensor_value_handle h,
    int *value_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *value_out = h->sensor_value_cpp->to_int();
    )
}

uhd_error uhd_sensor_value_to_realnum(
    uhd_sensor_value_handle h,
    double *value_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *value_out = h->sensor_value_cpp->to_real();
    )
}

uhd_error uhd_sensor_value_name(
    uhd_sensor_value_handle h,
    char* name_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        memset(name_out, '\0', strbuffer_len);
        strncpy(name_out, h->sensor_value_cpp->name.c_str(), strbuffer_len);
    )
}

uhd_error uhd_sensor_value_value(
    uhd_sensor_value_handle h,
    char* value_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        memset(value_out, '\0', strbuffer_len);
        strncpy(value_out, h->sensor_value_cpp->value.c_str(), strbuffer_len);
    )
}

uhd_error uhd_sensor_value_unit(
    uhd_sensor_value_handle h,
    char* unit_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        memset(unit_out, '\0', strbuffer_len);
        strncpy(unit_out, h->sensor_value_cpp->unit.c_str(), strbuffer_len);
    )
}

uhd_error uhd_sensor_value_data_type(
    uhd_sensor_value_handle h,
    uhd_sensor_value_data_type_t *data_type_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *data_type_out = uhd_sensor_value_data_type_t(h->sensor_value_cpp->type);
    )
}

uhd_error uhd_sensor_value_to_pp_string(
    uhd_sensor_value_handle h,
    char* pp_string_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        std::string pp_string_cpp = h->sensor_value_cpp->to_pp_string();
        memset(pp_string_out, '\0', strbuffer_len);
        strncpy(pp_string_out, pp_string_cpp.c_str(), strbuffer_len);
    )
}

uhd_error uhd_sensor_value_last_error(
    uhd_sensor_value_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}
