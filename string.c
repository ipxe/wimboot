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
 * String functions
 *
 */

#include <string.h>

/**
 * Copy memory area
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void * memcpy ( void *dest, const void *src, size_t len ) {
	void *edi = dest;
	const void *esi = src;
	int discard_ecx;

	/* Perform dword-based copy for bulk, then byte-based for remainder */
	__asm__ __volatile__ ( "rep movsl"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ), "2" ( len >> 2 )
			       : "memory" );
	__asm__ __volatile__ ( "rep movsb"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ), "2" ( len & 3 )
			       : "memory" );
	return dest;
}

/**
 * Set memory area
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void * memset ( void *dest, int c, size_t len ) {
	void *edi = dest;
	int eax = c;
	int discard_ecx;

	/* Expand byte to whole dword */
	eax |= ( eax << 8 );
	eax |= ( eax << 16 );

	/* Perform dword-based set for bulk, then byte-based for remainder */
	__asm__ __volatile__ ( "rep stosl"
			       : "=&D" ( edi ), "=&a" ( eax ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( eax ), "2" ( len >> 2 )
			       : "memory" );
	__asm__ __volatile__ ( "rep stosb"
			       : "=&D" ( edi ), "=&a" ( eax ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( eax ), "2" ( len & 3 )
			       : "memory" );
	return dest;
}
