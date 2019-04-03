# Default target executed when no arguments are given to make.
target = main

SRC = main.c vulkanlib.c
ifndef CC
  CC=gcc
endif

COMPILE_PLATFORM=$(shell uname | sed -e s/_.*//|tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
COMPILE_ARCH=$(shell uname -m | sed -e s/i.86/x86/ | sed -e 's/^arm.*/arm/')

OPTIMIZE = -O2 -ffast-math

CFLAGS=-Wall
CFLAGS += $(OPTIMIZE)

SOURCE_DIR = ~/game/vkQuake3/code/linux/
BINARY_DIR = ~/game/vkQuake3/code/linux/

#  XCB_CFLAGS = $(shell PKG_CONFIG --silence-errors --cflags xcb)
XCB_LIBS = $(shell pkg-config --cflags --libs xcb)
RENDERER_LIBS = $(shell pkg-config --cflags --libs vulkan)

ALL_LIBS = $(XCB_LIBS) $(RENDERER_LIBS) -ldl

main: clean info
	$(CC) $(CFLAGS) $(SRC) -o $(target) $(ALL_LIBS)
info:
	@echo ""
	@echo "  COMPILE_PLATFORM: $(COMPILE_PLATFORM)"
	@echo "  COMPILE_ARCH: $(COMPILE_ARCH)"
	@echo "  CC: $(CC)"
	@echo ""
	@echo "  CFLAGS:" $(CFLAGS)
	@echo "  LDFLAGS:" $(LDFLAGS)
	@echo "  LIBS:" $(ALL_LIBS)
	
clean:
	rm -rf $(target)
