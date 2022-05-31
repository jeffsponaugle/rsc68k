export
currentModule=Sound
currentModuleVar=SOUND
currentModuleTextDescription=Sound library

include $(PREFIX_FILE)

# These are used for getting and building the files

LIBNAMES = MP3
LIBVARS = MP3

CFILES = sound WaveMgr SoundStream SoundStreamWave SoundStreamMP3

ifeq ($(strip $(INCLUDE_LIB_SOUND_STREAM)),1)
CFILES += stream
endif

ifeq ($(strip $(INCLUDE_LIB_SOUND_OGG)),1)
CFILES += oggstream
endif

ASMFILES =

include $(POSTFIX_FILE)
