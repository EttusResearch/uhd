//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "Common.h"
#include <windows.h>
#include <memory>

namespace nirio {

class DmaBuf
{
public:
    static DmaBuf* allocate(size_t size, const char* heap = "system")
    {
        // Windows implementation: Use VirtualAlloc for better DMA-suitable memory
        // VirtualAlloc provides page-aligned memory that's more suitable for DMA
        // operations
        void* memory = VirtualAlloc(NULL, // Let system choose address
            size, // Size to allocate
            MEM_COMMIT | MEM_RESERVE, // Commit and reserve pages
            PAGE_READWRITE // Read/write access
        );
        if (!memory) {
            throw std::bad_alloc();
        }
        return new DmaBuf(memory, size);
    }

    ~DmaBuf()
    {
        if (buffer) {
            VirtualFree(const_cast<void*>(buffer), 0, MEM_RELEASE);
        }
    }

    volatile void* getPointer()
    {
        // Windows: Return the already allocated memory
        return buffer;
    }

    size_t getSize() const
    {
        return size;
    }

private:
    // Windows constructor: Direct memory allocation
    explicit DmaBuf(void* memory, size_t size)
        : size(size), buffer(static_cast<volatile void*>(memory))
    {
    }

    const size_t size;
    volatile void* buffer;
};

} // namespace nirio
