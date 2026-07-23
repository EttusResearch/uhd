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

#pragma once

#include "Type.h"
#include <string> // std::string

namespace nirio {

/**
 * Description of a resource as parsed from a bitfile.
 */
class ResourceInfo
{
public:
    ResourceInfo(const std::string& name, Type type);

    virtual ~ResourceInfo() = default;

    /**
     * Gets the name of this resource.
     *
     * @return name of this resource
     */
    const std::string& getName() const;

    /**
     * Gets the type of this resource.
     *
     * @return type of this resource
     */
    const Type& getType() const;

    /**
     * Whether this resource matches a given resource type.
     *
     * @param type type to match
     * @return whether this resource matches a given resource type
     */
    virtual bool matches(const std::string& name, NiFpgaEx_ResourceType type) const = 0;

protected:
    // NOTE: members can't be const or this can't be used in std::vector, etc.
    std::string name; ///< Name of this resource.
    Type type; ///< Type of this resource.
};

} // namespace nirio
