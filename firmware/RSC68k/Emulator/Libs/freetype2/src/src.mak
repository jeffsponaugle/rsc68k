export
currentModule=src
currentModuleVar=SRC
currentModuleTextDescription=Freetype 2 source

include $(PREFIX_FILE)

# These are used for getting and building the files

# CFILES =  

# ASMFILES =

LIBNAMES = autofit base cache cff cid gxvalid winfonts type42 type1 truetype smooth sfnt raster psnames pshinter psaux pfr pcf otvalid
           
LIBVARS = AUTOFIT BASE CACHE CFF CID GXVALID WINFONTS TYPE42 TYPE1 TRUETYPE SMOOTH SFNT RASTER PSNAMES PSHINTER PSAUX PFR PCF OTVALID

include $(POSTFIX_FILE)
