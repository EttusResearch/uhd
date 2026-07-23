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

ResourceInfo::ResourceInfo(const std::string& name, const Type type)
    : name(name), type(type)
{
}

const std::string& ResourceInfo::getName() const
{
    return name;
}

const Type& ResourceInfo::getType() const
{
    return type;
}

bool ResourceInfo::matches(
    const std::string& name, const NiFpgaEx_ResourceType type) const
{
    if (type == NiFpgaEx_ResourceType_Any)
        return this->name == name;

    // only matches name and type, so derived class much match the rest
    return this->name == name && this->type == nirio::getType(type);
}

} // namespace nirio
