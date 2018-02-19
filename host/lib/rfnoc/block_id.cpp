//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <uhd/exception.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhd/rfnoc/block_id.hpp>

#include <iostream>

using namespace uhd::rfnoc;

block_id_t::block_id_t() :
    _device_no(0),
    _block_name(""),
    _block_ctr(0)
{
}

block_id_t::block_id_t(const std::string &block_str)
    : _device_no(0),
    _block_name(""),
    _block_ctr(0)
{
    if (not set(block_str)) {
        throw uhd::value_error("block_id_t: Invalid block ID string.");
    }
}

block_id_t::block_id_t(
        const size_t device_no,
        const std::string &block_name,
        const size_t block_ctr
) : _device_no(device_no),
    _block_name(block_name),
    _block_ctr(block_ctr)
{
    if (not is_valid_blockname(block_name)) {
        throw uhd::value_error("block_id_t: Invalid block name.");
    }
}

bool block_id_t::is_valid_blockname(const std::string &block_name)
{
    return boost::regex_match(block_name, boost::regex(VALID_BLOCKNAME_REGEX));
}

bool block_id_t::is_valid_block_id(const std::string &block_name)
{
    return boost::regex_match(block_name, boost::regex(VALID_BLOCKID_REGEX));
}

std::string block_id_t::to_string() const
{
    return str(boost::format("%d/%s")
        % get_device_no()
        % get_local()
    );
}

std::string block_id_t::get_local() const
{
    return str(boost::format("%s_%d")
        % get_block_name()
        % get_block_count()
    );
}

uhd::fs_path block_id_t::get_tree_root() const
{
    return str(boost::format("/mboards/%d/xbar/%s")
        % get_device_no()
        % get_local()
    );
}

bool block_id_t::match(const std::string &block_str)
{
    boost::cmatch matches;
    if (not boost::regex_match(block_str.c_str(), matches, boost::regex(VALID_BLOCKID_REGEX))) {
        return false;
    }
    try {
        return  (matches[1] == "" or boost::lexical_cast<size_t>(matches[1]) == _device_no)
            and (matches[2] == "" or matches[2] == _block_name)
            and (matches[3] == "" or boost::lexical_cast<size_t>(matches[3]) == _block_ctr)
            and not (matches[1] == "" and matches[2] == "" and matches[3] == "");
    } catch (const std::bad_cast &e) {
        return false;
    }
    return false;
}

bool block_id_t::set(const std::string &new_name)
{
    boost::cmatch matches;
    if (not boost::regex_match(new_name.c_str(), matches, boost::regex(VALID_BLOCKID_REGEX))) {
        return false;
    }
    if (not (matches[1] == "")) {
        _device_no = boost::lexical_cast<size_t>(matches[1]);
    }
    if (not (matches[2] == "")) {
        _block_name = matches[2];
    }
    if (not (matches[3] == "")) {
        _block_ctr = boost::lexical_cast<size_t>(matches[3]);
    }
    return true;
}

bool block_id_t::set(
        const size_t device_no,
        const std::string &block_name,
        const size_t block_ctr
) {
    if (not set_block_name(block_name)) {
        return false;
    }
    set_device_no(device_no);
    set_block_count(block_ctr);
    return true;
}

bool block_id_t::set_block_name(const std::string &block_name)
{
    if (not is_valid_blockname(block_name)) {
        return false;
    }
    _block_name = block_name;
    return true;
}

