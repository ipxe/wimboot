OBJECTS := prefix.o startup.o callback.o main.o vsprintf.o
HEADERS := $(wildcard *.h)

OBJCOPY := objcopy

CFLAGS += -Os -ffreestanding -Wall -W -Werror -I.
CFLAGS += -m32 -march=i386

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
