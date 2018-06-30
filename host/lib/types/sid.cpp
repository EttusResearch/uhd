//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/sid.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/cast.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

using namespace uhd;

sid_t::sid_t()
    : _sid(0x0000), _set(false)
{
}

sid_t::sid_t(uint32_t sid)
    : _sid(sid), _set(true)
{
}

sid_t::sid_t(uint8_t src_addr, uint8_t src_ep, uint8_t dst_addr, uint8_t dst_ep)
    :  _sid(0x0000), _set(true)
{
    set_src_addr(src_addr);
    set_src_endpoint(src_ep);
    set_dst_addr(dst_addr);
    set_dst_endpoint(dst_ep);
}

sid_t::sid_t(const std::string &sid_str)
    : _sid(0x0000), _set(false)
{
    set_from_str(sid_str);
}

std::string sid_t::to_pp_string() const
{
    if (not _set) {
        return "x.x>x.x";
    }
    return str(boost::format("%d.%d>%d.%d")
        % get_src_addr()
        % get_src_endpoint()
        % get_dst_addr()
        % get_dst_endpoint()
    );
}

std::string sid_t::to_pp_string_hex() const
{
    if (not _set) {
        return "xx:xx>xx:xx";
    }
    return str(boost::format("%02x:%02x>%02x:%02x")
        % get_src_addr()
        % get_src_endpoint()
        % get_dst_addr()
        % get_dst_endpoint()
    );
}


void sid_t::set_sid(uint32_t new_sid)
{
    _set = true;
    _sid = new_sid;
}

void sid_t::set_from_str(const std::string &sid_str)
{
    const std::string dec_regex = "(\\d{1,3})\\.(\\d{1,3})[.:/><](\\d{1,3})\\.(\\d{1,3})";
    const std::string hex_regex = "([[:xdigit:]]{2}):([[:xdigit:]]{2})[.:/><]([[:xdigit:]]{2}):([[:xdigit:]]{2})";

    boost::cmatch matches;
    if (boost::regex_match(sid_str.c_str(), matches, boost::regex(dec_regex))) {
        set_src_addr(boost::lexical_cast<size_t>(matches[1]));
        set_src_endpoint(boost::lexical_cast<size_t>(matches[2]));
        set_dst_addr(boost::lexical_cast<size_t>(matches[3]));
        set_dst_endpoint(boost::lexical_cast<size_t>(matches[4]));
        return;
    }

    if (boost::regex_match(sid_str.c_str(), matches, boost::regex(hex_regex))) {
        set_src_addr(uhd::cast::hexstr_cast<size_t>(matches[1]));
        set_src_endpoint(uhd::cast::hexstr_cast<size_t>(matches[2]));
        set_dst_addr(uhd::cast::hexstr_cast<size_t>(matches[3]));
        set_dst_endpoint(uhd::cast::hexstr_cast<size_t>(matches[4]));
        return;
    }

    throw uhd::value_error(str(boost::format("Invalid SID representation: %s") % sid_str));
}

void sid_t::set_src(uint32_t new_addr) {
    set_sid((_sid & 0x0000FFFF) | ((new_addr & 0xFFFF) << 16));
}

void sid_t::set_dst(uint32_t new_addr) {
    set_sid((_sid & 0xFFFF0000) | (new_addr & 0xFFFF));
}

void sid_t::set_src_addr(uint32_t new_addr) {
    set_sid((_sid & 0x00FFFFFF) | ((new_addr & 0xFF) << 24));
}

void sid_t::set_src_endpoint(uint32_t new_addr) {
    set_sid((_sid & 0xFF00FFFF) | ((new_addr & 0xFF) << 16));
}

void sid_t::set_dst_addr(uint32_t new_addr) {
    set_sid((_sid & 0xFFFF00FF) | ((new_addr & 0xFF) << 8));
}

void sid_t::set_dst_endpoint(uint32_t new_addr) {
    set_sid((_sid & 0xFFFFFF00) | ((new_addr & 0xFF) << 0));
}

void sid_t::set_dst_xbarport(uint32_t new_xbarport)
{
    set_sid((_sid & 0xFFFFFF0F) | ((new_xbarport & 0xF) << 4));
}

void sid_t::set_dst_blockport(uint32_t new_blockport)
{
    set_sid((_sid & 0xFFFFFFF0) | ((new_blockport & 0xF) << 0));
}

sid_t sid_t::reversed() const
{
    return sid_t((get_dst() << 16) | get_src());
}

void sid_t::reverse()
{
    set_sid((get_dst() << 16) | get_src());
}

