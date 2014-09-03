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
#include "xca.h"
#include "cmdline.h"
#include "wimpatch.h"
#include "wimfile.h"
#include "pause.h"

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

/** bootmgr.exe path within WIM */
static const wchar_t bootmgr_path[] = L"\\Windows\\Boot\\PXE\\bootmgr.exe";

/** bootmgr.exe file */
static struct vdisk_file *bootmgr;

/** Minimal length of embedded bootmgr.exe */
#define BOOTMGR_MIN_LEN 16384

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
	uint16_t *attributes;

	/* Handle/modify/pass-through interrupt as required */
	if ( params->vector.interrupt == 0x13 ) {

		/* Intercept INT 13 calls for the emulated drive */
		emulate_int13 ( params );

	} else if ( ( params->vector.interrupt == 0x10 ) &&
		    ( params->ax == 0x4f01 ) &&
		    ( ! cmdline_gui ) ) {

		/* Mark all VESA video modes as unsupported */
		attributes = REAL_PTR ( params->es, params->di );
		call_interrupt ( params );
		*attributes &= ~0x0001;

	} else {

		/* Pass through interrupt */
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
 * Test if a paragraph is empty
 *
 * @v pgh		Paragraph
 * @ret is_empty	Paragraph is empty (all zeroes)
 */
static int is_empty_pgh ( const void *pgh ) {
	const uint32_t *dwords = pgh;

	return ( ( dwords[0] | dwords[1] | dwords[2] | dwords[3] ) == 0 );
}

/**
 * Read from file
 *
 * @v file		Virtual file
 * @v data		Data buffer
 * @v offset		Offset
 * @v len		Length
 */
static void read_file ( struct vdisk_file *file, void *data, size_t offset,
			size_t len ) {

	memcpy ( data, ( file->opaque + offset ), len );
}

/**
 * Add embedded bootmgr.exe extracted from bootmgr
 *
 * @v data		File data
 * @v len		Length
 * @ret file		Virtual file, or NULL
 *
 * bootmgr.exe is awkward to obtain, since it is not available as a
 * standalone file on the installation media, or on an installed
 * system, or in a Windows PE image as created by WAIK or WADK.  It
 * can be extracted from a typical boot.wim image using ImageX, but
 * this requires installation of the WAIK/WADK/wimlib.
 *
 * A compressed version of bootmgr.exe is contained within bootmgr,
 * which is trivial to obtain.
 */
static struct vdisk_file * add_bootmgr ( const void *data, size_t len ) {
	const uint8_t *compressed;
	size_t offset;
	size_t compressed_len;
	ssize_t ( * decompress ) ( const void *data, size_t len, void *buf );
	ssize_t decompressed_len;
	size_t padded_len;

	/* Look for an embedded compressed bootmgr.exe on a paragraph
	 * boundary.
	 */
	for ( offset = BOOTMGR_MIN_LEN ; offset < ( len - BOOTMGR_MIN_LEN ) ;
	      offset += 0x10 ) {

		/* Initialise checks */
		decompress = NULL;
		compressed = ( data + offset );
		compressed_len = ( len - offset );

		/* Check for an embedded LZNT1-compressed bootmgr.exe.
		 * Since there is no way for LZNT1 to compress the
		 * initial "MZ" bytes of bootmgr.exe, we look for this
		 * signature starting three bytes after a paragraph
		 * boundary, with a preceding tag byte indicating that
		 * these two bytes would indeed be uncompressed.
		 */
		if ( ( ( compressed[0x02] & 0x03 ) == 0x00 ) &&
		     ( compressed[0x03] == 'M' ) &&
		     ( compressed[0x04] == 'Z' ) ) {
			DBG ( "...checking for LZNT1-compressed bootmgr.exe at "
			      "+%#zx\n", offset );
			decompress = lznt1_decompress;
		}

		/* Check for an embedded XCA-compressed bootmgr.exe.
		 * The bytes 0x00, 'M', and 'Z' will always be
		 * present, and so the corresponding symbols must have
		 * a non-zero Huffman length.  The embedded image
		 * tends to have a large block of zeroes immediately
		 * beforehand, which we check for.  It's implausible
		 * that the compressed data could contain substantial
		 * runs of zeroes, so we check for that too, in order
		 * to eliminate some common false positive matches.
		 */
		if ( ( ( compressed[0x00] & 0x0f ) != 0x00 ) &&
		     ( ( compressed[0x26] & 0xf0 ) != 0x00 ) &&
		     ( ( compressed[0x2d] & 0x0f ) != 0x00 ) &&
		     ( is_empty_pgh ( compressed - 0x10 ) ) &&
		     ( ! is_empty_pgh ( ( compressed + 0x400 ) ) ) &&
		     ( ! is_empty_pgh ( ( compressed + 0x800 ) ) ) &&
		     ( ! is_empty_pgh ( ( compressed + 0xc00 ) ) ) ) {
			DBG ( "...checking for XCA-compressed bootmgr.exe at "
			      "+%#zx\n", offset );
			decompress = xca_decompress;
		}

		/* If we have not found a possible bootmgr.exe, skip
		 * to the next paragraph.
		 */
		if ( ! decompress )
			continue;

		/* Find length of decompressed image */
		decompressed_len = decompress ( compressed, compressed_len,
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
		decompress ( compressed, compressed_len, initrd );

		/* Add decompressed image */
		return vdisk_add_file ( "bootmgr.exe", initrd,
					decompressed_len, read_file );
	}

	DBG ( "...no embedded bootmgr.exe found\n" );
	return NULL;
}

/**
 * File handler
 *
 * @v name		File name
 * @v data		File data
 * @v len		Length
 * @ret rc		Return status code
 */
static int add_file ( const char *name, void *data, size_t len ) {
	struct vdisk_file *file;

	/* Store file */
	file = vdisk_add_file ( name, data, len, read_file );

	/* Check for special-case files */
	if ( strcasecmp ( name, "bootmgr.exe" ) == 0 ) {
		DBG ( "...found bootmgr.exe\n" );
		bootmgr = file;
	} else if ( strcasecmp ( name, "bootmgr" ) == 0 ) {
		DBG ( "...found bootmgr\n" );
		if ( ( ! bootmgr ) &&
		     ( bootmgr = add_bootmgr ( data, len ) ) ) {
			DBG ( "...extracted bootmgr.exe\n" );
		}
	} else if ( strcasecmp ( ( name + strlen ( name ) - 4 ),
				 ".wim" ) == 0 ) {
		DBG ( "...found WIM file %s\n", name );
		file->patch = patch_wim;
		if ( ( ! bootmgr ) &&
		     ( bootmgr = wim_add_file ( file, cmdline_index,
						bootmgr_path,
						L"bootmgr.exe" ) ) ) {
			DBG ( "...extracted bootmgr.exe\n" );
		}
	}

	return 0;
}

/**
 * Main entry point
 *
 */
int main ( void ) {
	size_t padded_len;
	void *raw_pe;
	struct loaded_pe pe;

	/* Print welcome banner */
	printf ( "\n\nwimboot " VERSION " -- Windows Imaging Format "
		 "bootloader -- http://ipxe.org/wimboot\n\n" );

	/* Process command line */
	process_cmdline ( cmdline );

	/* Extract files from initrd */
	if ( cpio_extract ( initrd, initrd_len, add_file ) != 0 )
		die ( "FATAL: could not extract initrd files\n" );

	/* Add INT 13 drive */
	callback.drive = initialise_int13();

	/* Read bootmgr.exe into memory */
	if ( ! bootmgr )
		die ( "FATAL: no bootmgr.exe\n" );
	if ( bootmgr->read == read_file ) {
		raw_pe = bootmgr->opaque;
	} else {
		padded_len = ( ( bootmgr->len + PAGE_SIZE - 1 ) &
			       ~( PAGE_SIZE -1 ) );
		raw_pe = ( initrd - padded_len );
		bootmgr->read ( bootmgr, raw_pe, 0, bootmgr->len );
	}

	/* Load bootmgr.exe into memory */
	if ( load_pe ( raw_pe, bootmgr->len, &pe ) != 0 )
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
	if ( cmdline_pause )
		pause();
	pe.entry ( &bootapps.bootapp );
	die ( "FATAL: bootmgr.exe returned\n" );
}
