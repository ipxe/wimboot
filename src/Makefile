VERSION := v2.0.0

OBJECTS := prefix.o startup.o callback.o main.o vsprintf.o string.o peloader.o
OBJECTS += int13.o vdisk.o cpio.o stdio.o lznt1.o xca.o die.o efi.o efimain.o
OBJECTS += efiguid.o efifile.o efipath.o efiboot.o efiblock.o cmdline.o
OBJECTS += wimpatch.o

OBJECTS_i386 := $(patsubst %.o,%.i386.o,$(OBJECTS))
OBJECTS_x86_64 := $(patsubst %.o,%.x86_64.o,$(OBJECTS))
OBJECTS_i386_x86_64 := $(patsubst %.o,%.i386.x86_64.o,$(OBJECTS))

HEADERS := $(wildcard *.h)

HOST_CC		:= $(CC)
OBJCOPY 	:= objcopy
AR		:= ar
RANLIB		:= ranlib
BINUTILS_DIR	:= /usr
BFD_DIR		:= $(BINUTILS_DIR)
ZLIB_DIR	:= /usr

HOST_CFLAGS += -Wall -W -Werror

CFLAGS += -Os -ffreestanding -Wall -W -Werror -nostdinc -I. -fshort-wchar
CFLAGS += -DVERSION="\"$(VERSION)\""

# Inhibit -fno-stack-protector
SP_TEST = $(CC) -fno-stack-protector -x c -c /dev/null \
                -o /dev/null >/dev/null 2>&1
SP_FLAGS := $(shell $(SP_TEST) && echo '-fno-stack-protector')
WORKAROUND_CFLAGS += $(SP_FLAGS)

# The combination of merged constants and our .i386.x86_64.o objects
# seriously confuses the linker.  Avoid.
CFLAGS += -fno-merge-constants

CFLAGS += $(WORKAROUND_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)

ifneq ($(DEBUG),)
CFLAGS += -DDEBUG=$(DEBUG)
endif

###############################################################################
#
# Final targets

all : wimboot wimboot.i386 wimboot.x86_64

wimboot : wimboot.x86_64 Makefile
	cp $< $@
	cp $@ ../$@

wimboot.% : wimboot.%.elf efireloc Makefile
	$(OBJCOPY) -Obinary $< $@
	./efireloc $< $@

wimboot.%.elf : prefix.%.o lib.%.a script.lds Makefile
	$(LD) -m elf_$* -T script.lds -o $@ -q -Map wimboot.$*.map \
		prefix.$*.o lib.$*.a

###############################################################################
#
# i386 objects

%.i386.o : %.S $(HEADERS) Makefile
	$(CC) $(CFLAGS) -m32 -march=i386 -DASSEMBLY -Ui386 -E $< | as --32 -o $@

%.i386.o : %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) -m32 -march=i386 -c $< -o $@

lib.i386.a : $(OBJECTS_i386) Makefile
	rm -f $@
	$(AR) r $@ $(OBJECTS_i386)
	$(RANLIB) $@

###############################################################################
#
# i386 objects to be linked into an x86_64 binary
#
# For objects built from .c files: convert the .i386.o object to
# elf64-x86-64 and prefix symbol names with __i386_
#
# For objects built from .S files: take the .x86_64.o object and
# prefix symbol names with __i386_
#
# Objects built from .S files already include explicit .code32
# directives, and so will always build 32-bit code.  However, the
# .i386.o object will use Elf32_Rel relocations, while the .x86_64.o
# object will use Elf64_Rela relocations.  The former cause spurious
# "relocation truncated to fit" errors when linking
# callback.i386.x86_64.o, since the inline addend (-BASE_ADDRESS) is
# interpreted as unsigned.  The Elf64_Rela addends are signed, and so
# the problem does not arise.

%.i386.x86_64.o : %.i386.o %.c Makefile
	$(OBJCOPY) -Oelf64-x86-64 --prefix-symbols=__i386_ $< $@

%.i386.x86_64.o : %.x86_64.o %.S Makefile
	$(OBJCOPY) --prefix-symbols=__i386_ $< $@

###############################################################################
#
# x86_64 objects

%.x86_64.o : %.S $(HEADERS) Makefile
	$(CC) $(CFLAGS) -m64 -DASSEMBLY -Ui386 -E $< | as --64 -o $@

%.x86_64.o : %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) -m64 -c $< -o $@

lib.x86_64.a : $(OBJECTS_x86_64) $(OBJECTS_i386_x86_64) Makefile
	rm -f $@
	$(AR) r $@ $(OBJECTS_x86_64) $(OBJECTS_i386_x86_64)
	$(RANLIB) $@

###############################################################################
#
# EFI relocator

EFIRELOC_CFLAGS := -I$(BINUTILS_DIR)/include -I$(BFD_DIR)/include \
		   -I$(ZLIB_DIR)/include -idirafter .
EFIRELOC_LDFLAGS := -L$(BINUTILS_DIR)/lib -L$(BFD_DIR)/lib -L$(ZLIB_DIR)/lib \
		    -lbfd -ldl -liberty -lz -Wl,--no-warn-search-mismatch

efireloc : efireloc.c Makefile
	$(CC) $(HOST_CFLAGS) $(EFIRELOC_CFLAGS) $< $(EFIRELOC_LDFLAGS) -o $@

###############################################################################
#
# Cleanup

clean :
	rm -f *.o *.a *.elf
	rm -f efireloc
	rm -f wimboot wimboot.i386 wimboot.x86_64 ../wimboot
