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
CFLAGS=-m68000 -O3 -fno-delete-null-pointer-checks -D_APP
INCLUDES=-I ../newlib/m68k-elf/include -I .. 

#
# Target name
#
BASENAME=Zork
OBJS=$(BASENAME)Startup.o ../Shared/sbrk.o $(BASENAME).o ../Shared/zlib/crc32.o ../Shared/16550.o ../Shared/Version.o $(BASENAME)Version.o \
     ../Utils/newlib/newlib/libc/machine/w65/mulsi3.o ../Utils/newlib/newlib/libc/machine/w65/divsi3.o ../Shared/Shared.o \
     ../Shared/Interrupt.o AsmUtils.o ../Shared/Stream.o ../Shared/ptc.o ../Shared/rtc.o ../Shared/muldi3.o dtextC.o \
     ../Shared/arith64.o actors.o ballop.o clockr.o demons.o dgame.o dinit.o dmain.o dso1.o dso2.o dso3.o dso4.o dso5.o \
     dso6.o dso7.o dsub.o dverb1.o dverb2.o gdt.o lightp.o local.o nobjs.o np1.o np2.o np3.o np.o nrooms.o objcts.o \
     rooms.o sobjs.o supp.o sverbs.o verbs.o villns.o ../Shared/FaultHandler.o FaultHandlerAsm.o  ../Shared/LineInput.o
OUTPUT=$(BASENAME).a
OUTPUTBIN=$(BASENAME).bin
OUTPUTODD=$(BASENAME)Odd.bin
OUTPUTEVEN=$(BASENAME)Even.bin
MAPFILE=$(BASENAME).map
LINKFILE=$(BASENAME).ld


Zork: $(OBJS)
	$(LINK) $(notdir $(OBJS)) ../newlib/m68k-elf/lib/crt0.o -T$(LINKFILE) -o $(OUTPUT) -Map=$(MAPFILE) -L ../newlib/m68k-elf/lib -lc
	$(OBJCOPY) -O binary Zork.a $(OUTPUTBIN)
	$(STAMP) $(OUTPUTBIN)
	$(SPLIT) $(OUTPUTBIN)

AsmUtils.o: ../Shared/AsmUtils.s ../Shared/AsmUtils.s ../Hardware/RSC68k.h ../Shared/16550.h
	cpp -I include -x assembler-with-cpp ../Shared/AsmUtils.s -o AsmUtilsProc.s
	$(AS) AsmUtilsProc.s -o AsmUtils.o
	@echo $(OBJS)

#
# Special case for ZorkStartup. It uses the C preprocessor in order to
# turn #defines into assembly-friendly equates
#

ZorkStartup.o: ZorkStartup.s ../Hardware/RSC68k.h ../Shared/16550.h
	cpp -I include -x assembler-with-cpp $(BASENAME)Startup.s -o $(BASENAME)StartupProc.s
	$(AS) $(BASENAME)StartupProc.s -o $(BASENAME)Startup.o
	@echo $(OBJS)

FaultHandlerAsm.o: ../Shared/FaultHandlerAsm.s ../Hardware/RSC68k.h ../Shared/16550.h
	cpp -I include -x assembler-with-cpp ../Shared/FaultHandlerAsm.s -o FaultHandlerAsmProc.s
	$(AS) FaultHandlerAsmProc.s -o FaultHandlerAsm.o
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
