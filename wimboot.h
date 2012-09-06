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

/** A segment:offset address */
struct segoff {
	/** Offset */
	uint16_t offset;
	/** Segment */
	uint16_t segment;
} __attribute__ (( packed ));

/** Callback parameters */
struct callback_params {
	/** Vector */
	union {
		uint32_t interrupt;
		struct segoff function;
	} vector;
	/** %eax value */
	uint32_t eax;
	/** %ebx value */
	uint32_t ebx;
	/** %ecx value */
	uint32_t ecx;
	/** %edx value */
	uint32_t edx;
	/** Reserved */
	uint32_t reserved0;
	/** Reserved */
	uint32_t reserved1;
	/** %esi value */
	uint32_t esi;
	/** %edi value */
	uint32_t edi;
	/** Reserved */
	uint32_t reserved2;
	/** %ds value */
	uint32_t ds;
	/** Reserved */
	uint32_t reserved3;
	/** %es value */
	uint32_t es;
	/** %fs value */
	uint32_t fs;
	/** %gs value */
	uint32_t gs;
	/** eflags value */
	uint32_t eflags;
} __attribute__ (( packed ));

/** Boot application parameters */
struct bootapp_params {
} __attribute__ (( packed ));

extern void call_real ( struct callback_params *params );
extern void call_interrupt ( struct callback_params *params );

#endif /* ASSEMBLY */

#endif /* _WIMBOOT_H */
