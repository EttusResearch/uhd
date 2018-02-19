//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_IHEX_READER_HPP
#define INCLUDED_IHEX_READER_HPP

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <stdint.h>
#include <string>
#include <vector>

namespace uhd {

class ihex_reader
{
public:
    // Arguments are: lower address bits, upper address bits, buff, length
    typedef boost::function<int(uint16_t,uint16_t,unsigned char*,uint16_t)> record_handle_type;

    /*
     * \param ihex_filename Path to the *.ihx file
     */
    ihex_reader(const std::string &ihex_filename);

    /*! Read an Intel HEX file and handle it record by record.
     *
     * Every record is individually passed off to a record handler function.
     *
     * \param record_handler The functor that will handle the records.
     *
     * \throws uhd::io_error if the HEX file is corrupted or unreadable.
     */
    void read(record_handle_type record_handler);

    /* Convert the ihex file to a bin file.
     *
     * *Note:* This function makes the assumption that the hex file is
     * contiguous, and starts at address zero.
     *
     * \param bin_filename Output filename.
     *
     * \throws uhd::io_error if the HEX file is corrupted or unreadable.
     */
    void to_bin_file(const std::string &bin_filename);

    /*! Copy the ihex file into a buffer.
     *
     * Very similar functionality as to_bin_file().
     *
     * *Note:* This function makes the assumption that the hex file is
     * contiguous, and starts at address zero.
     *
     * \throws uhd::io_error if the HEX file is corrupted or unreadable.
     */
    std::vector<uint8_t> to_vector(const size_t size_estimate = 0);

private:
    const std::string _ihex_filename;
};

}; /* namespace uhd */

#endif /* INCLUDED_IHEX_READER_HPP */

