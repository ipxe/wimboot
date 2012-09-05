OBJECTS := prefix.o startup.o callback.o main.o
HEADERS := $(wildcard *.h)

OBJCOPY := objcopy

CFLAGS += -Os -ffreestanding -Wall -W -Werror

all : wimboot

wimboot : wimboot.elf
	$(OBJCOPY) -Obinary $< $@

wimboot.elf : $(OBJECTS) script.lds
	$(LD) -m elf_i386 -T script.lds -o $@ $(OBJECTS)

%.o : %.S $(HEADERS) Makefile
	$(CC) $(CFLAGS) -DASSEMBLY -Ui386 -E $< | as --32 -o $@

%.o : %.c $(HEADERS) Makefile
	$(CC) -m32 $(CFLAGS) -c $< -o $@

clean :
	rm -f *.o *.elf wimboot
