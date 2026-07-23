/*
 * Copyright (c) 2014 National Instruments
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

#include "Fifo.h"
#include "ErrnoMap.h"
#include "Exception.h"
#include "nib310rio.h"
#include <unistd.h> // sysconf
#include <cassert> // assert
#include <cstdlib> // valloc

namespace nirio {

namespace {

const auto pageSize = sysconf(_SC_PAGESIZE);

const auto minimumDepth = 1 << 14; // 16384 elements

size_t pageAlign(const size_t value, const size_t size)
{
    return value & ~(size - 1);
}

size_t pageRound(const size_t value)
{
    const auto size = pageSize;
    return pageAlign(value + size - 1, size);
}

const class : public ErrnoMap
{
public:
    virtual void throwErrno(const int error) const
    {
        switch (error) {
            // if you, for example, try to start a started FIFO
            case EALREADY:
                return;
            // override because FIFO is busy, not the FPGA itself
            case EBUSY:
                throw FifoReservedException();
            // someone reset or otherwise stopped the FIFO behind our back
            case EPERM:
                throw TransferAbortedException();
            // if we acquire or release a bad amount
            //
            // NOTE: won't happen unless something happens behind our back
            case ENODATA:
                throw ElementsNotPermissibleToBeAcquiredException();
            case ENOTCONN:
                throw ResourceNotFoundException();
            // pass on the rest
            default:
                ErrnoMap::throwErrno(error);
        }
    }
} errnoMap;

} // unnamed namespace

Fifo::Fifo(const FifoInfo& fifo, const std::string& device)
    : FifoInfo(fifo)
    , device(device)
    , started(false)
    , hardwareElementBytes(
          FifoSysfsFile(device, number, "element_bytes", errnoMap).readU32())
    ,
    // depth assigned below
    // size assigned below
    buffer(NULL)
    , acquired(0)
    , next(0)
{
    // calculate depth and size
    calculateDimensions(minimumDepth, depth, size);
}

Fifo::~Fifo() noexcept(true) {}

// precondition: in constructor, or lock is locked
void Fifo::calculateDimensions(
    const size_t requestedDepth, size_t& actualDepth, size_t& actualSize) const
{
    actualSize  = pageRound(requestedDepth * hardwareElementBytes);
    actualDepth = actualSize / hardwareElementBytes;
}

// precondition: lock is locked
void Fifo::ensureConfigured()
{
    // in CPU mode, just pretend they called configure with the old depth
    if (!file)
        configure(depth, NULL);
}

// precondition: lock is locked
void Fifo::ensureConfiguredAndStarted()
{
    // conditionally configure
    ensureConfigured();
    // don't bother starting if we think it already is
    //
    // NOTE: even though it's possible someone reset or otherwise stopped the
    // FIFO behind our back, we don't have an extra start call for every
    // acquire/read/write
    if (!started)
        start();
}

// precondition: lock is locked
void Fifo::setBuffer()
{
    // if there's not an error, we should have a file
    assert(file);

    setBufferFd(dmaBuf->getDescriptor());

    // if we have a new buffer, we start from the beginning of it
    buffer   = const_cast<void*>(dmaBuf->getPointer());
    next     = 0;
    acquired = 0;
}

void Fifo::unsetBuffer()
{
    assert(file);

    setBufferFd(0);

    buffer   = NULL;
    next     = 0;
    acquired = 0;
}

void Fifo::setBufferFd(int fd)
{
    struct ioctl_nib310rio_fifo_set_buf arg;
    arg.fd = fd;
    file->ioctl(NIB310RIO_IOC_FIFO_SET_BUF, &arg);
}

void Fifo::configure(const size_t requestedDepth, size_t* const actualDepth)
{
    // validate parameters
    assert(requestedDepth != 0); // checked in NiFpga.cpp
    // grab the lock
    const std::lock_guard<std::recursive_mutex> guard(lock);
    // cannot do this while elements are acquired
    if (acquired)
        NIRIO_THROW(FifoElementsCurrentlyAcquiredException());

    // calculate the actual dimensions
    size_t localActualDepth, actualSize;
    calculateDimensions(requestedDepth, localActualDepth, actualSize);

    // We can reuse the buffer if reconfiguring a CPU buffer of the same size.
    if (file && actualSize == size) {
        // if the sizes are the same, the depths should be
        assert(localActualDepth == depth);
        // if a file is open, we should have a buffer
        // assert(buffer);
    }
    // otherwise, we've got work to do
    else {
        // mark it as stopped
        started = false;
        // if we have a file, unset the old buffer we configured
        //
        // NOTE: we reuse the opened file so no one can steal it out from under us
        if (file) {
            unsetBuffer();
        }
        // otherwise, open the cdev file for the first time
        else
            file.reset(new DeviceFile(DeviceFile::getFifoCdevPath(device, number),
                hostToTarget ? DeviceFile::WriteOnly : DeviceFile::ReadOnly,
                errnoMap));

        dmaBuf.reset(DmaBuf::allocate(actualSize, "system"));
        // set the buffer in the kernel
        setBuffer();
        // if everything's okay, remember the new sizes
        depth = localActualDepth;
        size  = 0; // fifoBuffer->getSize();
    }

    if (actualDepth)
        *actualDepth = depth;
}

void Fifo::start()
{
    // grab the lock
    const std::lock_guard<std::recursive_mutex> guard(lock);
    // conditionally configure
    ensureConfigured();
    // start regardless of whether we thought it was started in case someone
    // reset or otherwise stopped the FIFO behind our back
    //
    // NOTE: EALREADY should be mapped to Success
    file->ioctl(NIB310RIO_IOC_FIFO_START);
    started = true;
}

/**
 * Stops a started FIFO.
 *
 */
void Fifo::stop()
{
    // grab the lock
    const std::lock_guard<std::recursive_mutex> guard(lock);
    // cannot do this while elements are acquired, unless it's forced
    if (acquired)
        NIRIO_THROW(FifoElementsCurrentlyAcquiredException());
    // actually mark it as stopped
    //
    // NOTE: we don't bother writing 0 to nirio_started because setStopped will
    //       close the file which will make kernel stop the FIFO, _and_ we don't
    //       want stop to error if it's already stopped (because, for example,
    //       Session::reset will call Session::stop _after_ kernel mode already
    //       stopped all FIFOs)
    setStopped();
}

/**
 * Marks the FIFO as stopped and does any necessary cleanup if we know if
 * someone reset or otherwise stopped the FIFO behind our back.
 *
 */
void Fifo::setStopped()
{
    // grab the lock
    const std::lock_guard<std::recursive_mutex> guard(lock);
    // if anything is configured, tear it down
    //
    // NOTE: even if the FIFO isn't started, we need to relinquish control of the
    //       FIFO by closing the file
    if (file) {
        file.reset();
        buffer = NULL;
        // mark it as stopped
        started = false;
        // now that it's stopped, forget our previous progress
        next     = 0;
        acquired = 0;
        // remember depth and size in case they start again without a configure
    }
}

void Fifo::release(const size_t elements)
{
    // release of 0 elements should always succeed
    if (!elements)
        return;
    // grab the lock
    const std::lock_guard<std::recursive_mutex> guard(lock);
    // they shouldn't release more than they have
    if (elements > acquired)
        NIRIO_THROW(BadReadWriteCountException());

    // just pass it on, assuming kernel will error if wrong
    try {
        uint64_t elementsU64 = elements;
        file->ioctl(NIB310RIO_IOC_FIFO_RELEASE, &elementsU64);
    } catch (const TransferAbortedException&) {
        // if someone reset or otherwise stopped the FIFO behind our back, take note
        setStopped();
    }

    const char* buf               = static_cast<const char*>(buffer);
    const size_t bufSize          = depth * type.getElementBytes();
    const size_t acquiredInBytes  = acquired * type.getElementBytes();
    const size_t releasingInBytes = elements * type.getElementBytes();
    const size_t nextInBytes      = next * type.getElementBytes();

    if (nextInBytes >= acquiredInBytes) {
        VALGRIND_MAKE_MEM_NOACCESS(
            buf + (nextInBytes - acquiredInBytes), releasingInBytes);
    } else {
        // we've got a wrap to deal with:
        const size_t start = nextInBytes + bufSize - acquiredInBytes;
        const size_t len1  = std::min(bufSize - start, releasingInBytes);
        const size_t len2  = releasingInBytes - len1;
        VALGRIND_MAKE_MEM_NOACCESS(buf + start, len1);
        VALGRIND_MAKE_MEM_NOACCESS(buf, len2);
    }

    // if they successfully released, remember it
    acquired -= elements;
}

// precondition: lock is locked
// precondition: FIFO is configured and started, or there's an error
void Fifo::acquireWithWait(const size_t elementsRequested,
    const uint32_t timeoutMs,
    size_t* const elementsRemaining)
{
    struct ioctl_nib310rio_fifo_acquire fifo_acq;

    fifo_acq.elements   = elementsRequested;
    fifo_acq.timeout_ms = timeoutMs;
    try {
        file->ioctl(NIB310RIO_IOC_FIFO_ACQUIRE, &fifo_acq);
    } catch (const TransferAbortedException&) {
        // FIFO was stopped out from under us
        // clean up our members, restart, and try one more time to acquire
        setStopped();
        start();
        file->ioctl(NIB310RIO_IOC_FIFO_ACQUIRE, &fifo_acq);
    }

    if (elementsRemaining)
        *elementsRemaining = fifo_acq.available;
    if (fifo_acq.timed_out)
        NIRIO_THROW(FifoTimeoutException());
}

void Fifo::getElementsAvailable(size_t& elementsAvailable)
{
    uint64_t available;

    try {
        file->ioctl(NIB310RIO_IOC_FIFO_GET_AVAIL, &available);
    } catch (const TransferAbortedException&) {
        // FIFO was stopped out from under us
        // clean up members, restart, and try one more time
        setStopped();
        start();
        file->ioctl(NIB310RIO_IOC_FIFO_GET_AVAIL, &available);
    }

    elementsAvailable = available;
}

} // namespace nirio
