#
# Utilities
#
CC=/opt/cross/bin/m68k-elf-gcc
AS=/opt/cross/bin/m68k-elf-as
LINK=/opt/cross/bin/m68k-elf-ld
OBJCOPY=/opt/cross/bin/m68k-elf-objcopy
SPLIT=../Utils/split/split
STAMP=../Utils/stamp/stamp

#
# Compiler flags
#
CFLAGS=-m68000 -O3 -fno-delete-null-pointer-checks
INCLUDES=-I ../newlib/m68k-elf/include -I .. 

#
# Target name
#
BASENAME=BootLoader
OBJS=$(BASENAME)Startup.o ../Shared/sbrk.o $(BASENAME).o ../Shared/zlib/crc32.o ../Shared/16550.o ../Shared/Version.o $(BASENAME)Version.o $(BASENAME)Stream.o \
     ../Utils/newlib/newlib/libc/machine/w65/mulsi3.o ../Utils/newlib/newlib/libc/machine/w65/divsi3.o ../Shared/Flash.o
OUTPUT=$(BASENAME).a
OUTPUTBIN=$(BASENAME).bin
OUTPUTODD=$(BASENAME)Odd.bin
OUTPUTEVEN=$(BASENAME)Even.bin
MAPFILE=$(BASENAME).map
LINKFILE=$(BASENAME).ld


BootLoader: $(OBJS)
	$(LINK) $(notdir $(OBJS)) ../newlib/m68k-elf/lib/crt0.o -T$(LINKFILE) -o $(OUTPUT) -Map=$(MAPFILE) -L ../newlib/m68k-elf/lib -lc
	$(OBJCOPY) -O binary BootLoader.a $(OUTPUTBIN)
	$(STAMP) $(OUTPUTBIN)
	$(SPLIT) $(OUTPUTBIN)

#
# Special case for BootLoaderStartup. It uses the C preprocessor in order to
# turn #defines into assembly-friendly equates
#

BootLoaderStartup.o: BootLoaderStartup.s ../Hardware/RSC68k.h ../Shared/16550.h
	cpp -I include -x assembler-with-cpp $(BASENAME)Startup.s -o $(BASENAME)StartupProc.s
	$(AS) $(BASENAME)StartupProc.s -o $(BASENAME)Startup.o
	@echo $(OBJS)

$(BASENAME)Version.c:	FORCE
	../Scripts/subwcrev $(BASENAME)Version.tmpl >$(BASENAME)Version.c

FORCE:

$(BASENAME)Version.o:	$(BASENAME)Version.c $(BASENAME)Version.tmpl
	$(COMPILE.c) $(INCLUDES) $< -o $@

%.o: %.c $(DEFS)
	$(CC) $(INCLUDES) -c -o $(notdir $@) $< $(CFLAGS)

clean:
	rm -f $(notdir $(OBJS))
	rm -f *.map
	rm -f $(MAPFILE)
	rm -f $(OUTPUT)
	rm -f $(OUTPUTBIN)
	rm -f $(OUTPUTODD) $(OUTPUTEVEN)
	rm -f $(BASENAME)StartupProc.s
	rm -f $(BASENAME)Version.c
