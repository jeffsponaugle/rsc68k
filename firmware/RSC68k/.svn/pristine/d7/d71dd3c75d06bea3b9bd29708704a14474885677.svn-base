# Make sure our environment settings are present

ifeq ($(PLATFORM_NAME),"")
$(error PLATFORM_NAME Not set for $(currentModuleTextDescription) build)
endif

# Used internally so it makes later copying/migration easier

PROGRAM_DIR		:= 	$(PROGRAM_DIR)$(currentModule)/
PROGRAM_VER		:=	$($(currentModuleVar)_VER)
PROGRAM			=	$(PROGRAM_DIR)$(currentModule).lib
LIBRARY_DEPEND		=	$(PROGRAM_DIR)depend$(currentModule).inc

# Kill our object/target lists so we don't inherit others

OBJS			=
CFILES			=
CPPFILES		=
HFILES			=
ASMFILES 		=
OTHERFILES		=
DEPEND_EXCLUDE	=
CFILES_EXCLUDE =
OTHEROUTPUTS	=
LIBNAMES       =
LIBVARS		=
