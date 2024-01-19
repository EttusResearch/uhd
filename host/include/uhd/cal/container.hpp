//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace usrp { namespace cal {

/*! Generic parent class for calibration data
 *
 * Derive any class that stores cal data which needs to be stored/retrieved from
 * this parent class.
 */
class UHD_API container
{
public:
    virtual ~container() = default;

    //! Return the name of this calibration table
    virtual std::string get_name() const = 0;

    //! Return the device serial of this calibration table
    virtual std::string get_serial() const = 0;

    //! Timestamp of acquisition time
    virtual uint64_t get_timestamp() const = 0;

    //! Return a serialized version of this container
    virtual std::vector<uint8_t> serialize() = 0;

    //! Populate this class from the serialized data
    virtual void deserialize(const std::vector<uint8_t>& data) = 0;

    //! Generic factory for cal data from serialized data
    //
    // \tparam container_type The class type of cal data which should be
    //                        generated from \p data
    // \param data The serialized data to be turned into the cal class
    template <typename container_type>
    static std::shared_ptr<container_type> make(const std::vector<uint8_t>& data)
    {
        auto cal_data = container_type::make();
        cal_data->deserialize(data);
        return cal_data;
    }
};

}}} // namespace uhd::usrp::cal
