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
 * Description of a DMA FIFO as parsed from a bitfile.
 */
class FifoInfo : public ResourceInfo
{
public:
    FifoInfo(const std::string& name,
        Type type,
        NiFpgaEx_DmaFifo number,
        uint32_t controlSet,
        bool hostToTarget,
        const std::string& baseAddressTag);

    /**
     * Gets the FIFO number. This is sometimes referred to as the channel
     * number, though there's not necessarily a true DMA channel backing it.
     *
     * @return FIFO number
     */
    NiFpgaEx_DmaFifo getNumber() const;

    /**
     * Gets a unique number per FIFO that isn't necessarily the same as the
     * FIFO number.
     *
     * @return unique number per FIFO
     */
    uint32_t getControlSet() const;

    /**
     * Gets whether this is a target-to-host or "input" FIFO for reading.
     *
     * @return whether this is a target-to-host FIFO
     */
    bool isTargetToHost() const;

    /**
     * Gets whether this is a host-to-target or "output" FIFO for writing.
     *
     * @return whether this is a host-to-target FIFO
     */
    bool isHostToTarget() const;

    /**
     * Gets the BaseAddressTag, which is used to match Channels in the
     * DmaChannelAllocationList with RegisterBlocks in the RegisterBlockList.
     *
     * @return BaseAddressTag
     */
    const std::string& getBaseAddressTag() const;

    /**
     * Gets the Offset in the RegisterBlockList.
     *
     * @return Offset in the RegisterBlockList
     */
    uint32_t getOffset() const;

    /**
     * Sets the Offset as found in the RegisterBlockList.
     *
     * @param offset Offset as found in the RegisterBlockList
     */
    void setOffset(const uint32_t offset);

    /**
     * Gets whether setOffset has been called.
     *
     * @return whether setOffset has been called
     */
    bool isOffsetSet() const;

    virtual bool matches(const std::string& name, NiFpgaEx_ResourceType type) const;

protected:
    // NOTE: members can't be const or this can't be used in std::vector, etc.
    NiFpgaEx_DmaFifo number;
    uint32_t controlSet;
    bool hostToTarget;
    std::string baseAddressTag;
    uint32_t offset;
    bool offsetIsSet;
};

typedef std::vector<FifoInfo> FifoInfoVector;

} // namespace nirio
