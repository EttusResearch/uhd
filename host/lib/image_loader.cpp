//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <map>
#include <utility>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <uhd/exception.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/utils/log.hpp>

#include <uhd/utils/static.hpp>

namespace fs = boost::filesystem;

typedef std::map<std::string, uhd::image_loader::loader_fcn_t> loader_fcn_map_t;
typedef std::map<std::string, std::string> string_map_t;

// Nice typedefs for iterating over std::map
typedef std::pair<std::string, uhd::image_loader::loader_fcn_t> loader_fcn_pair_t;
typedef std::pair<std::string, std::string> string_pair_t;

UHD_SINGLETON_FCN(loader_fcn_map_t, get_image_loaders);
UHD_SINGLETON_FCN(string_map_t,     get_recovery_strings);

/*
 * Registration
 */
void uhd::image_loader::register_image_loader(const std::string &device_type,
                                              const loader_fcn_t &loader_fcn,
                                              const std::string &recovery_instructions){
    // UHD_LOGGER_TRACE("UHD") << "Registering image loader and recovery instructions for "
    //                                 << device_type;

    get_image_loaders().insert(loader_fcn_pair_t(device_type, loader_fcn));
    get_recovery_strings().insert(string_pair_t(device_type, recovery_instructions));
}

/*
 * Actual loading
 */
bool uhd::image_loader::load(const uhd::image_loader::image_loader_args_t &image_loader_args){

    // If "type=foo" given in args, see if we have an image loader for that
    if(image_loader_args.args.has_key("type")){
        std::string type = image_loader_args.args.get("type");
        if(get_image_loaders().find(type) == get_image_loaders().end()){
            throw uhd::runtime_error(str(boost::format("There is no image loader registered for given type \"%s\".")
                                         % type));
        }
        else return get_image_loaders().at(type)(image_loader_args);
    }
    else{
        for(const loader_fcn_pair_t &loader_fcn_pair:  get_image_loaders()){
            if(loader_fcn_pair.second(image_loader_args)) return true;
        }
        return false;
    }
}

/*
 * Get recovery instructions for particular device
 */
std::string uhd::image_loader::get_recovery_instructions(const std::string &device_type){
    if(get_recovery_strings().count(device_type) == 0){
        return "A firmware or FPGA loading process was interrupted by the user. This can leave your device in a non-working state.";
    }
    else return get_recovery_strings().at(device_type);
}
