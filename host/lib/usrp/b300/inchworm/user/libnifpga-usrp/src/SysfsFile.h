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

#pragma once

#include "DeviceFile.h"
#include "ErrnoMap.h"
#include "Status.h"
#include <sstream> // std::ostringstream
#include <string> // std::string

namespace nirio {

/**
 * Represents a sysfs attribute under the /sys virtual filesystem.
 */
class SysfsFile
{
public:
    SysfsFile(const std::string& device,
        const std::string& attribute,
        const ErrnoMap& errnoMap = ErrnoMap::instance);

    SysfsFile(const std::string& device,
        const std::string& subdevice,
        const std::string& attribute,
        const ErrnoMap& errnoMap = ErrnoMap::instance);

    explicit SysfsFile(
        const std::string& path, const ErrnoMap& errnoMap = ErrnoMap::instance);

    virtual ~SysfsFile() = default;

    const std::string& getPath() const;

    bool readBool() const;

    uint32_t readU32() const;

    uint32_t readU32Hex() const;

    std::string readLineNoErrno() const;

    void write(const std::string& value) const;

    template <typename T>
    void write(T value) const;

    bool exists() const;

    /**
     * Waits on this file existing, or for the timeout to expire. Note that
     * there are race conditions where the existence is satisfied temporarily
     * but not noticed, or where the existence is satisfied but changes after
     * this function returns, so care must be taken in these cases.
     *
     * @param milliseconds number of milliseconds to wait
     * @return whether the path was noticed existing before the timeout
     */
    bool waitUntilExists(size_t milliseconds) const;

    /**
     * Waits on this file not existing, or for the timeout to expire. Note
     * that there are race conditions where the existence is satisfied
     * temporarily but not noticed, or where the existence is satisfied but
     * changes after this function returns, so care must be taken in these
     * cases.
     *
     * @param milliseconds number of milliseconds to wait
     * @return whether the path was noticed not existing before the timeout
     */
    bool waitUntilDoesNotExist(size_t milliseconds) const;

protected:
    std::string path;

private:
    /**
     * Waits on the existence of this file to be satisfied, or for the
     * timeout to expire. Note that there are race conditions where the
     * existence is satisfied temporarily but not noticed, or where the
     * existence is satisfied but changes after this function returns, so
     * care must be taken in these cases.
     *
     * @param exists whether to wait until it exists instead of does not
     * @param path path to test for
     * @param milliseconds number of milliseconds to wait
     * @return whether the path was noticed existing before the timeout
     */
    bool waitUntilExistence(bool exists, size_t milliseconds) const;

    const ErrnoMap& errnoMap;
};

class FifoSysfsFile : public SysfsFile
{
public:
    FifoSysfsFile(const std::string& device,
        NiFpgaEx_DmaFifo fifo,
        const std::string& attribute,
        const ErrnoMap& errnoMap = ErrnoMap::instance);
};

template <typename T>
void SysfsFile::write(const T value) const
{
    std::ostringstream stream;
    stream << value;
    write(stream.str());
}

} // namespace nirio
