//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <mpm/types/mmap_regs_iface.hpp>
#include <mpm/exception.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>

using namespace mpm::types;


mmap_regs_iface::mmap_regs_iface(
    const std::string &path,
    const size_t length,
    const size_t offset,
    const bool read_only,
    const bool open_now
) : _path(path)
  , _length(length)
  , _offset(offset)
  , _read_only(read_only)
{
    if (open_now) {
        open();
    }
}

mmap_regs_iface::~mmap_regs_iface()
{
    try {
        close();
    } catch (...) {
        // This is not good
    }
}

void mmap_regs_iface::open()
{
    if (_fd < 0) {
        _fd = ::open(_path.c_str(), _read_only ? O_RDONLY : O_RDWR);
        if (_fd < 0) {
            throw mpm::runtime_error("Could not open file descriptor!");
        }
    }

    if (!_mmap) {
        _mmap = (uint32_t *) ::mmap(
            NULL,
            _length,
            PROT_READ | (_read_only ? 0 : PROT_WRITE),
            MAP_SHARED,
            _fd,
            (off_t) _offset
        );
        if (((void *) _mmap) == MAP_FAILED) {
            throw mpm::runtime_error("Failed to mmap!");
        }
    }
    log(mpm::types::log_level_t::TRACE, _path,
            "Opened mmap_regs_iface");
}

void mmap_regs_iface::close()
{
    if (_mmap) {
        int err = munmap((void *) _mmap, _length);
        if (err) {
            throw mpm::runtime_error("Couldn't munmap!");
        }
        _mmap = NULL;
    }

    if (_fd >= 0) {
        if (::close(_fd)) {
            throw mpm::runtime_error("Couldn't close file descriptor!");
        }
        _fd = -1;
    }
    log(mpm::types::log_level_t::TRACE, _path,
            "Closed mmap_regs_iface");
}

void mmap_regs_iface::poke32(
    const uint32_t addr,
    const uint32_t data
) {
    MPM_ASSERT_THROW(_mmap);
    _mmap[addr / sizeof(uint32_t)] = data;
}

uint32_t mmap_regs_iface::peek32(const uint32_t addr)
{
    MPM_ASSERT_THROW(_mmap);
    return _mmap[addr / sizeof(uint32_t)];
}

void mmap_regs_iface::log(
        mpm::types::log_level_t level,
        const std::string path,
        const char *comment
) {
    mpm::types::log_buf::make_singleton()->post(
            level,
            "MMAP_REGS_IFACE",
            str(boost::format("[UIO %s] %s")
                % path % comment)
    );
}
