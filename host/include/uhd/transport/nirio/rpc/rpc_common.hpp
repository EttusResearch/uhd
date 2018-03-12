//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_RPC_COMMON_HPP
#define INCLUDED_RPC_COMMON_HPP

#define USE_BINARY_ARCHIVE 0

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#if (USE_BINARY_ARCHIVE)
    #include <boost/archive/binary_oarchive.hpp>
    #include <boost/archive/binary_iarchive.hpp>
#else
    #include <boost/archive/text_oarchive.hpp>
    #include <boost/archive/text_iarchive.hpp>
#endif
#include <stdint.h>

namespace uhd { namespace usrprio_rpc {

//[Over-the-wire] IDs
typedef int32_t  func_id_t;
typedef uint64_t client_id_t;

#define build_client_id(host_id, process_id) \
    ((static_cast<uint64_t>(host_id) << 32) | static_cast<uint64_t>(process_id))
#define get_process_id_from_client_id(client_id) \
    (static_cast<int32_t>(client_id))
#define get_host_id_from_client_id(client_id) \
    (static_cast<uint32_t>(client_id >> 32))

//[Over-the-wire] Handshake format
struct hshake_args_t {
    uint32_t version;
    uint32_t oldest_comp_version;
    int32_t  boost_archive_version;
    client_id_t     client_id;
};

//[Over-the-wire] Header for RPC request and response
class func_args_header_t {
public:
    func_id_t       func_id;
    client_id_t     client_id;
    uint32_t func_args_size;

    static bool match_function(const func_args_header_t& a, const func_args_header_t& b) {
        return ((a.func_id == b.func_id) && (a.client_id == b.client_id));
    }
};

//[Internal] Storage for RPC header and arguments
class func_xport_buf_t {
public:
    func_args_header_t  header;
    std::vector<char>   data;
};

//[Internal] Serializer for RPC input parameters
class func_args_writer_t {
public:
    func_args_writer_t() : _stream(), _archive(_stream, boost::archive::no_header) {}

    template<typename data_t>
    void push(const data_t& d) {
        _archive << d;
    }

    template<typename data_t>
    func_args_writer_t& operator<< (const data_t& data) {
        push(data);
        return *this;
    }

    void store(std::vector<char>& data) const {
        const std::string& str = _stream.str();
        data.resize(str.length());
        data.assign((char*)str.c_str(), ((char*)str.c_str()) + str.length());
    }

private:
    std::ostringstream              _stream;
#if (USE_BINARY_ARCHIVE)
    boost::archive::binary_oarchive _archive;
#else
    boost::archive::text_oarchive   _archive;
#endif
};

//[Internal] Deserializer for RPC output parameters
class func_args_reader_t {
public:
    func_args_reader_t() : _stream(), _archive() {}

    template<typename data_t>
    void pull(data_t& d) const {
        if (_archive) (*_archive) >> d;
    }

    template<typename data_t>
    const func_args_reader_t& operator>> (data_t& data) const {
        pull(data);
        return *this;
    }

    void load(const std::vector<char>& data) {
        _stream.str(std::string(data.begin(), data.end()));
#if (USE_BINARY_ARCHIVE)
        _archive.reset(new boost::archive::binary_iarchive(_stream, boost::archive::no_header));
#else
        _archive.reset(new boost::archive::text_iarchive(_stream, boost::archive::no_header));
#endif
    }

private:
    std::istringstream                                  _stream;
#if (USE_BINARY_ARCHIVE)
    boost::scoped_ptr<boost::archive::binary_iarchive>  _archive;
#else
    boost::scoped_ptr<boost::archive::text_iarchive>    _archive;
#endif
};

class boost_serialization_archive_utils {
public:
    static int32_t get_version() {
    #if (USE_BINARY_ARCHIVE)
        typedef boost::archive::binary_oarchive archive_t;
    #else
        typedef boost::archive::text_oarchive   archive_t;
    #endif
        std::ostringstream stream;
        archive_t dummy_archive(stream, boost::archive::no_header);
        return static_cast<int32_t>(dummy_archive.get_library_version());
    }
};

}}

#undef USE_BINARY_ARCHIVE

#endif /* INCLUDED_RPC_COMMON_HPP */
