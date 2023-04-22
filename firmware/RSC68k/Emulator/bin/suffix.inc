# Temporary define for making things
# vim:ts=8:noexpandtab

#
# The 'echo -n ""' is needed because without it, make will not stop on errors
#
define MAKEIT
	$(MAKE) -f $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).mak all $(RELEASE_FLAG) SHELL=$(TOPSHELL)
	echo -n ""
endef

define DEPENDIT
	$(MAKE) -f $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).mak depend $(RELEASE_FLAG) SHELL=$(TOPSHELL)
	echo -n ""
endef

define CLEAN_DEPENDIT
        $(MAKE) -f $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).mak clean_depend $(RELEASE_FLAG) SHELL=$(TOPSHELL)
	echo -n ""
endef

define CLEANIT
	$(MAKE) -f $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).mak clean $(RELEASE_FLAG) SHELL=$(TOPSHELL)
	echo -n ""
endef

define GENERATEIT
	$(MAKE) -f $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).mak generate $(RELEASE_FLAG) SHELL=$(TOPSHELL)
	echo -n ""
endef

# Create our sub libraries list
LIBS := $(foreach LIBNAMES, $(LIBNAMES), $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).lib)

# This is used to create rules for all of the builds

LIBBUILD := $(foreach LIBNAMES, $(LIBNAMES), $(MAKEIT) ; )
DEPEND_BUILD := $(foreach LIBNAMES, $(LIBNAMES), $(DEPENDIT) ; )
CLEAN_DEPEND_BUILD := $(foreach LIBNAMES, $(LIBNAMES), $(CLEAN_DEPENDIT) ; )
CLEAN_BUILD := $(foreach LIBNAMES, $(LIBNAMES), $(CLEANIT) ; )
GENERATE_BUILD := $(foreach LIBNAMES, $(LIBNAMES), $(GENERATEIT) ; )

# DEPEND_BUILD := $(foreach LIBNAMES, $(LIBNAMES), $(MAKE) -f $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).mak depend $(RELEASE_FLAG) SHELL=$(TOPSHELL) ; )
#CLEAN_DEPEND_BUILD := $(foreach LIBNAMES, $(LIBNAMES), $(MAKE) -f $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).mak clean_depend $(RELEASE_FLAG) SHELL=$(TOPSHELL) ; )
#CLEAN_BUILD := $(foreach LIBNAMES, $(LIBNAMES), $(MAKE) -f $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).mak clean $(RELEASE_FLAG) SHELL=$(TOPSHELL) ; )
#GENERATE_BUILD := $(foreach LIBNAMES, $(LIBNAMES), $(MAKE) -f $(PROGRAM_DIR)$(LIBNAMES)/$(LIBNAMES).mak generate $(RELEASE_FLAG) SHELL=$(TOPSHELL) ; )

# Create a dependencies list and exclude those that are not included

DEPEND_LIST_C = $(filter-out $(DEPEND_EXCLUDE), $(CFILES))
DEPEND_LIST_C_EXPANDED = $(foreach DEPEND_LIST_C, $(DEPEND_LIST_C), $(PROGRAM_DIR)$(DEPEND_LIST_C).c $(PROGRAM_DIR)$(DEPEND_LIST_C).o )
DEPEND_LIST_CPP = $(filter-out $(DEPEND_EXCLUDE), $(CPPFILES))
DEPEND_LIST_CPP_EXPANDED = $(foreach DEPEND_LIST_CPP, $(DEPEND_LIST_CPP), $(PROGRAM_DIR)$(DEPEND_LIST_CPP).cpp $(PROGRAM_DIR)$(DEPEND_LIST_CPP).o )
DEPEND_LIST := $(DEPEND_LIST_C_EXPANDED) $(DEPEND_LIST_CPP_EXPANDED)

# Remove excluded CFILES 

CFILES_EXCLUDED := $(filter-out $(CFILES_EXCLUDE), $(CFILES))
ASMFILES_EXCLUDED := $(filter-out $(ASMFILES_EXCLUDE), $(ASMFILES))

# Output files

COBJS = $(foreach CFILES_EXCLUDED, $(CFILES_EXCLUDED), $(PROGRAM_DIR)$(CFILES_EXCLUDED).o)
ASMOBJS = $(foreach ASMFILES_EXCLUDED, $(ASMFILES_EXCLUDED), $(PROGRAM_DIR)$(ASMFILES_EXCLUDED).o)

OBJS	:= $(COBJS) $(ASMOBJS)

# This prevents inheritance from higher level -includes so that warnings of
# a lack of dependencies being made will cause an error.

DEPEND_MADE=

-include $(LIBRARY_DEPEND)

%.o: %.c
	@echo %.o %.c
	@echo Error - rule for file $< not covered
	exit 1

%.o: %.cpp
	@echo %.o %.cpp
	@echo Error - rule for file $< not covered
	exit 1

%.o: %.s
	@echo $<
	@echo @$(ASM) $(ASMFLAGS) $< -o $@
	@$(ASM) $(ASMFLAGS) $< -o $@

make_libs:
	@$(LIBBUILD)

###############################################################################
# 
# TARGET:  $(LIBRARY_NAME)<platform>.lib
# 
###############################################################################

depend_exists :
ifndef DEPEND_MADE
	@echo You forgot to run $(UTILITIES_DIR)/gmake depend - called from $(currentModule).mak
	exit 1
endif

making_targets :
	@echo ${PRODUCT}: Making $(currentModuleTextDescription)

$(PROGRAM) : depend_exists making_targets $(OBJS) $(OTHEROUTPUTS) make_libs
	@echo ${PRODUCT}: Linking $(currentModuleTextDescription)
ifeq ($(COMPILER), ARMUK)
	@echo "$(PROGRAM)" >> $(LINKCMDFILE)
	@$(ARMLIBRARIAN) $(LIBFLAGS) $(PROGRAM) $(OBJS)
endif

ifeq ($(COMPILER), IARARM)
ifneq ($(strip $(OBJS)),)
	@echo "$(PROGRAM)" >> $(LINKCMDFILE)
	@$(ARMLIBRARIAN) $(PROGRAM) $(OBJS)
endif
endif

ifeq ($(COMPILER), GHS)
	@$(ARMLIBRARIAN) $(LIBFLAGS) $(PROGRAM) $(OBJS) $(LIBS)
endif

.PHONY: add_linkfile

# Here's what we ultimately want to build

all : $(PROGRAM)

###############################################################################
# 
# TARGET:  clean
# 
###############################################################################

clean :
	@echo $(PRODUCT): Cleaning $(currentModuleTextDescription)
	@$(REMOVECMD) $(PROGRAM)	$(PROGRAM_DIR)*.o $(PROGRAM_DIR)*.lst $(PROGRAM_DIR)*.bak $(PROGRAM_DIR)*.i	$(PROGRAM_DIR)*.i.c $(PROGRAM_DIR)*.dbo $(CLEAN_FILES) $(GENERATED_FILES)
	@$(REMOVECMD) -rf $(CLEAN_DIRS)
	@$(CLEAN_BUILD)

###############################################################################
# 
# TARGET:  generate
# 
###############################################################################

generating_files :
	@echo $(PRODUCT): Generating $(GENERATED_FILES) for $(currentModuleTextDescription)

generate : generating_files $(GENERATED_FILES)
	@$(GENERATE_BUILD)



###############################################################################
# 
# TARGET:  depend
# 
###############################################################################

depend : $(GENERATED_FILES) .PHONY
	@echo $(PRODUCT): Making dependency file $(LIBRARY_DEPEND)
	@$(DEPEND_EXE) -a -s $(DEPEND_DEFINES) -t $(LIBRARY_DEPEND) -d $(DEPEND_LIST) $(DEPEND_INCLUDES)
	@$(DEPEND_BUILD)

###############################################################################
# 
# TARGET:  clean_depend
# 
###############################################################################

clean_depend :
	@echo $(PRODUCT): Cleaning dependency file $(LIBRARY_DEPEND)
	@$(REMOVECMD) $(LIBRARY_DEPEND)
	@$(CLEAN_DEPEND_BUILD)
