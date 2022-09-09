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

#include <string.h>
#include <stdio.h>
#include "wimboot.h"
#include "cmdline.h"
#include "efi.h"
#include "efifile.h"
#include "efiblock.h"
#include "efiboot.h"
#include "string.h"
#include "cpio.h"

/** SBAT section attributes */
#define __sbat __attribute__ (( section ( ".sbat" ), aligned ( 512 ) ))

/** SBAT metadata */
#define SBAT_CSV							\
	/* SBAT version */						\
	"sbat,1,SBAT Version,sbat,1,"					\
	"https://github.com/rhboot/shim/blob/main/SBAT.md"		\
	"\n"								\
	/* wimboot version */						\
	"wimboot," SBAT_GENERATION ",iPXE,wimboot," VERSION ","		\
	"https://ipxe.org/wimboot"					\
	"\n"

/** SBAT metadata (with no terminating NUL) */
const char sbat[ sizeof ( SBAT_CSV ) - 1 ] __sbat = SBAT_CSV;
/** EFI handover boot parameters */
struct efi_boot_params {
	/** Padding */
	uint8_t reserved[0x214];
        uint32_t code32_start; /* 0x214 */
	uint32_t initrd_prt; /* 0x218 */
	uint32_t initrd_len; /* 0x21c */
	uint8_t gap[8];
	/** Command line pointer */
	uint32_t cmd_line_ptr; /* 0x228 */
} __attribute__ (( packed ));

/** Start of uninitialised data section (defined by linker) */
extern char _bss[];

/** End of uninitialised data section (defined by linker) */
extern char _ebss[];

extern void *initrd;

extern size_t initrd_len;

/**
 * Process command line
 *
 * @v loaded		Loaded image protocol
 */
static void efi_cmdline ( EFI_LOADED_IMAGE_PROTOCOL *loaded ) {
	size_t cmdline_len = ( loaded->LoadOptionsSize / sizeof ( wchar_t ) );
	char cmdline[ cmdline_len + 1 /* NUL */ ];
	const wchar_t *wcmdline = loaded->LoadOptions;

	/* Convert command line to ASCII */
	snprintf ( cmdline, sizeof ( cmdline ), "%ls", wcmdline );

	/* Process command line */
	process_cmdline ( cmdline );
}

/**
 * EFI entry point
 *
 * @v cmdline		Command line (if not part of loaded image protocol)
 * @ret efirc		EFI status code
 */
static EFI_STATUS efi_main ( char *cmdline ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	union {
		EFI_LOADED_IMAGE_PROTOCOL *image;
		void *interface;
	} loaded;
	EFI_HANDLE vdisk = NULL;
	EFI_HANDLE vpartition = NULL;
	EFI_STATUS efirc;

	/* Print welcome banner */
	printf ( "\n\nwimboot " VERSION " -- Windows Imaging Format "
		 "bootloader -- https://ipxe.org/wimboot\n\n" );

	/* Get loaded image protocol */
	if ( ( efirc = bs->OpenProtocol ( efi_image_handle,
					  &efi_loaded_image_protocol_guid,
					  &loaded.interface, efi_image_handle,
					  NULL,
					  EFI_OPEN_PROTOCOL_GET_PROTOCOL ))!=0){
		die ( "Could not open loaded image protocol: %#lx\n",
		      ( ( unsigned long ) efirc ) );
	}

	/* Process command line */
	if ( cmdline ) {
		process_cmdline ( cmdline );
	} else {
		efi_cmdline ( loaded.image );
	}

	if(initrd_len){
		/* Extract files from initrd (syslinux) */
		cpio_extract ( initrd, initrd_len, efi_add_file );
	} else {
		/* Extract files from file system */
		efi_extract ( loaded.image->DeviceHandle );
	}
	/* Install virtual disk */
	efi_install ( &vdisk, &vpartition );

	/* Invoke boot manager */
	efi_boot ( bootmgfw, bootmgfw_path, vpartition );

	return 0;
}

/**
 * EFI entry point
 *
 * @v image_handle	Image handle
 * @v systab		EFI system table
 * @ret efirc		EFI status code
 */
EFI_STATUS EFIAPI efi_entry ( EFI_HANDLE image_handle,
			      EFI_SYSTEM_TABLE *systab ) {

	/* Record EFI handle and system table */
	efi_image_handle = image_handle;
	efi_systab = systab;

	/* Initialise stack cookie */
	init_cookie();

	/* Jump to main entry point */
	return efi_main ( NULL );
}

/**
 * Relocate PE
 * @v boot_params       Linux boot parameters
 */
extern char _payload[];
extern char _start[];
extern char _epayload[];
extern char _esbat[];
extern char _sbat[];
typedef struct _IMAGE_BASE_RELOCATION {
     uint32_t   VirtualAddress;
     uint32_t   SizeOfBlock;
     uint16_t   RelocBlocks[];
} IMAGE_BASE_RELOCATION;
typedef IMAGE_BASE_RELOCATION * PIMAGE_BASE_RELOCATION;
static void relocate_pe( struct efi_boot_params *boot_params )
{
     /* get offset */
     uint32_t kernel_offset = 0;
     uint32_t total_reloc = 0;
     uint32_t quit_loop = 0;
     PIMAGE_BASE_RELOCATION prelocat_table = NULL;

     kernel_offset = boot_params->code32_start -
          (uint32_t)( BASE_ADDRESS + ( _payload - _start) );

     prelocat_table = ( PIMAGE_BASE_RELOCATION )
          (_epayload + (_esbat - _sbat));

     while( 1 ) {
          uint32_t num_of_slot = (prelocat_table->SizeOfBlock
                                  - sizeof(IMAGE_BASE_RELOCATION )) / 2;
          uint16_t *p = prelocat_table -> RelocBlocks;
          uint16_t * addr_to_fix_2 = NULL;
          uint32_t * addr_to_fix_4 = NULL;
          uint64_t * addr_to_fix_8 = NULL;
          uint8_t  * next = NULL;
              
          for( uint32_t i = 0 ; i < num_of_slot ; i++) {
               if( ((*p) & 0xf000) == 0x3000 ) { //IMAGE_REL_BASED_HIGHLOW
                    addr_to_fix_4 = (uint32_t *)((intptr_t)
                         (prelocat_table->VirtualAddress +
                          BASE_ADDRESS + kernel_offset + ((*p) & 0xfff)));
                    *addr_to_fix_4 = *addr_to_fix_4 + kernel_offset;
                    total_reloc ++;
               } else if ( ((*p) & 0xf000) == 0x2000 ) { // IMAGE_REL_BASED_LOW
                    addr_to_fix_2 = (uint16_t *)((intptr_t)
                         (prelocat_table->VirtualAddress +
                          BASE_ADDRESS + kernel_offset + ((*p) & 0xff)));
                    *addr_to_fix_2 = *addr_to_fix_2 + kernel_offset;
                    total_reloc ++;
               } else if ( ((*p) & 0xf000) == 0xa000 ) { // IMAGE_REL_BASED_DIR64
                    addr_to_fix_8 = (uint64_t *)((intptr_t)
                         (prelocat_table->VirtualAddress +
                          BASE_ADDRESS + kernel_offset + ((*p) & 0xfff)));
                    *addr_to_fix_8 = *addr_to_fix_8 + kernel_offset;
                    total_reloc ++;
               }
               p ++;
          }
          next = (uint8_t * ) prelocat_table;
          next += prelocat_table->SizeOfBlock;
          prelocat_table = (PIMAGE_BASE_RELOCATION ) next;

          if(quit_loop) {
               break;
          }
          if(!prelocat_table->VirtualAddress) {
               quit_loop = 1;
          }
     };

     DBG ( "total %d relocated\n", total_reloc );

     total_reloc = 0;
}

/**
 * Legacy bzImage EFI handover entry point
 *
 * @v image_handle	Image handle
 * @v systab		EFI system table
 * @v boot_params	Linux boot parameters
 */
EFI_STATUS efi_handover ( EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab,
			  struct efi_boot_params *boot_params ) {
	char *cmdline;

	/* Initialise stack cookie */
	init_cookie();

        /* Do damned relocate, as we are loaded by grub, not really pe */
        relocate_pe(boot_params);
        
	/* Clear uninitialised data section */
	memset ( _bss, 0, ( _ebss - _bss ) );

        printf("Enter efi_handover");
        
	/* Record EFI handle and system table */
        efi_image_handle = image_handle;
        efi_systab = systab;

	/* Get command line */
	cmdline = ( ( char * ) ( intptr_t ) boot_params->cmd_line_ptr );

	if(boot_params){
		initrd = (void *)(intptr_t)boot_params->initrd_prt;
		initrd_len = (size_t)boot_params->initrd_len;
	}

	/* Jump to main entry point */
	return efi_main ( cmdline );
}
