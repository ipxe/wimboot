#ifndef _VDISK_H
#define _VDISK_H

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
 * Virtual disk emulation
 *
 */

#include <stdint.h>

/** Drive number */
#define VDISK_DRIVE 0x80

/** Number of cylinders */
#define VDISK_CYLINDERS 1024

/** Number of heads */
#define VDISK_HEADS 255

/** Number of sectors per track */
#define VDISK_SECTORS_PER_TRACK 63

/** Sector size */
#define VDISK_BLKSIZE 512

/** Number of sectors */
#define VDISK_COUNT 0xffffffffUL

/** Disk signature */
#define VDISK_SIGNATURE 0xc0ffeeee

/** Master Boot Record LBA */
#define VDISK_MBR_LBA 0

/** Master Boot Record count */
#define VDISK_MBR_COUNT 1

/** Partition table entry */
struct vdisk_partition {
	/** Bootable flag */
	uint8_t bootable;
	/** C/H/S start address */
	uint8_t chs_start[3];
	/** System indicator (partition type) */
	uint8_t type;
	/** C/H/S end address */
	uint8_t chs_end[3];
	/** Linear start address */
	uint32_t start;
	/** Linear length */
	uint32_t length;
} __attribute__ (( packed ));

/** Master Boot Record */
struct vdisk_mbr {	
	/** Code area */
	uint8_t code[440];
	/** Disk signature */
	uint32_t signature;
	/** Padding */
	uint8_t pad[2];
	/** Partition table */
	struct vdisk_partition partitions[4];
	/** 0x55aa MBR signature */
	uint16_t magic;
} __attribute__ (( packed ));

/** MBR magic */
#define VDISK_MBR_MAGIC 0xaa55

/** MBR boot partition indiciator */
#define VDISK_MBR_BOOTABLE 0x80

/** MBR type indicator for FAT32 */
#define VDISK_MBR_TYPE_FAT32 0x0c

/** Volume Boot Record LBA */
#define VDISK_VBR 0x80

extern void vdisk_read ( uint64_t lba, unsigned int count, void *data );

#endif /* _VDISK_H */
