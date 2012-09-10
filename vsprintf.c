/*
 * Quick and dirty wrapper around iPXE's unmodified vsprintf.c
 *
 */

#include <stdint.h>
#include <string.h>
#include "wimboot.h"

#define FILE_LICENCE(x)

#define __unused __attribute__ (( unused ))

typedef signed long ssize_t;

#include "ipxe/vsprintf.c"
