//
// Copyright 2014 Ettus Research LLC
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
