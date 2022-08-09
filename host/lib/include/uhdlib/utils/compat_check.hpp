//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <cstddef>
#include <string>

namespace uhd {

//! Compat number representation class
template <typename major_type, typename minor_type>
class compat_num
{
public:
    constexpr compat_num(major_type major, minor_type minor) : _major(major), _minor(minor) {}

    major_type get_major() const { return _major; }
    major_type get_minor() const { return _minor; }

    bool operator==(const compat_num<major_type, minor_type>& rhs) const
    {
        return _major == rhs.get_major() && _minor == rhs.get_minor();
    }

    bool operator!=(const compat_num<major_type, minor_type>& rhs) const
    {
        return !(*this == rhs);
    }

    bool operator<(const compat_num<major_type, minor_type>& rhs) const
    {
        return _major < rhs.get_major()
               || (_major == rhs.get_major() && _minor < rhs.get_minor());
    }

    bool operator<=(const compat_num<major_type, minor_type>& rhs) const
    {
        return *this == rhs || *this < rhs;
    }

    bool operator>(const compat_num<major_type, minor_type>& rhs) const
    {
        return _major > rhs.get_major()
               || (_major == rhs.get_major() && _minor > rhs.get_minor());
    }

    bool operator>=(const compat_num<major_type, minor_type>& rhs) const
    {
        return *this == rhs || *this > rhs;
    }

    std::string to_string() const
    {
        return std::to_string(_major) + ":" + std::to_string(_minor);
    }

protected:
    major_type _major;
    minor_type _minor;
};

//! Specialization of the compat_num class for 16-bit compat numbers
class compat_num16 : public compat_num<uint8_t, uint8_t>
{
public:
    compat_num16(const uint16_t compat_val)
        : compat_num<uint8_t, uint8_t>((compat_val >> 8) & 0xFF, compat_val & 0xFF)
    {
    }

    compat_num16(const uint8_t major, const uint8_t minor)
        : compat_num<uint8_t, uint8_t>(major, minor)
    {
    }

    uint16_t get() const
    {
        return static_cast<uint16_t>(_major) << 8 | _minor;
    };
};

//! Specialization of the compat_num class for 32-bit compat numbers
class compat_num32 : public compat_num<uint16_t, uint16_t>
{
public:
    constexpr compat_num32(const uint32_t compat_val)
        : compat_num<uint16_t, uint16_t>((compat_val >> 16) & 0xFFFF, compat_val & 0xFFFF)
    {
    }

    constexpr compat_num32(const uint16_t major, const uint16_t minor)
        : compat_num<uint16_t, uint16_t>(major, minor)
    {
    }

    uint32_t get() const
    {
        return static_cast<uint32_t>(_major) << 16 | _minor;
    };
};

/*! Checks for FPGA compatibility, and throws an exception on mismatch.
 *
 * \throws uhd::runtime_error on mismatch.
 */
void assert_fpga_compat(const size_t uhd_major,
    const size_t uhd_minor,
    const uint64_t fpga_compat,
    const std::string& fpga_component,
    const std::string& log_component,
    const bool fail_on_minor_behind = false);

/*! Checks for FPGA compatibility, and throws an exception on mismatch.
 *
 * \throws uhd::runtime_error on mismatch.
 */
void assert_fpga_compat(const size_t uhd_major,
    const size_t uhd_minor,
    const uint32_t fpga_compat,
    const std::string& fpga_component,
    const std::string& log_component,
    const bool fail_on_minor_behind = false);

} /* namespace uhd */
