//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/database.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <cmrc/cmrc.hpp>
#include <boost/filesystem.hpp>
#include <ctime>
#include <fstream>

CMRC_DECLARE(rc);

using namespace uhd::usrp::cal;
namespace rc = cmrc::rc;
namespace fs = boost::filesystem;

namespace {
constexpr char LOG_ID[] = "CAL::DATABASE";
constexpr char CAL_EXT[]          = ".cal";
//! This value is just for sanity checking. We pick a value (in bytes) that we
// are guaranteed to never exceed. Its only purpose is to avoid loading files
// that can't possibly be valid cal data based on the filesize. This can avoid
// someone bringing down a UHD session by trying to import a huge file, because
// we first load it entirely into heap space, and then deserialize it from there.
constexpr size_t CALDATA_MAX_SIZE = 10 * 1024 * 1024; // 10 MiB


//! Map a cal resource key into a source::RC path name
std::string get_cal_path_rc(const std::string& key)
{
    return std::string("cal/") + key + CAL_EXT;
}

//! Return true if a cal data resource with given key exists
bool has_cal_data_rc(const std::string& key)
{
    auto fs = rc::get_filesystem();
    return fs.is_file(get_cal_path_rc(key));
}

//! Return a byte array for a given cal resource
std::vector<uint8_t> get_cal_data_rc(const std::string& key)
{
    try {
        auto fs   = rc::get_filesystem();
        auto file = fs.open(get_cal_path_rc(key));
        return std::vector<uint8_t>(file.cbegin(), file.cend());
    } catch (const std::system_error&) {
        throw uhd::key_error(std::string("Unable to open resource with key: ") + key);
    }
}

void check_or_create_dir(fs::path dir)
{
    if (fs::exists(dir)) {
        if (fs::is_directory(dir)) {
            return;
        }
        UHD_LOG_ERROR(LOG_ID, "Path exists, but is not a directory: " << dir);
        throw uhd::runtime_error("Path exists, but is not a directory!");
    }

    if (!fs::create_directory(dir)) {
        UHD_LOG_ERROR(LOG_ID, "Cannot create cal data directory: " << dir);
        throw uhd::runtime_error("Cannot create cal data directory!");
    }
    UHD_LOG_DEBUG(LOG_ID, "Created directory: " << dir);
}

//! Make sure the calibration storage directory exists.
//
// The path returned by uhd::get_cal_data_path() might not exist (e.g., when run
// for the first time). This directory must be created before we try writing to
// it, or we won't be able to open the file.
//
// C++ doesn't have a mkdir -p equivalent, so we check the parent directory and
// the directory itself, in that order. Most of the time, the cal data path is
// in $XDG_DATA_HOME/uhd/cal_data. We assume that $XDG_DATA_HOME exists, and
// then first check $XDG_DATA_HOME/uhd, then $XDG_DATA_HOME/uhd/cal_data.
//
// This will not work if the user sets $UHD_CAL_DATA_PATH to an arbitrary path
// that requires multiple levels of directories to be created, but they will get
// a clear error message in that case.
void assert_cal_dir_exists()
{
    const auto cal_path = fs::path(uhd::get_cal_data_path());
    if (!cal_path.parent_path().empty()) {
        check_or_create_dir(cal_path.parent_path());
    }
    check_or_create_dir(cal_path);
}


//! Map a cal resource key into a filesystem path name (relative to get_cal_data_path())
std::string get_cal_path_fs(const std::string& key, const std::string& serial)
{
    return key + "_" + serial + CAL_EXT;
}

//! Return true if a cal data resource with given key exists
bool has_cal_data_fs(const std::string& key, const std::string& serial)
{
    auto const cal_file_path =
        fs::path(uhd::get_cal_data_path()) / get_cal_path_fs(key, serial);
    UHD_LOG_TRACE(LOG_ID, "Checking for file at " << cal_file_path.string());
    // We might want to check readability also
    return fs::exists(cal_file_path) && fs::is_regular_file(cal_file_path);
}

//! Return a byte array for a given filesystem resource
std::vector<uint8_t> get_cal_data_fs(const std::string& key, const std::string& serial)
{
    if (!has_cal_data_fs(key, serial)) {
        throw uhd::key_error(
            std::string("Cannot find cal file for key=") + key + ", serial=" + serial);
    }
    const auto cal_file_path =
        fs::path(uhd::get_cal_data_path()) / get_cal_path_fs(key, serial);
    // We read the filesize first to do a sanity check (is this file small
    // enough to reasonably be cal data?) and also to pre-allocate heap space in
    // which we'll load the full data for future deserialization.
    const size_t filesize = fs::file_size(cal_file_path);
    if (filesize > CALDATA_MAX_SIZE) {
        throw uhd::key_error(
            std::string("The following cal data file exceeds maximum size limitations: ")
            + cal_file_path.string());
    }
    std::vector<uint8_t> result(filesize, 0);
    std::ifstream file(cal_file_path.string(), std::ios::binary);
    UHD_LOG_TRACE(LOG_ID, "Reading " << filesize << " bytes from " << cal_file_path);
    file.read(reinterpret_cast<char*>(&result[0]), filesize);
    return result;
}

} // namespace

std::vector<uint8_t> database::read_cal_data(
    const std::string& key, const std::string& serial, const source source_type)
{
    if (source_type == source::FILESYSTEM || source_type == source::ANY) {
        if (has_cal_data_fs(key, serial)) {
            return get_cal_data_fs(key, serial);
        }
    }

    if (source_type == source::RC || source_type == source::ANY) {
        if (has_cal_data_rc(key)) {
            return get_cal_data_rc(key);
        }
    }

    const std::string err_msg =
        std::string("Calibration Data not found for: key=") + key + ", serial=" + serial;
    UHD_LOG_ERROR(LOG_ID, err_msg);
    throw uhd::key_error(err_msg);
}

bool database::has_cal_data(
    const std::string& key, const std::string& serial, const source source_type)
{
    if (source_type == source::FILESYSTEM || source_type == source::ANY) {
        if (has_cal_data_fs(key, serial)) {
            return true;
        }
    }

    if (source_type == source::RC || source_type == source::ANY) {
        if (has_cal_data_rc(key)) {
            return true;
        }
    }

    return false;
}

void database::write_cal_data(const std::string& key,
    const std::string& serial,
    const std::vector<uint8_t>& cal_data,
    const std::string& backup_ext)
{
    assert_cal_dir_exists();

    const auto cal_file_path =
        (fs::path(uhd::get_cal_data_path()) / get_cal_path_fs(key, serial)).string();

    if (fs::exists(cal_file_path)) {
        const auto ext = backup_ext.empty() ? std::to_string(time(NULL)) : backup_ext;
        const auto cal_file_path_backup = fs::path(uhd::get_cal_data_path())
                                          / (get_cal_path_fs(key, serial) + "." + ext);
        UHD_LOG_WARNING(LOG_ID,
            "Calibration data already exists for key: `"
                << key << "' serial: `" << serial
                << "'. Backing up to: " << cal_file_path_backup);
        fs::rename(fs::path(cal_file_path), cal_file_path_backup);
    }

    std::ofstream file(cal_file_path, std::ios::binary);
    UHD_LOG_DEBUG(LOG_ID, "Writing to " << cal_file_path);
    file.write(reinterpret_cast<const char*>(cal_data.data()), cal_data.size());
}
