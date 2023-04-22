export
currentModule=libjpeg
currentModuleVar=LIBJPEG
currentModuleTextDescription=libjpeg

include $(PREFIX_FILE)

# These are used for getting and building the files

# these generate no code, because we've disabled writing.
CFILES = jerror jdapimin jdatasrc jdapistd jcomapi jmemmgr jdmarker jdinput \
	jdmaster jmemansi jutils jquant1 jquant2 jdmerge jdcolor jdsample \
	jdpostct jddctmgr jchuff jcphuff jdphuff jdhuff jdcoefct jdmainct \
	jidctflt jidctfst jidctred jidctint

ASMFILES =

LIBNAMES =

LIBVARS =

include $(POSTFIX_FILE)
