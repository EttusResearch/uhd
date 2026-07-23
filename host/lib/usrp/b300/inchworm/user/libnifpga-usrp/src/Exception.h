/*
 * Copyright (c) 2016 National Instruments
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
#include "NiFpga.h"
#include "Status.h"
#include <exception>

namespace nirio {
class ExceptionBase
{
public:
    explicit ExceptionBase(int32_t code) : code(code) {}
    virtual ~ExceptionBase() = default;
    int32_t getCode() const
    {
        return code;
    }

private:
    int32_t code;
};

template <int32_t StatusCode>
class Exception : public virtual ExceptionBase, public virtual std::exception
{
public:
    Exception() : ExceptionBase(StatusCode) {}
};


#define NIRIO_DEFINE_EXCEPTION(code) \
    typedef Exception<NiFpga_Status_##code> code##Exception
#define NIRIO_THROW(excpt) throw excpt

NIRIO_DEFINE_EXCEPTION(AccessDenied);
NIRIO_DEFINE_EXCEPTION(BadDepth);
NIRIO_DEFINE_EXCEPTION(BadReadWriteCount);
NIRIO_DEFINE_EXCEPTION(BitfileReadError);
NIRIO_DEFINE_EXCEPTION(BufferInvalidSize);
NIRIO_DEFINE_EXCEPTION(ClockLostLock);
NIRIO_DEFINE_EXCEPTION(CommunicationTimeout);
NIRIO_DEFINE_EXCEPTION(CorruptBitfile);
NIRIO_DEFINE_EXCEPTION(DeviceTypeMismatch);
NIRIO_DEFINE_EXCEPTION(ElementsNotPermissibleToBeAcquired);
NIRIO_DEFINE_EXCEPTION(FeatureNotSupported);
NIRIO_DEFINE_EXCEPTION(FifoElementsCurrentlyAcquired);
NIRIO_DEFINE_EXCEPTION(FifoReserved);
NIRIO_DEFINE_EXCEPTION(FifoTimeout);
NIRIO_DEFINE_EXCEPTION(FpgaAlreadyRunning);
NIRIO_DEFINE_EXCEPTION(FpgaBusy);
NIRIO_DEFINE_EXCEPTION(FpgaBusyFpgaInterfaceCApi);
NIRIO_DEFINE_EXCEPTION(HardwareFault);
NIRIO_DEFINE_EXCEPTION(IncompatibleBitfile);
NIRIO_DEFINE_EXCEPTION(InvalidParameter);
NIRIO_DEFINE_EXCEPTION(InvalidResourceName);
NIRIO_DEFINE_EXCEPTION(InvalidSession);
NIRIO_DEFINE_EXCEPTION(MemoryFull);
NIRIO_DEFINE_EXCEPTION(OutOfHandles);
NIRIO_DEFINE_EXCEPTION(ResourceNotFound);
NIRIO_DEFINE_EXCEPTION(SignatureMismatch);
NIRIO_DEFINE_EXCEPTION(SoftwareFault);
NIRIO_DEFINE_EXCEPTION(TransferAborted);
NIRIO_DEFINE_EXCEPTION(VersionMismatch);
} // namespace nirio
