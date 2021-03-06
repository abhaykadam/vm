/*
 *  Multi2Sim
 *  Copyright (C) 2011  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <cpukernel.h>


/* Macros defined to prevent accidental use of 'mem_<xx>' instructions */
#define mem_access __COMPILATION_ERROR__
#define mem_read __COMPILATION_ERROR__
#define mem_write __COMPILATION_ERROR__
#define mem_zero __COMPILATION_ERROR__
#define mem_read_string __COMPILATION_ERROR__
#define mem_write_string __COMPILATION_ERROR__
#define mem_get_buffer __COMPILATION_ERROR__
#define fatal __COMPILATION_ERROR__
#define panic __COMPILATION_ERROR__
#define warning __COMPILATION_ERROR__
#ifdef assert
#undef assert
#endif
#define assert __COMPILATION_ERROR__


#define op_stdop_al_imm8(stdop, wb, uinst) \
void op_##stdop##_al_imm8_impl() \
{ \
	uint8_t al = isa_load_reg(x86_reg_al); \
	uint8_t imm8 = isa_inst.imm.b; \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%al\n\t" \
		#stdop " %3, %%al\n\t" \
		"mov %%al, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (al) \
		: "m" (al), "m" (imm8), "g" (flags) \
		: "al" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_reg(x86_reg_al, al); \
		x86_uinst_new(uinst, x86_dep_eax, 0, 0, x86_dep_eax, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_eax, 0, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_ax_imm16(stdop, wb, uinst) \
void op_##stdop##_ax_imm16_impl() \
{ \
	uint16_t ax = isa_load_reg(x86_reg_ax); \
	uint16_t imm16 = isa_inst.imm.w; \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%ax\n\t" \
		#stdop " %3, %%ax\n\t" \
		"mov %%ax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (ax) \
		: "m" (ax), "m" (imm16), "g" (flags) \
		: "ax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_reg(x86_reg_ax, ax); \
		x86_uinst_new(uinst, x86_dep_eax, 0, 0, x86_dep_eax, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_eax, 0, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_eax_imm32(stdop, wb, uinst) \
void op_##stdop##_eax_imm32_impl() \
{ \
	uint32_t eax = isa_load_reg(x86_reg_eax); \
	uint32_t imm32 = isa_inst.imm.d; \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%eax\n\t" \
		#stdop " %3, %%eax\n\t" \
		"mov %%eax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (eax) \
		: "m" (eax), "m" (imm32), "g" (flags) \
		: "eax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_reg(x86_reg_eax, eax); \
		x86_uinst_new(uinst, x86_dep_eax, 0, 0, x86_dep_eax, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_eax, 0, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_rm8_imm8(stdop, wb, uinst) \
void op_##stdop##_rm8_imm8_impl() \
{ \
	uint8_t rm8 = isa_load_rm8(); \
	uint8_t imm8 = isa_inst.imm.b; \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%al\n\t" \
		#stdop " %3, %%al\n\t" \
		"mov %%al, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (rm8) \
		: "m" (rm8), "m" (imm8), "g" (flags) \
		: "al" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_rm8(rm8); \
		x86_uinst_new(uinst, x86_dep_rm8, 0, 0, x86_dep_rm8, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_rm8, 0, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_rm16_imm16(stdop, wb, uinst) \
void op_##stdop##_rm16_imm16_impl() \
{ \
	uint16_t rm16 = isa_load_rm16(); \
	uint16_t imm16 = isa_inst.imm.w; \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%ax\n\t" \
		#stdop " %3, %%ax\n\t" \
		"mov %%ax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (rm16) \
		: "m" (rm16), "m" (imm16), "g" (flags) \
		: "ax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_rm16(rm16); \
		x86_uinst_new(uinst, x86_dep_rm16, 0, 0, x86_dep_rm16, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_rm16, 0, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_rm32_imm32(stdop, wb, uinst) \
void op_##stdop##_rm32_imm32_impl() \
{ \
	uint32_t rm32 = isa_load_rm32(); \
	uint32_t imm32 = isa_inst.imm.d; \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%eax\n\t" \
		#stdop " %3, %%eax\n\t" \
		"mov %%eax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (rm32) \
		: "m" (rm32), "m" (imm32), "g" (flags) \
		: "eax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_rm32(rm32); \
		x86_uinst_new(uinst, x86_dep_rm32, 0, 0, x86_dep_rm32, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_rm32, 0, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_rm16_imm8(stdop, wb, uinst) \
void op_##stdop##_rm16_imm8_impl() \
{ \
	uint16_t rm16 = isa_load_rm16(); \
	uint16_t imm8 = (int8_t) isa_inst.imm.b; \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%ax\n\t" \
		#stdop " %3, %%ax\n\t" \
		"mov %%ax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (rm16) \
		: "m" (rm16), "m" (imm8), "g" (flags) \
		: "ax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_rm16(rm16); \
		x86_uinst_new(uinst, x86_dep_rm16, 0, 0, x86_dep_rm16, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_rm16, 0, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_rm32_imm8(stdop, wb, uinst) \
void op_##stdop##_rm32_imm8_impl() \
{ \
	uint32_t rm32 = isa_load_rm32(); \
	uint32_t imm8 = (int8_t) isa_inst.imm.b; \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%eax\n\t" \
		#stdop " %3, %%eax\n\t" \
		"mov %%eax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (rm32) \
		: "m" (rm32), "m" (imm8), "g" (flags) \
		: "eax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_rm32(rm32); \
		x86_uinst_new(uinst, x86_dep_rm32, 0, 0, x86_dep_rm32, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_rm32, 0, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_rm8_r8(stdop, wb, uinst) \
void op_##stdop##_rm8_r8_impl() \
{ \
	uint8_t rm8 = isa_load_rm8(); \
	uint8_t r8 = isa_load_r8(); \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%al\n\t" \
		#stdop " %3, %%al\n\t" \
		"mov %%al, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (rm8) \
		: "m" (rm8), "m" (r8), "g" (flags) \
		: "al" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_rm8(rm8); \
		x86_uinst_new(uinst, x86_dep_rm8, x86_dep_r8, 0, x86_dep_rm8, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_rm8, x86_dep_r8, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_rm16_r16(stdop, wb, uinst) \
void op_##stdop##_rm16_r16_impl() \
{ \
	uint16_t rm16 = isa_load_rm16(); \
	uint16_t r16 = isa_load_r16(); \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%ax\n\t" \
		#stdop " %3, %%ax\n\t" \
		"mov %%ax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (rm16) \
		: "m" (rm16), "m" (r16), "g" (flags) \
		: "ax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_rm16(rm16); \
		x86_uinst_new(uinst, x86_dep_rm16, x86_dep_r16, 0, x86_dep_rm16, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_rm16, x86_dep_r16, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_rm32_r32(stdop, wb, uinst) \
void op_##stdop##_rm32_r32_impl() \
{ \
	uint32_t rm32 = isa_load_rm32(); \
	uint32_t r32 = isa_load_r32(); \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%eax\n\t" \
		#stdop " %3, %%eax\n\t" \
		"mov %%eax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (rm32) \
		: "m" (rm32), "m" (r32), "g" (flags) \
		: "eax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_rm32(rm32); \
		x86_uinst_new(uinst, x86_dep_rm32, x86_dep_r32, 0, x86_dep_rm32, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else  { \
		x86_uinst_new(uinst, x86_dep_rm32, x86_dep_r32, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_r8_rm8(stdop, wb, uinst) \
void op_##stdop##_r8_rm8_impl() \
{ \
	uint8_t r8 = isa_load_r8(); \
	uint8_t rm8 = isa_load_rm8(); \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%al\n\t" \
		#stdop " %3, %%al\n\t" \
		"mov %%al, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (r8) \
		: "m" (r8), "m" (rm8), "g" (flags) \
		: "al" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_r8(r8); \
		x86_uinst_new(uinst, x86_dep_r8, x86_dep_rm8, 0, x86_dep_r8, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_r8, x86_dep_rm8, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_r16_rm16(stdop, wb, uinst) \
void op_##stdop##_r16_rm16_impl() \
{ \
	uint16_t r16 = isa_load_r16(); \
	uint16_t rm16 = isa_load_rm16(); \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%ax\n\t" \
		#stdop " %3, %%ax\n\t" \
		"mov %%ax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (r16) \
		: "m" (r16), "m" (rm16), "g" (flags) \
		: "ax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_r16(r16); \
		x86_uinst_new(uinst, x86_dep_r16, x86_dep_rm16, 0, x86_dep_r16, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_r16, x86_dep_rm16, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_r32_rm32(stdop, wb, uinst) \
void op_##stdop##_r32_rm32_impl() \
{ \
	uint32_t r32 = isa_load_r32(); \
	uint32_t rm32 = isa_load_rm32(); \
	unsigned long flags = isa_regs->eflags; \
	__ISA_ASM_START__ \
	asm volatile ( \
		"push %4\n\t" \
		"popf\n\t" \
		"mov %2, %%eax\n\t" \
		#stdop " %3, %%eax\n\t" \
		"mov %%eax, %1\n\t" \
		"pushf\n\t" \
		"pop %0\n\t" \
		: "=g" (flags), "=m" (r32) \
		: "m" (r32), "m" (rm32), "g" (flags) \
		: "eax" \
	); \
	__ISA_ASM_END__ \
	if (wb) { \
		isa_store_r32(r32); \
		x86_uinst_new(uinst, x86_dep_r32, x86_dep_rm32, 0, x86_dep_r32, x86_dep_zps, x86_dep_cf, x86_dep_of); \
	} else { \
		x86_uinst_new(uinst, x86_dep_r32, x86_dep_rm32, 0, x86_dep_zps, x86_dep_cf, x86_dep_of, 0); \
	} \
	isa_regs->eflags = flags; \
}


#define op_stdop_all(stdop, wb, uinst) \
	op_stdop_al_imm8(stdop, wb, uinst) \
	op_stdop_ax_imm16(stdop, wb, uinst) \
	op_stdop_eax_imm32(stdop, wb, uinst) \
	op_stdop_rm8_imm8(stdop, wb, uinst) \
	op_stdop_rm16_imm16(stdop, wb, uinst) \
	op_stdop_rm32_imm32(stdop, wb, uinst) \
	op_stdop_rm16_imm8(stdop, wb, uinst) \
	op_stdop_rm32_imm8(stdop, wb, uinst) \
	op_stdop_rm8_r8(stdop, wb, uinst) \
	op_stdop_rm16_r16(stdop, wb, uinst) \
	op_stdop_rm32_r32(stdop, wb, uinst) \
	op_stdop_r8_rm8(stdop, wb, uinst) \
	op_stdop_r16_rm16(stdop, wb, uinst) \
	op_stdop_r32_rm32(stdop, wb, uinst)


op_stdop_all(adc, 1, x86_uinst_add)
op_stdop_all(add, 1, x86_uinst_add)
op_stdop_all(and, 1, x86_uinst_and)
op_stdop_all(cmp, 0, x86_uinst_sub)
op_stdop_all(or, 1, x86_uinst_or)
op_stdop_all(sbb, 1, x86_uinst_sub)
op_stdop_all(sub, 1, x86_uinst_sub)
op_stdop_all(test, 0, x86_uinst_and)
op_stdop_all(xor, 1, x86_uinst_xor)

