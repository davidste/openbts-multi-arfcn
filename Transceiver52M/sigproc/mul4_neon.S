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
	.global mul_cx_neon4
	.type mul_cx_neon4, %function
mul_cx_neon4:
	vld2.32     {q0-q1}, [r0]
	vld2.32     {q2-q3}, [r1]
	vmul.f32         q4, q0, q2
	vmul.f32         q5, q1, q3
	vsub.f32         q6, q4, q5
	vmul.f32         q4, q0, q3
	vmul.f32         q5, q2, q1
	vadd.f32         q7, q4, q5
	vst2.32     {q6-q7}, [r2] 
	bx               lr
	.size mul_cx_neon4, .-mul_cx_neon4
	.section .note.GNU-stack,"",%progbits
