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
 * Main entry point
 *
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "wimboot.h"
#include "peloader.h"

/** Start of our image (defined by linker) */
extern char _start[];

/** End of our image (defined by linker) */
extern char _end[];

/** Command line */
char *cmdline;

/** initrd */
void *initrd;

/** Length of initrd */
size_t initrd_len;

/** Memory regions */
enum {
	WIMBOOT_REGION = 0,
	PE_REGION,
	INITRD_REGION,
	NUM_REGIONS
};

static void wrap_call_interrupt ( struct bootapp_callback_params *params ) {

	if ( params->vector.interrupt == 0x13 ) {
		printf ( "Calling INT%02x,%04x\n",
			 params->vector.interrupt,
			 ( params->eax & 0xffff ) );
		__asm__ __volatile__ ( "xchgw %bx, %bx" );
	}
	call_interrupt ( params );
}

static void wrap_call_real ( struct bootapp_callback_params *params ) {
	printf ( "Calling %04x:%04x\n", params->vector.function.segment,
		 params->vector.function.offset );
	__asm__ __volatile__ ( "xchgw %bx, %bx" );
	call_real ( params );
}

/** Real-mode callback functions */
static struct bootapp_callback_functions callback_fns = {
	.call_interrupt = wrap_call_interrupt,
	.call_real = wrap_call_real,
};

/** Real-mode callbacks */
static struct bootapp_callback callback = {
	.fns = &callback_fns,
	.drive = 0x80,
};

/** Boot application descriptor set */
static struct {
	/** Boot application descriptor */
	struct bootapp_descriptor bootapp;
	/** Boot application memory descriptor */
	struct bootapp_memory_descriptor memory;
	/** Boot application memory descriptor regions */
	struct bootapp_memory_region regions[NUM_REGIONS];
	/** Boot application entry descriptor */
	struct bootapp_entry_descriptor entry;
	struct bootapp_entry_wtf1_descriptor wtf1;
	struct bootapp_entry_wtf2_descriptor wtf2;
	struct bootapp_entry_wtf3_descriptor wtf3;
	struct bootapp_entry_wtf3_descriptor wtf3_copy;
	/** Boot application callback descriptor */
	struct bootapp_callback_descriptor callback;
	/** Boot application pointless descriptor */
	struct bootapp_pointless_descriptor pointless;
} __attribute__ (( packed )) bootapps = {
	.bootapp = {
		.signature = BOOTAPP_SIGNATURE,
		.version = BOOTAPP_VERSION,
		.len = sizeof ( bootapps ),
		.arch = BOOTAPP_ARCH_I386,
		.memory = offsetof ( typeof ( bootapps ), memory ),
		.entry = offsetof ( typeof ( bootapps ), entry ),
		.xxx = offsetof ( typeof ( bootapps ), wtf3 ),
		.callback = offsetof ( typeof ( bootapps ), callback ),
		.pointless = offsetof ( typeof ( bootapps ), pointless ),
	},
	.memory = {
		.version = BOOTAPP_MEMORY_VERSION,
		.len = sizeof ( bootapps.memory ),
		.num_regions = NUM_REGIONS,
		.region_len = sizeof ( bootapps.regions[0] ),
		.reserved_len = sizeof ( bootapps.regions[0].reserved ),
	},
	.entry = {
		.signature = BOOTAPP_ENTRY_SIGNATURE,
		.flags = BOOTAPP_ENTRY_FLAGS,
	},
	.wtf1 = {
		.flags = 0x11000001,
		.len = sizeof ( bootapps.wtf1 ),
		.extra_len = ( sizeof ( bootapps.wtf2 ) +
			       sizeof ( bootapps.wtf3 ) ),
	},
	.wtf3 = {
		.version = 0x00000006,
		.len = sizeof ( bootapps.wtf3 ),
		.flags = 0x00010000,
		.xxx = 0x01,
		.mbr_signature = 0x12345678,
	},
	.wtf3_copy = {
		.version = 0x00000006,
		.len = sizeof ( bootapps.wtf3 ),
		.flags = 0x00010000,
		.xxx = 0x01,
		.mbr_signature = 0x12345678,
	},
	.callback = {
		.callback = &callback,
	},
	.pointless = {
		.version = BOOTAPP_POINTLESS_VERSION,
	},
};

/**
 * Main entry point
 *
 */
int main ( void ) {
	struct loaded_pe pe;

	/* Load PE image to memory */
	load_pe ( initrd, initrd_len, &pe );

	/* Complete boot application descriptor set */
	bootapps.bootapp.pe_base = pe.base;
	bootapps.bootapp.pe_len = pe.len;
	bootapps.regions[WIMBOOT_REGION].start_page = page_start ( _start );
	bootapps.regions[WIMBOOT_REGION].num_pages = page_len ( _start, _end );
	bootapps.regions[PE_REGION].start_page = page_start ( pe.base );
	bootapps.regions[PE_REGION].num_pages =
		page_len ( pe.base, ( pe.base + pe.len ) );
	bootapps.regions[INITRD_REGION].start_page = page_start ( initrd );
	bootapps.regions[INITRD_REGION].num_pages =
		page_len ( initrd, initrd + initrd_len );

	/* Jump to PE image */
	pe.entry ( &bootapps.bootapp );

	return 0;
}
