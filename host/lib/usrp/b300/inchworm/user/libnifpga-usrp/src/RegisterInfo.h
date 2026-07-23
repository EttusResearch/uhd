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

#include "ResourceInfo.h"
#include <vector> // std::vector

namespace nirio {

/**
 * Description of a control or indicator as parsed from a bitfile.
 */
class RegisterInfo : public ResourceInfo
{
public:
    RegisterInfo(const std::string& name,
        Type type,
        NiFpgaEx_Register offset,
        bool control,
        bool array,
        bool accessMayTimeout);

    /**
     * Gets the offset of this register.
     *
     * @return offset of this register
     */
    NiFpgaEx_Register getOffset() const;

    /**
     * Gets whether this is an indicator as opposed to a control.
     *
     * @return whether this is an indicator
     */
    bool isIndicator() const;

    /**
     * Gets whether this is a control as opposed to an indicator.
     *
     * @return whether this is a control
     */
    bool isControl() const;

    /**
     * Gets whether this is an array control or indicator.
     *
     * @return whether this is an array
     */
    bool isArray() const;

    /**
     * Gets whether reading or writing may timeout.
     *
     * @return whether reading or writing may timeout
     */
    bool isAccessMayTimeout() const;

    virtual bool matches(const std::string& name, NiFpgaEx_ResourceType type) const;

protected:
    // NOTE: members can't be const or this can't be used in std::vector, etc.
    NiFpgaEx_Register offset;
    bool indicator;
    bool array;
    bool accessMayTimeout;
};

typedef std::vector<RegisterInfo> RegisterInfoVector;

} // namespace nirio
