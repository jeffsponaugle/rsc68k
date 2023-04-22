export
currentModule=libpng
currentModuleVar=LIBPNG
currentModuleTextDescription=libpng

include $(PREFIX_FILE)

# These are used for getting and building the files

# these generate no code, because we've disabled writing.
NOCODE = pnggccrd pngvcrd pngwtran pngwutil
CFILES = png pngerror pngget pngmem pngpread pngread pngrio pngrtran pngrutil pngset pngtrans 

ASMFILES =

LIBNAMES =

LIBVARS =

include $(POSTFIX_FILE)
