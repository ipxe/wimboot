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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "wimboot.h"
#include "peloader.h"
#include "int13.h"
#include "vdisk.h"
#include "cpio.h"
#include "lznt1.h"

/** Start of our image (defined by linker) */
extern char _start[];

/** End of our image (defined by linker) */
extern char _end[];

/** Command line */
const char *cmdline;

/** initrd */
void *initrd;

/** Length of initrd */
size_t initrd_len;

/** bootmgr.exe image */
const void *bootmgr;

/** bootmgr.exe length */
size_t bootmgr_len;

/** Memory regions */
enum {
	WIMBOOT_REGION = 0,
	PE_REGION,
	INITRD_REGION,
	NUM_REGIONS
};

/**
 * Wrap interrupt callback
 *
 * @v params		Parameters
 */
static void call_interrupt_wrapper ( struct bootapp_callback_params *params ) {

	/* Intercept INT 13 calls for the emulated drive */
	if ( params->vector.interrupt == 0x13 ) {
		emulate_int13 ( params );
	} else {
		call_interrupt ( params );
	}
}

/** Real-mode callback functions */
static struct bootapp_callback_functions callback_fns = {
	.call_interrupt = call_interrupt_wrapper,
	.call_real = call_real,
};

/** Real-mode callbacks */
static struct bootapp_callback callback = {
	.fns = &callback_fns,
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
		.xxx = offsetof ( typeof ( bootapps ), wtf3_copy ),
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
		.flags = 0x00000006,
		.len = sizeof ( bootapps.wtf3 ),
		.boot_partition_offset = ( VDISK_VBR_LBA * VDISK_SECTOR_SIZE ),
		.xxx = 0x01,
		.mbr_signature = VDISK_MBR_SIGNATURE,
	},
	.wtf3_copy = {
		.flags = 0x00000006,
		.len = sizeof ( bootapps.wtf3 ),
		.boot_partition_offset = ( VDISK_VBR_LBA * VDISK_SECTOR_SIZE ),
		.xxx = 0x01,
		.mbr_signature = VDISK_MBR_SIGNATURE,
	},
	.callback = {
		.callback = &callback,
	},
	.pointless = {
		.version = BOOTAPP_POINTLESS_VERSION,
	},
};

/**
 * File handler
 *
 * @v name		File name
 * @v data		File data
 * @v len		Length
 * @ret rc		Return status code
 */
static int add_file ( const char *name, const void *data, size_t len ) {
	static unsigned int idx = 0;

	/* Sanity check */
	if ( idx >= VDISK_MAX_FILES ) {
		DBG ( "Too many files\n" );
		return -1;
	}

	/* Store file */
	DBG ( "Loading %s at %p+%#zx\n", name, data, len );
	vdisk_files[idx].name = name;
	vdisk_files[idx].data = data;
	vdisk_files[idx].len = len;
	idx++;

	/* Check for bootmgr.exe */
	if ( strcasecmp ( name, "bootmgr.exe" ) == 0 ) {
		bootmgr = data;
		bootmgr_len = len;
	}

	/* Check for bootmgr */
	if ( strcasecmp ( name, "bootmgr" ) == 0 ) {
		const uint8_t *compressed;
		size_t offset;
		size_t compressed_len;
		ssize_t decompressed_len;
		size_t padded_len;

		/* Look for an embedded compressed bootmgr.exe.  Since
		 * there is no way for LZNT1 to compress the initial
		 * "MZ" bytes of bootmgr.exe, we look for this
		 * signature starting three bytes after a paragraph
		 * boundary, with a preceding tag byte indicating that
		 * these two bytes would indeed be uncompressed.
		 */
		for ( offset = 0 ; offset < ( len - 5 ) ; offset += 16 ) {

			/* Check signature */
			compressed = ( data + offset );
			if ( ( ( compressed[2] & 0x03 ) != 0x00 ) ||
			     ( compressed[3] != 'M' ) ||
			     ( compressed[4] != 'Z' ) ) {
				continue;
			}
			compressed_len = ( len - offset );

			/* Find length of decompressed image */
			decompressed_len = lznt1_decompress ( compressed,
							      compressed_len,
							      NULL );
			if ( decompressed_len < 0 ) {
				/* May be a false positive signature match */
				continue;
			}

			/* Prepend decompressed image to initrd */
			DBG ( "...extracting embedded bootmgr.exe\n" );
			padded_len = ( ( decompressed_len + PAGE_SIZE - 1 ) &
				       ~( PAGE_SIZE - 1 ) );
			initrd -= padded_len;
			initrd_len += padded_len;
			lznt1_decompress ( compressed, compressed_len, initrd );

			/* Add decompressed image */
			return add_file ( "bootmgr.exe", initrd,
					  decompressed_len );
		}
	}

	return 0;
}

/**
 * Main entry point
 *
 */
int main ( void ) {
	struct loaded_pe pe;

	/* Print welcome banner */
	printf ( "\n\nwimboot " VERSION " -- Windows Imaging Format "
		 "bootloader -- http://ipxe.org/wimboot\n\n" );

	/* Extract files from initrd */
	if ( cpio_extract ( initrd, initrd_len, add_file ) != 0 )
		die ( "FATAL: could not extract initrd files\n" );

	/* Add INT 13 drive */
	callback.drive = initialise_int13();

	/* Load bootmgr.exe to memory */
	if ( ! bootmgr )
		die ( "FATAL: no bootmgr.exe\n" );
	if ( load_pe ( bootmgr, bootmgr_len, &pe ) != 0 )
		die ( "FATAL: Could not load bootmgr.exe\n" );

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
	DBG ( "Entering bootmgr.exe with parameters at %p\n", &bootapps );
	pe.entry ( &bootapps.bootapp );
	die ( "FATAL: bootmgr.exe returned\n" );
}

/**
 * Handle fatal errors
 *
 * @v fmt	Error message format string
 * @v ...	Arguments
 */
void die ( const char *fmt, ... ) {
	struct bootapp_callback_params params;
	va_list args;

	/* Print message */
	va_start ( args, fmt );
	vprintf ( fmt, args );
	va_end ( args );

	/* Wait for keypress */
	printf ( "Press a key to reboot..." );
	memset ( &params, 0, sizeof ( params ) );
	params.vector.interrupt = 0x16;
	call_interrupt ( &params );
	printf ( "\n" );

	/* Reboot system */
	reboot();
}