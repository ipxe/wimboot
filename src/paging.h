#ifndef _PAGING_H
#define _PAGING_H

/*
 * Copyright (C) 2021 Michael Brown <mbrown@fensystems.co.uk>.
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
 * Paging
 *
 */

#include <stdint.h>

/** Get CPU features */
#define CPUID_FEATURES 0x00000001

/** CPU supports PAE */
#define CPUID_FEATURE_EDX_PAE 0x00000040

/* CR0: paging */
#define CR0_PG 0x80000000

/* CR4: physical address extensions */
#define CR4_PAE 0x00000020

/* Page: present */
#define PG_P 0x01

/* Page: read/write */
#define PG_RW 0x02

/* Page: user/supervisor */
#define PG_US 0x04

/* Page: page size */
#define PG_PS 0x80

/** 2MB page size */
#define PAGE_SIZE_2MB 0x200000

/** 32-bit address space size */
#define ADDR_4GB 0x100000000ULL

/** Saved paging state */
struct paging_state {
	/** Control register 0 */
	unsigned long cr0;
	/** Control register 3 */
	unsigned long cr3;
	/** Control register 4 */
	unsigned long cr4;
};

/** Magic value for INT 15,e820 calls */
#define E820_SMAP 0x534d4150

/** An INT 15,e820 memory map entry */
struct e820_entry {
	/** Start of region */
	uint64_t start;
	/** Length of region */
	uint64_t len;
	/** Type of region */
	uint32_t type;
	/** Extended attributes (optional) */
	uint32_t attrs;
} __attribute__ (( packed ));

/** Normal RAM */
#define E820_TYPE_RAM 1

/** Region is enabled (if extended attributes are present) */
#define E820_ATTR_ENABLED 0x00000001UL

/** Region is non-volatile memory (if extended attributes are present) */
#define E820_ATTR_NONVOLATILE 0x00000002UL

extern int paging;

extern void init_paging ( void );
extern void enable_paging ( struct paging_state *state );
extern void disable_paging ( struct paging_state *state );
extern uint64_t relocate_memory ( void *start, size_t len );

#endif /* _PAGING_H */
