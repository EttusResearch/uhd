//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

	.arch armv7-a
	.fpu neon
	.syntax unified
	.text
	.align 2
	.global neon_item32_sc16_swap_16n
	.type neon_item32_sc16_swap_16n, %function
neon_item32_sc16_swap_16n:
.loop_swap:
	vld2.16      {q0, q1}, [r0]!
	vld2.16      {q2, q3}, [r0]!
	vswp               q0, q1
	vswp               q2, q3
	vst2.16      {q0, q1}, [r1]!
	vst2.16      {q2, q3}, [r1]!
	subs               r2, #1
	bne          .loop_swap
	bx                 lr
	.size neon_item32_sc16_swap_16n, .-neon_item32_sc16_swap_16n
	.section .note.GNU-stack,"",%progbits
