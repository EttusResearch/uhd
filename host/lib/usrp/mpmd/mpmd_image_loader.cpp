//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include "mpmd_impl.hpp"
#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/component_file.hpp>
#include <sstream>
#include <string>
#include <fstream>
#include <iterator>
#include <streambuf>

using namespace uhd;

namespace uhd{ namespace /*anon*/{
    const size_t MD5LEN = 32; // Length of a MD5 hash in chars

/*
 * Function to be registered with uhd_image_loader
 */
static bool mpmd_image_loader(const image_loader::image_loader_args_t &image_loader_args){
    // See if any MPM devices with the given args are found
    device_addrs_t devs = mpmd_find(image_loader_args.args);

    if (devs.size() != 1) {
        // TODO: Do we want to handle multiple devices here?
        //       Or force uhd_image_loader to only feed us a single device?
        return false;
    }
    // Grab the first device_addr and add the option to skip initialization
    device_addr_t dev_addr(devs[0]);
    dev_addr["skip_init"] = "1";
    // Make the device
    uhd::device::sptr usrp = uhd::device::make(dev_addr, uhd::device::USRP);
    uhd::property_tree::sptr tree = usrp->get_tree();

    // Populate the struct that we use to update the FPGA property
    uhd::usrp::component_file_t component_file;
    // Add an ID to the metadata
    component_file.metadata["id"] = "fpga";
    UHD_LOG_TRACE("MPMD IMAGE LOADER",
                  "FPGA ID added to the component dictionary");
    // Add the filename to the metadata
    // TODO: Current this field is the absolute path on the host. We're letting MPM
    //       handle cutting off the filename, but it would be better to just pass what we need to.
    std::string fpga_filepath = image_loader_args.fpga_path;
    component_file.metadata["filename"] = fpga_filepath;
    UHD_LOG_TRACE("MPMD IMAGE LOADER",
                  "FPGA filename added to the component dictionary: " << fpga_filepath);
    // Add the hash, if a hash file exists
    std::string fpga_hash_filepath = fpga_filepath + ".md5";
    std::ifstream fpga_hash_ifstream(fpga_hash_filepath.c_str(), std::ios::binary);
    std::string fpga_hash;
    if (fpga_hash_ifstream.is_open()) {
        // TODO: Verify that the hash read is valid, ie only contains 0-9a-f.
        fpga_hash.resize(MD5LEN);
        fpga_hash_ifstream.read( &fpga_hash[0], MD5LEN );
        fpga_hash_ifstream.close();
        component_file.metadata["md5"] = fpga_hash;
        UHD_LOG_TRACE("MPMD IMAGE LOADER", "Added FPGA hash to the component dictionary.");
    } else {
        // If there is no hash file, don't worry about it too much
        UHD_LOG_WARNING("MPMD IMAGE LOADER", "Could not open FPGA hash file: "
                      << fpga_hash_filepath);
    }

    // Read the FPGA image into a structure suitable to sent as a binary string to MPM
    // TODO: We don't have a default image specified because the image will depend on the
    //       device and configuration. We'll probably want to fix this later, but it will
    //       depend on how uhd_images_downloader deposits files.
    std::vector<uint8_t> data;
    std::ifstream fpga_ifstream(fpga_filepath.c_str(), std::ios::binary);
    if (fpga_ifstream.is_open()) {
        data.insert( data.begin(),
                     std::istreambuf_iterator<char>(fpga_ifstream),
                     std::istreambuf_iterator<char>());
        fpga_ifstream.close();
    } else {
        std::string err_msg("FPGA Bitfile does not exist. " + fpga_filepath);
        throw uhd::runtime_error(err_msg);
    }
    component_file.data = data;
    UHD_LOG_TRACE("MPMD IMAGE LOADER", "FPGA image read from file.");

    // Call RPC to update the component
    tree->access<uhd::usrp::component_file_t>("/mboards/0/components/fpga").set(component_file);
    UHD_LOG_TRACE("MPMD IMAGE LOADER", "Update component function succeeded.");

    return true;
}
}} //namespace uhd::/*anon*/

UHD_STATIC_BLOCK(register_mpm_image_loader){
    // TODO: Update recovery instructions
    std::string recovery_instructions = "Aborting. Your USRP MPM-enabled device will likely be unusable.";

    //TODO: 'n3xx' doesn't really fit the MPM abstraction, but this is simpler for the time being
    image_loader::register_image_loader("n3xx", mpmd_image_loader, recovery_instructions);
}
