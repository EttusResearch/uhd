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

#include "RegisterInfo.h"
#include <cassert> // assert

namespace nirio {

RegisterInfo::RegisterInfo(const std::string& name,
    const Type type,
    const NiFpgaEx_Register offset,
    const bool indicator,
    const bool array,
    const bool accessMayTimeout)
    : ResourceInfo(name, type)
    , offset(offset)
    , indicator(indicator)
    , array(array)
    , accessMayTimeout(accessMayTimeout)
{
}

NiFpgaEx_Register RegisterInfo::getOffset() const
{
    return offset;
}

bool RegisterInfo::isIndicator() const
{
    return indicator;
}

bool RegisterInfo::isControl() const
{
    return !indicator;
}

bool RegisterInfo::isArray() const
{
    return array;
}

bool RegisterInfo::isAccessMayTimeout() const
{
    return accessMayTimeout;
}

bool RegisterInfo::matches(
    const std::string& name, const NiFpgaEx_ResourceType type) const
{
    if (type == NiFpgaEx_ResourceType_Any)
        return ResourceInfo::matches(name, type);

    return ResourceInfo::matches(name, type) && nirio::isRegister(type)
           && nirio::isIndicator(type) == isIndicator()
           && nirio::isArray(type) == isArray();
}

} // namespace nirio
