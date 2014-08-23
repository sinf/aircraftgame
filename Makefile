
OPTIONS=-g -O0 -DMAIN -DUSE_SDL `sdl-config --cflags`
WARNINGS=-Wall -Wextra -Wno-parentheses
LIBS=-lX11 `sdl-config --libs` -lm -lGL
#LIBS=-lGL `sdl-config --libs` -lm -ldl
TARGET=prog
SOURCES=$(shell echo code/*.c)

.PHONY: all clean
all: $(TARGET)

clean:
	rm -f $(TARGET)

$(TARGET): $(SOURCES) $(shell echo code/*.h)
	gcc $(WARNINGS) $(OPTIONS) $(SOURCES) $(LIBS) -o $@

