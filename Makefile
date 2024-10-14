# Makefile - makefile
# Copyright (C) 2024 H. Thevindu J. Wijesekera

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

MAKEFLAGS += -j4

PROGRAM_NAME=clip-share-client

MIN_PROTO=1
MAX_PROTO=2

CC=gcc
CFLAGS=-c -pipe -I. --std=gnu11 -fstack-protector -fstack-protector-all -Wall -Wextra -Wdouble-promotion -Wformat=2 -Wformat-nonliteral -Wformat-security -Wnull-dereference -Winit-self -Wmissing-include-dirs -Wswitch-default -Wstrict-overflow=4 -Wconversion -Wfloat-equal -Wshadow -Wpointer-arith -Wundef -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Woverlength-strings
CFLAGS_DEBUG=-g -DDEBUG_MODE

OBJS=main.o client.o proto/selector.o proto/versions.o proto/methods.o utils/utils.o utils/net_utils.o utils/list_utils.o utils/config.o

LINK_FLAGS_BUILD=

ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(detected_OS),Linux)
	OBJS+= xclip/xclip.o xclip/xclib.o
	CFLAGS+= -ftree-vrp -Wformat-signedness -Wshift-overflow=2 -Wstringop-overflow=4 -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wjump-misses-init -Wlogical-op -Wvla-larger-than=65536
	CFLAGS_OPTIM=-Os
	LDLIBS=-lunistring -lX11 -lXmu -lXt
	LINK_FLAGS_BUILD=-no-pie -Wl,-s,--gc-sections
else ifeq ($(detected_OS),Windows)
	CFLAGS+= -ftree-vrp -Wformat-signedness -Wshift-overflow=2 -Wstringop-overflow=4 -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wjump-misses-init -Wlogical-op -Wvla-larger-than=65536
	CFLAGS+= -D__USE_MINGW_ANSI_STDIO
	CFLAGS_OPTIM=-O3
	LDLIBS=-l:libunistring.a -lws2_32 -lgdi32
	LINK_FLAGS_BUILD=-no-pie -mwindows
	PROGRAM_NAME:=$(PROGRAM_NAME).exe
else ifeq ($(detected_OS),Darwin)
export CPATH=$(shell brew --prefix)/include
export LIBRARY_PATH=$(shell brew --prefix)/lib
	CFLAGS+= -fobjc-arc
	CFLAGS_OPTIM=-O3
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