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

/* Refer to i386 symbols as needed */
#if __x86_64__
#define i386(symbol) __i386_ ## symbol
#else
#define i386(symbol) symbol
#endif

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

/* PE header architecture-dependent values */
#if __x86_64__
	.equ	coff_machine, 0x8664
	.equ	coff_characteristics, 0x2022
	.equ	opt_magic, 0x20b
#else
	.equ	coff_machine, 0x014c
	.equ	coff_characteristics, 0x2102
	.equ	opt_magic, 0x10b
#endif

	.text
	.code16
	.arch	i386
	.org	0
	.section ".prefix", "ax", @progbits

	.org	0
_prefix:
mz_header:
	.ascii	"MZ"
	.org	mz_header + 0x3c
mz_lfanew:
	.word	( pe_header - _prefix )
	.equ	mz_header_len, . - mz_header
	.size	mz_header,     . - mz_header

	.org	0x40
pe_header:
	.ascii	"PE"			/* Signature */
	.byte	0, 0
	.word	coff_machine		/* Machine */
	.word	4			/* NumberOfSections */
	.long	0x10d1a884		/* TimeDateStamp */
	.long	0			/* PointerToSymbolTable */
	.long	0			/* NumberOfSymbols */
	.word	( opt_header_len + data_directory_len ) /*SizeOfOptionalHeader*/
	.word	coff_characteristics	/* Characteristics */
	.equ	pe_header_len, . - pe_header
	.size	pe_header,     . - pe_header
opt_header:
	.word	opt_magic		/* Magic */
	.byte	42			/* MajorLinkerVersion */
	.byte	42			/* MinorLinkerVersion */
	.long	_text_total_len		/* SizeOfCode */
	.long	_data_total_len		/* SizeOfInitializedData */
	.long	_bss_total_len		/* SizeOfUninitializedData */
	.long	( efi_main - BASE_ADDRESS ) /* AddressOfEntryPoint */
	.long	( _text - BASE_ADDRESS ) /* BaseOfCode */
#if __x86_64__
	.quad	BASE_ADDRESS		/* ImageBase */
#else
	.long	( _data - BASE_ADDRESS ) /* BaseOfData */
	.long	BASE_ADDRESS		/* ImageBase */
#endif
	.long	0x10			/* SectionAlignment */
	.long	0x10			/* FileAlignment */
	.word	0			/* MajorOperatingSystemVersion */
	.word	0			/* MinorOperatingSystemVersion */
	.word	0			/* MajorImageVersion */
	.word	0			/* MinorImageVersion */
	.word	0			/* MajorSubsystemVersion */
	.word	0			/* MinorSubsystemVersion */
	.long	0			/* Win32VersionValue */
	.long	( _end - BASE_ADDRESS )	/* SizeOfImage */
	.long	_prefix_len		/* SizeOfHeaders */
	.long	0			/* Checksum */
	.word	0x0a			/* Subsystem */
	.word	0			/* DllCharacteristics */
#if __x86_64__
	.quad	0			/* SizeOfStackReserve */
	.quad	0			/* SizeOfStackCommit */
	.quad	0			/* SizeOfHeapReserve */
	.quad	0			/* SizeOfHeapCommit */
#else
	.long	0			/* SizeOfStackReserve */
	.long	0			/* SizeOfStackCommit */
	.long	0			/* SizeOfHeapReserve */
	.long	0			/* SizeOfHeapCommit */
#endif
	.long	0			/* LoaderFlags */
	.long	16			/* NumberOfRvaAndSizes */
	.equ	opt_header_len,	. - opt_header
	.size	opt_header,	. - opt_header
data_directory:
	.org	data_directory + ( 8 * 5 )
directory_entry_reloc:
	.long	( _reloc - BASE_ADDRESS )
	.long	0
	.org	data_directory + ( 8 * 6 )
directory_entry_debug:
	.long	( debug - _prefix )
	.long	debug_len
	.org	data_directory + ( 8 * 16 )
	.equ	data_directory_len, . - data_directory
	.size	data_directory,	    . - data_directory
coff_sections:
	.ascii	".bss16"		/* Name */
	.byte	0, 0
	.long	_bss16_len		/* Misc.VirtualSize */
	.long	( _bss16 - BASE_ADDRESS ) /* VirtualAddress */
	.long	0			/* SizeOfRawData */
	.long	0			/* PointerToRawData */
	.long	0			/* PointerToRelocations */
	.long	0			/* PointerToLinenumbers */
	.word	0			/* NumberOfRelocations */
	.word	0			/* NumberOfLinenumbers */
	.long	0xc8000080		/* Characteristics */
	.ascii	".payload"		/* Name */
	.long	_payload_len		/* Misc.VirtualSize */
	.long	( _payload - BASE_ADDRESS ) /* VirtualAddress */
	.long	_payload_len		/* SizeOfRawData */
	.long	_payload_pos		/* PointerToRawData */
	.long	0			/* PointerToRelocations */
	.long	0			/* PointerToLinenumbers */
	.word	0			/* NumberOfRelocations */
	.word	0			/* NumberOfLinenumbers */
	.long	0xe80000e0		/* Characteristics */
	.ascii	".bss"			/* Name */
	.byte	0, 0, 0, 0
	.long	_bss_len		/* Misc.VirtualSize */
	.long	( _bss - BASE_ADDRESS )	/* VirtualAddress */
	.long	0			/* SizeOfRawData */
	.long	0			/* PointerToRawData */
	.long	0			/* PointerToRelocations */
	.long	0			/* PointerToLinenumbers */
	.word	0			/* NumberOfRelocations */
	.word	0			/* NumberOfLinenumbers */
	.long	0xc8000080		/* Characteristics */
	.ascii	".reloc"		/* Name */
	.byte	0, 0
	.long	0			/* Misc.VirtualSize */
	.long	( _reloc - BASE_ADDRESS ) /* VirtualAddress */
	.long	0			/* SizeOfRawData */
	.long	_reloc_pos		/* PointerToRawData */
	.long	0			/* PointerToRelocations */
	.long	0			/* PointerToLinenumbers */
	.word	0			/* NumberOfRelocations */
	.word	0			/* NumberOfLinenumbers */
	.long	0x48000040		/* Characteristics */
	.equ	coff_sections_len, . - coff_sections
	.size	coff_sections,	   . - coff_sections

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

	.org	0x20e
kernel_version:
	.word	( version_string - _prefix - 0x200 )

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
	.long	0x7fffffff

version_string:
	.asciz	VERSION

	/* Setup code */
setup:
	/* Reset %cs so that labels work */
	pushw	%ds
	pushw	$( 1f - _prefix )
	lret
1:
	/* Fix up GDT */
	xorl	%eax, %eax
	movw	%cs, %ax
	shll	$4, %eax
	addl	%eax, ( gdt - _prefix + 2 )
	addl	%eax, ( gdt - _prefix + PREFIX_CS + 2 )

	/* Switch to protected mode and jump to startup code */
	cli
	data32 lgdt ( gdt - _prefix )
	movl	%cr0, %eax
	orb	$CR0_PE, %al
	movl	%eax, %cr0
	data32 ljmp $PREFIX_CS, $( 1f - _prefix )
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
	movl	%cs:( cmd_line_ptr - _prefix ), %eax
	movl	%eax, i386(cmdline)
	movl	%cs:( ramdisk_image - _prefix ), %eax
	movl	%eax, i386(initrd)
	movl	%cs:( ramdisk_size - _prefix ), %eax
	movl	%eax, i386(initrd_len)

	/* Jump to payload */
	ljmp	$PREFIX_FLAT_CS, $i386(startup)
	.size	setup, . - setup

	/* Global descriptor table */
gdt:
	.word	gdt_limit
	.long	( gdt - _prefix )
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

debug:
	.long	0			/* Characteristics */
	.long	0x10d1a884		/* TimeDateStamp */
	.word	0			/* MajorVersion */
	.word	0			/* MinorVersion */
	.long	2			/* Type */
	.long	debug_rsds_len		/* SizeOfData */
	.long	( debug_rsds - _prefix ) /* RVA */
	.long	( debug_rsds - _prefix ) /* FileOffset */
	.equ	debug_len, . - debug
	.size	debug,	   . - debug
debug_rsds:
	.ascii	"RSDS"			/* Signature */
	.long	0			/* Unknown */
	.long	0			/* Unknown2 */
	.long	0			/* Unknown3 */
	.long	0			/* Unknown4 */
	.long	0			/* Unknown5 */
	.asciz	"wimboot.efi"
	.equ	debug_rsds_len, . - debug_rsds
	.size	debug_rsds,	. - debug_rsds

	.org	( ( SETUP_SECTS + 1 ) * SECTOR_SIZE )
