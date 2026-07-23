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
#include "NiFpga.h"
#include <cassert> // assert
#include <cerrno> // errno

namespace nirio {

/**
 * Class used for checking and modifying the current status.
 */
class Status
{
public:
    typedef NiFpga_Status Code;

    /**
     * Creates a new status as a success or a specified status code.
     *
     * @param code status code to initialize to
     */
    explicit Status(const Code code = NiFpga_Status_Success) : code(code) {}

    /**
     * Resets the status back to a success or a specified status code.
     *
     * @param code status code to reset to
     */
    void reset(const Code code = NiFpga_Status_Success)
    {
        this->code = code;
    }

    /**
     * @return whether the status is an error
     */
    bool isError() const
    {
        return NiFpga_IsError(code);
    }

    /**
     * @return whether the status is not an error
     */
    bool isNotError() const
    {
        return NiFpga_IsNotError(code);
    }

    /**
     * @return whether the status is a warning
     */
    bool isWarning() const
    {
        return code > 0;
    }

    /**
     * @return whether the status is a success
     */
    bool isSuccess() const
    {
        return code == NiFpga_Status_Success;
    }

    /**
     * Conditionally sets a new status code if it was more severe. Warnings
     * and errors will override successes. Errors will override successes and
     * warnings. Nothing will override an error unless it is cleared.
     *
     * @param code new status code
     * @return status code value of the resulting status
     */
    Code merge(const Code code)
    {
        return NiFpga_MergeStatus(&this->code, code);
    }

    /**
     * @return status code value of the current status
     */
    Code getCode() const
    {
        return code;
    }

    /**
     * @return status code value of the current status
     */
    operator Code() const
    {
        return code;
    }

private:
    Code code;
};

} // namespace nirio
