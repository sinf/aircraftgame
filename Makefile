
OPTIONS=-DUSE_SDL `sdl-config --cflags` -nostartfiles -fno-stack-protector -Wl,-e,Main
WARNINGS=-Wall -Wextra -Wno-parentheses
LIBS=-lX11 `sdl-config --libs` -lm -lGL
#LIBS=-lGL `sdl-config --libs` -lm -ldl
TARGET=prog
DEBUG_TARGET=debug_prog
SOURCES=$(shell echo code/*.c)
DEPS=$(SOURCES) $(shell echo code/*.h)

SMALL_OPTIONS=$(OPTIONS) -O2 -Wl,-O2 -ffast-math -mfpmath=both
DEBUG_OPTIONS=$(OPTIONS) -O0 -g
#-flto -fwhole-program 

# Options for 32-bit build:
# -m32 -msse -L/usr/lib/i386-linux-gnu

.PHONY: all clean
all: $(TARGET) $(DEBUG_TARGET)

clean:
	rm -f $(TARGET) $(DEBUG_TARGET)

$(TARGET): $(DEPS)
	gcc $(WARNINGS) $(SMALL_OPTIONS) $(SOURCES) $(LIBS) -o $@
	strip $@ -s -R .comment -R .jcr -R .eh_frame_hdr -R .note.ABI-tag -R .gnu.version -R .note.gnu.build-id

$(DEBUG_TARGET): $(DEPS)
	gcc $(WARNINGS) $(DEBUG_OPTIONS) $(SOURCES) $(LIBS) -o $@

# Optimization flags to use when all libraries have been trimmed away (these break compatibility with libs)
# -mx32 -- use 32bit pointers on x86-64
# -mregparm=N -- how many args to pass via registers
