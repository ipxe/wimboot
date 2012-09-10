OBJECTS := prefix.o startup.o callback.o main.o vsprintf.o string.o peloader.o
OBJECTS += int13.o vdisk.o cpio.o stdio.o
HEADERS := $(wildcard *.h)

OBJCOPY := objcopy

CFLAGS += -Os -ffreestanding -Wall -W -Werror -nostdinc -I.
CFLAGS += -m32 -march=i386

ifneq ($(DEBUG),)
CFLAGS += -DDEBUG
endif

all : wimboot

wimboot : wimboot.elf
	$(OBJCOPY) -Obinary $< $@

wimboot.elf : $(OBJECTS) script.lds
	$(LD) -m elf_i386 -T script.lds -o $@ $(OBJECTS)

%.o : %.S $(HEADERS) Makefile
	$(CC) $(CFLAGS) -DASSEMBLY -Ui386 -E $< | as --32 -o $@

%.o : %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f *.o *.elf wimboot
