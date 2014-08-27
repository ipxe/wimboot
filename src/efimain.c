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
 * EFI entry point
 *
 */

#include <stdio.h>
#include "wimboot.h"
#include "efi.h"
#include "efifile.h"

/**
 * EFI entry point
 *
 * @v image_handle	Image handle
 * @v systab		EFI system table
 * @ret efirc		EFI status code
 */
EFI_STATUS EFIAPI efi_main ( EFI_HANDLE image_handle,
			     EFI_SYSTEM_TABLE *systab ) {
	EFI_BOOT_SERVICES *bs;
	union {
		EFI_LOADED_IMAGE_PROTOCOL *image;
		void *interface;
	} loaded;
	EFI_STATUS efirc;

	/* Record EFI handle and system table */
	efi_image_handle = image_handle;
	efi_systab = systab;
	bs = systab->BootServices;

	/* Print welcome banner */
	printf ( "\n\nwimboot " VERSION " -- Windows Imaging Format "
		 "bootloader -- http://ipxe.org/wimboot\n\n" );

	/* Get loaded image protocol */
	if ( ( efirc = bs->OpenProtocol ( image_handle,
					  &efi_loaded_image_protocol_guid,
					  &loaded.interface, image_handle, NULL,
					  EFI_OPEN_PROTOCOL_GET_PROTOCOL ))!=0){
		die ( "Could not open loaded image protocol: %#lx\n",
		      ( ( unsigned long ) efirc ) );
	}

	/* Extract files from file system */
	efi_extract ( loaded.image->DeviceHandle );

	return 0;
}
