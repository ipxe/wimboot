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
 * WIM dynamic patching
 *
 */

#include <stdio.h>
#include <string.h>
#include "wimboot.h"
#include "cmdline.h"
#include "vdisk.h"
#include "wim.h"
#include "wimpatch.h"

/** A WIM patch */
struct wim_patch {
	/** Boot metadata */
	struct wim_resource_header boot;
};

/**
 * Generate WIM patch
 *
 * @v file		Virtual file
 * @v boot_index	New boot index (or zero)
 * @v patch		Patch to fill in
 * @ret rc		Return status code
 */
static int generate_patch ( struct vdisk_file *file, unsigned int boot_index,
			    struct wim_patch *patch ) {
	struct wim_header header;
	int rc;

	/* Read WIM header */
	if ( ( rc = wim_header ( file, &header ) ) != 0 )
		return rc;

	/* Get selected boot image metadata */
	if ( ( rc = wim_metadata ( file, &header, boot_index,
				   &patch->boot ) ) != 0 )
		return rc;

	return 0;
}

/**
 * Patch WIM file
 *
 * @v file		Virtual file
 * @v data		Data buffer
 * @v offset		Offset
 * @v len		Length
 */
void patch_wim ( struct vdisk_file *file, void *data, size_t offset,
		 size_t len ) {
	static struct vdisk_file *cached_file;
	static struct wim_patch cached_patch;
	struct wim_patch *patch;
	struct wim_header *wimhdr;
	int rc;

	/* Do nothing unless boot index patching is enabled */
	if ( ! cmdline_index )
		return;

	/* Update cached patch if required */
	if ( file != cached_file ) {
		DBG ( "...patching WIM %s\n", file->name );
		if ( ( rc = generate_patch ( file, cmdline_index,
					     &cached_patch ) ) != 0 ) {
			die ( "Could not patch WIM\n" );
		}
		cached_file = file;
	}
	patch = &cached_patch;

	/* Patch boot index if applicable */
	if ( ( offset == 0 ) && ( len != 0 ) ) {
		wimhdr = data;
		DBG ( "...patched WIM: boot index %d to %d\n",
		      wimhdr->boot_index, cmdline_index );
		memcpy ( &wimhdr->boot, &patch->boot, sizeof ( wimhdr->boot ) );
		wimhdr->boot_index = cmdline_index;
	}
}
