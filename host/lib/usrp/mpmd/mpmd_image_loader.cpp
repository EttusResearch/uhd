//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_impl.hpp"
#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/component_file.hpp>
#include <uhd/utils/paths.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/convenience.hpp>
#include <sstream>
#include <string>
#include <fstream>
#include <iterator>
#include <streambuf>

using namespace uhd;

namespace uhd{ namespace /*anon*/{
    const size_t MD5LEN = 32; // Length of a MD5 hash in chars

/*
 * Helper function to generate a component_file_t using the input ID and path to file.
 */
uhd::usrp::component_file_t generate_component(
    const std::string& id,
    const std::string& filepath
) {
    uhd::usrp::component_file_t component_file;
    // Add an ID to the metadata
    component_file.metadata["id"] = id;
    UHD_LOG_TRACE("MPMD IMAGE LOADER",
                  "Component ID added to the component dictionary: " << id);
    // Add the filename to the metadata
    // Remove the path to the filename
    component_file.metadata["filename"] = boost::filesystem::path(filepath).filename().string();
    UHD_LOG_TRACE("MPMD IMAGE LOADER",
                  "Component filename added to the component dictionary: " << filepath);
    // Add the hash, if a hash file exists
    const std::string component_hash_filepath = filepath + ".md5";
    std::ifstream component_hash_ifstream(component_hash_filepath.c_str(), std::ios::binary);
    std::string component_hash;
    if (component_hash_ifstream.is_open()) {
        // TODO: Verify that the hash read is valid, ie only contains 0-9a-f.
        component_hash.resize(MD5LEN);
        component_hash_ifstream.read( &component_hash[0], MD5LEN );
        component_hash_ifstream.close();
        component_file.metadata["md5"] = component_hash;
        UHD_LOG_TRACE("MPMD IMAGE LOADER",
                      "Added component file hash to the component dictionary.");
    } else {
        // If there is no hash file, don't worry about it too much
        UHD_LOG_DEBUG("MPMD IMAGE LOADER", "Could not open component file hash file: "
                      << component_hash_filepath);
    }

    // Read the component file image into a structure suitable to sent as a binary string to MPM
    std::vector<uint8_t> data;
    std::ifstream component_ifstream(filepath.c_str(), std::ios::binary);
    if (component_ifstream.is_open()) {
        data.insert( data.begin(),
                     std::istreambuf_iterator<char>(component_ifstream),
                     std::istreambuf_iterator<char>());
        component_ifstream.close();
    } else {
        const std::string err_msg("Component file does not exist: " + filepath);
        throw uhd::runtime_error(err_msg);
    }
    component_file.data = data;
    return component_file;
}

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

    // Generate the component files
    uhd::usrp::component_files_t all_component_files;

    // FPGA component struct
    const auto fpga_path = [image_loader_args, dev_addr, tree]() -> std::string {
        // If the user provided a path to an fpga image, use that
        if (not image_loader_args.fpga_path.empty()) {
            if (boost::filesystem::exists(image_loader_args.fpga_path)) {
                return image_loader_args.fpga_path;
            } else {
                throw uhd::runtime_error(str(
                    boost::format("FPGA file provided does not exist: %s")
                    % image_loader_args.fpga_path
                ));
            }
        }
        // Otherwise, we need to generate one
        else {
            /*
            * The user can specify an FPGA type (HG, XG, AA), rather than a filename. If the user
            * does not specify one, this will default to the type currently on the device. If this
            * cannot be determined, then the user is forced to specify a filename.
            */
            const auto fpga_type = [image_loader_args, tree]() -> std::string {
                // If the user didn't provide a type, use the type of currently loaded image on
                // the device
                if (image_loader_args.args.has_key("fpga")) {
                    return image_loader_args.args.get("fpga");
                } else if (tree->exists("/mboards/0/components/fpga")) {
                        // Pull the FPGA info from the property tree
                        // The getter should return a vector of a single component_file_t,
                        // so grab the metadata from that
                        auto fpga_metadata =
                                tree->access<uhd::usrp::component_files_t>(
                                        "/mboards/0/components/fpga").get()[0].metadata;
                        return fpga_metadata.get("type", "");
                        // TODO: Do we want to pull the type from the filename if its not
                        // available in the metadata directly?
                }
                return "";
            }(); // generate_fpga_type lambda function
            UHD_LOG_TRACE("MPMD IMAGE LOADER", "FPGA type: " << fpga_type);

            if(!dev_addr.has_key("product") or fpga_type == ""){
                throw uhd::runtime_error(
                        "Found a device but could not auto-generate an image filename.");
            }
            else {
                return find_image_path(
                    str(boost::format("usrp_%s_fpga_%s.bit")
                        % (boost::algorithm::to_lower_copy(dev_addr["product"]))
                        % fpga_type));
            }
        }
    }(); // generate_fpga_path lambda function
    UHD_LOG_TRACE("MPMD IMAGE LOADER", "FPGA path: " << fpga_path);
    uhd::usrp::component_file_t comp_fpga = generate_component("fpga", fpga_path);
    all_component_files.push_back(comp_fpga);
    // DTS component struct
    // First, we need to determine the name
    const std::string base_name = boost::filesystem::change_extension(fpga_path, "").string();
    if (base_name == fpga_path) {
        const std::string err_msg("Can't cut extension from FPGA filename... " + fpga_path);
        throw uhd::runtime_error(err_msg);
    }
    const std::string dts_path = base_name + ".dts";
    // Then try to generate it
    try {
        uhd::usrp::component_file_t comp_dts = generate_component("dts", dts_path);
        all_component_files.push_back(comp_dts);
        UHD_LOG_TRACE("MPMD IMAGE LOADER", "FPGA and DTS images read from file.");
    } catch (const uhd::runtime_error& ex) {
        // If we can't find the DTS file, that's fine, continue without it
        UHD_LOG_WARNING("MPMD IMAGE LOADER", ex.what());
        UHD_LOG_TRACE("MPMD IMAGE LOADER", "FPGA images read from file.");
    }

    // Call RPC to update the component
    UHD_LOG_INFO("MPMD IMAGE LOADER", "Starting update. This may take a while.");
    tree->access<uhd::usrp::component_files_t>("/mboards/0/components/fpga").set(all_component_files);
    UHD_LOG_INFO("MPMD IMAGE LOADER", "Update component function succeeded.");

    return true;
}
}} //namespace uhd::/*anon*/

UHD_STATIC_BLOCK(register_mpm_image_loader){
    // TODO: Update recovery instructions
    const std::string recovery_instructions = "Aborting. Your USRP MPM-enabled device's update may or may not have\n"
                                              "completed. The contents of the image files may have been corrupted.\n"
                                              "Please verify those files as soon as possible.";

    //TODO: 'n3xx' doesn't really fit the MPM abstraction, but this is simpler for the time being
    image_loader::register_image_loader("n3xx", mpmd_image_loader, recovery_instructions);
}
