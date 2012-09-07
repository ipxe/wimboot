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
	mbr->partitions[0].bootable = VDISK_MBR_BOOTABLE;
	mbr->partitions[0].type = VDISK_MBR_TYPE_FAT32;
	mbr->partitions[0].start = VDISK_PARTITION_LBA;
	mbr->partitions[0].length = VDISK_PARTITION_COUNT;
	mbr->signature = VDISK_MBR_SIGNATURE;
	mbr->magic = VDISK_MBR_MAGIC;
}

/**
 * Read from virtual Volume Boot Record
 *
 * @v lba		Starting LBA
 * @v count		Number of blocks to read
 * @v data		Data buffer
 */
static void vdisk_vbr ( uint64_t lba __attribute__ (( unused )),
			unsigned int count __attribute__ (( unused )),
			void *data ) {
	struct vdisk_vbr *vbr = data;

	/* Construct VBR */
	memset ( vbr, 0, sizeof ( *vbr ) );
	vbr->jump[0] = VDISK_VBR_JUMP_WTF_MS;
	memcpy ( vbr->oemid, VDISK_VBR_OEMID, sizeof ( vbr->oemid ) );
	vbr->bytes_per_sector = VDISK_SECTOR_SIZE;
	vbr->sectors_per_cluster = VDISK_CLUSTER_COUNT;
	vbr->reserved_sectors = VDISK_RESERVED_SECTORS;
	vbr->fats = 1;
	vbr->media = VDISK_VBR_MEDIA;
	vbr->sectors_per_track = VDISK_SECTORS_PER_TRACK;
	vbr->heads = VDISK_HEADS;
	vbr->hidden_sectors = VDISK_VBR_LBA;
	vbr->sectors = VDISK_PARTITION_COUNT;
	vbr->sectors_per_fat = VDISK_SECTORS_PER_FAT;
	vbr->root = VDISK_ROOT_CLUSTER;
	vbr->fsinfo = VDISK_FSINFO_SECTOR;
	vbr->backup = VDISK_BACKUP_VBR_SECTOR;
	vbr->signature = VDISK_VBR_SIGNATURE;
	vbr->serial = VDISK_VBR_SERIAL;
	memcpy ( vbr->label, VDISK_VBR_LABEL, sizeof ( vbr->label ) );
	memcpy ( vbr->system, VDISK_VBR_SYSTEM, sizeof ( vbr->system ) );
	vbr->magic = VDISK_VBR_MAGIC;
}

/**
 * Read from virtual FSInfo
 *
 * @v lba		Starting LBA
 * @v count		Number of blocks to read
 * @v data		Data buffer
 */
static void vdisk_fsinfo ( uint64_t lba __attribute__ (( unused )),
			   unsigned int count __attribute__ (( unused )),
			   void *data ) {
	struct vdisk_fsinfo *fsinfo = data;

	/* Construct FSInfo */
	memset ( fsinfo, 0, sizeof ( *fsinfo ) );
	fsinfo->magic1 = VDISK_FSINFO_MAGIC1;
	fsinfo->magic2 = VDISK_FSINFO_MAGIC2;
	fsinfo->next_free = VDISK_FSINFO_NEXT_FREE;
	fsinfo->magic3 = VDISK_FSINFO_MAGIC3;
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
		.name = "MBR",
		.lba = VDISK_MBR_LBA,
		.count = VDISK_MBR_COUNT,
		.read = vdisk_mbr,
	},
	{
		.name = "VBR",
		.lba = VDISK_VBR_LBA,
		.count = VDISK_VBR_COUNT,
		.read = vdisk_vbr,
	},
	{
		.name = "FSInfo",
		.lba = VDISK_FSINFO_LBA,
		.count = VDISK_FSINFO_COUNT,
		.read = vdisk_fsinfo,
	},
	{
		.name = "VBR backup",
		.lba = VDISK_BACKUP_VBR_LBA,
		.count = VDISK_BACKUP_VBR_COUNT,
		.read = vdisk_vbr,
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
			if ( ( frag_start >= region_end ) ||
			     ( frag_end <= region_start ) ) {
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
		printf ( "     ...%p from %#llx+%#x (%s)\n",
			 data, frag_start, frag_count,
			 ( region ? region->name : "empty" ) );

		/* Generate data from this region */
		if ( region ) {
			region->read ( frag_start, frag_count, data );
		} else {
			memset ( data, 0, ( frag_count * VDISK_SECTOR_SIZE ) );
		}

		/* Move to next fragment */ 
		frag_start += frag_count;
		data += ( frag_count * VDISK_SECTOR_SIZE );

	} while ( frag_start != end );
}
