//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <stdint.h>
#include <windows.h>
#include <winioctl.h>

// Windows-specific types for B310 driver compatibility
typedef uint64_t tAlignedU64;

// B310 Windows Driver IOCTL Constants
#define B310_WIN_IOCTL(type, function, access) \
    CTL_CODE((0x8000 + (type)), (0x800 + (function)), METHOD_BUFFERED, (access))

// Register access IOCTLs
#define B310_WIN_IOC_READ32  B310_WIN_IOCTL(1, 102, FILE_ANY_ACCESS)
#define B310_WIN_IOC_READ64  B310_WIN_IOCTL(1, 103, FILE_ANY_ACCESS)
#define B310_WIN_IOC_WRITE32 B310_WIN_IOCTL(1, 104, FILE_ANY_ACCESS)
#define B310_WIN_IOC_WRITE64 B310_WIN_IOCTL(1, 105, FILE_ANY_ACCESS)

// FIFO IOCTLs
#define B310_WIN_IOC_FIFO_STOP      B310_WIN_IOCTL(1, 110, FILE_ANY_ACCESS)
#define B310_WIN_IOC_FIFO_START     B310_WIN_IOCTL(1, 111, FILE_ANY_ACCESS)
#define B310_WIN_IOC_FIFO_SET_BUF   B310_WIN_IOCTL(1, 112, FILE_ANY_ACCESS)
#define B310_WIN_IOC_FIFO_ACQUIRE   B310_WIN_IOCTL(1, 113, FILE_ANY_ACCESS)
#define B310_WIN_IOC_FIFO_RELEASE   B310_WIN_IOCTL(1, 114, FILE_ANY_ACCESS)
#define B310_WIN_IOC_FIFO_GET_AVAIL B310_WIN_IOCTL(1, 115, FILE_ANY_ACCESS)

#define B310_WIN_IOC_GET_SESSION_COUNT B310_WIN_IOCTL(1, 120, FILE_ANY_ACCESS)

// Additional test IOCTLs (for reference/future use)
#define B310_WIN_IOC_TEST_INTERRUPTS 0x80012070 // winIOCtlCode(1, 4, 0)
#define B310_WIN_IOC_TEST_ATTRIBUTES 0x80012074 // winIOCtlCode(1, 5, 0)

// Data structures for B310 Windows driver IOCTLs

// Input structure for read32 operation
typedef struct tIn_b310Win_read32
{
    uint32_t offset;
} tIn_b310Win_read32;

// Output structure for read32 operation
typedef struct tOut_b310Win_read32
{
    uint32_t value;
} tOut_b310Win_read32;

// Input structure for read64 operation
typedef struct tIn_b310Win_read64
{
    uint32_t offset;
} tIn_b310Win_read64;

// Output structure for read64 operation
typedef struct tOut_b310Win_read64
{
    tAlignedU64 value;
} tOut_b310Win_read64;

// Input structure for write32 operation
typedef struct tIn_b310Win_write32
{
    uint32_t offset;
    uint32_t value;
} tIn_b310Win_write32;

// Input structure for write64 operation
typedef struct tIn_b310Win_write64
{
    uint32_t offset;
    tAlignedU64 value;
} tIn_b310Win_write64;

// Backwards compatibility typedefs for existing code
typedef tIn_b310Win_read32 tIn_B310_read32;
typedef tOut_b310Win_read32 tOut_B310_read32;
typedef tIn_b310Win_read64 tIn_B310_read64;
typedef tOut_b310Win_read64 tOut_B310_read64;
typedef tIn_b310Win_write32 tIn_B310_write32;
typedef tIn_b310Win_write64 tIn_B310_write64;

// FIFO operation structures

// Input structure for FIFO stop operation
typedef struct tIn_b310Win_fifoStop
{
    uint32_t channel;
    int32_t status;
} tIn_b310Win_fifoStop;

// Output structure for FIFO stop operation
typedef struct tOut_b310Win_fifoStop
{
    int32_t status;
} tOut_b310Win_fifoStop;

// Input structure for FIFO start operation
typedef struct tIn_B310_fifoStart
{
    uint32_t channel;
    int32_t status;
} tIn_B310_fifoStart;

// Output structure for FIFO start operation
typedef struct tOut_B310_fifoStart
{
    int32_t status;
} tOut_B310_fifoStart;

// FIFO operation structures
typedef struct tIn_b310Win_fifoSetBuf
{
    uint32_t channel;
    tAlignedU64 buffer;
    uint32_t fifoSizeBytes;
    int32_t status;
} tIn_b310Win_fifoSetBuf;

typedef struct tOut_b310Win_fifoSetBuf
{
    int32_t status;
} tOut_b310Win_fifoSetBuf;

typedef struct tIn_b310Win_fifoAcquire
{
    uint32_t channel;
    uint32_t timeoutMs;
    tAlignedU64 elements;
    tAlignedU64 available;
    uint32_t timedOut;
    int32_t status;
} tIn_b310Win_fifoAcquire;

typedef struct tOut_b310Win_fifoAcquire
{
    tAlignedU64 available;
    uint32_t timedOut;
    int32_t status;
} tOut_b310Win_fifoAcquire;

typedef struct tIn_b310Win_fifoRelease
{
    uint32_t channel;
    tAlignedU64 elements;
    int32_t status;
} tIn_b310Win_fifoRelease;

typedef struct tOut_b310Win_fifoRelease
{
    int32_t status;
} tOut_b310Win_fifoRelease;

typedef struct tIn_b310Win_fifoGetAvail
{
    uint32_t channel;
    tAlignedU64 available;
    int32_t status;
} tIn_b310Win_fifoGetAvail;

typedef struct tOut_b310Win_fifoGetAvail
{
    tAlignedU64 available;
    int32_t status;
} tOut_b310Win_fifoGetAvail;

// Backwards compatibility typedefs for FIFO operations
typedef tIn_b310Win_fifoStop tIn_B310_fifoStop;
typedef tOut_b310Win_fifoStop tOut_B310_fifoStop;
typedef tIn_b310Win_fifoSetBuf tIn_B310_fifoSetBuf;
typedef tOut_b310Win_fifoSetBuf tOut_B310_fifoSetBuf;
typedef tIn_b310Win_fifoAcquire tIn_B310_fifoAcquire;
typedef tOut_b310Win_fifoAcquire tOut_B310_fifoAcquire;
typedef tIn_b310Win_fifoRelease tIn_B310_fifoRelease;
typedef tOut_b310Win_fifoRelease tOut_B310_fifoRelease;
typedef tIn_b310Win_fifoGetAvail tIn_B310_fifoGetAvail;
typedef tOut_b310Win_fifoGetAvail tOut_B310_fifoGetAvail;

// Output structure for get session count operation
typedef struct tOut_b310Win_get_session_count
{
    uint32_t count;
} tOut_b310Win_get_session_count;

// Backwards compatibility typedefs for session count operation
typedef tOut_b310Win_get_session_count tOut_B310_get_session_count;
