//
// Copyright 2011-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/container.hpp>
#include <uhd/cal/database.hpp>
#include <uhd/cal/iq_cal.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/utils/csv.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/apply_corrections.hpp>
#include <uhdlib/utils/paths.hpp>
#include <unordered_map>
#include <boost/filesystem.hpp> // For deprecated CSV reader only
#include <complex>
#include <cstdio>
#include <fstream>
#include <mutex>

using namespace uhd::usrp::cal;

std::mutex corrections_mutex;

/***********************************************************************
 * FE apply corrections implementation
 **********************************************************************/
namespace {

// Cache the loaded data so we don't have to serialize on every tune
std::unordered_map<std::string, iq_cal::sptr> fe_cal_cache;

// Deprecated CSV file loader. Delete this function once we remove CSV support.
// Then, also delete the uhd::csv module.
bool load_legacy_fe_corrections(const std::string& cal_key,
    const std::string& db_serial,
    const std::string& file_prefix)
{
    namespace fs                             = boost::filesystem;
    const std::string file_prefix_deprecated = file_prefix + "_cal_v0.2_";
    // make the calibration file path
    const fs::path cal_data_path = fs::path(uhd::get_appdata_path()) / ".uhd" / "cal"
                                   / (file_prefix_deprecated + db_serial + ".csv");
    UHD_LOG_TRACE(
        "CAL", "Checking for deprecated CSV-based cal data at " << cal_data_path);
    if (not fs::exists(cal_data_path)) {
        return false;
    }

    // The serial/timestamp don't really matter, we never look them  up once we
    // generate the container here.
    auto iq_cal_container = iq_cal::make(file_prefix, db_serial, 0);

    // parse csv file
    std::ifstream cal_data(cal_data_path.string().c_str());
    const uhd::csv::rows_type rows = uhd::csv::to_rows(cal_data);
    bool read_data = false, skip_next = false;
    for (const uhd::csv::row_type& row : rows) {
        if (not read_data and not row.empty() and row[0] == "DATA STARTS HERE") {
            read_data = true;
            skip_next = true;
            continue;
        }
        if (not read_data)
            continue;
        if (skip_next) {
            skip_next = false;
            continue;
        }

        iq_cal_container->set_cal_coeff(
            std::stod(row[0]), {std::stod(row[1]), std::stod(row[2])});
    }
    fe_cal_cache.insert({cal_key, iq_cal_container});
    UHD_LOGGER_INFO("CAL") << "Calibration data loaded: " << cal_data_path.string();
    return true;
}

void apply_fe_corrections(uhd::property_tree::sptr sub_tree,
    const std::string& db_serial,
    const uhd::fs_path& fe_path,
    const std::string& file_prefix,
    const double lo_freq)
{
    const auto cal_key = file_prefix + ":" + db_serial;
    // Check if we need to load cal data
    if (!fe_cal_cache.count(cal_key)) {
        if (database::has_cal_data(file_prefix, db_serial)) {
            try {
                const auto cal_data = database::read_cal_data(file_prefix, db_serial);
                fe_cal_cache.insert({cal_key, container::make<iq_cal>(cal_data)});
                UHD_LOG_DEBUG("CAL",
                    "Loaded calibration data for " << file_prefix
                                                   << " serial=" << db_serial);
            } catch (const uhd::exception& ex) {
                UHD_LOG_WARNING("CAL",
                    "Error occurred reading cal data: `" << ex.what()
                                                         << "'. Skipping future loads.");
                fe_cal_cache.insert({cal_key, nullptr});
            }
            // Delete the following else clause once we remove CSV support
        } else if (load_legacy_fe_corrections(cal_key, db_serial, file_prefix)) {
            UHD_LOG_WARNING("CAL",
                "Found deprecated (CSV-based) cal data format. This feature will go away "
                "in the future, please convert your calibration data to the new binary "
                "format, or re-run your self-cal routines. For more information, see "
                "https://files.ettus.com/manual/page_calibration.html");
        } else {
            // If there is no cal data, store a nullptr so we can skip the check
            // next time.
            fe_cal_cache.insert({cal_key, nullptr});
            UHD_LOG_TRACE("CAL",
                "No calibration data found for " << file_prefix
                                                 << " serial=" << db_serial);
        }
    }

    // Check if valid data even exists
    if (fe_cal_cache.at(cal_key) == nullptr) {
        return;
    }

    // OK we have cal data: Now apply it
    sub_tree->access<std::complex<double>>(fe_path).set(
        fe_cal_cache.at(cal_key)->get_cal_coeff(lo_freq));
}

} // namespace

/******************************************************************************
 * Wrapper routines with nice try/catch + print, RFNoC device version
 *****************************************************************************/
void uhd::usrp::apply_tx_fe_corrections(property_tree::sptr sub_tree,
    const std::string& db_serial,
    const uhd::fs_path tx_fe_corr_path,
    const double lo_freq)
{
    std::lock_guard<std::mutex> l(corrections_mutex);
    try {
        apply_fe_corrections(
            sub_tree, db_serial, tx_fe_corr_path + "/iq_balance/value", "tx_iq", lo_freq);
    } catch (const std::exception& e) {
        UHD_LOGGER_ERROR("CAL") << "Failure in apply_tx_fe_corrections: " << e.what();
    }

    try {
        apply_fe_corrections(
            sub_tree, db_serial, tx_fe_corr_path + "/dc_offset/value", "tx_dc", lo_freq);
    } catch (const std::exception& e) {
        UHD_LOGGER_ERROR("CAL") << "Failure in apply_tx_fe_corrections: " << e.what();
    }
}

void uhd::usrp::apply_rx_fe_corrections(property_tree::sptr sub_tree,
    const std::string& db_serial,
    const uhd::fs_path rx_fe_corr_path,
    const double lo_freq)
{
    std::lock_guard<std::mutex> l(corrections_mutex);
    try {
        apply_fe_corrections(
            sub_tree, db_serial, rx_fe_corr_path + "/iq_balance/value", "rx_iq", lo_freq);
    } catch (const std::exception& e) {
        UHD_LOGGER_ERROR("CAL") << "Failure in apply_tx_fe_corrections: " << e.what();
    }
}

/******************************************************************************
 * Gen-2 versions
 *****************************************************************************/
void uhd::usrp::apply_tx_fe_corrections(
    property_tree::sptr sub_tree, // starts at mboards/x
    const std::string& slot, // name of dboard slot
    const double lo_freq // actual lo freq
)
{
    // extract eeprom serial
    const uhd::fs_path db_path = "dboards/" + slot + "/tx_eeprom";
    const std::string db_serial =
        sub_tree->access<uhd::usrp::dboard_eeprom_t>(db_path).get().serial;
    const uhd::fs_path corr_path("tx_frontends/" + slot);

    apply_tx_fe_corrections(sub_tree, db_serial, corr_path, lo_freq);
}

void uhd::usrp::apply_rx_fe_corrections(
    property_tree::sptr sub_tree, // starts at mboards/x
    const std::string& slot, // name of dboard slot
    const double lo_freq // actual lo freq
)
{
    const uhd::fs_path db_path = "dboards/" + slot + "/rx_eeprom";
    const std::string db_serial =
        sub_tree->access<uhd::usrp::dboard_eeprom_t>(db_path).get().serial;
    const uhd::fs_path corr_path("rx_frontends/" + slot);

    apply_rx_fe_corrections(sub_tree, db_serial, corr_path, lo_freq);
}
