// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_BLOCK_ID_HPP
#define INCLUDED_UHD_TYPES_BLOCK_ID_HPP

#include <uhd/config.hpp>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <string>

namespace uhd {
    struct fs_path;

    namespace rfnoc {

    /*!
     * Identifies an RFNoC block.
     *
     * An RFNoC block ID is a string such as: 0/FFT_1
     *
     * The rules for formatting such a string are:
     *
     * DEVICE/BLOCKNAME_COUNTER
     *
     * DEVICE: Identifies the device (usually the motherboard index)
     * BLOCKNAME: A name given to this block
     * COUNTER: If is are more than one block with a BLOCKNAME, this counts up.
     *
     * So, 0/FFT_1 means we're addressing the second block called FFT
     * on the first device.
     *
     * This class can represent these block IDs.
     */
    class UHD_RFNOC_API block_id_t
    {
    public:
        block_id_t();
        block_id_t(const std::string &block_str);
        //! \param device_no Device number
        //! \param block_name Block name
        //! \param block_ctr Which block of this type is this on this device?
        block_id_t(const size_t device_no, const std::string &block_name, const size_t block_ctr=0);

        //! Return a string like this: "0/FFT_1" (includes all components, if set)
        std::string to_string() const;

        //! Check if a given string is valid as a block name.
        //
        // Note: This only applies to the block *name*, not the entire block ID.
        // Examples:
        // * is_valid_blockname("FFT") will return true.
        // * is_valid_blockname("FIR_Filter") will return false, because an underscore
        //   is not allowed in a block name.
        //
        // Internally, this matches the string with uhd::rfnoc::VALID_BLOCKNAME_REGEX.
        static bool is_valid_blockname(const std::string &block_name);

        //! Check if a given string is valid as a block ID.
        //
        // Note: This does necessary require a complete complete ID. If this returns
        // true, then it is a valid input for block_id_t::match().
        //
        // Examples:
        // * is_valid_block_id("FFT") will return true.
        // * is_valid_block_id("0/Filter_1") will return true.
        // * is_valid_block_id("0/Filter_Foo") will return false.
        //
        // Internally, this matches the string with uhd::rfnoc::VALID_BLOCKID_REGEX.
        static bool is_valid_block_id(const std::string &block_id);

        //! Check if block_str matches this block.
        //
        // A match is a less strict version of equality.
        // Less specific block IDs will match more specific ones,
        // e.g. "FFT" will match "0/FFT_1", "1/FFT_2", etc.
        // "FFT_1" will only match the former, etc.
        bool match(const std::string &block_str);

        // Getters

        //! Short for to_string()
        std::string get() const { return to_string(); };

        //! Like get(), but only returns the local part ("FFT_1")
        std::string get_local() const;

        //! Returns the property tree root for this block (e.g. "/mboards/0/xbar/FFT_1/")
        uhd::fs_path get_tree_root() const;

        //! Return device number
        size_t get_device_no() const { return _device_no; };

        //! Return block count
        size_t get_block_count() const { return _block_ctr; };

        //! Return block name
        std::string get_block_name() const { return _block_name; };

        // Setters

        //! Set from string such as "0/FFT_1", "FFT_0", ...
        //  Returns true if successful (i.e. if string valid)
        bool set(const std::string &new_name);

        //! Sets from individual compontents, like calling set_device_no(), set_block_name()
        //  and set_block_count() one after another, only if \p block_name is invalid, stops
        //  and returns false before chaning anything
        bool set(const size_t device_no, const std::string &block_name, const size_t block_ctr=0);

        //! Set the device number
        void set_device_no(size_t device_no) { _device_no = device_no; };

        //! Set the block name. Will return false if invalid block string.
        bool set_block_name(const std::string &block_name);

        //! Set the block count.
        void set_block_count(size_t count) { _block_ctr = count; };

        // Overloaded operators

        //! Assignment: Works like set(std::string)
        block_id_t operator = (const std::string &new_name) {
            set(new_name);
            return *this;
        }

        bool operator == (const block_id_t &block_id) const {
            return (_device_no == block_id.get_device_no())
                and (_block_name == block_id.get_block_name())
                and (_block_ctr == block_id.get_block_count());
        }

        bool operator != (const block_id_t &block_id) const {
            return not (*this == block_id);
        }

        bool operator < (const block_id_t &block_id) const {
            return (
                _device_no < block_id.get_device_no()
                or (_device_no == block_id.get_device_no() and _block_name < block_id.get_block_name())
                or (_device_no == block_id.get_device_no() and _block_name == block_id.get_block_name() and _block_ctr < block_id.get_block_count())
           );
        }

        bool operator > (const block_id_t &block_id) const {
            return (
                _device_no > block_id.get_device_no()
                or (_device_no == block_id.get_device_no() and _block_name > block_id.get_block_name())
                or (_device_no == block_id.get_device_no() and _block_name == block_id.get_block_name() and _block_ctr > block_id.get_block_count())
           );
        }

        //! Check if a string matches the entire block ID (not like match())
        bool operator == (const std::string &block_id_str) const {
            return get() == block_id_str;
        }

        //! Check if a string matches the entire block ID (not like match())
        bool operator == (const char *block_id_str) const {
            std::string comp = std::string(block_id_str);
            return *this == comp;
        }

        //! Type-cast operator does the same as to_string()
        operator std::string() const {
            return to_string();
        }

        //! Increment the block count ("FFT_1" -> "FFT_2")
        block_id_t operator++() {
            _block_ctr++;
            return *this;
        }

        //! Increment the block count ("FFT_1" -> "FFT_2")
        block_id_t operator++(int) {
            _block_ctr++;
            return *this;
        }

    private:
        size_t _device_no;
        std::string _block_name;
        size_t _block_ctr;
    };

    //! Shortcut for << block_id.to_string()
    inline std::ostream& operator<< (std::ostream& out, block_id_t block_id) {
        out << block_id.to_string();
        return out;
    }

}} //namespace uhd::rfnoc

#endif /* INCLUDED_UHD_TYPES_BLOCK_ID_HPP */
// vim: sw=4 et:
