/*
 * Copyright (C) 2012 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * @file
 *
 * Real-mode callbacks
 *
 */

#include "wimboot.h"

/* Offsets within parameter block */
#define PARAM_VECTOR 0x00
#define PARAM_EAX 0x04
#define PARAM_EBX 0x08
#define PARAM_ECX 0x0c
#define PARAM_EDX 0x10
#define PARAM_ESI 0x1c
#define PARAM_EDI 0x20
#define PARAM_DS 0x28
#define PARAM_ES 0x30
#define PARAM_FS 0x34
#define PARAM_GS 0x38
#define PARAM_EFLAGS 0x3c
#define PARAM_LEN 0x40

/** Protected-mode bit in CR0 */
#define CR0_PE 0x00000001

/** Paging bit in CR0 */
#define CR0_PG 0x80000000

	.arch	i386
	.code32

	/* Call an arbitrary real-mode function */
	.section ".text", "ax", @progbits
	.globl	call_real
call_real:
	/* Preserve registers */
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/* Switch to real-mode stack, store original stack pointer,
	 * copy parameter block to stack, store original parameter block
	 * address.
	 */
	movl	20(%esp), %esi
	movl	%esp, %ebp
	movl	$_ermstack, %esp
	pushl	%ebp
	subl	$PARAM_LEN, %esp
	movl	%esp, %edi
	movl	$( PARAM_LEN / 4 ), %ecx
	cld
	rep movsl
	movl	%esp, %ebp	/* %ebp points to parameter block copy */
	subl	$PARAM_LEN, %esi
	pushl	%esi

	/* Set up 16-bit segment registers and stack */
	ljmp	$REAL_CS, $( 1f - BASE_ADDRESS )
1:	.code16
	movw	$REAL_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	subl	$BASE_ADDRESS, %esp
	subl	$BASE_ADDRESS, %ebp

	/* Switch to real mode */
	movl	%cr0, %eax
	pushl	%eax
	andl	$~( CR0_PG | CR0_PE ), %eax
	movl	%eax, %cr0
	data32 ljmp $BASE_SEG, $( 1f - BASE_ADDRESS )
1:	movw	$BASE_SEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	addr32 data32 lidt %cs:( rm_idtr - BASE_ADDRESS )

	/* Load registers from parameter block copy */
	movl	PARAM_EAX(%bp), %eax
	movl	PARAM_EBX(%bp), %ebx
	movl	PARAM_ECX(%bp), %ecx
	movl	PARAM_EDX(%bp), %edx
	movl	PARAM_ESI(%bp), %esi
	movl	PARAM_EDI(%bp), %edi
	movw	PARAM_DS(%bp), %ds
	movw	PARAM_ES(%bp), %es
	movw	PARAM_FS(%bp), %fs
	movw	PARAM_GS(%bp), %gs

	/* Call real-mode function with interrupts enabled */
	pushw	%bp
	sti
	lcall	*PARAM_VECTOR(%bp)
	cli
	popw	%bp

	/* Save registers and flags to parameter block copy */
	pushfl
	popl	PARAM_EFLAGS(%bp)
	movl	%eax, PARAM_EAX(%bp)
	movl	%ebx, PARAM_EBX(%bp)
	movl	%ecx, PARAM_ECX(%bp)
	movl	%edx, PARAM_EDX(%bp)
	movl	%esi, PARAM_ESI(%bp)
	movl	%edi, PARAM_EDI(%bp)
	movw	%ds, PARAM_DS(%bp)
	movw	%es, PARAM_ES(%bp)
	movw	%fs, PARAM_FS(%bp)
	movw	%gs, PARAM_GS(%bp)

	/* Switch back to protected mode */
	addr32 data32 lgdt %cs:( gdtr - BASE_ADDRESS )
	addr32 data32 lidt %cs:( idtr - BASE_ADDRESS )
	popl	%eax
	movl	%eax, %cr0
	data32 ljmp $FLAT_CS, $1f
1:	.code32
	movw	$FLAT_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	addl	$BASE_ADDRESS, %esp

	/* Copy modified parameter block back to original location and
	 * return to original stack
	 */
	popl	%edi
	movl	%esp, %esi
	movl	$( PARAM_LEN / 4 ), %ecx
	cld
	rep movsl
	movl	0(%esi), %esp

	/* Restore registers and return */
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
	.size	call_real, . - call_real

	/* Call an arbitrary real-mode interrupt */
	.section ".text", "ax", @progbits
	.globl	call_interrupt
call_interrupt:
	/* Enter function */
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%esi

	/* Extract INT number from parameter block and update INT instruction */
	movl	8(%ebp), %esi		/* %esi points to parameter block */
	movl	PARAM_VECTOR(%esi), %ebx
	movb	%bl, ( dynamic_int + 1 )

	/* Overwrite vector with dynamic INT code fragment */
	movw	$BASE_SEG, (PARAM_VECTOR + 2)(%esi)
	movl	$( dynamic_int - BASE_ADDRESS ), %eax
	movw	%ax, PARAM_VECTOR(%esi)

	/* Call dynamic INT code fragment */
	pushl	%esi
	call	call_real
	popl	%esi

	/* Restore INT number in parameter block */
	movl	%ebx, PARAM_VECTOR(%esi)

	/* Restore registers and return */
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
	.size	call_interrupt, . - call_interrupt

	/* Dynamic interrupt code fragment */
	.section ".text16", "ax", @progbits
dynamic_int:
	int	$0x00
	lret
	.size	dynamic_int, . - dynamic_int

	/* Real-mode interrupt descriptor table */
	.section ".data16", "ax", @progbits
rm_idtr:
	.word	( ( 256 * 4 ) - 1 )	/* Limit (256 segment:offset pairs) */
	.long	0			/* Base */
	.size	rm_idtr, . - rm_idtr

	/* Real-mode stack */
	.section ".stack16", "aw", @nobits
	.align	8
_rmstack:
	.space	8192
	.size	_rmstack, . - _rmstack
_ermstack:
