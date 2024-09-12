MAKEFLAGS += -j4

PROGRAM_NAME=clip-share-client

MIN_PROTO=1
MAX_PROTO=1

CC=gcc
CFLAGS=-c -pipe -I. --std=gnu11
CFLAGS_DEBUG=-g -DDEBUG_MODE

OBJS=main.o proto/selector.o proto/versions.o proto/methods.o utils/utils.o utils/net_utils.o utils/list_utils.o utils/config.o

LINK_FLAGS_BUILD=

ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(detected_OS),Linux)
	OBJS+= xclip/xclip.o xclip/xclib.o
	CFLAGS_OPTIM=-Os
	LDLIBS=-lunistring -lX11 -lXmu -lXt
	LINK_FLAGS_BUILD=-no-pie -Wl,-s,--gc-sections
else ifeq ($(detected_OS),Windows)
	CFLAGS+= -D__USE_MINGW_ANSI_STDIO
	CFLAGS_OPTIM=-O3
	LINK_FLAGS_BUILD=-no-pie -mwindows
	PROGRAM_NAME:=$(PROGRAM_NAME).exe
else ifeq ($(detected_OS),Darwin)
export CPATH=$(shell brew --prefix)/include
export LIBRARY_PATH=$(shell brew --prefix)/lib
	CFLAGS+= -fobjc-arc
	CFLAGS_OPTIM=-O3
	CFLAGS+= -D__USE_MINGW_ANSI_STDIO
else
$(error ClipShare is not supported on this platform!)
endif
CFLAGS+= -DPROTOCOL_MIN=$(MIN_PROTO) -DPROTOCOL_MAX=$(MAX_PROTO)
CFLAGS_OPTIM+= -Werror

# append '_debug' to objects for debug executable to prevent overwriting objects for main build
DEBUG_OBJS=$(OBJS:.o=_debug.o)

$(PROGRAM_NAME): $(OBJS)
	$(CC) $^ $(LINK_FLAGS_BUILD) $(LDLIBS) -o $@

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -fno-pie $^ -o $@

$(DEBUG_OBJS): %_debug.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) $^ -o $@

.PHONY: clean debug

debug: $(DEBUG_OBJS)
	$(CC) $^ $(LDLIBS) -o $(PROGRAM_NAME)

clean:
	$(RM) $(OBJS) $(DEBUG_OBJS)
	$(RM) $(PROGRAM_NAME)