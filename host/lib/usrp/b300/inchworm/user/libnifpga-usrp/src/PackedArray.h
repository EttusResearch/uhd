/*
 * Copyright (c) 2020 National Instruments
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
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

namespace nirio {

namespace {

template <int logicalBits>
struct BitsToType;

template <>
struct BitsToType<1>
{
    typedef uint8_t type;
};

template <>
struct BitsToType<8>
{
    typedef uint8_t type;
};

template <>
struct BitsToType<16>
{
    typedef uint16_t type;
};

// Helpers for dealing with packing elements with a bitwidth < 32
// into a uint32_t array.
template <int logicalBits, typename T = typename BitsToType<logicalBits>::type>
struct PackedArray
{
    static void set(uint32_t* array, size_t index, T value)
    {
        const auto shift = bitIndex(index);

        array += arrayIndex(index);
        *array &= ~(mask << shift);
        *array |= (static_cast<uint32_t>(value) & mask) << shift;
    }

    static T get(uint32_t* array, size_t index)
    {
        return (array[arrayIndex(index)] >> bitIndex(index)) & mask;
    }

    static void rightJustify(uint32_t* in, size_t count)
    {
        if (needJustify(count))
            *in >>= justifyShift(count);
    }

    static void leftJustify(uint32_t* in, size_t count)
    {
        if (needJustify(count))
            *in <<= justifyShift(count);
    }

    static T* logicalCast(void* in)
    {
        return static_cast<T*>(in);
    }

    static const T* logicalCast(const void* in)
    {
        return static_cast<const T*>(in);
    }

    static bool needJustify(size_t count)
    {
        return count < elements;
    }

private:
    static size_t arrayIndex(size_t index)
    {
        return index * logicalBits / packedBits;
    }

    static size_t bitIndex(size_t index)
    {
        return packedBits - (((1 + index) * logicalBits) % packedBits);
    }

    static size_t justifyShift(size_t count)
    {
        return (elements - count) * logicalBits;
    }

    static const size_t packedBits = std::numeric_limits<uint32_t>::digits;
    static const size_t elements   = packedBits / logicalBits;
    static const uint32_t mask     = (1UL << logicalBits) - 1;

    static_assert((packedBits % logicalBits) == 0);
    static_assert(logicalBits < 32);
};

} // namespace

template <int logicalBits>
static size_t packedArraySize(size_t size)
{
    return (size * logicalBits + 31) / 32;
}

template <int logicalBits>
static void packArray(uint32_t* out, const void* in_, size_t inSize)
{
    using pa = PackedArray<logicalBits>;
    auto in  = pa::logicalCast(in_);

    for (size_t i = 0; i < inSize; i++)
        pa::set(out, i, in[i]);

    pa::rightJustify(out, inSize);
}

template <>
void packArray<32>(uint32_t* out, const void* in, size_t inSize)
{
    memcpy(out, in, inSize * 4);
}

template <>
void packArray<64>(uint32_t* out, const void* in, size_t inSize)
{
    auto in32 = static_cast<const uint32_t*>(in);
    inSize *= 2;

    for (size_t i = 0; i < inSize; i += 2) {
        out[i]     = in32[i + 1];
        out[i + 1] = in32[i];
    }
}

template <int logicalBits>
static void unpackArray(uint32_t* in, void* out_, size_t outSize)
{
    using pa = PackedArray<logicalBits>;
    auto out = pa::logicalCast(out_);

    pa::leftJustify(in, outSize);

    for (size_t i = 0; i < outSize; i++)
        out[i] = pa::get(in, i);
}

template <>
void unpackArray<32>(uint32_t* in, void* out, size_t outSize)
{
    memcpy(out, in, outSize * 4);
}

template <>
void unpackArray<64>(uint32_t* in, void* out, size_t outSize)
{
    auto out32 = static_cast<uint32_t*>(out);
    outSize *= 2;

    for (size_t i = 0; i < outSize; i += 2) {
        out32[i]     = in[i + 1];
        out32[i + 1] = in[i];
    }
}

} // namespace nirio
