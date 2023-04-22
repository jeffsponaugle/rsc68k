export
currentModule=Libs
currentModuleVar=LIBS
currentModuleTextDescription=Application Level Libraries

include $(PREFIX_FILE)

# These are used for getting and building the files

# CFILES =  

# ASMFILES =

LIBNAMES = Gfx libpng zlib Sound freetype2 window fontmgr widget libjpeg languages lex libgif network
           
LIBVARS = GFX LIBPNG ZLIB SOUND FREETYPE2 WINDOW FONTMGR WIDGET LIBJPEG LANGUAGES LEX LIBGIF NETWORK

include $(POSTFIX_FILE)
