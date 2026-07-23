/*
 * Copyright (c) 2013 National Instruments
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "SysfsFile.h"
#include "Timer.h"
#include <cstdio> // sscanf
#define __STDC_FORMAT_MACROS // PRIu32
#include "Exception.h"
#include <sched.h> // sched_yield
#include <sys/stat.h>
#include <cinttypes> // PRIu32
#include <fstream> // std::ifstream
#include <limits> // std::numeric_limits

namespace nirio {

namespace {

const std::string baseSysfsPath("/sys/class/nib310rio");

std::string getSubdevicePath(const std::string& device, const std::string& subdevice)
{
    return joinPath(baseSysfsPath, (device + subdevice));
}
} // unnamed namespace

SysfsFile::SysfsFile(
    const std::string& device, const std::string& attribute, const ErrnoMap& errnoMap)
    : path(joinPath(baseSysfsPath, device, attribute)), errnoMap(errnoMap)
{
}

SysfsFile::SysfsFile(const std::string& device,
    const std::string& subdevice,
    const std::string& attribute,
    const ErrnoMap& errnoMap)
    : path(joinPath(getSubdevicePath(device, subdevice), attribute)), errnoMap(errnoMap)
{
}

SysfsFile::SysfsFile(const std::string& path, const ErrnoMap& errnoMap)
    : path(path), errnoMap(errnoMap)
{
}

const std::string& SysfsFile::getPath() const
{
    return path;
}

bool SysfsFile::readBool() const
{
    const DeviceFile file(path, DeviceFile::ReadOnly, errnoMap);
    // Start with invalid value to ensure value is read.
    char buffer = '2';
    file.read(&buffer, 1);
    if (buffer != '0' && buffer != '1')
        NIRIO_THROW(SoftwareFaultException());
    return buffer == '1';
}

uint32_t SysfsFile::readU32() const
{
    const DeviceFile file(path, DeviceFile::ReadOnly, errnoMap);
    // enough room for longest possible uint32_t plus a trailing '\0'
    assert(std::numeric_limits<uint32_t>::max() == 4294967295U);
    char buffer[sizeof("4294967295") + 1] = {};
    file.read(buffer, sizeof(buffer));
    uint32_t result;
    if (sscanf(buffer, "%" PRIu32, &result) != 1)
        NIRIO_THROW(SoftwareFaultException());
    return result;
}

uint32_t SysfsFile::readU32Hex() const
{
    const DeviceFile file(path, DeviceFile::ReadOnly, errnoMap);
    return nirio::readU32Hex(file);
}

std::string SysfsFile::readLineNoErrno() const
{
    // easy way to get arbitrarily long strings from a file, but doesn't allow
    // for arbitrary errnos to be returned
    //
    std::ifstream file(path.c_str());
    if (!file) {
        NIRIO_THROW(ResourceNotFoundException());
        return std::string();
    }
    std::string line;
    std::getline(file, line);
    if (file.fail() || file.bad())
        NIRIO_THROW(SoftwareFaultException());
    return line;
}

void SysfsFile::write(const std::string& value) const
{
    const DeviceFile file(path, DeviceFile::WriteOnly, errnoMap);
    file.write(value.c_str(), value.size());
}

bool SysfsFile::exists() const
{
    struct stat s;
    return !::stat(path.c_str(), &s);
}

bool SysfsFile::waitUntilExistence(const bool exists, const size_t milliseconds) const
{
    // start the clock
    const Timer timer(milliseconds);
    struct stat s;
    // try until we timeout
    do {
        // return if the path existence is what we wanted
        if (exists == !::stat(path.c_str(), &s))
            return true;
        // NOTE: "In the Linux implementation, sched_yield() always succeeds":
        //    http://man7.org/linux/man-pages/man2/sched_yield.2.html
        sched_yield();
    } while (!timer.isTimedOut());
    // didn't notice the correct existence in time
    return false;
}

bool SysfsFile::waitUntilExists(const size_t milliseconds) const
{
    return waitUntilExistence(true, milliseconds);
}

bool SysfsFile::waitUntilDoesNotExist(const size_t milliseconds) const
{
    return waitUntilExistence(false, milliseconds);
}

FifoSysfsFile::FifoSysfsFile(const std::string& device,
    const NiFpgaEx_DmaFifo fifo,
    const std::string& attribute,
    const ErrnoMap& errnoMap)
    : SysfsFile(baseSysfsPath, errnoMap)
{
    std::ostringstream temp;
    temp << device << "fifo" << fifo;
    path = joinPath(path, temp.str(), attribute);
}

} // namespace nirio
