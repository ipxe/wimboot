/*
 * Copyright (C) 2014 Michael Brown <mbrown@fensystems.co.uk>.
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
 * EFI file system access
 *
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include "wimboot.h"
#include "vdisk.h"
#include "cmdline.h"
#include "cpio.h"
#include "wimpatch.h"
#include "wimfile.h"
#include "efi.h"
#include "efifile.h"
#include "efipath.h"
#include "efi/Protocol/LoadFile2.h"
#include "efi/Guid/LinuxEfiInitrdMedia.h"

/** bootmgfw.efi path within WIM */
static const wchar_t bootmgfw_path[] = L"\\Windows\\Boot\\EFI\\bootmgfw.efi";

/** Other paths within WIM */
static const wchar_t *efi_wim_paths[] = {
	L"\\Windows\\Boot\\DVD\\EFI\\boot.sdi",
	L"\\Windows\\Boot\\DVD\\EFI\\BCD",
	L"\\Windows\\Boot\\Fonts\\segmono_boot.ttf",
	L"\\Windows\\Boot\\Fonts\\segoen_slboot.ttf",
	L"\\Windows\\Boot\\Fonts\\segoe_slboot.ttf",
	L"\\Windows\\Boot\\Fonts\\wgl4_boot.ttf",
	L"\\sms\\boot\\boot.sdi",
	NULL
};

/** Linux initrd media device path */
static struct {
	VENDOR_DEVICE_PATH vendor;
	EFI_DEVICE_PATH_PROTOCOL end;
} __attribute__ ((packed)) efi_initrd_path = {
	.vendor = {
		.Header = EFI_DEVPATH_INIT (efi_initrd_path.vendor,
									MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP),
		.Guid = LINUX_EFI_INITRD_MEDIA_GUID,
	},
	.end = EFI_DEVPATH_END_INIT (efi_initrd_path.end),
};

/** bootmgfw.efi file */
struct vdisk_file *bootmgfw;


/** WIM image file */
static struct vdisk_file *bootwim;

static void ( * efi_read_func ) ( struct vdisk_file *file, void *data,
								  size_t offset, size_t len );

/**
 * Get architecture-specific boot filename
 *
 * @ret bootarch	Architecture-specific boot filename
 */
static const CHAR16 * efi_bootarch ( void ) {
	static const CHAR16 bootarch_full[] = EFI_REMOVABLE_MEDIA_FILE_NAME;
	const CHAR16 *tmp;
	const CHAR16 *bootarch = bootarch_full;

	for ( tmp = bootarch_full ; *tmp ; tmp++ ) {
		if ( *tmp == L'\\' )
			bootarch = ( tmp + 1 );
	}
	return bootarch;
}

/**
 * Read from EFI file
 *
 * @v vfile		Virtual file
 * @v data		Data buffer
 * @v offset		Offset
 * @v len		Length
 */
static void efi_read_file ( struct vdisk_file *vfile, void *data,
			    size_t offset, size_t len ) {
	EFI_FILE_PROTOCOL *file = vfile->opaque;
	UINTN size = len;
	EFI_STATUS efirc;

	/* Set file position */
	if ( ( efirc = file->SetPosition ( file, offset ) ) != 0 ) {
		die ( "Could not set file position: %#lx\n",
		      ( ( unsigned long ) efirc ) );
	}

	/* Read from file */
	if ( ( efirc = file->Read ( file, &size, data ) ) != 0 ) {
		die ( "Could not read from file: %#lx\n",
		      ( ( unsigned long ) efirc ) );
	}
}

/**
 * Patch BCD file
 *
 * @v vfile		Virtual file
 * @v data		Data buffer
 * @v offset		Offset
 * @v len		Length
 */
static void efi_patch_bcd ( struct vdisk_file *vfile __unused, void *data,
			    size_t offset, size_t len ) {
	static const wchar_t search[] = L".exe";
	static const wchar_t replace[] = L".efi";
	size_t i;

	/* Do nothing if BCD patching is disabled */
	if ( cmdline_rawbcd )
		return;

	/* Patch any occurrences of ".exe" to ".efi".  In the common
	 * simple cases, this allows the same BCD file to be used for
	 * both BIOS and UEFI systems.
	 */
	for ( i = 0 ; ( i + sizeof ( search ) ) < len ; i++ ) {
		if ( wcscasecmp ( ( data + i ), search ) == 0 ) {
			memcpy ( ( data + i ), replace, sizeof ( replace ) );
			DBG ( "...patched BCD at %#zx: \"%ls\" to \"%ls\"\n",
			      ( offset + i ), search, replace );
		}
	}
}

/**
 * File handler
 *
 * @v name		File name
 * @v data		File data
 * @v len		Length
 * @ret rc		Return status code
 */
static int efi_add_file ( const char *name, void *data, size_t len) {
	struct vdisk_file *vfile;
	char bootarch[32];

	snprintf ( bootarch, sizeof ( bootarch ), "%ls", efi_bootarch() );

	vfile = vdisk_add_file ( name, data, len, efi_read_func );

	/* Check for special-case files */
	if ( ( strcasecmp ( name, bootarch ) == 0 ) ||
		 ( strcasecmp ( name, "bootmgfw.efi" ) == 0 ) ) {
		DBG ( "...found bootmgfw.efi file %s\n", name );
		bootmgfw = vfile;
	} else if ( strcasecmp ( name, "BCD" ) == 0 ) {
		DBG ( "...found BCD\n" );
		vdisk_patch_file ( vfile, efi_patch_bcd );
	} else if ( strcasecmp ( ( name + ( strlen ( name ) - 4 ) ),
				".wim" ) == 0 ) {
		DBG ( "...found WIM file %s\n", name );
		bootwim = vfile;
	}

	return 0;
}

/**
 * Extract files from Linux initrd media
 *
 * @ret rc		Return status code
 */
static int
efi_extract_initrd (void)
{
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_HANDLE lf2_handle;
	EFI_LOAD_FILE2_PROTOCOL *lf2;
	EFI_DEVICE_PATH_PROTOCOL *dp = ( EFI_DEVICE_PATH_PROTOCOL * ) &efi_initrd_path;
	UINTN initrd_len = 0;
	UINTN pages;
	void *initrd;
	EFI_PHYSICAL_ADDRESS phys;
	EFI_STATUS efirc;

	/* Locate initrd media device */
	efirc = bs->LocateDevicePath ( &efi_load_file2_protocol_guid,
								   &dp, &lf2_handle );
	if ( efirc != EFI_SUCCESS )
		return -1;
	DBG ( "...found initrd media device\n" );

	/* Get LoadFile2 protocol */
	efirc = bs->HandleProtocol ( lf2_handle, &efi_load_file2_protocol_guid,
								 ( void ** ) &lf2 );
	if ( efirc != EFI_SUCCESS )
		die ( "Could not get LoadFile2 protocol.\n" );

	/* Get initrd size */
	efirc = lf2->LoadFile ( lf2, dp, FALSE, &initrd_len, NULL );
	if ( initrd_len == 0 )
		die ( "Could not get initrd size\n" );

	/* Allocate memory */
	pages = ( ( initrd_len + PAGE_SIZE - 1 ) / PAGE_SIZE );
	if ( ( efirc = bs->AllocatePages ( AllocateAnyPages,
					   EfiLoaderData, pages,
					   &phys ) ) != 0 ) {
		die ( "Could not allocate %ld pages: %#lx\n",
		      ( ( unsigned long ) pages ), ( ( unsigned long ) efirc ) );
	}
	initrd = ( ( void * ) ( intptr_t ) phys );

	/* Read initrd */
	efirc = lf2->LoadFile ( lf2, dp, FALSE, &initrd_len, initrd );
	if (efirc != EFI_SUCCESS)
		die ("Could not read initrd.\n");

	efi_read_func = vdisk_read_mem_file;
	if ( cpio_extract ( initrd, initrd_len, efi_add_file ) != 0 )
		die ( "FATAL: could not extract initrd files\n" );

	return 0;
}

/**
 * Extract files from EFI file system
 *
 * @v handle		Device handle
 */
void efi_extract ( EFI_HANDLE handle ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	union {
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
		void *interface;
	} fs;
	struct {
		EFI_FILE_INFO file;
		CHAR16 name[ VDISK_NAME_LEN + 1 /* WNUL */ ];
	} __attribute__ (( packed )) info;
	char name[ VDISK_NAME_LEN + 1 /* NUL */ ];
	EFI_FILE_PROTOCOL *root;
	EFI_FILE_PROTOCOL *file;
	UINTN size;
	CHAR16 *wname;
	EFI_STATUS efirc;

	/* Extract files from initrd media */
	if ( efi_extract_initrd () == 0 )
	  goto proc_wim;

	/* Open file system */
	if ( ( efirc = bs->OpenProtocol ( handle,
					  &efi_simple_file_system_protocol_guid,
					  &fs.interface, efi_image_handle, NULL,
					  EFI_OPEN_PROTOCOL_GET_PROTOCOL ))!=0){
		die ( "Could not open simple file system: %#lx\n",
		      ( ( unsigned long ) efirc ) );
	}

	/* Open root directory */
	if ( ( efirc = fs.fs->OpenVolume ( fs.fs, &root ) ) != 0 ) {
		die ( "Could not open root directory: %#lx\n",
		      ( ( unsigned long ) efirc ) );
	}

	/* Close file system */
	bs->CloseProtocol ( handle, &efi_simple_file_system_protocol_guid,
			    efi_image_handle, NULL );

	/* Read root directory */
	while ( 1 ) {

		/* Read directory entry */
		size = sizeof ( info );
		if ( ( efirc = root->Read ( root, &size, &info ) ) != 0 ) {
			die ( "Could not read root directory: %#lx\n",
			      ( ( unsigned long ) efirc ) );
		}
		if ( size == 0 )
			break;

		/* Ignore subdirectories */
		if ( info.file.Attribute & EFI_FILE_DIRECTORY )
			continue;

		/* Open file */
		wname = info.file.FileName;
		if ( ( efirc = root->Open ( root, &file, wname,
					    EFI_FILE_MODE_READ, 0 ) ) != 0 ) {
			die ( "Could not open \"%ls\": %#lx\n",
			      wname, ( ( unsigned long ) efirc ) );
		}

		/* Add file */
		snprintf ( name, sizeof ( name ), "%ls", wname );
		efi_read_func = efi_read_file;
		efi_add_file ( name, file, info.file.FileSize );
	}

proc_wim:
	/* Process WIM image */
	if ( bootwim ) {
		vdisk_patch_file ( bootwim, patch_wim );
		if ( ( ! bootmgfw ) &&
		     ( bootmgfw = wim_add_file ( bootwim, cmdline_index,
						 bootmgfw_path,
						 efi_bootarch() ) ) ) {
			DBG ( "...extracted %ls\n", bootmgfw_path );
		}
		wim_add_files ( bootwim, cmdline_index, efi_wim_paths );
	}

	/* Check that we have a boot file */
	if ( ! bootmgfw ) {
		die ( "FATAL: no %ls or bootmgfw.efi found\n",
		      efi_bootarch() );
	}
}
