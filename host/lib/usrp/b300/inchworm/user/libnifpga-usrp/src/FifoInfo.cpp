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

#include "FifoInfo.h"
#include <cassert> // assert

namespace nirio {

FifoInfo::FifoInfo(const std::string& name,
    const Type type,
    const NiFpgaEx_DmaFifo number,
    const uint32_t controlSet,
    const bool hostToTarget,
    const std::string& baseAddressTag)
    : ResourceInfo(name, type)
    , number(number)
    , controlSet(controlSet)
    , hostToTarget(hostToTarget)
    , baseAddressTag(baseAddressTag)
    , offset(0)
    , offsetIsSet(false)

{
}

NiFpgaEx_DmaFifo FifoInfo::getNumber() const
{
    return number;
}

uint32_t FifoInfo::getControlSet() const
{
    return controlSet;
}

bool FifoInfo::isTargetToHost() const
{
    return !hostToTarget;
}

bool FifoInfo::isHostToTarget() const
{
    return hostToTarget;
}

const std::string& FifoInfo::getBaseAddressTag() const
{
    return baseAddressTag;
}

uint32_t FifoInfo::getOffset() const
{
    return offset;
}

void FifoInfo::setOffset(const uint32_t offset)
{
    this->offset = offset;
    offsetIsSet  = true;
}

bool FifoInfo::isOffsetSet() const
{
    return offsetIsSet;
}

bool FifoInfo::matches(const std::string& name, const NiFpgaEx_ResourceType type) const
{
    if (type == NiFpgaEx_ResourceType_Any)
        return ResourceInfo::matches(name, type);

    return ResourceInfo::matches(name, type) && nirio::isDmaFifo(type)
           && nirio::isTargetToHostFifo(type) == isTargetToHost();
}

} // namespace nirio
