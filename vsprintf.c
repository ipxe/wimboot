/*
 * Quick and dirty wrapper around iPXE's unmodified vsprintf.c
 *
 */

#include <stddef.h>
#include "wimboot.h"

#define FILE_LICENCE(x)

#define __unused __attribute__ (( unused ))

#undef container_of
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#include "ipxe/vsprintf.c"

/**
 * Convert wide character to multibyte sequence
 *
 * @v buf		Buffer
 * @v wc		Wide character
 * @v ps		Shift state
 * @ret len		Number of characters written
 *
 * This is a stub implementation, sufficient to handle basic ASCII
 * characters.
 */
size_t wcrtomb ( char *buf, wchar_t wc, mbstate_t *ps __unused ) {
	*buf = wc;
	return 1;
}

/**
 * Print character to console
 *
 * @v character		Character to print
 */
int putchar ( int character ) {
	struct callback_params params;

	/* Convert LF to CR,LF */
	if ( character == '\n' )
		putchar ( '\r' );

	/* Print character */
	params.vector.interrupt = 0x10;
	params.eax = ( 0x0e00 | character );
	params.ebx = 0x0007;
	call_interrupt ( &params );

	return 0;
}
