
# Export all macros/variables
# vim: ts=4 noexpandtab

export
currentModule=base

OPTIMIZATION = 1

APP_START	=	0x20200000

# Shut off gmake noise and also get rid of default rules

MAKEFLAGS		= -r -s

# WAD Related

# For EA50
#WADKEY		:=	ba4ca2e399c432d727ec4dd274020756d972f45f48da797e562fbc0c7c9aaf55d02baa48d8260874a163aab49393525300a54ac37424ecdbffa0826710611a0a
#WADFILE		:=	master.core

# For JET
#WADKEY		=	1a58795d01528eca225d352fb2dae277b2fae9eadb205338722b64c3ed115c131d3b555d0747c1baba7c946b56d3ef2c087cf7977be0ce87f7de0383063cf35b
#WADFILE		=	core.wad

# For UC

WADKEY		=	2d3e02275f50604841181b5a241c165d543e142d344c4837542a391558126108272a500b4613586124125106214f331c440b473e102c4d302a0b39030c3a425f
WADFILE		=	boot.bin

#
# Define what shell utility we're using so other makefiles don't barf
# These are cygwin commands (see www.cygwin.com).
#

TOPSHELL		= $(UTILITIES_DIR)/sh.exe
SHELL			= $(TOPSHELL)
REMOVECMD		= -$(UTILITIES_DIR)/rm.exe -f 
MKDIRCMD		= $(UTILITIES_DIR)/mkdir.exe -p
SEDCMD			= $(UTILITIES_DIR)/sed.exe
CATCMD			= $(UTILITIES_DIR)/cat.exe
HEXREMAP		= $(UTILITIES_DIR)/hexremap.exe
DEPEND_EXE		= $(UTILITIES_DIR)/depend.exe
POSTFIX_FILE 		= $(UTILITIES_DIR)/suffix.inc
PREFIX_FILE		= $(UTILITIES_DIR)/prefix.inc
PIAGEN			= $(UTILITIES_DIR)/piagen.exe
CRCFIX			= $(UTILITIES_DIR)/crcfix.exe
CP			= $(UTILITIES_DIR)/cp.exe
MKLFS			= $(UTILITIES_DIR)/mklfs.exe
ZIPCMD			= $(UTILITIES_DIR)/zip.exe
WADCMD			= $(UTILITIES_DIR)/makewad.exe
ZIPPASSWD=m234irjv9ervmpoaweru92u3r9uaw0ajf94rj2mvj293rjvm23rjvm23jrkasndfiozolxvm2o3
FILES			= files-archive
MAKE			= $(UTILITIES_DIR)/gmake.exe

ifndef ARM_PATH
$(error You forgot to define ARM_PATH! Do this in your environment variables!)
endif

ifndef COMPILER
$(error You forgot to define COMPILER! Set to ARMUK for ARM, Inc. or GHS for Greenhills)
endif

# Create an internal ARM_PATH2 that uses forward slashes instead of backslashes
# (Weird shell requirement)

ARM_PATH2		=		$(subst \,/,$(ARM_PATH))
CCFLAGS			=	

# Define where we find the ARM tools
# The Variable ARM_PATH is expected to be defined in the NT-DOS environment

ifeq ($(COMPILER), ARMUK)
ARM_BIN         =       $(ARM_PATH2)/bin
CC              =       $(ARM_BIN)/armcc
ASM             =       $(ARM_BIN)/tasm
ARMLINKER       =       $(ARM_BIN)/armlink
ARMLIBRARIAN    =       $(ARM_BIN)/armlib
ELF2HEX         =       $(ARM_BIN)/fromelf
endif

# Define common directories

BOOT_DIR		= Boot
APP_DIR			= Application
LIB_DIR			= Libs
STARTUP_DIR		= Startup
UTILITIES_DIR		= //archive/t/uc/bin
#UTILITIES_DIR		= bin

# Define the final target since it relies on the platform specific stuff

BASENAME		= app
PROGRAM			= $(BASENAME).axf
PROGRAM_BIN		= $(BASENAME).img

APCS			= -apcs /nofp/softfp/noswst/nointer/narrow/nonreent

# For more rigorous checking by compiler
ifeq ($(COMPILER), ARMUK)
MORECHECKS      = -fa -fh -fp -fv -fx
# -fa			Flow control analysis (uninitialized variables)
# -fh 			Checks all external objects are declared before use, all file scoped static objects are used
endif

ifeq ($(COMPILER), ARMUK)
ifeq ($(OPTIMIZATION),1)
CCFLAGS			+= -c -li -g+ -fz -zc -fa -proc ARM7TDMI -O2 -I. $(APCS)
else
CCFLAGS			+= -c -li -g+ -fz -zc -fa -proc ARM7TDMI -O0 -I. $(APCS)
endif
CCFLAGS			+= -Wb -W+adfgnpvx
endif

CCFLAGS                 += -DPNG_USER_CONFIG -DPIL_VIDEO_ONLY
CLEAN 			=
BUILD_START		=
BUILD_DONE		=

# C compile flags for optimized compiles. 

ifeq ($(COMPILER), ARMUK)
CCOPTFLAGS      = -c -li -fz -proc ARM9 -O2 -g+ $(INCLUDE_OPT) $(APCS)
endif

# Define common libraries

APP_LIB			= $(APP_DIR)/app.lib
STARTUP_LIB		= $(STARTUP_DIR)/startup.lib
BOOT_AXF		= $(BOOT_DIR)/boot.axf
LIB_LIB			= $(LIB_DIR)/lib.lib

# Decide whether to link in the boot library or not

LIBS = $(APP_LIB) $(STARTUP_LIB) $(LIB_LIB)

# Define 2 different scatter files to avoid dependency problems.
# With only one and you forget to do 'make clean' before building a
# release build, the left over scatter file with Boot area directives
# causes problems.
#
ifeq ($(COMPILER), ARMUK)
ifdef RELEASE
SCATTER_FILE = release.sct
else
SCATTER_FILE = debug.sct
endif
endif

#
# Some common link flags
#
ifeq ($(COMPILER), ARMUK)
LINKFLAGS       = -map -info sizes -info totals
endif

#
# opfw specific link flags.
# $(BOOT_AXF) uses uses another set.
#
ifeq ($(COMPILER), ARMUK)
PROG_LFLAGS		= $(LINKFLAGS)	\
					-sym $(BASENAME).sym	\
					-list $(BASENAME).map	\
					-ro-base $(APP_START)	
endif

ifdef RELEASE

ifeq ($(COMPILER), ARMUK)
PROG_LFLAGS		+= -entry $(BOOT_START)
endif

CCFLAGS         += -D_RELEASE -DUSE_ARM_ASM
CCOPTFLAGS      += -D_RELEASE 

else

# Doing it for development (debug)

ifeq ($(COMPILER), ARMUK)
LINKFLAGS       += -debug
endif

CCFLAGS			+= -D_DEBUG 
CCOPTFLAGS		+= -D_DEBUG 

endif	# if RELEASE (else clause)

ifeq ($(COMPILER), ARMUK)
ASMFLAGS = -list -proc ARM9TM -g -dwarf2 -apcs /nointer/noswst/nofp/noreent -li


# ASMFLAGS += -PD "SDRAM_SIZE EQU $(SDRAM_SIZE)"

ifdef RELEASE
ASMFLAGS += -PD "_RELEASE SETL {TRUE}" -PD "__arm__ SETL {TRUE}"
endif
ASMFLAGS += -PD "__arm__ SETL {TRUE}"
endif

#
# Flags for librarian
# -c    to create a new library
# -p    to respect path names (so the debugger can find the source file)
#
ifeq ($(COMPILER), ARMUK)
LIBFLAGS        = -c -p
endif

#
# Flags for hex file and Checksum Generation
#
ifeq ($(COMPILER), ARMUK)
ELF2HEX_FLAGS	= -nodebug
endif

#
# This file lists all of the library modules to be linked.
# It is created in this top level makefile then is updated
# by the subordinate make files.  The intent is to avoid having to
# change the upper level make files to account for the lower level
# libraries.
#

LINKCMDFILE		= toplink.cmd

#
# Hex/checksum program definitions
#

BOOT_HEX			= Boot.hex
APP_HEX				= App.hex

#------------------------------------------------------------------------
#
#	TARGET: all
#	-----------
#
# This is the default build target.
# For ROM images, make a hex file.  For RAM builds, stop at the axf file
# (that the debugger uses).
#
#------------------------------------------------------------------------

all : $(CLEAN)
	$(BUILD_START)
	$(MAKE) -f makefile all_debug SHELL=$(TOPSHELL)
	$(BUILD_DONE)

release: $(CLEAN)
	$(BUILD_START)
	$(MAKE) -f makefile all_release RELEASE=1 SHELL=$(TOPSHELL)
	$(BUILD_DONE)

all_debug: $(PROGRAM)

all_release: $(APP_HEX) $(BOOT_HEX)

###############################################################################
# 
# TARGET:  depend
# 
###############################################################################

depend: 
	$(MAKE) -f $(APP_DIR)/app.mak depend SHELL=$(TOPSHELL)
	$(MAKE) -f $(STARTUP_DIR)/startup.mak depend SHELL=$(TOPSHELL)
	$(MAKE) -f $(LIB_DIR)/libs.mak depend SHELL=$(TOPSHELL)

###############################################################################
# 
# TARGET:  clean_depend
# 
###############################################################################

clean_depend: 
	$(MAKE) -f $(APP_DIR)/app.mak clean_depend SHELL=$(TOPSHELL)
	$(MAKE) -f $(LIB_DIR)/libs.mak clean_depend SHELL=$(TOPSHELL)
	
optimize: force
ifeq ($(OPTIMIZATION),1)
	@echo Optimization enabled
else
	@echo Optimization disabled
endif

#
#
#	$(PROGRAM)
#	----------
# Define how to build the final application.
#
#

ifeq ($(COMPILER), ARMUK)
$(PROGRAM): optimize $(LIBS) $(SCATTER_FILE)
	@echo Linking $(PROGRAM)
	@echo s@$(ARMLINKER) -via $(LINKCMDFILE)
	@$(ARMLINKER) -via $(LINKCMDFILE)
	@$(ELF2HEX) $(PROGRAM) -bin $(PROGRAM_BIN)
	@$(CRCFIX) $(PROGRAM_BIN)
	@$(ZIPCMD) -u -r -D -P $(ZIPPASSWD) master.img app.img -x *.svn-base *.svn thumbs.db
	@echo "Adding BIOS to master.img"
	@echo $(ZIPCMD) -u -r -P $(ZIPPASSWD) master.img BIOSes\\\*.* BIOSes/UCEngine/syscheck.img BIOSes/UCEngine/emergency.img
	-@$(ZIPCMD) -u -r -P $(ZIPPASSWD) master.img BIOSes\\\*.* BIOSes/UCEngine/syscheck.img BIOSes/UCEngine/emergency.img
	@echo "Adding fonts to master.img"
	-@(cd files-archive; $(ZIPCMD) -u -r -D -P $(ZIPPASSWD) ../master.img * -x *.mov *.svn-base *.svn Thumbs.db)
	@$(WADCMD) $(WADKEY) master.img $(WADFILE)
	@$(REMOVECMD) $(MASTER).img
	@echo "** Build complete **"
endif

$(BOOT_LIB):	force
	@$(MAKE) -f $(BOOT_DIR)/boot.mak all $(RELEASE_FLAG) SHELL=$(TOPSHELL)

$(APP_LIB):	force
	@$(MAKE) -f $(APP_DIR)/app.mak all $(RELEASE_FLAG) SHELL=$(TOPSHELL)

$(LIB_LIB):	force
	@$(MAKE) -f $(LIB_DIR)/libs.mak all $(RELEASE_FLAG) SHELL=$(TOPSHELL)

$(STARTUP_LIB):	force
	@$(MAKE) -f $(STARTUP_DIR)/startup.mak all $(RELEASE_FLAG) SHELL=$(TOPSHELL)


#
#
#	$(SCATTER_FILE)
#	---------------
#
# Create scatter file from memory map info in platform.env
#
#

ifeq ($(COMPILER), ARMUK)
$(SCATTER_FILE): Makefile
	$(REMOVECMD) $(SCATTER_FILE)
	@echo "; Memory Map/Scatter file" > $@
	@echo "; Do not edit -- automatically generated file" >> $@

	@echo "Init $(BOOT_START) $(strip $(BOOT_LENGTH))" >> $@
	@echo "{" >> $@
	@echo "    BOOT_CODE $(strip $(BOOT_START))" >> $@
	@echo "    {" >> $@
	@echo "			Startup.o	(Init, +First)" >> $@
#	@echo "			*.o (+RO)" >> $@
	@echo "    }" >> $@
	@echo "}" >> $@
endif

#
#	$(APP_HEX)
#	-----------
#
# Define how to build the hex output from the final program ELF file
#

$(APP_HEX) : $(PROGRAM)
	$(ELF2HEX) $(ELF2HEX_FLAGS) $(PROGRAM) -i32 temp.hex
	$(CHKSUM) $(CHKSUM_FLAGS) /opcode /reloc:$(APP_RELOC) \
		/start:$(strip $(APP_START)) /length:$(strip $(APP_LENGTH)) \
		temp.hex $(APP_HEX)
	@$(REMOVECMD) temp.hex
	@echo "*** $(APP_HEX) file generated ***"

#
#	$(BOOT_HEX)
#	-----------
#
# Define how to build just the boot hex file
#

$(BOOT_HEX) : $(BOOT_AXF)
	$(ELF2HEX) $(ELF2HEX_FLAGS) $(BOOT_AXF) -i32 temp.hex
	$(CHKSUM) $(CHKSUM_FLAGS) /boot /reloc:$(BOOT_RELOC) \
		/start:$(BOOT_START) /length:$(strip $(BOOT_LENGTH)) \
		temp.hex $(BOOT_HEX)
	@$(REMOVECMD) temp.hex
	@echo "*** $(BOOT_HEX) file generated ***"

#
#	force
#	-----
# phony target: force
#
# This is used to make sure the subordinate makes get run unconditionally
# and to create our link command file.
#

force:
ifeq ($(COMPILER), ARMUK)
	@echo "-o $(PROGRAM) $(PROG_LFLAGS)" > $(LINKCMDFILE)
	@echo "Startup/Startup.o" >> $(LINKCMDFILE)
endif

#
#
#	clean
#	-----
#
# Do a 'make clean' to get a fresh start.
#

clean :
	@echo Shart >>pork.txt
	$(REMOVECMD) -R *.o *.lst $(PROGRAM) *.d *.bak *.map *.sym *.hex *.sct *.i.c *.i *.img *.bak \
		$(LINKCMDFILE) $(SCATTER_FILE)

#
#	clean_all
#	---------
#
# Do a 'make clean_all' to clean all subdirectories as well.
#
#

clean_all:
	@echo $(MAKE)
	$(MAKE) -f makefile clean_all_internal SHELL=$(TOPSHELL)

clean_all_internal : clean
	$(MAKE) -f $(APP_DIR)/app.mak clean SHELL=$(TOPSHELL)
	$(MAKE) -f $(LIB_DIR)/libs.mak clean SHELL=$(TOPSHELL)
	$(MAKE) -f $(STARTUP_DIR)/startup.mak clean SHELL=$(TOPSHELL)
