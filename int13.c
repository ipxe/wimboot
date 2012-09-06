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
 * INT 13 emulation
 *
 */

#include <string.h>
#include <stdio.h>
#include "wimboot.h"
#include "int13.h"
#include "vdisk.h"

/**
 * INT 13, 08 - Get drive parameters
 *
 * @ret ch		Low bits of maximum cylinder number
 * @ret cl (bits 7:6)	High bits of maximum cylinder number
 * @ret cl (bits 5:0)	Maximum sector number
 * @ret dh		Maximum head number
 * @ret dl		Number of drives
 * @ret ah		Status code
 */
static void int13_get_parameters ( struct bootapp_callback_params *params ) {
	unsigned int num_drives;
	unsigned int min_num_drives;

	/* Calculate number of drives to report */
	num_drives = BIOS_DRIVE_COUNT;
	min_num_drives = ( EMULATED_DRIVE & 0x7f );
	if ( num_drives < min_num_drives )
		num_drives = min_num_drives;

	/* Fill in drive parameters */
	params->ch = ( MAX_CHS_CYLINDER & 0xff );
	params->cl = ( ( ( MAX_CHS_CYLINDER >> 8 ) << 6 ) | MAX_CHS_SECTOR );
	params->dh = MAX_CHS_HEAD;
	params->dl = BIOS_DRIVE_COUNT + 1;

	/* Success */
	params->ah = 0;
}

/**
 * INT 13, 15 - Get disk type
 *
 * @ret cx:dx		Sector count
 * @ret ah		Type code
 */
static void int13_get_disk_type ( struct bootapp_callback_params *params ) {

	/* Fill in disk type */
	params->cx = ( MAX_SECTOR >> 16 );
	params->dx = ( MAX_SECTOR & 0xffff );
	params->ah = INT13_DISK_TYPE_HDD;
}

/**
 * INT 13, 41 - Extensions installation check
 *
 * @v bx		0x55aa
 * @ret bx		0xaa55
 * @ret cx		Extensions API support bitmap
 * @ret ah		API version
 */
static void int13_extension_check ( struct bootapp_callback_params *params ) {

	/* Fill in extension information */
	params->bx = 0xaa55;
	params->cx = INT13_EXTENSION_LINEAR;
	params->ah = INT13_EXTENSION_VER_1_X;
}

/**
 * INT 13, 48 - Get extended parameters
 *
 * @v ds:si		Drive parameter table
 * @ret ah		Status code
 */
static void
int13_get_extended_parameters ( struct bootapp_callback_params *params ) {
	struct int13_disk_parameters *disk_params;

	/* Fill in extended parameters */
	disk_params = REAL_PTR ( params->ds, params->si );
	memset ( disk_params, 0, sizeof ( *disk_params ) );
	disk_params->bufsize = sizeof ( *disk_params );
	disk_params->flags = INT13_FL_DMA_TRANSPARENT;
	disk_params->sectors = ( MAX_SECTOR + 1 );
	disk_params->sector_size = VDISK_BLKSIZE;

	/* Success */
	params->ah = 0;
}

/**
 * INT 13, 42 - Extended read
 *
 * @v ds:si		Disk address packet
 * @ret ah		Status code
 */
static void int13_extended_read ( struct bootapp_callback_params *params ) {
	struct int13_disk_address *disk_address;
	void *data;
	int rc;

	/* Read from emulated disk */
	disk_address = REAL_PTR ( params->ds, params->si );
	data = REAL_PTR ( disk_address->buffer.segment,
			  disk_address->buffer.offset );
	if ( ( rc = vdisk_read ( disk_address->lba, disk_address->count,
				 data ) ) != 0 ) {
		params->ah = INT13_STATUS_READ_ERROR;
		params->eflags |= CF;
		return;
	}

	/* Success */
	params->ah = 0;
}

/**
 * Emulate INT 13 drive
 *
 * @v params		Parameters
 */
void emulate_int13 ( struct bootapp_callback_params *params ) {
	int command = params->ah;

	/* Populate eflags with a sensible starting value */
	__asm__ ( "pushfl\n\t"
		  "popl %0\n\t"
		  : "=r" ( params->eflags ) );
	params->eflags &= ~CF;

	/* Handle command */
	switch ( command ) {
	case INT13_GET_PARAMETERS:
		int13_get_parameters ( params );
		break;
	case INT13_GET_DISK_TYPE:
		int13_get_disk_type ( params );
		break;
	case INT13_EXTENSION_CHECK:
		int13_extension_check ( params );
		break;
	case INT13_GET_EXTENDED_PARAMETERS:
		int13_get_extended_parameters ( params );
		break;
	case INT13_EXTENDED_READ:
		int13_extended_read ( params );
		break;
	default:
		printf ( "Unrecognised INT 13,%02x\n", command );
		params->eflags |= CF;
		break;
	}
}
