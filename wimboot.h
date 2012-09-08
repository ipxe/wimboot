#ifndef _WIMBOOT_H
#define _WIMBOOT_H

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
 * WIM boot loader
 *
 */

/** Base segment address
 *
 * We place everything at 2000:0000, since this region is used by the
 * Microsoft first-stage loaders (e.g. pxeboot.n12, etfsboot.com).
 */
#define BASE_SEG 0x2000

/** Base linear address */
#define BASE_ADDRESS ( BASE_SEG << 4 )

/** 32 bit protected mode flat code segment */
#define FLAT_CS 0x08

/** 32 bit protected mode flat data segment */
#define FLAT_DS 0x10

/** 16 bit real mode code segment */
#define REAL_CS 0x18

/** 16 bit real mode data segment */
#define REAL_DS 0x20

#ifndef ASSEMBLY

#include <stdint.h>
#include <bootapp.h>

/** Page size */
#define PAGE_SIZE 4096

/**
 * Calculate start page number
 *
 * @v address		Address
 * @ret page		Start page number
 */
static inline unsigned int page_start ( const void *address ) {
	return ( ( ( intptr_t ) address ) / PAGE_SIZE );
}

/**
 * Calculate end page number
 *
 * @v address		Address
 * @ret page		End page number
 */
static inline unsigned int page_end ( const void *address ) {
	return ( ( ( ( intptr_t ) address ) + PAGE_SIZE - 1 ) / PAGE_SIZE );
}

/**
 * Calculate page length
 *
 * @v start		Start address
 * @v end		End address
 * @ret num_pages	Number of pages
 */
static inline unsigned int page_len ( const void *start, const void *end ) {
	return ( page_end ( end ) - page_start ( start ) );
}

extern void call_real ( struct bootapp_callback_params *params );
extern void call_interrupt ( struct bootapp_callback_params *params );
extern void __attribute__ (( noreturn, format ( printf, 1, 2 ) ))
die ( const char *fmt, ... );
extern void __attribute__ (( noreturn )) reboot ( void );

#endif /* ASSEMBLY */

#endif /* _WIMBOOT_H */
