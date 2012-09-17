VERSION := v1.0.0

OBJECTS := prefix.o startup.o callback.o main.o vsprintf.o string.o peloader.o
OBJECTS += int13.o vdisk.o cpio.o stdio.o lznt1.o xca.o
HEADERS := $(wildcard *.h)

OBJCOPY := objcopy

CFLAGS += -Os -ffreestanding -Wall -W -Werror -nostdinc -I.
CFLAGS += -m32 -march=i386
CFLAGS += -DVERSION="\"$(VERSION)\""

# Inhibit -fno-stack-protector
SP_TEST = $(CC) -fno-stack-protector -x c -c /dev/null \
                -o /dev/null >/dev/null 2>&1
SP_FLAGS := $(shell $(SP_TEST) && echo '-fno-stack-protector')
WORKAROUND_CFLAGS += $(SP_FLAGS)

CFLAGS += $(WORKAROUND_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)

ifneq ($(DEBUG),)
CFLAGS += -DDEBUG=$(DEBUG)
endif

all : wimboot

wimboot : wimboot.elf
	$(OBJCOPY) -Obinary $< $@
	cp $@ ../$@

wimboot.elf : $(OBJECTS) script.lds
	$(LD) -m elf_i386 -T script.lds -o $@ $(OBJECTS)

%.o : %.S $(HEADERS) Makefile
	$(CC) $(CFLAGS) -DASSEMBLY -Ui386 -E $< | as --32 -o $@

%.o : %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f *.o *.elf wimboot ../wimboot
