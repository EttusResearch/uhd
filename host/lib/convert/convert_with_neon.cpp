//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <arm_neon.h>

extern "C" {
void neon_item32_sc16_swap_16n(void *, void *, int iter);
}

static const int SIMD_WIDTH = 16;

using namespace uhd::convert;

DECLARE_CONVERTER(fc32, 1, sc16_item32_le, 1, PRIORITY_SIMD){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    size_t i;

    float32x4_t Q0 = vdupq_n_f32(float(scale_factor));
    for (i=0; i < (nsamps & ~0x0f); i+=8) {
        float32x4_t Q1 = vld1q_f32(reinterpret_cast<const float *>(&input[i]));
        float32x4_t Q4 = vld1q_f32(reinterpret_cast<const float *>(&input[i+2]));
        float32x4_t Q7 = vld1q_f32(reinterpret_cast<const float *>(&input[i+4]));
        float32x4_t Q10 = vld1q_f32(reinterpret_cast<const float *>(&input[i+6]));

        float32x4_t Q2 = vmulq_f32(Q1, Q0);
        int32x4_t Q3 = vcvtq_s32_f32(Q2);
        int16x4_t D8 = vmovn_s32(Q3);
        int16x4_t D9 = vrev32_s16(D8);
        vst1_s16((reinterpret_cast<int16_t *>(&output[i])), D9);

        float32x4_t Q5 = vmulq_f32(Q4, Q0);
        int32x4_t Q6 = vcvtq_s32_f32(Q5);
        int16x4_t D10 = vmovn_s32(Q6);
        int16x4_t D11 = vrev32_s16(D10);
        vst1_s16((reinterpret_cast<int16_t *>(&output[i+2])), D11);

        float32x4_t Q8 = vmulq_f32(Q7, Q0);
        int32x4_t Q9 = vcvtq_s32_f32(Q8);
        int16x4_t D12 = vmovn_s32(Q9);
        int16x4_t D13 = vrev32_s16(D12);
        vst1_s16((reinterpret_cast<int16_t *>(&output[i+4])), D13);

        float32x4_t Q11 = vmulq_f32(Q10, Q0);
        int32x4_t Q13 = vcvtq_s32_f32(Q11);
        int16x4_t D14 = vmovn_s32(Q13);
        int16x4_t D15 = vrev32_s16(D14);
        vst1_s16((reinterpret_cast<int16_t *>(&output[i+6])), D15);
    }

    xx_to_item32_sc16<uhd::htowx>(input+i, output+i, nsamps-i, scale_factor);
}

DECLARE_CONVERTER(sc16_item32_le, 1, fc32, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    fc32_t *output = reinterpret_cast<fc32_t *>(outputs[0]);

    size_t i;

    float32x4_t Q1 = vdupq_n_f32(float(scale_factor));
    for (i=0; i < (nsamps & ~0xf); i+=8) {
        int16x4_t D0 = vld1_s16(reinterpret_cast<const int16_t *>(&input[i]));
        int16x4_t D2 = vld1_s16(reinterpret_cast<const int16_t *>(&input[i+2]));
        int16x4_t D4 = vld1_s16(reinterpret_cast<const int16_t *>(&input[i+4]));
        int16x4_t D6 = vld1_s16(reinterpret_cast<const int16_t *>(&input[i+6]));

        int16x4_t D1 = vrev32_s16(D0);
        int32x4_t Q2 = vmovl_s16(D1);
        float32x4_t Q3 = vcvtq_f32_s32(Q2);
        float32x4_t Q4 = vmulq_f32(Q3, Q1);
        vst1q_f32((reinterpret_cast<float *>(&output[i])), Q4);

        int16x4_t D3 = vrev32_s16(D2);
        int32x4_t Q5 = vmovl_s16(D3);
        float32x4_t Q6 = vcvtq_f32_s32(Q5);
        float32x4_t Q7 = vmulq_f32(Q6, Q1);
        vst1q_f32((reinterpret_cast<float *>(&output[i+2])), Q7);

        int16x4_t D5 = vrev32_s16(D4);
        int32x4_t Q8 = vmovl_s16(D5);
        float32x4_t Q9 = vcvtq_f32_s32(Q8);
        float32x4_t Q10 = vmulq_f32(Q9, Q1);
        vst1q_f32((reinterpret_cast<float *>(&output[i+4])), Q10);

        int16x4_t D7 = vrev32_s16(D6);
        int32x4_t Q11 = vmovl_s16(D7);
        float32x4_t Q12 = vcvtq_f32_s32(Q11);
        float32x4_t Q13 = vmulq_f32(Q12, Q1);
        vst1q_f32((reinterpret_cast<float *>(&output[i+6])), Q13);
    }

    item32_sc16_to_xx<uhd::htowx>(input+i, output+i, nsamps-i, scale_factor);
}

DECLARE_CONVERTER(sc16, 1, sc16_item32_le, 1, PRIORITY_SIMD){
    const sc16_t *input = reinterpret_cast<const sc16_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    size_t i = nsamps / SIMD_WIDTH;

    if (i)
        neon_item32_sc16_swap_16n((void *) input, (void *) output, i);

    i *= SIMD_WIDTH;

    xx_to_item32_sc16<uhd::htowx>(input+i, output+i, nsamps-i, scale_factor);
}

DECLARE_CONVERTER(sc16_item32_le, 1, sc16, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    sc16_t *output = reinterpret_cast<sc16_t *>(outputs[0]);

    size_t i = nsamps / SIMD_WIDTH;

    if (i)
        neon_item32_sc16_swap_16n((void *) input, (void *) output, i);

    i *= SIMD_WIDTH;

    item32_sc16_to_xx<uhd::wtohx>(input+i, output+i, nsamps-i, scale_factor);
}
