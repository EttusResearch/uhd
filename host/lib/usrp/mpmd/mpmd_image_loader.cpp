//
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_impl.hpp"
#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/exception.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/types/component_file.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/features/fpga_load_notification_iface.hpp>
#include <uhdlib/utils/prefs.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

using namespace uhd;

namespace uhd { namespace /*anon*/ {
const size_t MD5LEN = 32; // Length of a MD5 hash in chars

/*
 * Helper function to generate a component_file_t using the input ID and path to file.
 */
uhd::usrp::component_file_t generate_component(
    const std::string& id, const std::string& filepath)
{
    uhd::usrp::component_file_t component_file;
    // Add an ID to the metadata
    component_file.metadata["id"] = id;
    UHD_LOG_TRACE(
        "MPMD IMAGE LOADER", "Component ID added to the component dictionary: " << id);
    // Add the filename to the metadata
    // Remove the path to the filename
    component_file.metadata["filename"] =
        boost::filesystem::path(filepath).filename().string();
    UHD_LOG_TRACE("MPMD IMAGE LOADER",
        "Component filename added to the component dictionary: " << filepath);
    // Add the hash, if a hash file exists
    const std::string component_hash_filepath = filepath + ".md5";
    std::ifstream component_hash_ifstream(
        component_hash_filepath.c_str(), std::ios::binary);
    std::string component_hash;
    if (component_hash_ifstream.is_open()) {
        // TODO: Verify that the hash read is valid, ie only contains 0-9a-f.
        component_hash.resize(MD5LEN);
        component_hash_ifstream.read(&component_hash[0], MD5LEN);
        component_hash_ifstream.close();
        component_file.metadata["md5"] = component_hash;
        UHD_LOG_TRACE("MPMD IMAGE LOADER",
            "Added component file hash to the component dictionary.");
    } else {
        // If there is no hash file, don't worry about it too much
        UHD_LOG_DEBUG("MPMD IMAGE LOADER",
            "Could not open component file hash file: " << component_hash_filepath);
    }

    // Read the component file image into a structure suitable to sent as a binary string
    // to MPM
    std::vector<uint8_t> data;
    std::ifstream component_ifstream(filepath.c_str(), std::ios::binary);
    if (component_ifstream.is_open()) {
        data.insert(data.begin(),
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
 * Helper function to generate a component_file_t using the input ID and contents.
 */
uhd::usrp::component_file_t generate_component(const std::string& id,
    const std::vector<uint8_t>& contents,
    const uhd::dict<std::string, std::string>& metadata)
{
    uhd::usrp::component_file_t component_file;
    // Add an ID to the metadata
    component_file.metadata["id"] = id;
    component_file.metadata.update(metadata);
    UHD_LOG_TRACE(
        "MPMD IMAGE LOADER", "Component ID added to the component dictionary: " << id);
    component_file.data = contents;
    return component_file;
}

boost::optional<std::vector<uint8_t>> parse_dts_from_lvbitx(
    const boost::property_tree::ptree& pt)
{
    std::string dts;
    try {
        dts = pt.get<std::string>("Bitfile.Project.CompilationResultsTree."
                                  "CompilationResults.deviceTreeOverlay");
    } catch (boost::property_tree::ptree_error&) {
        UHD_LOG_WARNING(
            "MPMD IMAGE LOADER", "Could not find DTS in .lvbitx file, not including it");
        return boost::none;
    }

    if (dts.size() % 2 != 0) {
        throw uhd::runtime_error(
            "The deviceTreeOverlay is corrupt in the specified .lvbitx file");
    }

    std::vector<uint8_t> text;
    for (size_t i = 0; i < dts.size() / 2; i++) {
        const char s[3]     = {dts[i * 2], dts[i * 2 + 1], '\0'};
        const uint8_t value = static_cast<uint8_t>(strtoul(s, nullptr, 16));
        text.push_back(value);
    }

    return text;
}

std::vector<uint8_t> parse_bitstream_from_lvbitx(const boost::property_tree::ptree& pt)
{
    std::string encoded_bitstream = pt.get<std::string>("Bitfile.Bitstream");

    // Strip the whitespace
    encoded_bitstream.erase(std::remove_if(encoded_bitstream.begin(),
                                encoded_bitstream.end(),
                                [](char c) { return std::isspace(c); }),
        encoded_bitstream.end());

    // Base64-decode the result
    namespace bai    = boost::archive::iterators;
    using base64_dec = bai::transform_width<bai::binary_from_base64<char*>, 8, 6>;

    std::vector<uint8_t> bitstream(base64_dec(encoded_bitstream.data()),
        base64_dec(encoded_bitstream.data() + encoded_bitstream.length()));

    // Remove null bytes that were formed from the padding
    const size_t pad_count =
        std::count(encoded_bitstream.begin(), encoded_bitstream.end(), '=');
    bitstream.erase(bitstream.end() - pad_count, bitstream.end());

    return bitstream;
}

static std::string get_fpga_path(
    const image_loader::image_loader_args_t& image_loader_args,
    device_addr_t dev_addr,
    uhd::property_tree::sptr tree)
{
    // If the user provided a path to an fpga image, use that
    if (not image_loader_args.fpga_path.empty()) {
        if (boost::filesystem::exists(image_loader_args.fpga_path)) {
            return image_loader_args.fpga_path;
        } else {
            throw uhd::runtime_error(
                "FPGA file provided does not exist: " + image_loader_args.fpga_path);
        }
    }
    // Otherwise, we need to generate one
    else {
        /*
         * The user can specify an FPGA type (HG, XG, AA), rather than a
         * filename. If the user does not specify one, this will default to
         * the type currently on the device. If this cannot be determined,
         * then the user is forced to specify a filename.
         */
        const auto fpga_type = [image_loader_args, tree]() -> std::string {
            // If the user didn't provide a type, use the type of currently
            // loaded image on the device
            if (image_loader_args.args.has_key("fpga")) {
                return image_loader_args.args.get("fpga");
            } else if (tree->exists("/mboards/0/components/fpga")) {
                // Pull the FPGA info from the property tree
                // The getter should return a vector of a single
                // component_file_t, so grab the metadata from that
                auto fpga_metadata = tree->access<uhd::usrp::component_files_t>(
                                             "/mboards/0/components/fpga")
                                         .get()[0]
                                         .metadata;
                return fpga_metadata.get("type", "");
            }
            return "";
        }(); // generate_fpga_type lambda function
        UHD_LOG_TRACE("MPMD IMAGE LOADER", "FPGA type: " << fpga_type);

        if (!dev_addr.has_key("product")) {
            throw uhd::runtime_error("Found a device but could not "
                                     "auto-generate an image filename.");
        } else if (fpga_type.empty()) {
            return find_image_path(
                "usrp_" + boost::algorithm::to_lower_copy(dev_addr["product"]) + "_fpga.bit");
        } else {
            return find_image_path(
                "usrp_" + boost::algorithm::to_lower_copy(dev_addr["product"]) + "_fpga_" + fpga_type + ".bit");
        }
    }
}

static uhd::usrp::component_files_t lvbitx_to_component_files(
    std::string fpga_path, bool delay_reload)
{
    uhd::usrp::component_files_t all_component_files;

    boost::property_tree::ptree pt;
    try {
        boost::property_tree::xml_parser::read_xml(fpga_path, pt);
    } catch (const boost::property_tree::xml_parser::xml_parser_error& ex) {
        throw uhd::runtime_error(
            std::string("Got error parsing lvbitx file: ") + ex.what());
    }

    const auto bitstream = parse_bitstream_from_lvbitx(pt);

    uhd::dict<std::string, std::string> fpga_metadata;
    fpga_metadata.set("filename", "usrp_x410_fpga_LV.bin");
    fpga_metadata.set("reset", delay_reload ? "false" : "true");

    uhd::usrp::component_file_t comp_fpga =
        generate_component("fpga", bitstream, fpga_metadata);
    all_component_files.push_back(comp_fpga);

    const auto maybe_dts = parse_dts_from_lvbitx(pt);
    if (maybe_dts) {
        const auto dts = maybe_dts.get();

        uhd::dict<std::string, std::string> dts_metadata;
        dts_metadata.set("filename", "usrp_x410_fpga_LV.dts");
        dts_metadata.set("reset", delay_reload ? "false" : "true");

        uhd::usrp::component_file_t comp_dts =
            generate_component("dts", dts, dts_metadata);
        all_component_files.push_back(comp_dts);
    }

    return all_component_files;
}

static uhd::usrp::component_files_t bin_dts_to_component_files(
    std::string fpga_path, bool delay_reload)
{
    uhd::usrp::component_files_t all_component_files;

    uhd::usrp::component_file_t comp_fpga = generate_component("fpga", fpga_path);

    // If we want to delay the image reloading, explicitly turn off the
    // component reset flag
    if (delay_reload) {
        comp_fpga.metadata.set("reset", "false");
    }
    all_component_files.push_back(comp_fpga);
    // DTS component struct
    // First, we need to determine the name
    const std::string base_name =
        boost::filesystem::change_extension(fpga_path, "").string();
    if (base_name == fpga_path) {
        const std::string err_msg(
            "Can't cut extension from FPGA filename... " + fpga_path);
        throw uhd::runtime_error(err_msg);
    }
    UHD_LOG_TRACE("MPMD IMAGE LOADER", "base_name = " << base_name);
    const std::string dts_path = base_name + ".dts";
    // Then try to generate it
    try {
        uhd::usrp::component_file_t comp_dts = generate_component("dts", dts_path);
        // If we want to delay the image reloading, explicitly turn off the
        // component reset flag
        if (delay_reload) {
            comp_dts.metadata.set("reset", "false");
        }
        all_component_files.push_back(comp_dts);
        UHD_LOG_TRACE("MPMD IMAGE LOADER", "FPGA and DTS images read from file.");
    } catch (const uhd::runtime_error& ex) {
        // If we can't find the DTS file, that's fine, continue without it
        UHD_LOG_WARNING("MPMD IMAGE LOADER", ex.what());
        UHD_LOG_TRACE("MPMD IMAGE LOADER", "FPGA images read from file.");
    }

    return all_component_files;
}

static void mpmd_send_fpga_to_device(
    const image_loader::image_loader_args_t& image_loader_args, device_addr_t dev_addr)
{
    // Skip initializing the device
    dev_addr["skip_init"] = "1";

    // Make the device
    uhd::device::sptr usrp        = uhd::device::make(dev_addr, uhd::device::USRP);
    uhd::property_tree::sptr tree = usrp->get_tree();

    // Generate the component files
    uhd::usrp::component_files_t all_component_files;

    // Determine if we need to just reload the currently-loaded components.
    // Typically called after doing delayed reload.
    if (image_loader_args.just_reload) {
        UHD_LOG_TRACE("MPMD IMAGE LOADER",
            "Just reloading components. Creating stub components for reset operation.");
        uhd::usrp::component_file_t comp_fpga_stub;
        comp_fpga_stub.metadata["id"] = "fpga";
        // Set the "just_reload" field to force a MPM reset leading to
        // component reload.
        comp_fpga_stub.metadata["just_reload"] = "true";

        all_component_files.push_back(comp_fpga_stub);
    } else if (not image_loader_args.id.empty()
               && not image_loader_args.component.empty()) {
        uhd::usrp::component_file_t component = generate_component(image_loader_args.id,
            image_loader_args.component,
            image_loader_args.metadata);
        all_component_files.push_back(component);
    } else {
        // Determine if we need to delay the reload of fpga/dts components.
        const bool delay_reload = image_loader_args.delay_reload;
        UHD_LOG_TRACE(
            "MPMD IMAGE LOADER", "Delay reload?: " << (delay_reload ? "Yes" : "No"));

        // FPGA component struct
        const auto fpga_path = get_fpga_path(image_loader_args, dev_addr, tree);
        UHD_LOG_TRACE("MPMD IMAGE LOADER", "FPGA path: " << fpga_path);

        // If the fpga_path is a lvbitx file, parse it as such
        if (boost::filesystem::extension(fpga_path) == ".lvbitx") {
            all_component_files = lvbitx_to_component_files(fpga_path, delay_reload);
        } else {
            all_component_files = bin_dts_to_component_files(fpga_path, delay_reload);
        }
    }

    // Call RPC to update the component
    UHD_LOG_INFO("MPMD IMAGE LOADER", "Starting update. This may take a while.");
    tree->access<uhd::usrp::component_files_t>("/mboards/0/components/fpga")
        .set(all_component_files);
    UHD_LOG_INFO("MPMD IMAGE LOADER", "Update component function succeeded.");
}

/*
 * Function to be registered with uhd_image_loader
 */
static bool mpmd_image_loader(const image_loader::image_loader_args_t& image_loader_args)
{
    // See if any MPM devices with the given args are found
    device_addr_t find_hint = prefs::get_usrp_args(image_loader_args.args);
    find_hint.set("find_all", "1"); // We need to find all devices
    device_addrs_t devs = mpmd_find(find_hint);

    if (devs.size() != 1) {
        UHD_LOG_ERROR("MPMD IMAGE LOADER", "mpmd_image_loader only supports a single device.");
        return false;
    }
    // Grab the first device_addr
    device_addr_t dev_addr(devs[0]);

    mpmd_send_fpga_to_device(image_loader_args, dev_addr);

    {
        // All MPM devices use RFNoC
        auto graph = rfnoc::rfnoc_graph::make(find_hint);
        for (size_t mb_index = 0; mb_index < graph->get_num_mboards(); mb_index++) {
            auto mboard = graph->get_mb_controller(mb_index);
            if (mboard->has_feature<uhd::features::fpga_load_notification_iface>()) {
                auto& notifier =
                    mboard->get_feature<uhd::features::fpga_load_notification_iface>();
                notifier.onload();
            }
        }
    }

    return true;
}

}} // namespace uhd::

UHD_STATIC_BLOCK(register_mpm_image_loader)
{
    // TODO: Update recovery instructions
    const std::string recovery_instructions =
        "Aborting. Your USRP MPM-enabled device's update may or may not have\n"
        "completed. The contents of the image files may have been corrupted.\n"
        "Please verify those files as soon as possible.";

    // TODO: 'n3xx' doesn't really fit the MPM abstraction, but this is simpler for the
    // time being
    image_loader::register_image_loader("n3xx", mpmd_image_loader, recovery_instructions);
    image_loader::register_image_loader("e3xx", mpmd_image_loader, recovery_instructions);
    image_loader::register_image_loader("x4xx", mpmd_image_loader, recovery_instructions);
}
