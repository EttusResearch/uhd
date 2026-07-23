/*
 * Copyright (c) 2020 National Instruments
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

#include "Common.h"
#include "DeviceFile.h"
#include "linux/dma-heap.h"
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace nirio {

class DmaBuf
{
public:
    static DmaBuf* allocate(size_t size, const char* heap = "system")
    {
        DeviceFile heapFile(joinPath("/dev/dma_heap", heap), DeviceFile::ReadWrite);
        struct dma_heap_allocation_data arg;

        arg.len        = size;
        arg.fd         = 0;
        arg.fd_flags   = O_RDWR | O_CLOEXEC;
        arg.heap_flags = 0;

        heapFile.ioctl(DMA_HEAP_IOCTL_ALLOC, &arg);

        return new DmaBuf(arg.fd, size);
    }

    volatile void* getPointer()
    {
        if (bufFile.isMapped())
            return buffer;

        buffer = bufFile.mapMemory(size);
        return buffer;
    }

    size_t getSize() const
    {
        return size;
    }

    int getDescriptor() const
    {
        return bufFile.getDescriptor();
    }

private:
    explicit DmaBuf(int descriptor, size_t size)
        : bufFile(descriptor, DeviceFile::ReadWrite), size(size), buffer(NULL)
    {
    }

    DeviceFile bufFile;
    const size_t size;
    volatile void* buffer;
};

} // namespace nirio
