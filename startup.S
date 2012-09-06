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
 * Startup code
 *
 */

#include "wimboot.h"

	.arch	i386

	/* Startup code */
	.section ".text", "ax", @progbits
	.globl	startup
startup:
	/* Reload GDT, IDT, and all segment registers, and set up stack */
	lgdt	gdt
	lidt	idt
	ljmp	$FLAT_CS, $1f
1:	movw	$FLAT_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	movl	$_estack, %esp

	/* Jump to C code */
	call	main

	/* Should never return */
1:	hlt
	jmp	1b
	.size	startup, . - startup

	/* Global descriptor table */
	.section ".rodata16", "aw", @progbits
	.globl	gdt
gdt:
	.word	gdt_limit
	.long	gdt
	/* 32 bit protected mode flat code segment */
	.org	( gdt + FLAT_CS )
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0
	/* 32 bit protected mode flat data segment */
	.org	( gdt + FLAT_DS )
	.word	0xffff, 0
	.byte	0, 0x93, 0xcf, 0
	/* 16 bit flat real mode code segment with base BASE_ADDRESS */
	.org	( gdt + REAL_CS )
	.word	0xffff, ( BASE_ADDRESS & 0xffff )
	.byte	( BASE_ADDRESS >> 16 ), 0x9b, 0x8f, 0
	/* 16 bit flat real mode data segment with base BASE_ADDRESS */
	.org	( gdt + REAL_DS )
	.word	0xffff, ( BASE_ADDRESS & 0xffff )
	.byte	( BASE_ADDRESS >> 16 ), 0x93, 0x8f, 0
	.size	gdt, . - gdt
	.equ	gdt_limit, . - gdt - 1

	/* Interrupt descriptor table */
	.section ".data16", "ax", @progbits
	.globl	idt
idt:
	.word	0	/* Limit */
	.long	0	/* Base */
	.size	idtr, . - idtr

	/* Stack */
	.section ".bss", "aw", @nobits
	.align	8
_stack:
	.space	65536
	.size	_stack, . - _stack
_estack:
