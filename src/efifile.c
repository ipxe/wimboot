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
#include "wimboot.h"
#include "vdisk.h"
#include "efi.h"
#include "efifile.h"

/**
 * Read from EFI file
 *
 * @v data		Data buffer
 * @v opaque		Opaque token
 * @v offset		Offset
 * @v len		Length
 */
static void efi_read_file ( void *data, void *opaque, size_t offset,
			    size_t len ) {
	EFI_FILE_PROTOCOL *file = opaque;
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
		CHAR16 name[ sizeof ( vdisk_files[0].name ) ];
	} __attribute__ (( packed )) info;
	struct vdisk_file *vdisk_file;
	EFI_FILE_PROTOCOL *root;
	EFI_FILE_PROTOCOL *file;
	UINTN size;
	EFI_STATUS efirc;
	unsigned int idx = 0;

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

		/* Sanity check */
		if ( idx >= VDISK_MAX_FILES )
			die ( "Too many files\n" );

		/* Open file */
		if ( ( efirc = root->Open ( root, &file, info.file.FileName,
					    EFI_FILE_MODE_READ, 0 ) ) != 0 ) {
			die ( "Could not open \"%ls\": %#lx\n",
			      info.file.FileName, ( ( unsigned long ) efirc ) );
		}

		/* Add file */
		vdisk_file = &vdisk_files[idx++];
		snprintf ( vdisk_file->name, sizeof ( vdisk_file->name ),
			   "%ls", info.file.FileName );
		vdisk_file->opaque = file;
		vdisk_file->len = info.file.FileSize;
		vdisk_file->read = efi_read_file;
		DBG ( "Using %s via %p len %#zx\n", vdisk_file->name, 
		      vdisk_file->opaque, vdisk_file->len );
	}
}
