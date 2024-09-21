//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_SIMD_FEATURES_HPP
#define INCLUDED_LIBUHD_SIMD_FEATURES_HPP

#include <uhd/config.hpp>
#include <uhd/utils/log.hpp>
#include <string>

namespace uhd { namespace convert {

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))

UHD_INLINE bool cpu_has_sse2()
{
    return __builtin_cpu_supports("sse2");
}

UHD_INLINE bool cpu_has_ssse3()
{
    return __builtin_cpu_supports("ssse3");
}

UHD_INLINE bool cpu_has_avx2()
{
    return __builtin_cpu_supports("avx2");
}

UHD_INLINE bool cpu_has_avx512f()
{
    return __builtin_cpu_supports("avx512f");
}

#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))

#include <intrin.h>

namespace detail {

inline void cpuid(int info[4], int func_id)
{
    __cpuid(info, func_id);
}

inline void cpuidex(int info[4], int func_id, int subfunc_id)
{
    __cpuidex(info, func_id, subfunc_id);
}

inline unsigned long long xgetbv(unsigned int index)
{
    return _xgetbv(index);
}

} // namespace detail

UHD_INLINE bool cpu_has_sse2()
{
    int info[4];
    detail::cpuid(info, 1);
    return (info[3] & (1 << 26)) != 0;
}

UHD_INLINE bool cpu_has_ssse3()
{
    int info[4];
    detail::cpuid(info, 1);
    return (info[2] & (1 << 9)) != 0;
}

UHD_INLINE bool cpu_has_avx2()
{
    int info[4];
    detail::cpuid(info, 1);

    bool os_uses_xsave = (info[2] & (1 << 27)) != 0;
    bool cpu_has_avx   = (info[2] & (1 << 28)) != 0;

    if (!os_uses_xsave || !cpu_has_avx)
        return false;

    // Check if OS supports AVX registers
    unsigned long long xcr0 = detail::xgetbv(0);
    if ((xcr0 & 0x6) != 0x6)
        return false;

    // Check AVX2 support
    detail::cpuidex(info, 7, 0);
    return (info[1] & (1 << 5)) != 0;
}

UHD_INLINE bool cpu_has_avx512f()
{
    int info[4];
    detail::cpuid(info, 1);

    // Check XSAVE support
    bool os_uses_xsave = (info[2] & (1 << 27)) != 0;
    if (!os_uses_xsave)
        return false;

    // Check if OS supports AVX-512 registers (ZMM, opmask)
    unsigned long long xcr0 = detail::xgetbv(0);
    if ((xcr0 & 0xe6) != 0xe6)
        return false;

    // Check AVX512F support
    detail::cpuidex(info, 7, 0);
    return (info[1] & (1 << 16)) != 0;
}

#else
// Fallback for other platforms (ARM, etc.) - SIMD not detected at runtime
// These will always return false, meaning SIMD converters won't be registered

UHD_INLINE bool cpu_has_sse2()
{
    return false;
}

UHD_INLINE bool cpu_has_ssse3()
{
    return false;
}

UHD_INLINE bool cpu_has_avx2()
{
    return false;
}

UHD_INLINE bool cpu_has_avx512f()
{
    return false;
}

#endif

inline std::string get_simd_capabilities_string()
{
    std::string caps;
    if (cpu_has_sse2()) {
        caps += "SSE2 ";
    }
    if (cpu_has_ssse3()) {
        caps += "SSSE3 ";
    }
    if (cpu_has_avx2()) {
        caps += "AVX2 ";
    }
    if (cpu_has_avx512f()) {
        caps += "AVX512F ";
    }
    if (caps.empty()) {
        caps = "(none)";
    }
    return caps;
}

inline void log_simd_capabilities()
{
    UHD_LOG_DEBUG("CONVERT",
        "Detected CPU SIMD capabilities: " << get_simd_capabilities_string());
}

inline void log_simd_converter_registered(
    const char* simd_type, const char* in_form, const char* out_form, int prio)
{
    // Suppress unused-parameter warnings when trace logging is disabled at compile time
    (void)simd_type;
    (void)in_form;
    (void)out_form;
    (void)prio;
    UHD_LOG_TRACE("CONVERT",
        simd_type << " converter registered: " << in_form << " -> " << out_form
                  << " (priority " << prio << ")");
}

}} // namespace uhd::convert

#endif /* INCLUDED_LIBUHD_SIMD_FEATURES_HPP */
