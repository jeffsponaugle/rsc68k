export
currentModule=Widget
currentModuleVar=WIDGET
currentModuleTextDescription=Widget library

include $(PREFIX_FILE)

# These are used for getting and building the files

CFILES =  widget

ASMFILES =

LIBNAMES = console button video text image graph elements slider radio checkbox touch
           
LIBVARS = CONSOLE BUTTON VIDEO TEXT IMAGE GRAPH ELEMENTS SLIDER RADIO CHECKBOX TOUCH

include $(POSTFIX_FILE)
