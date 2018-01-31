//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhdlib/utils/ihex.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <sstream>
#include <fstream>

using namespace uhd;

/*!
 * Verify checksum of a Intel HEX record
 * \param record a line from an Intel HEX file
 * \return true if record is valid, false otherwise
 */
static bool checksum(const std::string& record)
{
    size_t len = record.length();
    unsigned char sum = 0;
    unsigned int val;

    for (size_t i = 1; i < len; i += 2) {
        std::istringstream(record.substr(i, 2)) >> std::hex >> val;
        sum += val;
    }

    if (sum == 0)
       return true;
    else
       return false;
}


/*!
 * Parse Intel HEX record
 *
 * \param record a line from an Intel HEX file
 * \param len output length of record
 * \param addr output address
 * \param type output type
 * \param data output data
 * \return true if record is sucessfully read, false on error
 */
static bool parse_record(
        const std::string& record,
        uint16_t &len,
        uint16_t &addr,
        uint16_t &type,
        unsigned char* data
) {
    unsigned int i;
    unsigned int val;

    if (record.substr(0, 1) != ":")
        return false;

    std::istringstream(record.substr(1, 2)) >> std::hex >> len;
    std::istringstream(record.substr(3, 4)) >> std::hex >> addr;
    std::istringstream(record.substr(7, 2)) >> std::hex >> type;

    if (len > (2 * (record.length() - 9)))  // sanity check to prevent buffer overrun
        return false;

    for (i = 0; i < len; i++) {
        std::istringstream(record.substr(9 + 2 * i, 2)) >> std::hex >> val;
        data[i] = (unsigned char) val;
    }

    return true;
}


ihex_reader::ihex_reader(const std::string &ihex_filename)
    : _ihex_filename(ihex_filename)
{
    // nop
}


void ihex_reader::read(ihex_reader::record_handle_type record_handler)
{
    const char *filename = _ihex_filename.c_str();

    /* Fields used in every record. */
    uint16_t len = 0;
    uint16_t type = 0;
    uint16_t lower_address_bits = 0x0000;
    static const int MAX_RECORD_LENGTH = 255;
    unsigned char data[MAX_RECORD_LENGTH];

    /* Can be set by the Intel HEX record 0x04, used for all 0x00 records
     * thereafter. Note this field takes the place of the 'index' parameter in
     * libusb calls, and is necessary for FX3's 32-bit addressing. */
    uint16_t upper_address_bits = 0x0000;

    std::ifstream file;
    file.open(filename, std::ifstream::in);

    if(!file.good()) {
        throw uhd::io_error("ihex_reader::read(): cannot open firmware input file");
    }

    while (!file.eof()) {
        int32_t ret = 0;
        std::string record;
        file >> record;

        if (!(record.length() > 0))
            continue;

        /* Check for valid Intel HEX record. */
        if (!checksum(record)
            || !parse_record(record, len, lower_address_bits, type, data)) {
            throw uhd::io_error("ihex_reader::read(): bad intel hex record checksum");
        }

        /* Type 0x00: Data. */
        if (type == 0x00) {
            ret = record_handler(lower_address_bits, upper_address_bits, data, len);

            if (ret < 0) {
                throw uhd::io_error("ihex_reader::read(): record hander returned failure code");
            }
        }

        /* Type 0x01: EOF. */
        else if (type == 0x01) {
            if (lower_address_bits != 0x0000 || len != 0 ) {
                throw uhd::io_error("ihex_reader::read(): For EOF record, address must be 0, length must be 0.");
            }

            /* Successful termination! */
            file.close();
            return;
        }

        /* Type 0x04: Extended Linear Address Record. */
        else if (type == 0x04) {
            if (lower_address_bits != 0x0000 || len != 2 ) {
                throw uhd::io_error("ihex_reader::read(): For ELA record, address must be 0, length must be 2.");
            }

            upper_address_bits = ((uint16_t)((data[0] & 0x00FF) << 8))\
                                 + ((uint16_t)(data[1] & 0x00FF));
        }

        /* Type 0x05: Start Linear Address Record. */
        else if (type == 0x05) {
            if (lower_address_bits != 0x0000 || len != 4 ) {
                throw uhd::io_error("ihex_reader::read(): For SLA record, address must be 0, length must be 4.");
            }

            /* The firmware load is complete.  We now need to tell the CPU
             * to jump to an execution address start point, now contained within
             * the data field.  Parse these address bits out, and then push the
             * instruction. */
            upper_address_bits = ((uint16_t)((data[0] & 0x00FF) << 8))\
                                 + ((uint16_t)(data[1] & 0x00FF));
            lower_address_bits = ((uint16_t)((data[2] & 0x00FF) << 8))\
                                 + ((uint16_t)(data[3] & 0x00FF));

            record_handler(lower_address_bits, upper_address_bits, 0, 0);
        }

        /* If we receive an unknown record type, error out. */
        else {
            throw uhd::io_error(str(boost::format("ihex_reader::read(): unsupported record type: %X.") % type));
        }
    }

    /* There was no valid EOF. */
    throw uhd::io_error("ihex_reader::read(): No EOF record found.");
}

// We need a functor for the cast, a lambda would be perfect...
int _file_writer_callback(
    boost::shared_ptr<std::ofstream> output_file,
    unsigned char *buff,
    uint16_t len
) {
    output_file->write((const char *) buff, len);
    return 0;
}

void ihex_reader::to_bin_file(const std::string &bin_filename)
{
    boost::shared_ptr<std::ofstream> output_file(boost::make_shared<std::ofstream>());
    output_file->open(bin_filename.c_str(), std::ios::out | std::ios::binary);
    if (not output_file->is_open()) {
        throw uhd::io_error(str(boost::format("Could not open file for writing: %s") % bin_filename));
    }

    this->read(boost::bind(&_file_writer_callback, output_file, _3, _4));

    output_file->close();
}

// We need a functor for the cast, a lambda would be perfect...
int _vector_writer_callback(
    std::vector<uint8_t>& vector,
    unsigned char *buff,
    uint16_t len
) {
    for (size_t i = 0; i < len; i++) {
        vector.push_back(buff[i]);
    }
    return 0;
}

#define DEFAULT_SIZE_ESTIMATE 8000000
std::vector<uint8_t> ihex_reader::to_vector(const size_t size_estimate)
{
    std::vector<uint8_t> buf;
    buf.reserve(size_estimate == 0 ? DEFAULT_SIZE_ESTIMATE : size_estimate);

    this->read(boost::bind(&_vector_writer_callback, boost::ref(buf), _3, _4));

    return buf;
}

