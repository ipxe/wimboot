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

#define WARM_REBOOT_FLAG 0x0472
#define WARM_REBOOT_MAGIC 0x1234
#define KC_CMD 0x64
#define KC_CMD_RESET 0xfe

	.arch	i386
	.code32

	/* Startup code */
	.section ".text", "ax", @progbits
	.globl	startup
startup:
	/* Reload GDT, IDT, and all segment registers, and set up stack */
	lgdt	gdtr
	lidt	idtr
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
	jmp	reboot
	.size	startup, . - startup

	/* Reboot system */
	.section ".text", "ax", @progbits
	.globl	reboot
reboot:
	/* Attempt a warm reboot via the keyboard controller */
	movw	$WARM_REBOOT_MAGIC, WARM_REBOOT_FLAG
	movb	$KC_CMD_RESET, %al
	outb	%al, $KC_CMD
	/* If that failed, generate a CPU triple fault */
	int	$0xff
	/* If even that failed, hang the system */
1:	hlt
	jmp	1b
	.size	reboot, . - reboot

	/* Global descriptor table */
	.section ".rodata16", "aw", @progbits
	.align	8
gdt:
	.globl	gdtr
gdtr:
	.word	gdt_limit
	.long	gdt
	/* 64 bit long mode code segment */
	.org	( gdt + LM_CS )
	.word	0, 0
	.byte	0, 0x9b, 0x20, 0
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
	.section ".bss16", "aw", @nobits
	.align	8
idt:
	.space	( 256 * 8 )	/* 256 8-byte entries */
	.size	idt, . - idt
	.equ	idt_limit, . - idt - 1

	/* Interrupt descriptor table register */
	.section ".rodata16", "aw", @progbits
	.globl	idtr
idtr:
	.word	idt_limit
	.long	idt
	.size	idtr, . - idtr

	/* Stack */
	.section ".stack", "aw", @nobits
	.align	8
_stack:
	.space	( 64 * 1024 )
	.size	_stack, . - _stack
_estack:
