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

#include "Type.h"
#include <cassert> // assert

namespace nirio {

Type::Type(const size_t logicalBits, const size_t elementBytes, const bool isSigned)
    : logicalBits(logicalBits), elementBytes(elementBytes), typeIsSigned(isSigned)
{
}

size_t Type::getLogicalBits() const
{
    return logicalBits;
}

size_t Type::getElementBytes() const
{
    return elementBytes;
}

bool Type::isSigned() const
{
    return typeIsSigned;
}

bool Type::operator==(const Type& other) const
{
    return logicalBits == other.logicalBits && elementBytes == other.elementBytes
           && typeIsSigned == other.typeIsSigned;
}

bool Type::operator!=(const Type& other) const
{
    return !(*this == other);
}


Type getType(const NiFpgaEx_ResourceType type)
{
    switch (type) {
        case NiFpgaEx_ResourceType_IndicatorBool:
        case NiFpgaEx_ResourceType_ControlBool:
        case NiFpgaEx_ResourceType_IndicatorArrayBool:
        case NiFpgaEx_ResourceType_ControlArrayBool:
        case NiFpgaEx_ResourceType_TargetToHostFifoBool:
        case NiFpgaEx_ResourceType_HostToTargetFifoBool:
            return Bool();
        case NiFpgaEx_ResourceType_IndicatorI8:
        case NiFpgaEx_ResourceType_ControlI8:
        case NiFpgaEx_ResourceType_IndicatorArrayI8:
        case NiFpgaEx_ResourceType_ControlArrayI8:
        case NiFpgaEx_ResourceType_TargetToHostFifoI8:
        case NiFpgaEx_ResourceType_HostToTargetFifoI8:
            return I8();
        case NiFpgaEx_ResourceType_IndicatorU8:
        case NiFpgaEx_ResourceType_ControlU8:
        case NiFpgaEx_ResourceType_IndicatorArrayU8:
        case NiFpgaEx_ResourceType_ControlArrayU8:
        case NiFpgaEx_ResourceType_TargetToHostFifoU8:
        case NiFpgaEx_ResourceType_HostToTargetFifoU8:
            return U8();
        case NiFpgaEx_ResourceType_IndicatorI16:
        case NiFpgaEx_ResourceType_ControlI16:
        case NiFpgaEx_ResourceType_IndicatorArrayI16:
        case NiFpgaEx_ResourceType_ControlArrayI16:
        case NiFpgaEx_ResourceType_TargetToHostFifoI16:
        case NiFpgaEx_ResourceType_HostToTargetFifoI16:
            return I16();
        case NiFpgaEx_ResourceType_IndicatorU16:
        case NiFpgaEx_ResourceType_ControlU16:
        case NiFpgaEx_ResourceType_IndicatorArrayU16:
        case NiFpgaEx_ResourceType_ControlArrayU16:
        case NiFpgaEx_ResourceType_TargetToHostFifoU16:
        case NiFpgaEx_ResourceType_HostToTargetFifoU16:
            return U16();
        case NiFpgaEx_ResourceType_IndicatorI32:
        case NiFpgaEx_ResourceType_ControlI32:
        case NiFpgaEx_ResourceType_IndicatorArrayI32:
        case NiFpgaEx_ResourceType_ControlArrayI32:
        case NiFpgaEx_ResourceType_TargetToHostFifoI32:
        case NiFpgaEx_ResourceType_HostToTargetFifoI32:
            return I32();
        case NiFpgaEx_ResourceType_IndicatorU32:
        case NiFpgaEx_ResourceType_ControlU32:
        case NiFpgaEx_ResourceType_IndicatorArrayU32:
        case NiFpgaEx_ResourceType_ControlArrayU32:
        case NiFpgaEx_ResourceType_TargetToHostFifoU32:
        case NiFpgaEx_ResourceType_HostToTargetFifoU32:
            return U32();
        case NiFpgaEx_ResourceType_IndicatorI64:
        case NiFpgaEx_ResourceType_ControlI64:
        case NiFpgaEx_ResourceType_IndicatorArrayI64:
        case NiFpgaEx_ResourceType_ControlArrayI64:
        case NiFpgaEx_ResourceType_TargetToHostFifoI64:
        case NiFpgaEx_ResourceType_HostToTargetFifoI64:
            return I64();
        case NiFpgaEx_ResourceType_IndicatorU64:
        case NiFpgaEx_ResourceType_ControlU64:
        case NiFpgaEx_ResourceType_IndicatorArrayU64:
        case NiFpgaEx_ResourceType_ControlArrayU64:
        case NiFpgaEx_ResourceType_TargetToHostFifoU64:
        case NiFpgaEx_ResourceType_HostToTargetFifoU64:
            return U64();
        case NiFpgaEx_ResourceType_IndicatorSgl:
        case NiFpgaEx_ResourceType_ControlSgl:
        case NiFpgaEx_ResourceType_IndicatorArraySgl:
        case NiFpgaEx_ResourceType_ControlArraySgl:
        case NiFpgaEx_ResourceType_TargetToHostFifoSgl:
        case NiFpgaEx_ResourceType_HostToTargetFifoSgl:
            return Sgl();
        case NiFpgaEx_ResourceType_IndicatorDbl:
        case NiFpgaEx_ResourceType_ControlDbl:
        case NiFpgaEx_ResourceType_IndicatorArrayDbl:
        case NiFpgaEx_ResourceType_ControlArrayDbl:
        case NiFpgaEx_ResourceType_TargetToHostFifoDbl:
        case NiFpgaEx_ResourceType_HostToTargetFifoDbl:
            return Dbl();
        default:
            // someone passed a bad resource type
            assert(false);
            return UnsupportedType();
    }
}

bool isIndicator(const NiFpgaEx_ResourceType type)
{
    switch (type) {
        case NiFpgaEx_ResourceType_IndicatorBool:
        case NiFpgaEx_ResourceType_IndicatorI8:
        case NiFpgaEx_ResourceType_IndicatorU8:
        case NiFpgaEx_ResourceType_IndicatorI16:
        case NiFpgaEx_ResourceType_IndicatorU16:
        case NiFpgaEx_ResourceType_IndicatorI32:
        case NiFpgaEx_ResourceType_IndicatorU32:
        case NiFpgaEx_ResourceType_IndicatorI64:
        case NiFpgaEx_ResourceType_IndicatorU64:
        case NiFpgaEx_ResourceType_IndicatorSgl:
        case NiFpgaEx_ResourceType_IndicatorDbl:
        case NiFpgaEx_ResourceType_IndicatorArrayBool:
        case NiFpgaEx_ResourceType_IndicatorArrayI8:
        case NiFpgaEx_ResourceType_IndicatorArrayU8:
        case NiFpgaEx_ResourceType_IndicatorArrayI16:
        case NiFpgaEx_ResourceType_IndicatorArrayU16:
        case NiFpgaEx_ResourceType_IndicatorArrayI32:
        case NiFpgaEx_ResourceType_IndicatorArrayU32:
        case NiFpgaEx_ResourceType_IndicatorArrayI64:
        case NiFpgaEx_ResourceType_IndicatorArrayU64:
        case NiFpgaEx_ResourceType_IndicatorArraySgl:
        case NiFpgaEx_ResourceType_IndicatorArrayDbl:
            return true;
        default:
            return false;
    }
}

bool isControl(const NiFpgaEx_ResourceType type)
{
    switch (type) {
        case NiFpgaEx_ResourceType_ControlBool:
        case NiFpgaEx_ResourceType_ControlI8:
        case NiFpgaEx_ResourceType_ControlU8:
        case NiFpgaEx_ResourceType_ControlI16:
        case NiFpgaEx_ResourceType_ControlU16:
        case NiFpgaEx_ResourceType_ControlI32:
        case NiFpgaEx_ResourceType_ControlU32:
        case NiFpgaEx_ResourceType_ControlI64:
        case NiFpgaEx_ResourceType_ControlU64:
        case NiFpgaEx_ResourceType_ControlSgl:
        case NiFpgaEx_ResourceType_ControlDbl:
        case NiFpgaEx_ResourceType_ControlArrayBool:
        case NiFpgaEx_ResourceType_ControlArrayI8:
        case NiFpgaEx_ResourceType_ControlArrayU8:
        case NiFpgaEx_ResourceType_ControlArrayI16:
        case NiFpgaEx_ResourceType_ControlArrayU16:
        case NiFpgaEx_ResourceType_ControlArrayI32:
        case NiFpgaEx_ResourceType_ControlArrayU32:
        case NiFpgaEx_ResourceType_ControlArrayI64:
        case NiFpgaEx_ResourceType_ControlArrayU64:
        case NiFpgaEx_ResourceType_ControlArraySgl:
        case NiFpgaEx_ResourceType_ControlArrayDbl:
            return true;
        default:
            return false;
    }
}

bool isRegister(const NiFpgaEx_ResourceType type)
{
    return isIndicator(type) || isControl(type);
}

bool isArray(const NiFpgaEx_ResourceType type)
{
    switch (type) {
        case NiFpgaEx_ResourceType_IndicatorArrayBool:
        case NiFpgaEx_ResourceType_IndicatorArrayI8:
        case NiFpgaEx_ResourceType_IndicatorArrayU8:
        case NiFpgaEx_ResourceType_IndicatorArrayI16:
        case NiFpgaEx_ResourceType_IndicatorArrayU16:
        case NiFpgaEx_ResourceType_IndicatorArrayI32:
        case NiFpgaEx_ResourceType_IndicatorArrayU32:
        case NiFpgaEx_ResourceType_IndicatorArrayI64:
        case NiFpgaEx_ResourceType_IndicatorArrayU64:
        case NiFpgaEx_ResourceType_IndicatorArraySgl:
        case NiFpgaEx_ResourceType_IndicatorArrayDbl:
        case NiFpgaEx_ResourceType_ControlArrayBool:
        case NiFpgaEx_ResourceType_ControlArrayI8:
        case NiFpgaEx_ResourceType_ControlArrayU8:
        case NiFpgaEx_ResourceType_ControlArrayI16:
        case NiFpgaEx_ResourceType_ControlArrayU16:
        case NiFpgaEx_ResourceType_ControlArrayI32:
        case NiFpgaEx_ResourceType_ControlArrayU32:
        case NiFpgaEx_ResourceType_ControlArrayI64:
        case NiFpgaEx_ResourceType_ControlArrayU64:
        case NiFpgaEx_ResourceType_ControlArraySgl:
        case NiFpgaEx_ResourceType_ControlArrayDbl:
            return true;
        default:
            return false;
    }
}

bool isTargetToHostFifo(const NiFpgaEx_ResourceType type)
{
    switch (type) {
        case NiFpgaEx_ResourceType_TargetToHostFifoBool:
        case NiFpgaEx_ResourceType_TargetToHostFifoI8:
        case NiFpgaEx_ResourceType_TargetToHostFifoU8:
        case NiFpgaEx_ResourceType_TargetToHostFifoI16:
        case NiFpgaEx_ResourceType_TargetToHostFifoU16:
        case NiFpgaEx_ResourceType_TargetToHostFifoI32:
        case NiFpgaEx_ResourceType_TargetToHostFifoU32:
        case NiFpgaEx_ResourceType_TargetToHostFifoI64:
        case NiFpgaEx_ResourceType_TargetToHostFifoU64:
        case NiFpgaEx_ResourceType_TargetToHostFifoSgl:
        case NiFpgaEx_ResourceType_TargetToHostFifoDbl:
            return true;
        default:
            return false;
    }
}

bool isHostToTargetFifo(const NiFpgaEx_ResourceType type)
{
    switch (type) {
        case NiFpgaEx_ResourceType_HostToTargetFifoBool:
        case NiFpgaEx_ResourceType_HostToTargetFifoI8:
        case NiFpgaEx_ResourceType_HostToTargetFifoU8:
        case NiFpgaEx_ResourceType_HostToTargetFifoI16:
        case NiFpgaEx_ResourceType_HostToTargetFifoU16:
        case NiFpgaEx_ResourceType_HostToTargetFifoI32:
        case NiFpgaEx_ResourceType_HostToTargetFifoU32:
        case NiFpgaEx_ResourceType_HostToTargetFifoI64:
        case NiFpgaEx_ResourceType_HostToTargetFifoU64:
        case NiFpgaEx_ResourceType_HostToTargetFifoSgl:
        case NiFpgaEx_ResourceType_HostToTargetFifoDbl:
            return true;
        default:
            return false;
    }
}

bool isDmaFifo(const NiFpgaEx_ResourceType type)
{
    return isTargetToHostFifo(type) || isHostToTargetFifo(type);
}

} // namespace nirio
