/*
 * Copyright 2015 Ettus Research LLC
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* C-Interface for multi_usrp_clock */

#include <uhd/utils/static.hpp>
#include <uhd/usrp_clock/multi_usrp_clock.hpp>

#include <uhd/usrp_clock/usrp_clock.h>

#include <boost/thread/mutex.hpp>

#include <string.h>
#include <map>

/****************************************************************************
 * Registry / Pointer Management
 ***************************************************************************/
/* Public structs */
struct uhd_usrp_clock {
    size_t usrp_clock_index;
    std::string last_error;
};

/* Not public: We use this for our internal registry */
struct usrp_clock_ptr {
    uhd::usrp_clock::multi_usrp_clock::sptr ptr;
    static size_t usrp_clock_counter;
};
size_t usrp_clock_ptr::usrp_clock_counter = 0;
typedef struct usrp_clock_ptr usrp_clock_ptr;
/* Prefer map, because the list can be discontiguous */
typedef std::map<size_t, usrp_clock_ptr> usrp_clock_ptrs;

UHD_SINGLETON_FCN(usrp_clock_ptrs, get_usrp_clock_ptrs);
/* Shortcut for accessing the underlying USRP clock sptr from a uhd_usrp_clock_handle* */
#define USRP_CLOCK(h_ptr) (get_usrp_clock_ptrs()[h_ptr->usrp_clock_index].ptr)

/****************************************************************************
 * Generate / Destroy API calls
 ***************************************************************************/
static boost::mutex _usrp_clock_find_mutex;
uhd_error uhd_usrp_clock_find(
    const char* args,
    uhd_string_vector_t *devices_out
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_clock_find_mutex);

        uhd::device_addrs_t devs = uhd::device::find(std::string(args), uhd::device::CLOCK);
        devices_out->string_vector_cpp.clear();
        for(const uhd::device_addr_t &dev:  devs){
            devices_out->string_vector_cpp.push_back(dev.to_string());
        }
    )
}

static boost::mutex _usrp_clock_make_mutex;
uhd_error uhd_usrp_clock_make(
    uhd_usrp_clock_handle *h,
    const char *args
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_clock_make_mutex);

        size_t usrp_clock_count = usrp_clock_ptr::usrp_clock_counter;
        usrp_clock_ptr::usrp_clock_counter++;

        // Initialize USRP Clock
        uhd::device_addr_t device_addr(args);
        usrp_clock_ptr P;
        P.ptr = uhd::usrp_clock::multi_usrp_clock::make(device_addr);

        // Dump into registry
        get_usrp_clock_ptrs()[usrp_clock_count] = P;

        // Update handle
        (*h) = new uhd_usrp_clock;
        (*h)->usrp_clock_index = usrp_clock_count;
    )
}

static boost::mutex _usrp_clock_free_mutex;
uhd_error uhd_usrp_clock_free(
    uhd_usrp_clock_handle *h
){
    UHD_SAFE_C(
        boost::mutex::scoped_lock lock(_usrp_clock_free_mutex);

        if(!get_usrp_clock_ptrs().count((*h)->usrp_clock_index)){
            return UHD_ERROR_INVALID_DEVICE;
        }

        get_usrp_clock_ptrs().erase((*h)->usrp_clock_index);
        delete *h;
        *h = NULL;
    )
}

uhd_error uhd_usrp_clock_last_error(
    uhd_usrp_clock_handle h,
    char* error_out,
    size_t strbuffer_len
){
    UHD_SAFE_C(
        memset(error_out, '\0', strbuffer_len);
        strncpy(error_out, h->last_error.c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_clock_get_pp_string(
    uhd_usrp_clock_handle h,
    char* pp_string_out,
    size_t strbuffer_len
){
    UHD_SAFE_C_SAVE_ERROR(h,
        memset(pp_string_out, '\0', strbuffer_len);
        strncpy(pp_string_out, USRP_CLOCK(h)->get_pp_string().c_str(), strbuffer_len);
    )
}

uhd_error uhd_usrp_clock_get_num_boards(
    uhd_usrp_clock_handle h,
    size_t *num_boards_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *num_boards_out = USRP_CLOCK(h)->get_num_boards();
    )
}

uhd_error uhd_usrp_clock_get_time(
    uhd_usrp_clock_handle h,
    size_t board,
    uint32_t *clock_time_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        *clock_time_out = USRP_CLOCK(h)->get_time(board);
    )
}

uhd_error uhd_usrp_clock_get_sensor(
    uhd_usrp_clock_handle h,
    const char* name,
    size_t board,
    uhd_sensor_value_handle *sensor_value_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        delete (*sensor_value_out)->sensor_value_cpp;
        (*sensor_value_out)->sensor_value_cpp = new uhd::sensor_value_t(USRP_CLOCK(h)->get_sensor(name, board));
    )
}

uhd_error uhd_usrp_clock_get_sensor_names(
    uhd_usrp_clock_handle h,
    size_t board,
    uhd_string_vector_handle *sensor_names_out
){
    UHD_SAFE_C_SAVE_ERROR(h,
        (*sensor_names_out)->string_vector_cpp = USRP_CLOCK(h)->get_sensor_names(board);
    )
}
