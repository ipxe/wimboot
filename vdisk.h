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
#define VDISK_CYLINDERS 1024 /* Maximum possible */

/** Number of heads */
#define VDISK_HEADS 255

/** Number of sectors per track */
#define VDISK_SECTORS_PER_TRACK 63

/** Sector size (in bytes) */
#define VDISK_SECTOR_SIZE 512

/** Cluster size (in sectors) */
#define VDISK_CLUSTER_COUNT 64

/** Cluster size (in bytes) */
#define VDISK_CLUSTER_SIZE ( VDISK_CLUSTER_COUNT * VDISK_SECTOR_SIZE )

/** Number of FAT clusters */
#define VDISK_CLUSTERS 0x03ffc000ULL /* Fill 2TB disk */

/** Number of sectors allocated for FAT */
#define VDISK_SECTORS_PER_FAT						\
	( ( ( VDISK_CLUSTERS * sizeof ( uint32_t ) +			\
	      VDISK_CLUSTER_SIZE - 1 ) / VDISK_CLUSTER_SIZE )		\
	  * VDISK_CLUSTER_COUNT )

/** Number of reserved sectors */
#define VDISK_RESERVED_SECTORS 64

/** Partition start LBA */
#define VDISK_PARTITION_LBA VDISK_CLUSTER_COUNT

/** Total number of sectors within partition */
#define VDISK_PARTITION_COUNT						\
	( VDISK_RESERVED_SECTORS + VDISK_SECTORS_PER_FAT +		\
	  ( VDISK_CLUSTERS * VDISK_CLUSTER_COUNT ) )

/** Number of sectors */
#define VDISK_COUNT ( VDISK_PARTITION_LBA + VDISK_PARTITION_COUNT )

/*****************************************************************************
 *
 * Master Boot Record
 *
 *****************************************************************************
 */

/** Master Boot Record LBA */
#define VDISK_MBR_LBA 0x00000000

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
	/** 0x55aa signature */
	uint16_t magic;
} __attribute__ (( packed ));

/** MBR boot partition indiciator */
#define VDISK_MBR_BOOTABLE 0x80

/** MBR type indicator for FAT32 */
#define VDISK_MBR_TYPE_FAT32 0x0c

/** MBR signature */
#define VDISK_MBR_SIGNATURE 0xc0ffeeee

/** MBR magic */
#define VDISK_MBR_MAGIC 0xaa55

/*****************************************************************************
 *
 * Volume Boot Record
 *
 *****************************************************************************
 */

/** Volume Boot Record LBA */
#define VDISK_VBR_LBA VDISK_PARTITION_LBA

/** Volume Boot Record count */
#define VDISK_VBR_COUNT 1

/** Volume Boot Record */
struct vdisk_vbr {
	/** Jump instruction */
	uint8_t jump[3];
	/** OEM identifier */
	char oemid[8];
	/** Number of bytes per sector */
	uint16_t bytes_per_sector;
	/** Number of sectors per cluster */
	uint8_t sectors_per_cluster;
	/** Number of reserved sectors */
	uint16_t reserved_sectors;
	/** Number of FATs */
	uint8_t fats;
	/** Number of root directory entries (FAT12/FAT16 only) */
	uint16_t root_directory_entries;
	/** Total number of sectors (0 if more than 65535) */
	uint16_t sectors_short;
	/** Media descriptor type */
	uint8_t media;
	/** Number of sectors per FAT (FAT12/FAT16 only) */
	uint16_t sectors_per_fat_short;
	/** Number of sectors per track */
	uint16_t sectors_per_track;
	/** Number of heads */
	uint16_t heads;
	/** Number of hidden sectors (i.e. LBA of start of partition) */
	uint32_t hidden_sectors;
	/** Total number of sectors */
	uint32_t sectors;

	/* FAT32-specific fields */

	/** Sectors per FAT */
	uint32_t sectors_per_fat;
	/** Flags */
	uint16_t flags;
	/** FAT version number */
	uint16_t version;
	/** Root directory cluster */
	uint32_t root;
	/** FSInfo sector */
	uint16_t fsinfo;
	/** Backup boot sector */
	uint16_t backup;
	/** Reserved */
	uint8_t reserved[12];
	/** Drive number */
	uint8_t drive;
	/** Windows NT flags */
	uint8_t nt_flags;
	/** Signature */
	uint8_t signature;
	/** Volume ID serial */
	uint32_t serial;
	/** Label (space-padded) */
	char label[11];
	/** System identifier */
	char system[8];
	/** Boot code */
	uint8_t code[420];
	/** 0x55aa signature */
	uint16_t magic;
} __attribute__ (( packed ));

/** VBR jump instruction
 *
 * bootmgr.exe will actually fail unless this is present.  Someone
 * must read specification documents without bothering to understand
 * what's really happening.
 */
#define VDISK_VBR_JUMP_WTF_MS 0xe9

/** VBR OEM ID */
#define VDISK_VBR_OEMID "wimboot\0"

/** VBR media type */
#define VDISK_VBR_MEDIA 0xf8

/** VBR signature */
#define VDISK_VBR_SIGNATURE 0x29

/** VBR serial number */
#define VDISK_VBR_SERIAL 0xf00df00d

/** VBR label */
#define VDISK_VBR_LABEL "wimboot    "

/** VBR system identifier */
#define VDISK_VBR_SYSTEM "FAT32   "

/** VBR magic */
#define VDISK_VBR_MAGIC 0xaa55

/*****************************************************************************
 *
 * FSInfo
 *
 *****************************************************************************
 */

/** FSInfo sector */
#define VDISK_FSINFO_SECTOR 0x00000001

/** FSInfo LBA */
#define VDISK_FSINFO_LBA ( VDISK_VBR_LBA + VDISK_FSINFO_SECTOR )

/** FSInfo count */
#define VDISK_FSINFO_COUNT 1

/** FSInfo */
struct vdisk_fsinfo {
	/** First signature */
	uint32_t magic1;
	/** Reserved */
	uint8_t reserved_1[480];
	/** Second signature */
	uint32_t magic2;
	/** Free cluster count */
	uint32_t free_count;
	/** Next free cluster */
	uint32_t next_free;
	/** Reserved */
	uint8_t reserved_2[12];
	/** Third signature */
	uint32_t magic3;
} __attribute__ (( packed ));

/** FSInfo first signature */
#define VDISK_FSINFO_MAGIC1 0x41615252

/** FSInfo second signature */
#define VDISK_FSINFO_MAGIC2 0x61417272

/** FSInfo next free cluster */
#define VDISK_FSINFO_NEXT_FREE 0xffffffff /* No free clusters */

/** FSInfo third signature */
#define VDISK_FSINFO_MAGIC3 0xaa550000

/*****************************************************************************
 *
 * Backup Volume Boot Record
 *
 *****************************************************************************
 */

/** Backup Volume Boot Record sector */
#define VDISK_BACKUP_VBR_SECTOR 0x00000006

/** Backup Volume Boot Record LBA */
#define VDISK_BACKUP_VBR_LBA ( VDISK_VBR_LBA + VDISK_BACKUP_VBR_SECTOR )

/** Backup Volume Boot Record count */
#define VDISK_BACKUP_VBR_COUNT 1

/*****************************************************************************
 *
 * Root directory
 *
 *****************************************************************************
 */

/** Root directory cluster */
#define VDISK_ROOT_CLUSTER 2

/*****************************************************************************
 *
 * Directory entries
 *
 *****************************************************************************
 */

/** A directory entry */
struct vdisk_directory_entry {
	/** Filename */
	char filename[8];
	/** Extension */
	char extension[3];
	/** Attributes */
	uint8_t attr;
	/** Reserved */
	uint8_t reserved;
	/** Creation time in tenths of a second */
	uint8_t created_deciseconds;
	/** Creation time (HMS packed) */
	uint16_t created_time;
	/** Creation date (YMD packed) */
	uint16_t created_date;
	/** Last accessed date (YMD packed) */
	uint16_t accessed_date;
	/** High 16 bits of starting cluster number */
	uint16_t cluster_high;
	/** Modification time (HMS packed) */
	uint16_t modified_time;
	/** Modification date (YMD packed) */
	uint16_t modified_date;
	/** Low 16 bits of starting cluster number */
	uint16_t cluster_low;
	/** Size */
	uint32_t size;
} __attribute__ (( packed ));

/** Directory entry attributes */
enum vdisk_directory_entry_attributes {
	VDISK_READ_ONLY = 0x01,
	VDISK_DIRECTORY = 0x10,
};

extern void vdisk_read ( uint64_t lba, unsigned int count, void *data );

#endif /* _VDISK_H */
