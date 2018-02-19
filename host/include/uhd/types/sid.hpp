//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_SID_HPP
#define INCLUDED_UHD_TYPES_SID_HPP

#include <uhd/config.hpp>
#include <iostream>
#include <stdint.h>

namespace uhd {
    /*!
     * \brief Represents a stream ID (SID).
     *
     * A stream ID (SID) is an identifier for data.
     * It is a 32-Bit value which consists of 16 Bits
     * for the source address and 16 Bits for the destination
     * address.
     * Every address is split into two parts: The _address_, which
     * identifies the device used, and the _endpoint_, which identifies
     * a specific object inside the given device (e.g., a block).
     * *Note:* In the case where there are several crossbars on a single
     * device, each crossbar gets its own address.
     * Both address and endpoint are 8 bits in length. If a 16-bit address
     * is required, we use the combination of the 8-bit address and the 8-bit
     * endpoint.
     *
     * <pre>
     * +-------------+--------------+-------------+--------------+
     * | SRC address | SRC endpoint | DST address | DST endpoint |
     * +-------------+--------------+-------------+--------------+
     * </pre>
     *
     * \section sid_str_repr String Representation (pretty printing)
     *
     * The string representation of a SID is of the form
     *
     *     2.3>0.6
     *
     * The '>' symbol shows the direction, so in this case,
     * data is flowing from address 2.3 to 0.6.
     *
     * As a convention, ':' is used instead of '.' when giving the
     * SID in hexadecimal numbers, and two characters are used for each
     * address part. As an example, the following two SIDs are identical:
     *
     *     2.3>0.16 (decimal)
     *     02:03>00:10 (hexadecimal)
     *
     * The format is:
     *     SRC_ADDRESS.SRC_ENDPOINT>DST_ADDRESS.DST_ENDPOINT
     *
     *
     * \section sid_block_ports Block Ports
     *
     * In the special case where a block on a crossbar is addressed, the
     * endpoint is further split up into two parts of four bits each: The
     * first four bits specify the port number on the crossbar, whereas the
     * lower four bits represent the *block port*. As an example, consider
     * the following SID, given in hexadecimal:
     *
     *    00:10>02:A1
     *
     * In this example, assume data is flowing from the host computer to an
     * X300. The crossbar address is 02. The endpoint is A1, which means we
     * are accessing a block on crossbar port A (the tenth port), and are addressing
     * block port 1.
     *
     */
    class UHD_API sid_t
    {
    public:
        //! Create an unset SID
        sid_t();
        //! Create a sid_t object from a 32-Bit SID value
        sid_t(uint32_t sid);
        //! Create a sid_t object from its four components
        sid_t(uint8_t src_addr, uint8_t src_ep, uint8_t dst_addr, uint8_t dst_ep);
        //! Convert a string representation of a SID into its numerical representation
        sid_t(const std::string &);

        //! Return a decimal string representation of the SID.
        std::string to_pp_string() const;
        //! Return a hexadecimal string representation of the SID.
        std::string to_pp_string_hex() const;

        //! Returns true if this actually holds a valid SID
        bool is_set() const { return _set; };

        // Getters
        //
        //! Alias for get_sid()
        inline uint32_t get() const { return get_sid(); };
        //! Returns a 32-Bit representation of the SID if set, or zero otherwise.
        inline uint32_t get_sid() const { return _set ? _sid : 0; };
        //! Return the 16-bit source address of this SID
        inline uint32_t get_src() const {
            return (_sid >> 16) & 0xFFFF;
        }
        //! Return the 16-bit destination address of this SID
        inline uint32_t get_dst() const {
            return _sid & 0xFFFF;
        }
        //! Return 8-bit address of the source
        inline uint32_t get_src_addr() const {
            return (get_src() >> 8) & 0xFF;
        }
        //! Return endpoint of the source
        inline uint32_t get_src_endpoint() const {
            return get_src() & 0xFF;
        }
        //! Return crossbar port of the source
        inline uint32_t get_src_xbarport() const {
            return (get_src_endpoint() >> 4) & 0xF;
        }
        //! Return block port of the source
        inline uint32_t get_src_blockport() const {
            return (get_src_endpoint()) & 0xF;
        }
        //! Return 8-bit address of the destination
        inline uint32_t get_dst_addr() const {
            return (get_dst() >> 8) & 0xFF;
        }
        //! Return endpoint of the destination
        inline uint32_t get_dst_endpoint() const {
            return get_dst() & 0xFF;
        }
        //! Return crossbar port of the source
        inline uint32_t get_dst_xbarport() const {
            return (get_dst_endpoint() >> 4) & 0xF;
        }
        //! Return block port of the source
        inline uint32_t get_dst_blockport() const {
            return (get_dst_endpoint()) & 0xF;
        }

        // Setters

        //! Alias for set_sid()
        void set(uint32_t new_sid) { set_sid(new_sid); };
        //! Convert a string representation of a SID into a numerical one
        // Throws uhd::value_error if the string is not a valid SID
        // representation.
        void set_from_str(const std::string &);
        void set_sid(uint32_t new_sid);
        //! Set the source address of this SID
        //  (the first 16 Bits)
        void set_src(uint32_t new_addr);
        //! Set the destination address of this SID
        //  (the last 16 Bits)
        void set_dst(uint32_t new_addr);
        void set_src_addr(uint32_t new_addr);
        void set_src_endpoint(uint32_t new_addr);
        void set_dst_addr(uint32_t new_addr);
        void set_dst_endpoint(uint32_t new_addr);
        void set_dst_xbarport(uint32_t new_xbarport);
        void set_dst_blockport(uint32_t new_blockport);

        // Manipulators

        //! Swaps dst and src address and returns the new SID.
        sid_t reversed() const;

        //! Swaps dst and src in-place. This modifies the current SID.
        void reverse();

        // Overloaded operators

        sid_t operator = (const uint32_t new_sid) {
            set_sid(new_sid);
            return *this;
        }

        sid_t operator = (sid_t &sid) {
            set_sid(sid.get_sid());
            return *this;
        }

        sid_t operator = (const sid_t &sid) {
            set_sid(sid.get_sid());
            return *this;
        }

        sid_t operator = (const std::string &sid_str) {
            set_from_str(sid_str);
            return *this;
        }

        bool operator == (const sid_t &sid) const {
            return (not _set and not sid.is_set()) or (_sid == sid.get_sid());
        }

        bool operator == (uint32_t sid) const {
            return _set and _sid == sid;
        }

        bool operator == (const std::string &sid_str) const {
            sid_t rhs(sid_str);
            return *this == rhs;
        }

        // overloaded type casts are tricky, but for now we'll need them
        // for backward compatibility. consider them deprecated.

        //! If the SID is not set, always returns zero.
        //  Use is_set() to check if the return value is valid.
        operator uint32_t() const {
            return get();
        }

        operator bool() const {
            return _set;
        }

    private:
        uint32_t _sid;
        bool _set;
    };

    //! Stream output operator. Honors std::ios::hex.
    inline std::ostream& operator<< (std::ostream& out, const sid_t &sid) {
        std::ios_base::fmtflags ff = out.flags();
        if (ff & std::ios::hex) {
            out << sid.to_pp_string_hex();
        } else {
            out << sid.to_pp_string();
        }
        return out;
    }

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_SID_HPP */
// vim: sw=4 et:
