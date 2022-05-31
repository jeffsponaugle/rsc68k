export
currentModule=Gfx
currentModuleVar=GFX
currentModuleTextDescription=Graphics library

include $(PREFIX_FILE)

# These are used for getting and building the files

CFILES =  GraphicsLib 

ASMFILES = FastGraphics

include $(POSTFIX_FILE)
