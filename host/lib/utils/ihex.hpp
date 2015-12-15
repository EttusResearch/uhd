//
// Copyright 2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_IHEX_READER_HPP
#define INCLUDED_IHEX_READER_HPP

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <vector>

namespace uhd {

class ihex_reader
{
public:
    // Arguments are: lower address bits, upper address bits, buff, length
    typedef boost::function<int(boost::uint16_t,boost::uint16_t,unsigned char*,boost::uint16_t)> record_handle_type;

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
    std::vector<boost::uint8_t> to_vector(const size_t size_estimate = 0);

private:
    const std::string _ihex_filename;
};

}; /* namespace uhd */

#endif /* INCLUDED_IHEX_READER_HPP */

