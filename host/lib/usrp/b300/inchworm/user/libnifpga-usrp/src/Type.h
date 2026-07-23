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

#include "NiFpga.h"
#include <type_traits> // std::is_signed

namespace nirio {

/**
 * Encapsulates a supported LabVIEW data type. This is non-templatized so that
 * there is a common base class for all types defined below.
 */
class Type
{
public:
    /**
     * The number of bits required to store all valid values of this type. So
     * Bools are _1_, U8s are 8, I64s are 64, etc.
     *
     * @return number of bits required to store all valid values
     */
    size_t getLogicalBits() const;

    /**
     * The size in bytes of an actual element as transferred between the C API
     * boundary and the user/kernel boundary. This should be equal to
     * sizeof(Type), so Bools are 1, U8s are 1, I64s are 8, etc.
     *
     * @return size in bytes of an actual element
     */
    size_t getElementBytes() const;

    /**
     * Whether this type is signed instead of unsigned. So Bool is false, U8
     * is false, I64 is true, etc.
     *
     * @return whether this type is signed
     */
    bool isSigned() const;

    /**
     * Whether two types are exactly the same type.
     *
     * @return whether two types are exactly the same type
     */
    bool operator==(const Type& other) const;

    /**
     * Whether two types are not exactly the same type.
     *
     * @return whether two types are not exactly the same type
     */
    bool operator!=(const Type& other) const;

protected:
    Type(size_t logicalBits, size_t elementBytes, bool isSigned);

    // NOTE: members can't be const or this can't be used in std::vector, etc.
    size_t logicalBits;
    size_t elementBytes;
    bool typeIsSigned; // had to disambiguate name from function and keyword
};

/**
 * Encapsulates a supported LabVIEW data type. This is templatized so that the
 * Type typedef is accessible, and also so that various properties can be
 * accessed as statics instead of function calls.
 *
 * @tparam T actual C++ element type
 * @tparam LogicalBits number of bits required to store all valid values
 */
template <typename T, size_t LogicalBits = sizeof(T) * 8, size_t ElementBytes = sizeof(T)>
class TypeTemplate : public Type
{
public:
    /**
     * Actual C++ element type.
     */
    typedef T CType;

    /**
     * The number of bits required to store all logical values of this type.
     * Bools are 1, U8s are 8, I64s are 64, etc.
     */
    static const auto logicalBits = LogicalBits;

    /**
     * The minimal size in bytes required to hold an actual element value.
     * Bools are 1, U8s are 1, I64s are 8, etc.
     */
    static const auto elementBytes = ElementBytes;

    /**
     * Whether this type is signed instead of unsigned. Bool is false, U8 is
     * false, I64 is true, etc.
     */
    static const bool isSigned = std::is_signed<CType>::value;

    TypeTemplate() : Type(logicalBits, elementBytes, isSigned) {}
};

typedef TypeTemplate<void, 0, 0> UnsupportedType;
typedef TypeTemplate<NiFpga_Bool, 1> Bool;
typedef TypeTemplate<int8_t> I8;
typedef TypeTemplate<uint8_t> U8;
typedef TypeTemplate<int16_t> I16;
typedef TypeTemplate<uint16_t> U16;
typedef TypeTemplate<int32_t> I32;
typedef TypeTemplate<uint32_t> U32;
typedef TypeTemplate<int64_t> I64;
typedef TypeTemplate<uint64_t> U64;
typedef TypeTemplate<float> Sgl;
typedef TypeTemplate<double> Dbl;

Type getType(NiFpgaEx_ResourceType type);

bool isIndicator(NiFpgaEx_ResourceType type);

bool isControl(NiFpgaEx_ResourceType type);

bool isRegister(NiFpgaEx_ResourceType type);

bool isArray(NiFpgaEx_ResourceType type);

bool isTargetToHostFifo(NiFpgaEx_ResourceType type);

bool isHostToTargetFifo(NiFpgaEx_ResourceType type);

bool isDmaFifo(NiFpgaEx_ResourceType type);

} // namespace nirio
