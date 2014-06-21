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
 * bzImage prefix
 *
 */

#include "wimboot.h"

/** Standard number of setup sectors */
#define SETUP_SECTS 4

/** Sector size */
#define SECTOR_SIZE 512

/** Protected-mode code will be loaded high */
#define LOADED_HIGH 0x01

/** Protected-mode code will be loaded to this linear address */
#define LOADED_HIGH_ADDRESS 0x100000

/** Protected-mode bit in CR0 */
#define CR0_PE 0x01

/** 32-bit protected mode code segment based at real-mode %cs:0000 */
#define PREFIX_CS 0x08

/** 32 bit protected mode flat code segment */
#define PREFIX_FLAT_CS 0x10

/** 32 bit protected mode flat data segment */
#define PREFIX_FLAT_DS 0x18

	.text
	.code16
	.arch	i386
	.org	0
	.section ".prefix", "ax", @progbits

	.org	0x1f1
setup_sects:
	.byte	SETUP_SECTS

	.org	0x1fe
boot_flag:
	.word	0xaa55

	.org	0x200
jump:
	.byte	0xeb, setup - 1f
1:

	.org	0x202
header:
	.ascii	"HdrS"

	.org	0x206
version:
	.word	0x203	/* Version 2.03 */

	.org	0x211
loadflags:
	.byte	LOADED_HIGH

	.org	0x218
ramdisk_image:
	.long	0	/* Filled in by boot loader */

	.org	0x21c
ramdisk_size:
	.long	0	/* Filled in by boot loader */

	.org	0x228
cmd_line_ptr:
	.long	0	/* Filled in by boot loader */

	.org	0x22c
ramdisk_max:
	.long	0xffffffff

	/* Setup code */
setup:
	/* Reset %cs so that labels work */
	pushw	%ds
	pushw	$1f
	lret
1:
	/* Fix up GDT */
	xorl	%eax, %eax
	movw	%cs, %ax
	shll	$4, %eax
	addl	%eax, ( gdt + 2 )
	addl	%eax, ( gdt + PREFIX_CS + 2 )

	/* Switch to protected mode and jump to startup code */
	cli
	data32 lgdt gdt
	movl	%cr0, %eax
	orb	$CR0_PE, %al
	movl	%eax, %cr0
	data32 ljmp $PREFIX_CS, $1f
	.code32
1:
	/* Load data segment registers */
	movw	$PREFIX_FLAT_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Zero real-mode and protected-mode .bss sections */
	xorl	%eax, %eax
	movl	$_bss16, %edi
	movl	$_bss16_len, %ecx
	rep stosb
	movl	$_bss, %edi
	movl	$_bss_len, %ecx
	rep stosb

	/* Copy payload to runtime address */
	movl	$LOADED_HIGH_ADDRESS, %esi
	movl	$_payload, %edi
	movl	$_payload_len, %ecx
	cld
	rep movsb

	/* Copy parameters required by runtime */
	movl	%cs:cmd_line_ptr, %eax
	movl	%eax, cmdline
	movl	%cs:ramdisk_image, %eax
	movl	%eax, initrd
	movl	%cs:ramdisk_size, %eax
	movl	%eax, initrd_len

	/* Jump to payload */
	ljmp	$PREFIX_FLAT_CS, $startup
	.size	setup, . - setup

	/* Global descriptor table */
gdt:
	.word	gdt_limit
	.long	gdt
	/* 32-bit protected mode code segment based at real-mode %cs:0000 */
	.org	( gdt + PREFIX_CS )
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0
	/* 32 bit protected mode flat code segment */
	.org	( gdt + PREFIX_FLAT_CS )
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0
	/* 32 bit protected mode flat data segment */
	.org	( gdt + PREFIX_FLAT_DS )
	.word	0xffff, 0
	.byte	0, 0x93, 0xcf, 0	
	.size	gdt, . - gdt
	.equ	gdt_limit, . - gdt - 1

	.org	( ( SETUP_SECTS + 1 ) * SECTOR_SIZE )
