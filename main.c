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
 * Main entry point
 *
 */

#include <stdint.h>
#include "wimboot.h"

/** Command line */
char *cmdline;

/**
 * Print character to console
 *
 * @v character		Character to print
 */
static void putchar ( uint8_t character ) {
	struct callback_params params;

	/* Convert LF to CR,LF */
	if ( character == '\n' )
		putchar ( '\r' );

	/* Print character */
	params.vector.interrupt = 0x10;
	params.eax = ( 0x0e00 | character );
	params.ebx = 0x0007;
	call_interrupt ( &params );
}

/**
 * Main entry point
 *
 */
int main ( void ) {
	const char *hello = "Hello world\n";

	/* Print test message */
	while ( *hello )
		putchar ( *(hello++) );

	/* Print command line, if any */
	if ( cmdline ) {
		while ( *cmdline )
			putchar ( *(cmdline++) );
		putchar ( '\n' );
	}

	return 0;
}
