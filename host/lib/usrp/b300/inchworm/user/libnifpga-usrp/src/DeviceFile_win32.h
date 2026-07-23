//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "ErrnoMap.h"
#include "Status.h"
#include <windows.h>
#include <winioctl.h>
#include <string>

namespace nirio {

/**
 * A Windows device file, usually accessed via \\.\DeviceName paths.
 *
 * Device handles will be created with proper security attributes to ensure
 * they are not inherited by child processes.
 */
class DeviceFile
{
public:
    enum Access { ReadOnly, WriteOnly, ReadWrite };

    explicit DeviceFile(HANDLE handle,
        const Access access,
        const ErrnoMap& errnoMap = ErrnoMap::instance);

    ~DeviceFile();

    HANDLE getHandle() const;

    size_t read(void* buffer, size_t size) const;

    size_t write(const void* buffer, size_t size) const;

    off_t seek(off_t offset, int whence) const;

    void ioctl(DWORD controlCode, void* buffer = NULL) const;

    volatile void* mapMemory(const size_t size);

    void unmapMemory();

    bool isMapped() const;

    /**
     * Converts Linux-style device paths to Windows device paths.
     * For example: "/dev/b300_pcie0" -> "\\.\b310k0"
     *
     * @param linuxPath Linux-style device path
     * @return Windows device path
     */
    static std::string convertToWindowsPath(const std::string& linuxPath);

private:
    const Access access;
    HANDLE deviceHandle;
    HANDLE mappingHandle;
    volatile uint8_t* mapped;
    size_t mappedSize;
    const ErrnoMap& errnoMap;

    static DWORD accessToDesiredAccess(Access access);
    void throwWindowsError(DWORD error) const;

    DeviceFile(const DeviceFile&)            = delete;
    DeviceFile& operator=(const DeviceFile&) = delete;
};

} // namespace nirio
