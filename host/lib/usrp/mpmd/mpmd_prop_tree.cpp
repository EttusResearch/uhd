//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// property tree initialization code

#include "mpmd_impl.hpp"
#include <uhd/property_tree.hpp>
#include <uhd/types/component_file.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>

using namespace uhd;
using namespace uhd::mpmd;

namespace {
    //! Timeout value for the update_component RPC call (ms)
    constexpr size_t MPMD_UPDATE_COMPONENT_TIMEOUT = 20000;

    /*! Update a component using all required files. For example, when updating the FPGA image
     * (.bit or .bin), users can provide a new overlay image (DTS) to apply in addition.
     *
     * \param comps Vector of component files to be updated
     * \param mb Reference to the actual device
     */
    uhd::usrp::component_files_t _update_component(
        const uhd::usrp::component_files_t& comps,
        mpmd_mboard_impl *mb
    ) {
        // Construct the arguments to update component
        std::vector<std::vector<uint8_t>> all_data;
        std::vector<std::map<std::string, std::string>> all_metadata;
        // Also construct a copy of just the metadata to store in the property tree
        uhd::usrp::component_files_t all_comps_copy;

        for  (const auto& comp : comps) {
            // Make a map for update components args
            std::map<std::string, std::string> metadata;
            // Make a component copy to add to the property tree
            uhd::usrp::component_file_t comp_copy;
            // Copy the metadata
            for (const auto& key : comp.metadata.keys()) {
                metadata[key] = comp.metadata[key];
                comp_copy.metadata[key] = comp.metadata[key];
            }
            // Copy to the update component args
            all_data.push_back(comp.data);
            all_metadata.push_back(metadata);
            // Copy to the property tree
            all_comps_copy.push_back(comp_copy);
        }

        // Now call update component
        const size_t update_component_timeout = MPMD_UPDATE_COMPONENT_TIMEOUT * comps.size();
        mb->rpc->set_timeout(update_component_timeout);
        mb->rpc->notify_with_token("update_component", all_metadata, all_data);
        mb->set_timeout_default();

        return all_comps_copy;
    }

    /*
     * Query the device to get the metadata for desired component
     *
     * \param comp_name String component name
     * \param mb Reference to the actual device
     * \return component files containing the component metadata
     */
    uhd::usrp::component_files_t _get_component_info(
        const std::string &comp_name,
        mpmd_mboard_impl *mb
    ) {
        UHD_LOG_TRACE("MPMD", "Getting component info for " << comp_name);
        const auto component_metadata = mb->rpc->request<std::map<std::string, std::string>>(
            "get_component_info", comp_name);
        // Copy the contents of the component metadata into a object we can return
        uhd::usrp::component_file_t return_component;
        auto &return_metadata = return_component.metadata;
        for (auto item : component_metadata) {
            return_metadata[item.first] = item.second;
        }
        return uhd::usrp::component_files_t {return_component};
    }
}

void mpmd_impl::init_property_tree(
        uhd::property_tree::sptr tree,
        fs_path mb_path,
        mpmd_mboard_impl *mb
) {
    /*** Device info ****************************************************/
    if (not tree->exists("/name")) {
        tree->create<std::string>("/name")
            .set(mb->device_info.get("description", "Unknown MPM device"))
        ;
    }
    tree->create<std::string>(mb_path / "name")
        .set(mb->device_info.get("name", "UNKNOWN"));
    tree->create<std::string>(mb_path / "serial")
        .set(mb->device_info.get("serial", "n/a"));
    tree->create<std::string>(mb_path / "connection")
        .set(mb->device_info.get("connection", "UNKNOWN"));
    tree->create<size_t>(mb_path / "link_max_rate").set(1e9 / 8);
    tree->create<std::string>(mb_path / "mpm_version")
        .set(mb->device_info.get("mpm_version", "UNKNOWN"));
    tree->create<std::string>(mb_path / "fpga_version")
        .set(mb->device_info.get("fpga_version", "UNKNOWN"));

    /*** Clocking *******************************************************/
    tree->create<std::string>(mb_path / "clock_source/value")
        .add_coerced_subscriber([mb](const std::string &clock_source){
            // FIXME: Undo these changes
            //mb->rpc->notify_with_token("set_clock_source", clock_source);
            auto current_src = mb->rpc->request_with_token<std::string>(
                "get_clock_source"
            );
            if (current_src != clock_source) {
                UHD_LOG_WARNING("MPMD",
                    "Setting clock source at runtime is currently not "
                    "supported. Use clock_source=XXX as a device arg to "
                    "select a clock source. The current source is: "
                    << current_src);
            }
        })
        .set_publisher([mb](){
            return mb->rpc->request_with_token<std::string>(
                "get_clock_source"
            );
        })
    ;
    tree->create<std::vector<std::string>>(
            mb_path / "clock_source/options")
        .set_publisher([mb](){
            return mb->rpc->request_with_token<std::vector<std::string>>(
                "get_clock_sources"
            );
        })
    ;
    tree->create<std::string>(mb_path / "time_source/value")
        .add_coerced_subscriber([mb](const std::string &time_source){
            //mb->rpc->notify_with_token("set_time_source", time_source);
            // FIXME: Undo these changes
            auto current_src = mb->rpc->request_with_token<std::string>(
                "get_time_source"
            );
            if (current_src != time_source) {
                UHD_LOG_WARNING("MPMD",
                    "Setting time source at runtime is currently not "
                    "supported. Use time_source=XXX as a device arg to "
                    "select a time source. The current source is: "
                    << current_src);
            }
        })
        .set_publisher([mb](){
            return mb->rpc->request_with_token<std::string>(
                "get_time_source"
            );
        })
    ;
    tree->create<std::vector<std::string>>(
            mb_path / "time_source/options")
        .set_publisher([mb](){
            return mb->rpc->request_with_token<std::vector<std::string>>(
                "get_time_sources"
            );
        })
    ;

    /*** Sensors ********************************************************/
    auto sensor_list =
        mb->rpc->request_with_token<std::vector<std::string>>(
            "get_mb_sensors"
        );
    UHD_LOG_DEBUG("MPMD",
        "Found " << sensor_list.size() << " motherboard sensors."
    );
    for (const auto& sensor_name : sensor_list) {
        UHD_LOG_TRACE("MPMD",
            "Adding motherboard sensor `" << sensor_name << "'"
        );
        tree->create<sensor_value_t>(
                mb_path / "sensors" / sensor_name)
            .set_publisher([mb, sensor_name](){
                return sensor_value_t(
                    mb->rpc->request_with_token<sensor_value_t::sensor_map_t>(
                        "get_mb_sensor", sensor_name
                    )
                );
            })
            .set_coercer([](const sensor_value_t &){
                throw uhd::runtime_error(
                    "Trying to write read-only sensor value!"
                );
                return sensor_value_t("", "", "");
            })
        ;
    }

    /*** EEPROM *********************************************************/
    tree->create<uhd::usrp::mboard_eeprom_t>(mb_path / "eeprom")
        .add_coerced_subscriber([mb](const uhd::usrp::mboard_eeprom_t& mb_eeprom){
            eeprom_map_t eeprom_map;
            for (const auto& key : mb_eeprom.keys()) {
                eeprom_map[key] = std::vector<uint8_t>(
                        mb_eeprom[key].cbegin(),
                        mb_eeprom[key].cend()
                );
            }
            mb->rpc->notify_with_token("set_mb_eeprom", eeprom_map);
        })
        .set_publisher([mb](){
            auto mb_eeprom =
                mb->rpc->request_with_token<std::map<std::string, std::string>>(
                    "get_mb_eeprom"
                );
            uhd::usrp::mboard_eeprom_t mb_eeprom_dict(
                mb_eeprom.cbegin(), mb_eeprom.cend()
            );
            return mb_eeprom_dict;
        })
    ;

    /*** Updateable Components ******************************************/
    std::vector<std::string> updateable_components =
            mb->rpc->request<std::vector<std::string>>(
                    "list_updateable_components"
            );
    // TODO: Check the 'id' against the registered property
    UHD_LOG_DEBUG("MPMD",
                "Found " << updateable_components.size() << " updateable motherboard components."
            );
    for (const auto& comp_name : updateable_components) {
        UHD_LOG_TRACE("MPMD",
                "Adding motherboard component: " << comp_name);
        tree->create<uhd::usrp::component_files_t>(mb_path / "components" / comp_name)
            .set_coercer([mb](const uhd::usrp::component_files_t& comp_files){
                return _update_component(
                        comp_files,
                        mb
                );
            })
            .set_publisher([mb, comp_name](){
                return _get_component_info(
                        comp_name,
                        mb
                );
            })
        ; // Done adding component to property tree
    }

    /*** MTUs ***********************************************************/
    tree->create<size_t>(mb_path / "mtu/recv")
        .add_coerced_subscriber([](const size_t){
            throw uhd::runtime_error(
                "Attempting to write read-only value (MTU)!");
        })
        .set_publisher([mb](){
            return mb->get_mtu(uhd::RX_DIRECTION);
        })
    ;
    tree->create<size_t>(mb_path / "mtu/send")
        .add_coerced_subscriber([](const size_t){
            throw uhd::runtime_error(
                "Attempting to write read-only value (MTU)!");
        })
        .set_publisher([mb](){
            return mb->get_mtu(uhd::TX_DIRECTION);
        })
    ;
}

