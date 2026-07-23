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

#include "ErrnoMap.h"
#include "Status.h"
#include <sys/ioctl.h>
#include <string> // std::string

namespace nirio {

/**
 * A device file, usually in /dev or /sys.
 *
 * Open file descriptors will use the O_CLOEXEC flag, so that copies will not be
 * held by child processes after a call to fork() and exec().
 */
class DeviceFile
{
public:
    enum Access { ReadOnly, WriteOnly, ReadWrite };

    DeviceFile(const std::string& path,
        Access access,
        const ErrnoMap& errnoMap = ErrnoMap::instance);

    explicit DeviceFile(
        int fd, const Access access, const ErrnoMap& errnoMap = ErrnoMap::instance);

    ~DeviceFile();

    int getDescriptor() const;

    size_t read(void* buffer, size_t size) const;

    size_t write(const void* buffer, size_t size) const;

    off_t seek(off_t offset, int whence) const;

    void ioctl(unsigned long int request, void* buffer = NULL) const;

    volatile void* mapMemory(const size_t size);

    void unmapMemory();

    bool isMapped() const;

    template <typename T>
    T mappedRead(size_t offset) const
    {
        if (!mapped)
            NIRIO_THROW(SoftwareFaultException());

        // ensures proper flushing/sequencing before the read
        // XXX:
        //__asm__ __volatile__("lfence":::"memory");
        return *reinterpret_cast<volatile T*>(mapped + offset);
    }

    template <typename T>
    void mappedWrite(size_t offset, T value) const
    {
        if (!mapped)
            NIRIO_THROW(SoftwareFaultException());

        *reinterpret_cast<volatile T*>(mapped + offset) = value;
        // ensures proper flushing/sequencing/syncing after the write
        // XXX:
        //__asm__ __volatile__("sfence":::"memory");
    }

    /**
     * Creates a path to a character device file for a given device, such as
     * "/dev/nib310rio0"
     *
     * @param device device name, such as "nib310rio0"
     * @return path to a character device file
     */
    static std::string getCdevPath(const std::string& device);

    /**
     * Creates a path to a character device file for FIFO of a given device,
     * such as "/dev/nib310rio0fifo0".
     *
     * @param device device name, such as "nib310rio0"
     * @param fifo FIFO number
     * @return path to a character device file
     */
    static std::string getFifoCdevPath(const std::string& device, NiFpgaEx_DmaFifo fifo);

private:
    const Access access;
    int descriptor;
    volatile uint8_t* mapped;
    size_t mappedSize;
    const ErrnoMap& errnoMap;

    DeviceFile(const DeviceFile&)            = delete;
    DeviceFile& operator=(const DeviceFile&) = delete;
};

static inline uint32_t readU32Hex(const DeviceFile& file)
{
    // enough room for longest possible 32bit hex value plus a trailing '\0'
    char buffer[sizeof("0xffffffff") + 1] = {};
    file.read(buffer, sizeof(buffer));
    uint32_t result;
    if (sscanf(buffer, "%x", &result) != 1)
        NIRIO_THROW(SoftwareFaultException());
    return result;
}

} // namespace nirio
