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
	/** Lookup table */
	struct wim_resource_header lookup;
	/** Boot metadata */
	struct wim_resource_header boot;
};

/**
 * Get WIM image metadata
 *
 * @v file		Virtual file
 * @v lookup		Lookup table
 * @v index		Image index
 * @v meta		Metadata to fill in
 */
static void get_metadata ( struct vdisk_file *file,
			   struct wim_resource_header *lookup,
			   unsigned int index,
			   struct wim_resource_header *meta ) {
	struct wim_lookup_entry entry;
	size_t offset = lookup->offset;
	size_t remaining = ( lookup->zlen__flags & WIM_RESHDR_ZLEN_MASK );
	unsigned int found = 0;

	/* Fail if lookup table itself is compressed */
	if ( lookup->zlen__flags & ( WIM_RESHDR_COMPRESSED |
				     WIM_RESHDR_PACKED_STREAMS ) ) {
		die ( "Cannot handle compressed WIM lookup table in %s\n",
		      file->name );
	}

	/* Look for metadata entry */
	DBG ( "...lookup table at [%#zx,%#zx)\n", offset,
	      ( offset + remaining ) );
	while ( remaining >= sizeof ( entry ) ) {

		/* Read entry */
		file->read ( file, &entry, offset, sizeof ( entry ) );

		/* Look for our target entry */
		if ( entry.resource.zlen__flags & WIM_RESHDR_METADATA ) {
			found++;
			DBG ( "...found image %d metadata at %#zx\n",
			      found, offset );
			if ( found == index ) {
				memcpy ( meta, &entry.resource,
					 sizeof ( *meta ) );
				return;
			}
		}

		/* Move to next entry */
		offset += sizeof ( entry );
		remaining -= sizeof ( entry );
	}

	/* Fail if index was not found */
	die ( "Cannot find WIM image index %d in %s\n", index, file->name );
}

/**
 * Generate WIM patch
 *
 * @v file		Virtual file
 * @v boot_index	New boot index, or zero
 * @v patch		Patch to fill in
 */
static void generate_patch ( struct vdisk_file *file, unsigned int boot_index,
			     struct wim_patch *patch ) {
	struct wim_header wimhdr;

	/* Read lookup table */
	file->read ( file, &wimhdr, 0, sizeof ( wimhdr ) );
	memcpy ( &patch->lookup, &wimhdr.lookup, sizeof ( patch->lookup ) );

	/* Get boot image metadata, if applicable */
	if ( boot_index )
		get_metadata ( file, &patch->lookup, boot_index, &patch->boot );
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

	/* Do nothing unless boot index patching is enabled */
	if ( ! cmdline_index )
		return;

	/* Update cached patch if required */
	if ( file != cached_file ) {
		DBG ( "...patching WIM %s\n", file->name );
		generate_patch ( file, cmdline_index, &cached_patch );
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
