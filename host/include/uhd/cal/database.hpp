//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <stddef.h>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace cal {

//! Identify the source of calibration data, i.e., where was it stored
//
// This enum lists the sources in reverse order of priority, i.e., user-provided
// data has the highest priority, and hard-coded data from the resource compiler
// has the lowest priority.
enum class source {
    NONE, //!< No calibration data available
    ANY, //!< Undefined source
    RC, //!< Internal Resource Compiler (i.e., hard-coded within UHD)
    FLASH, //!< Stored on device flash memory, e.g. EEPROM
    FILESYSTEM, //!< Stored on the local filesystem
    USER //!< Provided by the user
};

/*! Calibration Data Storage/Retrieval Class
 *
 * UHD can store calibration data on disk or compiled within UHD. This class
 * provides access to both locations.
 *
 * \section cal_db_blob Format of binary data
 *
 * This class can read and write binary data, but it does not verify the data
 * or expect any kind of format. It simply manages BLOBs (binary large objects).
 * It is up to the consumers and producers of this data to agree on a format.
 * Typically, since this class stores calibration data, it will be consuming
 * data that was produced by uhd::usrp::cal::container::serialize().
 *
 * \section cal_db_serial Serial number and key
 *
 * Calibration data is indexed by two keys: An arbitrary key that describes the
 * type of calibration data (e.g., "rx_iq") and a serial number. The serial
 * number has to uniquely identify the device for which the calibration data was
 * obtained. This can either be the serial number of the daughterboard (if the
 * calibration data only relates to the daughterboard), the motherboard (for
 * example, if there is no such thing as a daughterboard, or the data only
 * relates to the motherboard), it can be combination of both daughterboard and
 * motherboard serial (if the calibration data is only valid for a combination),
 * or it can be a combination of a device serial number and a channel index
 * (if a device with single serial has different channels that have separate
 * characteristics).
 *
 * It is up to the individual device drivers which value they use for the serial
 * numbers and keys.
 *
 * Note that the serial number is irrelevant when the data is pulled out of the
 * resource compiler. By definition, it is not permitted to store data in the
 * resource compiler that is specific to a certain serial number, only data that
 * applies to an entire family of devices is permitted.
 */
class UHD_API database
{
public:
    //! Return a calibration data set as a serialized string
    //
    // Note: the \p source_type parameter can be used to specify where to read
    //                          cal data from. However, this class only has
    //                          access to RC and FILESYSTEM type cal data. ANY
    //                          will pick FILESYSTEM data if both are available,
    //                          and RC data if only RC data is available.
    // \param key The calibration type key (e.g., "rx_iq")
    // \param serial The serial number of the device this data is for. See also
    //               \ref cal_db_serial
    // \param source_type Where to read the calibration data from. See comments
    //                    above. For anything other than RC, FILESYSTEM, or ANY,
    //                    this will always throw a uhd::key_error because this
    //                    class does not have access to user data or EEPROM data.
    //
    // \throws uhd::key_error if no calibration data is found matching the source
    //                        type.
    static std::vector<uint8_t> read_cal_data(const std::string& key,
        const std::string& serial,
        const source source_type = source::ANY);

    //! Check if calibration data exists for a given source type
    //
    // This can be called before calling read_cal_data() to avoid having to
    // catch an exception. If \p source_type is FILESYSTEM, then it will only
    // return true if a file is found with the appropriate cal data. The same
    // is true for RC. If \p is ANY, then having either RC or FILESYSTEM data
    // will yield true.
    //
    // \param key The calibration type key (e.g., "rx_iq")
    // \param serial The serial number of the device this data is for. See also
    //               \ref cal_db_serial
    // \param source_type Where to read the calibration data from. For anything
    //                    other than RC, FILESYSTEM, or ANY, this will always
    //                    return false because this class does not have access
    //                    to user data or EEPROM data.
    // \return true if calibration data is available that matches this key/serial
    //              pair.
    static bool has_cal_data(const std::string& key,
        const std::string& serial,
        const source source_type = source::ANY);

    //! Store calibration data to the local filesystem database
    //
    // This implies a source type of FILESYSTEM. Note that writing the data does
    // not apply it to a currently running UHD session. Devices will typically
    // load calibration data at initialization time, and thus this call will
    // take effect only for future UHD sessions.
    //
    // If calibration data for this key/serial pair already exists in the
    // database, the original data will be backed up by renaming the original
    // file from `filename.cal` to `filename.cal.$TIMESTAMP`. Alternatively, a
    // custom extension can be chosen instead of `$TIMESTAMP`.
    //
    // \param key The calibration type key (e.g., "rx_iq")
    // \param serial The serial number of the device this data is for. See also
    //               \ref cal_db_serial
    // \param cal_data The calibration data to be written
    // \param backup_ext A custom extension for backing up calibration data. If
    //                   left empty, a POSIX timestamp is used.
    static void write_cal_data(const std::string& key,
        const std::string& serial,
        const std::vector<uint8_t>& cal_data,
        const std::string& backup_ext = "");

    //! Function type to look up if there is cal data given a key and serial
    using has_data_fn_type = std::function<bool(const std::string&, const std::string&)>;

    //! Function type to return serialized cal data key and serial
    //
    // These functions should throw a uhd::runtime_error if called with invalid
    // key/serial pairs, although database will internally always call the
    // corresponding 'has' function before calling this.
    using get_data_fn_type =
        std::function<std::vector<uint8_t>(const std::string&, const std::string&)>;

    //! Register a lookup function for cal data
    //
    // \param has_cal_data A function object to a function that returns true if
    //        cal data is available
    // \param get_cal_data A function object to a function that returns serialized
    //        cal data
    // \param source_type Reserved. Must be source::FLASH.
    static void register_lookup(has_data_fn_type has_cal_data,
        get_data_fn_type get_cal_data,
        const source source_type = source::FLASH);
};


}}} // namespace uhd::usrp::cal
