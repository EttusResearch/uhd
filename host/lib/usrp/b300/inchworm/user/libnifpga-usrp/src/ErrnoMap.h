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

#include "Common.h"
#include "Exception.h"
#include "NiFpga.h"
#include <cerrno>

namespace nirio {
/**
 * Strategy class for mapping system error codes like those that come from errno
 * to NI-RIO exceptions
 */
class ErrnoMap
{
public:
    // single instance that can safely passed in as the default parameter of
    // reference type
    static const ErrnoMap instance;

    virtual ~ErrnoMap() = default;

    /**
     * Maps a system error code to an exception
     *
     * @param error incoming system error code
     */
    virtual void throwErrno(const int error) const
    {
        switch (error) {
            case 0:
                return;
            case EIO:
                throw HardwareFaultException();
            case ENOMEM:
                throw MemoryFullException();
            case EBUSY:
                throw FpgaBusyException();
            case E2BIG:
                throw BadDepthException();
            case EINVAL:
                throw InvalidParameterException();
            case EOPNOTSUPP:
                throw FeatureNotSupportedException();
            case EMFILE:
                throw OutOfHandlesException();
            case ENOENT:
            case EACCES:
                throw InvalidResourceNameException();
            case ETIMEDOUT:
                throw CommunicationTimeoutException();
            case ENOLCK:
                throw ClockLostLockException();
            // There currently isn't a RIO error that matches up perfectly with EPROTO.
            case EPROTO:
            default:
                DEBUG_PRINT_ERRNO();
                DEBUG_PRINT_STACK();
                throw SoftwareFaultException();
        }
    }

protected:
    // public callers should use instance instead
    ErrnoMap() = default;

private:
    // prevents losing virtuality of map function by making copy
    ErrnoMap(const ErrnoMap&) = delete;
};

} // namespace nirio
