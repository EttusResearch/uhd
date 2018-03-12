//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <mpm/types/log_buf.hpp>
#include <boost/noncopyable.hpp>
#include <string>
#include <cstdint>

namespace mpm { namespace types {

    class mmap_regs_iface : public boost::noncopyable
    {
    public:
        mmap_regs_iface(
            const std::string &path,
            const size_t length,
            const size_t offset,
            const bool read_only = true,
            const bool open_now = true
        );

        //! Will call close()
        ~mmap_regs_iface();

        //! Open the file descriptor and mmap. Safe to call multiple times.
        void open();

        //! Close the file descriptor and mmap. Safe to call multiple times.
        void close();

        //! Write \p data to \p addr
        void poke32(const uint32_t addr, const uint32_t data);

        //! Read data from \p addr
        uint32_t peek32(const uint32_t addr);

    private:
        void log(
                mpm::types::log_level_t level,
                const std::string path,
                const char *comment
        );

        const std::string _path;
        const size_t _length;
        const size_t _offset;
        const bool _read_only;
        int _fd = -1;
        uint32_t *_mmap = NULL;

    };

}} /* namespace mpm::types */
