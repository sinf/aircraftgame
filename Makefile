
OPTIONS=-std=c99 -Wall -Wno-parentheses -DUSE_SDL `sdl2-config --cflags` -nostartfiles -Wl,-e,Main
LIBS=-lX11 `sdl2-config --libs` -lm -lGL
#LIBS=-lGL `sdl2-config --libs` -lm -ldl
TARGET=prog
DEBUG_TARGET=debug_prog
PACKED_TARGET=demo.sh
SOURCES=$(shell echo code/*.c)
DEPS=$(SOURCES) $(shell echo code/*.h)

SMALL_OPTIONS=$(OPTIONS) -O2 -Wl,-O2 -ffast-math -mfpmath=both -fno-stack-protector
DEBUG_OPTIONS=$(OPTIONS) -O0 -g -DDEBUG
#-flto -fwhole-program 

# Options for 32-bit build:
# -m32 -msse -L/usr/lib/i386-linux-gnu

.PHONY: all clean
all: $(DEBUG_TARGET)
#all: $(TARGET) $(DEBUG_TARGET) $(PACKED_TARGET)

clean:
	rm -f $(TARGET) $(DEBUG_TARGET) $(PACKED_TARGET)

$(TARGET): $(DEPS)
	gcc $(SMALL_OPTIONS) $(SOURCES) $(LIBS) -o $@
	strip $@ -s -R .comment -R .jcr -R .eh_frame_hdr -R .note.ABI-tag -R .gnu.version -R .note.gnu.build-id

$(DEBUG_TARGET): $(DEPS)
	gcc $(DEBUG_OPTIONS) $(SOURCES) $(LIBS) -o $@

# \044 is the escape code for $
LAUNCHER='F=./4k;dd bs=1 skip=64<\0440|lzma -cd>\044F;chmod +x \044F;\044F;rm \044F;exit\n'

$(PACKED_TARGET): $(TARGET)
	printf $(LAUNCHER) > $@
	lzma -cz9 $< >> $@
	chmod +x $@

# Optimization flags to use when all libraries have been trimmed away (these break compatibility with libs)
# -mx32 -- use 32bit pointers on x86-64
# -mregparm=N -- how many args to pass via registers
