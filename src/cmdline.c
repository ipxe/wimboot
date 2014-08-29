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
 * Command line
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "wimboot.h"
#include "cmdline.h"

/** Use raw (unpatched) BCD files */
int cmdline_rawbcd;

/**
 * Process command line
 *
 * @v cmdline		Command line
 */
void process_cmdline ( char *cmdline ) {
	char *tmp = cmdline;
	char *key;
	char *value;

	/* Do nothing if we have no command line */
	if ( ( cmdline == NULL ) || ( cmdline[0] == '\0' ) )
		return;
	DBG ( "Command line: \"%s\"\n", cmdline );

	/* Parse command line */
	while ( *tmp ) {

		/* Skip whitespace */
		while ( isspace ( *tmp ) )
			tmp++;

		/* Find value (if any) and end of this argument */
		key = tmp;
		value = NULL;
		while ( *tmp ) {
			if ( isspace ( *tmp ) ) {
				*(tmp++) = '\0';
				break;
			} else if ( *tmp == '=' ) {
				*(tmp++) = '\0';
				value = tmp;
			} else {
				tmp++;
			}
		}

		/* Process this argument */
		if ( strcmp ( key, "rawbcd" ) == 0 ) {
			cmdline_rawbcd = 1;
		} else if ( key == cmdline ) {
			/* Ignore unknown initial arguments, which may
			 * be the program name.
			 */
		} else {
			die ( "Unrecognised argument \"%s%s%s\"\n", key,
			      ( value ? "=" : "" ), ( value ? value : "" ) );
		}
	}
}
