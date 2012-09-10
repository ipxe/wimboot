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
 * Standard Input/Output
 *
 */

#include <stdio.h>
#include <string.h>
#include "bootapp.h"
#include "wimboot.h"

/**
 * Print character to console
 *
 * @v character		Character to print
 */
int putchar ( int character ) {
	struct bootapp_callback_params params;

	/* Convert LF to CR,LF */
	if ( character == '\n' )
		putchar ( '\r' );

	/* Print character to bochs debug port */
	__asm__ __volatile__ ( "outb %b0, $0xe9"
			       : : "a" ( character ) );

	/* Print character to BIOS console */
	memset ( &params, 0, sizeof ( params ) );
	params.vector.interrupt = 0x10;
	params.eax = ( 0x0e00 | character );
	params.ebx = 0x0007;
	call_interrupt ( &params );

	return 0;
}
