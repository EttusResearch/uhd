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

#include "DeviceFile.h"
#include "Exception.h"
#include "Timer.h"
#include <fcntl.h> // open, close, read, write
#include <sched.h> // sched_yield
#include <sys/ioctl.h> // ioctl
#include <sys/mman.h> // mmap, munmap
#include <unistd.h>
#include <cassert> // assert
#include <sstream> // std::ostringstream

namespace nirio {

namespace {

const int invalidDescriptor = -1;

int accessToOpenFlag(const DeviceFile::Access access)
{
    switch (access) {
        default:
            assert(false); // fall through
        case DeviceFile::ReadOnly:
            return O_RDONLY;
        case DeviceFile::WriteOnly:
            return O_WRONLY;
        case DeviceFile::ReadWrite:
            return O_RDWR;
    }
}

int accessToMmapProtection(const DeviceFile::Access access)
{
    switch (access) {
        default:
            assert(false); // fall through
        case DeviceFile::ReadOnly:
            return PROT_READ;
        case DeviceFile::WriteOnly:
            return PROT_WRITE;
        case DeviceFile::ReadWrite:
            return PROT_READ | PROT_WRITE;
    }
}

} // unnamed namespace

DeviceFile::DeviceFile(
    const std::string& path, const Access access, const ErrnoMap& errnoMap)
    : access(access)
    , descriptor(invalidDescriptor)
    , mapped(NULL)
    , mappedSize(0)
    , errnoMap(errnoMap)
{
    // keep trying to open as long as file not found and we haven't timed out,
    // as some virtual files can take a couple seconds before popping up
    const Timer timer(2000);
    do {
        // open the file with O_CLOEXEC to ensure child processes don't inherit
        // open handles
        descriptor = ::open(path.c_str(), accessToOpenFlag(access) | O_CLOEXEC);
    } while (descriptor == invalidDescriptor
             && (errno == ENOENT || // "No such file or directory"
                 errno == EACCES) // "Permission denied"
             && !timer.isTimedOut()
             // NOTE: "In the Linux implementation, sched_yield() always succeeds":
             //    http://man7.org/linux/man-pages/man2/sched_yield.2.html
             && sched_yield() == 0);
    // finally, err if we never successfully opened
    if (descriptor == invalidDescriptor)
        errnoMap.throwErrno(errno);
}

DeviceFile::DeviceFile(int fd, const Access access, const ErrnoMap& errnoMap)
    : access(access), descriptor(fd), mapped(NULL), mappedSize(0), errnoMap(errnoMap)
{
}

DeviceFile::~DeviceFile()
{
    // unmap if necessary
    if (mapped) {
        try {
            // If this fails, there's not much we can do
            unmapMemory();
        } catch (...) {
        }
    }

    ::close(descriptor);
}

int DeviceFile::getDescriptor() const
{
    return descriptor;
}

size_t DeviceFile::read(void* const buffer, const size_t size) const
{
    // file must be open and readable
    if (access == WriteOnly)
        NIRIO_THROW(SoftwareFaultException());

    const auto result = ::read(descriptor, buffer, size);
    if (result == -1)
        errnoMap.throwErrno(errno);
    return static_cast<size_t>(result);
}

size_t DeviceFile::write(const void* const buffer, const size_t size) const
{
    // file must be open and writeable
    if (access == ReadOnly)
        NIRIO_THROW(SoftwareFaultException());

    const auto result = ::write(descriptor, buffer, size);
    if (result == -1)
        errnoMap.throwErrno(errno);
    return static_cast<size_t>(result);
}

off_t DeviceFile::seek(off_t offset, int whence) const
{
    const auto result = ::lseek(descriptor, offset, whence);

    if (result == -1)
        errnoMap.throwErrno(errno);

    return result;
}

void DeviceFile::ioctl(const unsigned long int request, void* const buffer) const
{
    if (::ioctl(descriptor, request, buffer) == -1)
        errnoMap.throwErrno(errno);
}

volatile void* DeviceFile::mapMemory(const size_t size)
{
    // file must be open and not mapped
    if (mapped)
        NIRIO_THROW(SoftwareFaultException());

    // try the mapping
    mapped = static_cast<volatile uint8_t*>(
        mmap(NULL, size, accessToMmapProtection(access), MAP_SHARED, descriptor, 0));
    // NOTE: we don't use MAP_FAILED to prevent "use of old-style cast" warning
    if (mapped != reinterpret_cast<void*>(-1))
        mappedSize = size;
    else
        errnoMap.throwErrno(errno);

    return mapped;
}

void DeviceFile::unmapMemory()
{
    // file must be open and mapped
    if (!mapped)
        NIRIO_THROW(SoftwareFaultException());

    if (munmap(const_cast<uint8_t*>(mapped), mappedSize) == 0) {
        mapped     = NULL;
        mappedSize = 0;
    } else
        errnoMap.throwErrno(errno);
}

bool DeviceFile::isMapped() const
{
    return mapped;
}

std::string DeviceFile::getCdevPath(const std::string& device)
{
    return joinPath("/dev", device);
}

std::string DeviceFile::getFifoCdevPath(
    const std::string& device, const NiFpgaEx_DmaFifo fifo)
{
    std::ostringstream temp;
    temp << device << "fifo" << fifo;
    return joinPath("/dev", temp.str());
}

} // namespace nirio
