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
 * Virtual disk
 *
 */

#include <string.h>
#include <stdio.h>
#include "wimboot.h"
#include "vdisk.h"

/**
 * Read from virtual Master Boot Record
 *
 * @v lba		Starting LBA
 * @v count		Number of blocks to read
 * @v data		Data buffer
 */
static void vdisk_mbr ( uint64_t lba __attribute__ (( unused )),
			unsigned int count __attribute__ (( unused )),
			void *data ) {
	struct vdisk_mbr *mbr = data;

	/* Construct MBR */
	memset ( mbr, 0, sizeof ( *mbr ) );
	mbr->magic = VDISK_MBR_MAGIC;
	mbr->signature = VDISK_SIGNATURE;
	mbr->partitions[0].bootable = VDISK_MBR_BOOTABLE;
	mbr->partitions[0].type = VDISK_MBR_TYPE_FAT32;
	mbr->partitions[0].start = VDISK_VBR;
	mbr->partitions[0].length = ( VDISK_COUNT - VDISK_VBR );
}

/** A virtual disk region */
struct vdisk_region {
	/** Name */
	const char *name;
	/** Starting LBA */
	uint64_t lba;
	/** Number of blocks */
	unsigned int count;
	/**
	 * Read data from this region
	 *
	 * @v start		Starting LBA
	 * @v count		Number of blocks to read
	 * @v data		Data buffer
	 */
	void ( * read ) ( uint64_t lba, unsigned int count, void *data );
};

/** Virtual disk regions */
static struct vdisk_region vdisk_regions[] = {
	{
		.name = "mbr",
		.lba = VDISK_MBR_LBA,
		.count = VDISK_MBR_COUNT,
		.read = vdisk_mbr,
	},
};

/**
 * Read from virtual disk
 *
 * @v lba		Starting LBA
 * @v count		Number of blocks to read
 * @v data		Data buffer
 */
void vdisk_read ( uint64_t lba, unsigned int count, void *data ) {
	struct vdisk_region *region = NULL;
	uint64_t start = lba;
	uint64_t end = ( lba + count );
	uint64_t frag_start = start;
	uint64_t frag_end;
	uint64_t region_start;
	uint64_t region_end;
	unsigned int frag_count;
	unsigned int i;

	do {
		/* Truncate fragment to region boundaries */
		frag_end = end;
		for ( i = 0 ; i < ( sizeof ( vdisk_regions ) /
				    sizeof ( vdisk_regions[0] ) ) ; i++ ) {
			region = &vdisk_regions[i];
			region_start = region->lba;
			region_end = ( region_start + region->count );

			/* Avoid crossing start of any region */
			if ( ( frag_start < region_start ) &&
			     ( frag_end > region_start ) ){
				frag_end = region_start;
			}

			/* Ignore unless we overlap with this region */
			if ( frag_start >= region_end ) {
				region = NULL;
				continue;
			}

			/* Avoid crossing end of region */
			if ( frag_end > region_end )
				frag_end = region_end;

			/* Found a suitable region */
			break;
		}
		frag_count = ( frag_end - frag_start );
		printf ( "Region %#llx+%#x to %p (%s)\n",
			 frag_start, frag_count, data,
			 ( region ? region->name : "empty" ) );

		/* Generate data from this region */
		if ( region ) {
			region->read ( frag_start, frag_count, data );
		} else {
			memset ( data, 0, ( frag_count * VDISK_BLKSIZE ) );
		}

		/* Move to next fragment */ 
		frag_start += frag_count;
		data += ( frag_count * VDISK_BLKSIZE );

	} while ( frag_start != end );
}
