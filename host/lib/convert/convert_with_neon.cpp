//
// Copyright 2011-2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <arm_neon.h>

using namespace uhd::convert;

DECLARE_CONVERTER(fc32, 1, sc16_item32_le, 1, PRIORITY_SIMD){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    size_t i;

    float32x4_t Q0 = vdupq_n_f32(float(scale_factor));
    for (i=0; i < (nsamps & ~0x03); i+=2) {
        float32x4_t Q1 = vld1q_f32(reinterpret_cast<const float *>(&input[i]));
        float32x4_t Q2 = vmulq_f32(Q1, Q0);
        int32x4_t Q3 = vcvtq_s32_f32(Q2);
        int16x4_t D8 = vmovn_s32(Q3);
        int16x4_t D9 = vrev32_s16(D8);
        vst1_s16((reinterpret_cast<int16_t *>(&output[i])), D9);
    }

    xx_to_item32_sc16<uhd::htowx>(input+i, output+i, nsamps-i, scale_factor);
}

DECLARE_CONVERTER(sc16_item32_le, 1, fc32, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    fc32_t *output = reinterpret_cast<fc32_t *>(outputs[0]);

    size_t i;

    float32x4_t Q1 = vdupq_n_f32(float(scale_factor));
    for (i=0; i < (nsamps & ~0x03); i+=2) {
        int16x4_t D0 = vld1_s16(reinterpret_cast<const int16_t *>(&input[i]));
        int16x4_t D1 = vrev32_s16(D0);
        int32x4_t Q2 = vmovl_s16(D1);
        float32x4_t Q3 = vcvtq_f32_s32(Q2);
        float32x4_t Q4 = vmulq_f32(Q3, Q1);
        vst1q_f32((reinterpret_cast<float *>(&output[i])), Q4);
    }

    item32_sc16_to_xx<uhd::htowx>(input+i, output+i, nsamps-i, scale_factor);
}
