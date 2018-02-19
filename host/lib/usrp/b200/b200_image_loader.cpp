//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>

#include <uhd/exception.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/static.hpp>

#include "b200_iface.hpp"
#include "b200_impl.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

namespace uhd{

static b200_iface::sptr get_b200_iface(const image_loader::image_loader_args_t& image_loader_args,
                                       mboard_eeprom_t &mb_eeprom, usb_device_handle::sptr& handle,
                                       bool user_specified){

    std::vector<usb_device_handle::sptr> dev_handles = get_b200_device_handles(image_loader_args.args);
    std::vector<usb_device_handle::sptr> applicable_dev_handles;
    b200_iface::sptr iface;
    mboard_eeprom_t eeprom; // Internal use

    if(dev_handles.size() > 0){
        for(usb_device_handle::sptr dev_handle:  dev_handles){
            if(dev_handle->firmware_loaded()){
                iface = b200_iface::make(usb_control::make(dev_handle, 0));
                eeprom = b200_impl::get_mb_eeprom(iface);
                if(user_specified){
                    if(image_loader_args.args.has_key("serial") and
                       eeprom.get("serial") != image_loader_args.args.get("serial")){
                           continue;
                    }
                    if(image_loader_args.args.has_key("name") and
                       eeprom.get("name") != image_loader_args.args.get("name")){
                           continue;
                    }
                    applicable_dev_handles.push_back(dev_handle);
                }
                else applicable_dev_handles.push_back(dev_handle);
            }
        }

        // At this point, we should have a single B2XX
        if(applicable_dev_handles.size() == 1){
            mb_eeprom = eeprom;
            handle = applicable_dev_handles[0];
            return iface;
        }
        else if(applicable_dev_handles.size() > 1){
            std::string err_msg = "Could not resolve given args to a single B2XX device.\n"
                                  "Applicable devices:\n";

            for (usb_device_handle::sptr dev_handle: applicable_dev_handles) {
                eeprom = b200_impl::get_mb_eeprom(
                    b200_iface::make(usb_control::make(dev_handle, 0))
                );
                err_msg += str(boost::format(" * %s (serial=%s)\n")
                               % B2XX_STR_NAMES.get(get_b200_product(dev_handle, mb_eeprom), "B2XX")
                               % mb_eeprom.get("serial"));
            }

            err_msg += "\nSpecify one of these devices with the given args to load an image onto it.";

            throw uhd::runtime_error(err_msg);
         }
    }

    // No applicable devices found, return empty sptr so we can exit
    iface.reset();
    mb_eeprom = eeprom;
    return iface;
}

static bool b200_image_loader(const image_loader::image_loader_args_t &image_loader_args){
    if(!image_loader_args.load_fpga)
        return false;

    bool user_specified = (image_loader_args.args.has_key("serial") or
                           image_loader_args.args.has_key("name"));

    // See if a B2x0 with the given args is found
    mboard_eeprom_t mb_eeprom;
    usb_device_handle::sptr handle;
    b200_iface::sptr iface = get_b200_iface(image_loader_args, mb_eeprom, handle, user_specified);
    if(!iface) return false; // No initialized B2x0 found

    std::string fpga_path;
    if(image_loader_args.fpga_path == ""){
        /*
         * Normally, we can auto-generate the FPGA filename from what's in the EEPROM,
         * but if the applicable value is not in the EEPROM, the user must give a specific
         * filename for us to use.
         */
        std::string product = mb_eeprom.get("product");
        if(not B2XX_PRODUCT_ID.has_key(boost::lexical_cast<uint16_t>(product))){
            if(user_specified){
                // The user specified a bad device but expects us to know what it is
                throw uhd::runtime_error("Could not determine model. You must manually specify an FPGA image filename.");
            }
            else{
                return false;
            }
        }
        else{
            fpga_path = find_image_path(B2XX_FPGA_FILE_NAME.get(get_b200_product(handle, mb_eeprom)));
        }
    }
    else fpga_path = image_loader_args.fpga_path;

    std::cout << boost::format("Unit: USRP %s (%s)")
                 % B2XX_STR_NAMES.get(get_b200_product(handle, mb_eeprom), "B2XX")
                 % mb_eeprom.get("serial")
              << std::endl;

    iface->load_fpga(fpga_path, true);

    return true;
}

UHD_STATIC_BLOCK(register_b200_image_loader){
    std::string recovery_instructions = "This device is likely in an unusable state. Power-cycle the\n"
                                        "device, and the firmware/FPGA will be reloaded the next time\n"
                                        "UHD uses the device.";

    image_loader::register_image_loader("b200", b200_image_loader, recovery_instructions);
}

} /* namespace uhd */
