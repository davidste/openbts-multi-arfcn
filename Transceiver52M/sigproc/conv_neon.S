/*
 * Copyright (C) 2012 Thomas Tsou <tom@tsou.cc>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * See the COPYING file in the main directory for details.
 */

	.arch armv7-a
	.fpu neon
	.text
	.align 2
	.p2align 4,,15
	.global conv_real_neon4
	.type conv_real_neon4, %function
conv_real_neon4:
	vld2.32     {q0-q1}, [r1]
	push            {r4, lr}
	ldr              r4, =8
.loop4:
	vld2.32     {q2-q3}, [r0], r4
	vmul.f32         q4,   q2, q0
	vmul.f32         q5,   q3, q0
	vpadd.f32       d12,   d8, d9
	vpadd.f32       d13,  d10, d11
	vpadd.f32       d14,  d12, d13
	vst1.64       {d14}, [r2]!
	subs             r3,   r3, #1
	bne          .loop4
	pop             {r4, pc}
	.size conv_real_neon4, .-conv_real_neon4
	.align 2
	.p2align 4,,15
	.global conv_real_neon8
	.type conv_real_neon8, %function
conv_real_neon8:
	vld2.32     {q0-q1}, [r1]!
	vld2.32     {q2-q3}, [r1]
	push         {r4-r6, lr}
	add              r4, r0, #32
	ldr              r5, =8
	ldr              r6, =8
.loop8:
	vld2.32     {q4-q5}, [r0], r5
	vld2.32     {q6-q7}, [r4], r6
	vmul.f32         q8,   q4, q0
	vmul.f32         q9,   q6, q2
	vadd.f32        q10,   q8, q9
	vpadd.f32       d22,  d20, d21
	vmul.f32         q8,   q5, q0
	vmul.f32         q9,   q7, q2
	vadd.f32        q10,   q8, q9
	vpadd.f32       d23,  d20, d21
	vpadd.f32       d24,  d22, d23
	vst1.64       {d24}, [r2]!
	subs             r3,   r3, #1
	bne          .loop8
	pop          {r4-r6, pc}
	.size conv_real_neon8, .-conv_real_neon8
	.align 2
	.p2align 4,,15
	.global conv_real_neon16
	.type conv_real_neon16, %function
conv_real_neon16:
	vld2.32     {q0-q1}, [r1]!
	vld2.32     {q2-q3}, [r1]!
	vld2.32     {q4-q5}, [r1]!
	vld2.32     {q6-q7}, [r1]
	push        {r4-r10, lr}
	add              r4, r0, #32
	add              r5, r0, #64
	add              r6, r0, #96
	ldr              r7, =8
	ldr              r8, =8
	ldr              r9, =8
	ldr             r10, =8
.loop16:
	vld2.32     {q8-q9}, [r0], r7
	vld2.32   {q10-q11}, [r4], r8
	vld2.32   {q12-q13}, [r5], r9
	vld2.32   {q14-q15}, [r6], r10

	vmul.f32         q1,   q8, q0
	vmul.f32         q3,  q10, q2
	vmul.f32         q5,  q12, q4
	vmul.f32         q7,  q14, q6
	vadd.f32         q8,   q1, q3
	vadd.f32        q10,   q5, q7
	vadd.f32        q12,   q8, q10

	vmul.f32         q1,   q9, q0
	vmul.f32         q3,  q11, q2
	vmul.f32         q5,  q13, q4
	vmul.f32         q7,  q15, q6
	vadd.f32         q8,   q1, q3
	vadd.f32        q10,   q5, q7
	vadd.f32        q13,   q8, q10

	vpadd.f32        d2,  d24, d25
	vpadd.f32        d3,  d26, d27
	vpadd.f32        d6,   d2, d3
	vst1.64        {d6}, [r2]!
	subs             r3,   r3, #1
	bne          .loop16
	pop         {r4-r10, pc}
	.size conv_real_neon16, .-conv_real_neon16
	.section .note.GNU-stack,"",%progbits
