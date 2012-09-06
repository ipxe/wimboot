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
#include <stddef.h>
#include <stdio.h>
#include "wimboot.h"

/** Command line */
char *cmdline;

/** initrd */
void *initrd;

/** Length of initrd */
size_t initrd_len;

/**
 * Main entry point
 *
 */
int main ( void ) {

	/* Print test message */
	printf ( "Hello world\n" );

	/* Print command line, if any */
	if ( cmdline )
		printf ( "%s\n", cmdline );

	/* Print initrd location, if any */
	if ( initrd_len )
		printf ( "initrd at %p+%zx\n", initrd, initrd_len );

	return 0;
}
