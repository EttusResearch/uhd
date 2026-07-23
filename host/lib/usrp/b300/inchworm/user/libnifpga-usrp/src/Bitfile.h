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

#include "FifoInfo.h"
#include "RegisterInfo.h"
#include "Status.h"
#include <memory> // std::unique_ptr

namespace nirio {

/**
 * A LabVIEW FPGA bitfile object, parsed from a *.lvbitx file path.
 */
class Bitfile
{
public:
    explicit Bitfile(const std::string& path);

    const std::string& getPath() const;

    const std::string& getSignature() const;

    const std::string& getTargetClass() const;

    std::vector<char> getBitstream() const;

    NiFpgaEx_Register getBaseAddressOnDevice() const;

    NiFpgaEx_Register getSignatureRegister() const;

    NiFpgaEx_Register getControlRegister() const;

    NiFpgaEx_Register getResetRegister() const;

    NiFpgaEx_Register getIrqEnableRegister() const;

    NiFpgaEx_Register getIrqMaskRegister() const;

    NiFpgaEx_Register getIrqStatusRegister() const;

    bool isFifosSupportClear() const;

    bool isFifosSupportBridgeFlush() const;

    bool isResetAutoClears() const;

    bool isAutoRunWhenDownloaded() const;

    const RegisterInfoVector& getRegisters() const;

    const FifoInfoVector& getFifos() const;

    uint32_t getBitstreamVersion() const;

private:
    const std::string path;
    std::string signature;
    std::string targetClass;
    NiFpgaEx_Register baseAddressOnDevice;
    NiFpgaEx_Register signatureRegister;
    NiFpgaEx_Register controlRegister;
    NiFpgaEx_Register resetRegister;
    NiFpgaEx_Register irqEnable;
    NiFpgaEx_Register irqMask;
    NiFpgaEx_Register irqStatus;
    bool fifosSupportClear;
    bool fifosSupportBridgeFlush;
    bool resetAutoClears;
    bool autoRunWhenDownloaded;
    RegisterInfoVector registers;
    FifoInfoVector fifos;
    uint32_t bitstreamVersion;

    Bitfile(const Bitfile&)            = delete;
    Bitfile& operator=(const Bitfile&) = delete;
};

} // namespace nirio
