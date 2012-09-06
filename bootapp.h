#ifndef _BOOTAPP_H
#define _BOOTAPP_H

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
 * Boot application data structures
 *
 */

#include <stdint.h>

/** A segment:offset address */
struct segoff {
	/** Offset */
	uint16_t offset;
	/** Segment */
	uint16_t segment;
} __attribute__ (( packed ));

/** Real-mode callback parameters */
struct bootapp_callback_params {
	/** Vector */
	union {
		/** Interrupt number */
		uint32_t interrupt;
		/** Segment:offset address of real-mode function */
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
	/** Placeholder (for %esp?) */
	uint32_t unused_esp;
	/** Placeholder (for %ebp?) */
	uint32_t unused_ebp;
	/** %esi value */
	uint32_t esi;
	/** %edi value */
	uint32_t edi;
	/** Placeholder (for %cs?) */
	uint32_t unused_cs;
	/** %ds value */
	uint32_t ds;
	/** Placeholder (for %ss?) */
	uint32_t unused_ss;
	/** %es value */
	uint32_t es;
	/** %fs value */
	uint32_t fs;
	/** %gs value */
	uint32_t gs;
	/** eflags value */
	uint32_t eflags;
} __attribute__ (( packed ));

/** Real-mode callback function table */
struct bootapp_callback_functions {
	/**
	 * Call an arbitrary real-mode interrupt
	 *
	 * @v params		Parameters
	 */
	void ( * call_interrupt ) ( struct bootapp_callback_params *params );
	/**
	 * Call an arbitrary real-mode function
	 *
	 * @v params		Parameters
	 */
	void ( * call_real ) ( struct bootapp_callback_params *params );
} __attribute__ (( packed ));

/** Real-mode callbacks */
struct bootapp_callback {
	/** Real-mode callback function table */
	struct bootapp_callback_functions *fns;
	/** Drive number for INT13 calls */
	uint32_t drive;
} __attribute__ (( packed ));

/** Boot application descriptor */
struct bootapp_descriptor {
	/** Signature */
	char signature[8];
	/** Version */
	uint32_t version;
	/** Total length */
	uint32_t len;
	/** COFF machine type */
	uint32_t arch;
	/** Reserved */
	uint32_t reserved_0x14;
	/** Loaded PE image base address */
	void *pe_base;
	/** Reserved */
	uint32_t reserved_0x1c;
	/** Length of loaded PE image */
	uint32_t pe_len;
	/** Offset to memory descriptor */
	uint32_t memory;
	/** Offset to boot application entry descriptor */
	uint32_t btapent;
	/** Offset to ??? */
	uint32_t xxx;
	/** Offset to callback descriptor */
	uint32_t callback;
	/** Offset to pointless descriptor */
	uint32_t pointless;
	/** Reserved */
	uint32_t reserved_0x38;
} __attribute__ (( packed ));

/** "BOOT APP" magic signature */
#define BOOTAPP_SIGNATURE "BOOT APP"

/** Boot application descriptor version */
#define BOOTAPP_VERSION 2

/** i386 architecture */
#define BOOTAPP_ARCH_I386 0x014c

/** Memory region descriptor */
struct bootapp_memory_region {
	/** Reserved (for struct list_head?) */
	uint8_t reserved[8];
	/** Start page address */
	uint32_t start_page;
	/** Reserved */
	uint8_t reserved_0x0c[12];
	/** Number of pages */
	uint32_t num_pages;
	/** Reserved */
	uint8_t reserved_0x1c[8];
	/** Flags */
	uint32_t flags;
} __attribute__ (( packed ));

/** Memory descriptor */
struct bootapp_memory_descriptor {
	/** Version */
	uint32_t version;
	/** Length of descriptor (excluding region descriptors) */
	uint32_t len;
	/** Number of regions */
	uint32_t num_regions;
	/** Length of each region descriptor */
	uint32_t region_len;
	/** Length of reserved area at start of each region descriptor */
	uint32_t reserved_len;
} __attribute__ (( packed ));

/** Boot application memory descriptor version */
#define BOOTAPP_MEMORY_VERSION 1

/** Boot application callback descriptor */
struct bootapp_callback_descriptor {
	/** Real-mode callbacks */
	struct bootapp_callback *callback;
	/** Reserved */
	uint32_t reserved;
} __attribute__ (( packed ));

#endif /* _BOOTAPP_H */
