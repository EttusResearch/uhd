//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "DeviceFile_win32.h"
#include "Exception.h"
#include "Timer.h"
#include "tInterfaceIoctl_B310.h"
#include <io.h>
#include <windows.h>
#include <winioctl.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

namespace nirio {

namespace {

const HANDLE invalidHandle = INVALID_HANDLE_VALUE;

DWORD accessToPageProtection(const DeviceFile::Access access)
{
    switch (access) {
        default:
            assert(false); // fall through
        case DeviceFile::ReadOnly:
            return PAGE_READONLY;
        case DeviceFile::WriteOnly:
            return PAGE_READWRITE; // Windows requires read access for mapping
        case DeviceFile::ReadWrite:
            return PAGE_READWRITE;
    }
}

DWORD accessToFileMapAccess(const DeviceFile::Access access)
{
    switch (access) {
        default:
            assert(false); // fall through
        case DeviceFile::ReadOnly:
            return FILE_MAP_READ;
        case DeviceFile::WriteOnly:
            return FILE_MAP_WRITE;
        case DeviceFile::ReadWrite:
            return FILE_MAP_ALL_ACCESS;
    }
}

// Convert Windows error to errno-like values for compatibility
int windowsErrorToErrno(DWORD winError)
{
    switch (winError) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return ENOENT;
        case ERROR_ACCESS_DENIED:
            return EACCES;
        case ERROR_INVALID_HANDLE:
            return EBADF;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            return ENOMEM;
        case ERROR_INVALID_PARAMETER:
            return EINVAL;
        case ERROR_SHARING_VIOLATION:
            return EBUSY;
        case ERROR_OPERATION_ABORTED:
            return EINTR;
        default:
            return EIO; // Generic I/O error
    }
}

} // unnamed namespace

// Static member function implementations
DWORD DeviceFile::accessToDesiredAccess(const Access access)
{
    switch (access) {
        default:
            assert(false); // fall through
        case ReadOnly:
            return GENERIC_READ;
        case WriteOnly:
            return GENERIC_WRITE;
        case ReadWrite:
            return GENERIC_READ | GENERIC_WRITE;
    }
}

std::string DeviceFile::convertToWindowsPath(const std::string& linuxPath)
{
    // Convert Linux B300/B310 paths to Windows device paths
    // Based on detected B310 device:
    // - Service: b310k
    // - Driver: b310k.sys
    // - Device: USRP-B310 Test Device

    if (linuxPath.find("/dev/") == 0) {
        std::string deviceName = linuxPath.substr(5); // Remove "/dev/"

        // Handle B300/B310 specific device naming
        if (deviceName.find("b300_pcie") == 0) {
            // Convert "/dev/b300_pcie0" to "\\.\b310k0"
            // Extract device number if present
            std::string deviceNum = "0";
            size_t lastDigit      = deviceName.find_last_of("0123456789");
            if (lastDigit != std::string::npos) {
                deviceNum = deviceName.substr(lastDigit);
            }
            // Use the confirmed driver name from driverquery: b310k
            return "\\\\.\\" + std::string("b310k") + deviceNum;
        }

        // Handle FIFO devices for B300/B310
        if (deviceName.find("b300_pcie") == 0
            && deviceName.find("fifo") != std::string::npos) {
            // Extract base device name and FIFO info
            // e.g., "b300_pcie0fifo1" -> "b310k0fifo1"
            std::string baseName   = deviceName;
            std::string deviceNum  = "0";
            std::string fifoSuffix = "";

            // Find FIFO suffix
            size_t fifoPos = baseName.find("fifo");
            if (fifoPos != std::string::npos) {
                fifoSuffix = baseName.substr(fifoPos);
                baseName   = baseName.substr(0, fifoPos);
            }

            // Extract device number
            size_t lastDigit = baseName.find_last_of("0123456789");
            if (lastDigit != std::string::npos) {
                deviceNum = baseName.substr(lastDigit);
            }

            return "\\\\.\\" + std::string("b310k") + deviceNum + fifoSuffix;
        }

        // Generic device path conversion (fallback)
        // Convert to uppercase for consistency
        std::transform(
            deviceName.begin(), deviceName.end(), deviceName.begin(), ::toupper);
        return "\\\\.\\" + deviceName;
    }

    // If it's already a Windows path, return as-is
    if (linuxPath.find("\\\\.\\") == 0) {
        return linuxPath;
    }

    // Default: assume it's a device name and prepend Windows device prefix
    std::string deviceName = linuxPath;
    std::transform(deviceName.begin(), deviceName.end(), deviceName.begin(), ::toupper);
    return "\\\\.\\" + deviceName;
}

DeviceFile::DeviceFile(HANDLE handle, const Access access, const ErrnoMap& errnoMap)
    : access(access)
    , deviceHandle(handle)
    , mappingHandle(NULL)
    , mapped(NULL)
    , mappedSize(0)
    , errnoMap(errnoMap)
{
}

DeviceFile::~DeviceFile()
{
    // Unmap if necessary
    if (mapped) {
        try {
            unmapMemory();
        } catch (...) {
            // If this fails, there's not much we can do
        }
    }

    if (deviceHandle != invalidHandle) {
        CloseHandle(deviceHandle);
    }
}

HANDLE DeviceFile::getHandle() const
{
    return deviceHandle;
}

size_t DeviceFile::read(void* const buffer, const size_t size) const
{
    // File must be open and readable
    if (access == WriteOnly)
        NIRIO_THROW(SoftwareFaultException());

    DWORD bytesRead = 0;
    if (!ReadFile(deviceHandle, buffer, static_cast<DWORD>(size), &bytesRead, NULL)) {
        throwWindowsError(GetLastError());
    }

    return static_cast<size_t>(bytesRead);
}

size_t DeviceFile::write(const void* const buffer, const size_t size) const
{
    // File must be open and writeable
    if (access == ReadOnly)
        NIRIO_THROW(SoftwareFaultException());

    DWORD bytesWritten = 0;
    if (!WriteFile(deviceHandle, buffer, static_cast<DWORD>(size), &bytesWritten, NULL)) {
        throwWindowsError(GetLastError());
    }

    return static_cast<size_t>(bytesWritten);
}

off_t DeviceFile::seek(off_t offset, int whence) const
{
    DWORD moveMethod;
    switch (whence) {
        case SEEK_SET:
            moveMethod = FILE_BEGIN;
            break;
        case SEEK_CUR:
            moveMethod = FILE_CURRENT;
            break;
        case SEEK_END:
            moveMethod = FILE_END;
            break;
        default:
            NIRIO_THROW(SoftwareFaultException());
    }

    LARGE_INTEGER distanceToMove;
    distanceToMove.QuadPart = offset;

    LARGE_INTEGER newFilePointer;
    if (!SetFilePointerEx(deviceHandle, distanceToMove, &newFilePointer, moveMethod)) {
        throwWindowsError(GetLastError());
    }

    return static_cast<off_t>(newFilePointer.QuadPart);
}

void DeviceFile::ioctl(DWORD controlCode, void* buffer) const
{
    DWORD bytesReturned = 0;

    // For read operations, we need to handle input/output separately
    // The buffer is expected to be a combined structure with input followed by output
    if (controlCode == B310_WIN_IOC_READ32 || controlCode == B310_WIN_IOC_READ64) {
        void* inputBuffer  = buffer;
        void* outputBuffer = nullptr;
        DWORD inputSize    = 0;
        DWORD outputSize   = 0;

        if (controlCode == B310_WIN_IOC_READ32) {
            inputSize  = sizeof(tIn_B310_read32); // 4 bytes
            outputSize = sizeof(tOut_B310_read32); // 4 bytes
            // Output buffer starts after input buffer
            outputBuffer = static_cast<char*>(buffer) + inputSize;
        } else if (controlCode == B310_WIN_IOC_READ64) {
            inputSize    = sizeof(tIn_B310_read64); // 4 bytes
            outputSize   = sizeof(tOut_B310_read64); // 8 bytes
            outputBuffer = static_cast<char*>(buffer) + inputSize;
        }

        // Use overlapped I/O
        OVERLAPPED overlapped = {};

        if (!DeviceIoControl(deviceHandle,
                controlCode,
                inputBuffer,
                inputSize, // Only input structure size (4 bytes for read32)
                outputBuffer,
                outputSize, // Only output structure size (4 bytes for read32)
                &bytesReturned,
                &overlapped)) {
            DWORD lastError = GetLastError();
            char errorBuffer[256];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                lastError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                errorBuffer,
                sizeof(errorBuffer),
                NULL);

            fprintf(stderr,
                "DeviceIoControl failed - Code: 0x%08X, Control Code: 0x%08X, Input "
                "Size: %d, Output Size: %d, Message: %s\n",
                lastError,
                controlCode,
                inputSize,
                outputSize,
                errorBuffer);
            fflush(stderr);

            throwWindowsError(lastError);
        }
        return;
    }

    // For write operations, use input buffer only (no output buffer)
    if (controlCode == B310_WIN_IOC_WRITE32 || controlCode == B310_WIN_IOC_WRITE64) {
        DWORD inputSize = 0;

        if (controlCode == B310_WIN_IOC_WRITE32) {
            inputSize = sizeof(tIn_B310_write32); // 8 bytes (offset + value)
        } else if (controlCode == B310_WIN_IOC_WRITE64) {
            inputSize = sizeof(tIn_B310_write64); // 12 bytes (offset + aligned value)
        }

        // Use overlapped I/O as it is required by the kernel driver.
        OVERLAPPED overlapped = {};

        if (!DeviceIoControl(deviceHandle,
                controlCode,
                buffer,
                inputSize, // Input structure size
                NULL, // No output buffer for write operations
                0, // No output size
                &bytesReturned,
                &overlapped)) {
            DWORD lastError = GetLastError();
            char errorBuffer[256];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                lastError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                errorBuffer,
                sizeof(errorBuffer),
                NULL);

            fprintf(stderr,
                "DeviceIoControl failed - Code: 0x%08X, Control Code: 0x%08X, Input "
                "Size: %d, Output Size: 0, Message: %s\n",
                lastError,
                controlCode,
                inputSize,
                errorBuffer);
            fflush(stderr);

            throwWindowsError(lastError);
        }
        return;
    }

    // For FIFO operations, handle input/output buffers separately
    if (controlCode == B310_WIN_IOC_FIFO_START || controlCode == B310_WIN_IOC_FIFO_STOP
        || controlCode == B310_WIN_IOC_FIFO_SET_BUF
        || controlCode == B310_WIN_IOC_FIFO_ACQUIRE
        || controlCode == B310_WIN_IOC_FIFO_RELEASE
        || controlCode == B310_WIN_IOC_FIFO_GET_AVAIL) {
        void* inputBuffer  = buffer;
        void* outputBuffer = nullptr;
        DWORD inputSize    = 0;
        DWORD outputSize   = 0;

        switch (controlCode) {
            case B310_WIN_IOC_FIFO_START:
                inputSize    = sizeof(tIn_B310_fifoStart); // 8 bytes (channel + status)
                outputSize   = sizeof(tOut_B310_fifoStart); // 4 bytes (status only)
                outputBuffer = static_cast<char*>(buffer) + inputSize;
                break;
            case B310_WIN_IOC_FIFO_STOP:
                inputSize    = sizeof(tIn_B310_fifoStop); // 8 bytes (channel + status)
                outputSize   = sizeof(tOut_B310_fifoStop); // 4 bytes (status only)
                outputBuffer = static_cast<char*>(buffer) + inputSize;
                break;
            case B310_WIN_IOC_FIFO_SET_BUF:
                inputSize = sizeof(tIn_B310_fifoSetBuf); // 20 bytes (channel + buffer +
                                                         // fifoSizeBytes + status)
                outputSize   = sizeof(tOut_B310_fifoSetBuf); // 4 bytes (status only)
                outputBuffer = static_cast<char*>(buffer) + inputSize;
                break;
            case B310_WIN_IOC_FIFO_ACQUIRE:
                inputSize = sizeof(
                    tIn_B310_fifoAcquire); // 12 bytes (channel + timeoutMs + status)
                outputSize =
                    sizeof(tOut_B310_fifoAcquire); // 20 bytes (elements + available +
                                                   // timedOut + status)
                outputBuffer = static_cast<char*>(buffer) + inputSize;
                break;
            case B310_WIN_IOC_FIFO_RELEASE:
                inputSize = sizeof(
                    tIn_B310_fifoRelease); // 16 bytes (channel + elements + status)
                outputSize   = sizeof(tOut_B310_fifoRelease); // 4 bytes (status only)
                outputBuffer = static_cast<char*>(buffer) + inputSize;
                break;
            case B310_WIN_IOC_FIFO_GET_AVAIL:
                inputSize = sizeof(tIn_B310_fifoGetAvail); // 8 bytes (channel + status)
                outputSize =
                    sizeof(tOut_B310_fifoGetAvail); // 12 bytes (available + status)
                outputBuffer = static_cast<char*>(buffer) + inputSize;
                break;
        }

        // Use overlapped I/O
        OVERLAPPED overlapped = {};

        if (!DeviceIoControl(deviceHandle,
                controlCode,
                inputBuffer,
                inputSize,
                outputBuffer,
                outputSize,
                &bytesReturned,
                &overlapped)) {
            DWORD lastError = GetLastError();
            char errorBuffer[256];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                lastError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                errorBuffer,
                sizeof(errorBuffer),
                NULL);

            fprintf(stderr,
                "DeviceIoControl failed - Code: 0x%08X, Control Code: 0x%08X, Input "
                "Size: %d, Output Size: %d, Message: %s\n",
                lastError,
                controlCode,
                inputSize,
                outputSize,
                errorBuffer);
            fflush(stderr);

            throwWindowsError(lastError);
        }
        return;
    }

    // For session count operations, use output buffer only (no input buffer)
    if (controlCode == B310_WIN_IOC_GET_SESSION_COUNT) {
        DWORD outputSize = sizeof(tOut_B310_get_session_count); // 4 bytes (count)

        // Use overlapped I/O
        OVERLAPPED overlapped = {};

        if (!DeviceIoControl(deviceHandle,
                controlCode,
                NULL,
                0,
                buffer,
                outputSize,
                &bytesReturned,
                &overlapped)) {
            DWORD lastError = GetLastError();
            char errorBuffer[256];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                lastError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                errorBuffer,
                sizeof(errorBuffer),
                NULL);

            fprintf(stderr,
                "DeviceIoControl failed - Code: 0x%08X, Control Code: 0x%08X, Input "
                "Size: %d, Output Size: %d, Message: %s\n",
                lastError,
                controlCode,
                0,
                outputSize,
                errorBuffer);
            fflush(stderr);

            throwWindowsError(lastError);
        }
        return;
    }

    // Throw error in case of unsupported IOCTL
    NIRIO_THROW(FeatureNotSupportedException());
}

volatile void* DeviceFile::mapMemory(const size_t size)
{
    // File must be open and not mapped
    if (mapped)
        NIRIO_THROW(SoftwareFaultException());

    // For device drivers, we typically don't use file mapping but rather
    // use a custom IOCTL to map device memory (like BAR space)
    // This is a simplified implementation that may need driver-specific handling

    // Create a file mapping object
    mappingHandle = CreateFileMapping(deviceHandle,
        NULL,
        accessToPageProtection(access),
        0,
        static_cast<DWORD>(size),
        NULL);

    if (mappingHandle == NULL) {
        throwWindowsError(GetLastError());
    }

    // Map the view
    mapped = static_cast<volatile uint8_t*>(
        MapViewOfFile(mappingHandle, accessToFileMapAccess(access), 0, 0, size));

    if (mapped == NULL) {
        DWORD error = GetLastError();
        CloseHandle(mappingHandle);
        mappingHandle = NULL;
        throwWindowsError(error);
    }

    mappedSize = size;
    return mapped;
}

void DeviceFile::unmapMemory()
{
    // File must be open and mapped
    if (!mapped)
        NIRIO_THROW(SoftwareFaultException());

    if (UnmapViewOfFile(const_cast<uint8_t*>(mapped))) {
        mapped     = NULL;
        mappedSize = 0;

        if (mappingHandle) {
            CloseHandle(mappingHandle);
            mappingHandle = NULL;
        }
    } else {
        throwWindowsError(GetLastError());
    }
}

bool DeviceFile::isMapped() const
{
    return mapped != NULL;
}

void DeviceFile::throwWindowsError(DWORD error) const
{
    // Convert Windows error to errno and use existing error map
    int posixError = windowsErrorToErrno(error);
    errnoMap.throwErrno(posixError);
}

} // namespace nirio
