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
 * EFI boot manager invocation
 *
 */

#include <stdio.h>
#include <string.h>
#include "wimboot.h"
#include "efi.h"
#include "efipath.h"
#include "efiboot.h"

/**
 * Find end of file name
 *
 * @v name		File name
 * @ret name_end	End of file name
 */
static const CHAR16 * efi_name_end ( const CHAR16 *name ) {

	while ( *name )
		name++;
	return name;
}

/**
 * Try booting from file on EFI device
 *
 * @v parent		Parent EFI device path
 * @v name		File name
 * @ret efirc		EFI status code
 */
static EFI_STATUS efi_try_boot ( EFI_DEVICE_PATH_PROTOCOL *parent,
				 const CHAR16 *name ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_DEVICE_PATH_PROTOCOL *parent_end = efi_devpath_end ( parent );
	const CHAR16 *name_end = efi_name_end ( name );
	size_t prefix_len = ( ( ( void * ) parent_end ) -
			      ( ( void * ) parent ) );
	size_t name_len = ( ( ( void * ) name_end ) - ( ( void * ) name ) );
	size_t filepath_len = ( SIZE_OF_FILEPATH_DEVICE_PATH +
				name_len + sizeof ( CHAR16 ) /* NUL */ );
	struct {
		union {
			EFI_DEVICE_PATH_PROTOCOL path;
			uint8_t pad[prefix_len];
		} __attribute__ (( packed )) prefix;
		union {
			FILEPATH_DEVICE_PATH filepath;
			uint8_t pad[filepath_len];
		} __attribute__ (( packed )) filepath;
		EFI_DEVICE_PATH_PROTOCOL end;
	} __attribute__ (( packed )) path;
	EFI_HANDLE handle;
	EFI_STATUS efirc;

	/* Construct device path */
	memcpy ( &path.prefix.path, parent, prefix_len );
	efi_devpath_init ( &path.filepath.filepath.Header, MEDIA_DEVICE_PATH,
			   MEDIA_FILEPATH_DP, filepath_len );
	memcpy ( path.filepath.filepath.PathName, name,
		 ( name_len + sizeof ( CHAR16 ) /* NUL */ ) );
	efi_devpath_end_init ( &path.end );

	/* Load image */
	if ( ( efirc = bs->LoadImage ( FALSE, efi_image_handle,
				       &path.prefix.path, NULL, 0,
				       &handle ) ) != 0 ) {
		printf ( "Could not load %ls: %#lx\n",
			 name, ( ( unsigned long ) efirc ) );
		return efirc;
	}

	/* Start image */
	if ( ( efirc = bs->StartImage ( handle, NULL, NULL ) ) != 0 ) {
		printf ( "Could not start %ls: %#lx\n",
			 name, ( ( unsigned long ) efirc ) );
		return efirc;
	}

	return 0;
}

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
			bootarch = tmp;
	}
	return bootarch;
}

/**
 * Boot from EFI device
 *
 * @v parent		Parent EFI device path
 */
void efi_boot ( EFI_DEVICE_PATH_PROTOCOL *parent ) {

	/* Try booting from architecture-specific boot file first, then from
	 * bootmgfw.efi.
	 */
	efi_try_boot ( parent, efi_bootarch() );
	efi_try_boot ( parent, L"\\bootmgfw.efi" );
	die ( "Could not boot\n" );
}
