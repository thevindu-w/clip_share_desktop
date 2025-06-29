# Makefile - makefile
# Copyright (C) 2024-2025 H. Thevindu J. Wijesekera

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

SRC_DIR=src
BUILD_DIR=build

MIN_PROTO=1
MAX_PROTO=3

CC=gcc
CPP=cpp
CFLAGS=-c -pipe -I$(SRC_DIR) --std=gnu11 -fstack-protector -fstack-protector-all -Wall -Wextra -Wdouble-promotion -Wformat=2 -Wformat-nonliteral -Wformat-security -Wnull-dereference -Winit-self -Wmissing-include-dirs -Wswitch-default -Wstrict-overflow=4 -Wconversion -Wfloat-equal -Wshadow -Wpointer-arith -Wundef -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Woverlength-strings
CFLAGS_DEBUG=-g -DDEBUG_MODE

OBJS_C=main.o clients/cli_client.o clients/gui_client.o clients/udp_scan.o proto/selector.o proto/versions.o proto/methods.o utils/utils.o utils/net_utils.o utils/list_utils.o utils/config.o utils/kill_others.o
OBJS_BIN=res/page.o
OBJS_M=

OTHER_DEPENDENCIES=
LINK_FLAGS_BUILD=

ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(detected_OS),Linux)
	OBJS_C+= xclip/xclip.o xclip/xclib.o
	CFLAGS+= -ftree-vrp -Wformat-signedness -Wshift-overflow=2 -Wstringop-overflow=4 -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wjump-misses-init -Wlogical-op -Wvla-larger-than=65536
	CFLAGS_OPTIM=-Os
	LDLIBS=-lmicrohttpd -lunistring -lX11 -lXmu -lXt
	LINK_FLAGS_BUILD=-no-pie -Wl,-s,--gc-sections
else ifeq ($(detected_OS),Windows)
	CFLAGS+= -ftree-vrp -Wformat-signedness -Wshift-overflow=2 -Wstringop-overflow=4 -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wjump-misses-init -Wlogical-op -Wvla-larger-than=65536
	CFLAGS+= -D__USE_MINGW_ANSI_STDIO
	CFLAGS_OPTIM=-O3
	OTHER_DEPENDENCIES+= res/win/app.coff
	LDLIBS=-l:libmicrohttpd.a -l:libunistring.a -lws2_32 -lgdi32 -lUserenv
	LINK_FLAGS_BUILD=-no-pie
	PROGRAM_NAME:=$(PROGRAM_NAME).exe
else ifeq ($(detected_OS),Darwin)
export CPATH:=$(CPATH):$(shell brew --prefix)/include
export LIBRARY_PATH:=$(LIBRARY_PATH):$(shell brew --prefix)/lib
	OBJS_M=utils/mac_utils.o
	CFLAGS+= -fobjc-arc
	CFLAGS_OPTIM=-O3
	CFLAGS+= -fobjc-arc
	CFLAGS_OPTIM=-O3
	LDLIBS=-framework AppKit -lmicrohttpd -lunistring -lobjc
else
$(error ClipShare is not supported on this platform!)
endif
CFLAGS+= -DINFO_NAME=\"clip_share\" -DPROTOCOL_MIN=$(MIN_PROTO) -DPROTOCOL_MAX=$(MAX_PROTO)
CFLAGS_OPTIM+= -Werror

VERSION_FILE=$(SRC_DIR)/res/version
ifeq (4.2,$(firstword $(sort $(MAKE_VERSION) 4.2)))
	VERSION_INFO=$(file < $(VERSION_FILE))
else
	VERSION_INFO=$(shell cat $(VERSION_FILE))
endif
VERSION_MAJOR=$(word 2 ,$(subst =, ,$(filter VERSION_MAJOR=%,$(VERSION_INFO))))
VERSION_MINOR=$(word 2 ,$(subst =, ,$(filter VERSION_MINOR=%,$(VERSION_INFO))))
VERSION_PATCH=$(word 2 ,$(subst =, ,$(filter VERSION_PATCH=%,$(VERSION_INFO))))
VERSION=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
CFLAGS+= -DVERSION=\"$(VERSION)\"

OBJS_C:=$(addprefix $(BUILD_DIR)/,$(OBJS_C))
OBJS_M:=$(addprefix $(BUILD_DIR)/,$(OBJS_M))
OBJS_BIN:=$(addprefix $(BUILD_DIR)/,$(OBJS_BIN))
OTHER_DEPENDENCIES:=$(addprefix $(BUILD_DIR)/,$(OTHER_DEPENDENCIES))

# append '_debug' to objects for debug executable to prevent overwriting objects for main build
DEBUG_OBJS_C=$(OBJS_C:.o=_debug.o)
DEBUG_OBJS_BIN=$(OBJS_BIN:.o=_debug.o)
DEBUG_OBJS=$(DEBUG_OBJS_C) $(DEBUG_OBJS_BIN)

OBJS=$(OBJS_C) $(OBJS_M) $(OBJS_BIN)
ALL_DEPENDENCIES=$(OBJS) $(DEBUG_OBJS) $(OTHER_DEPENDENCIES) $(OBJS_BIN:$(BUILD_DIR)/%.o=$(SRC_DIR)/%_.c)
DIRS=$(foreach file,$(ALL_DEPENDENCIES),$(dir $(file)))
DIRS:=$(sort $(DIRS))

$(PROGRAM_NAME): $(OBJS) $(OTHER_DEPENDENCIES)
	$(CC) $^ $(LINK_FLAGS_BUILD) $(LDLIBS) -o $@

.SECONDEXPANSION:
$(ALL_DEPENDENCIES): %: | $$(dir %)

$(OBJS_C): $(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
$(OBJS_M): $(BUILD_DIR)/%.o: $(SRC_DIR)/%.m
$(OBJS_BIN): $(BUILD_DIR)/%.o: $(SRC_DIR)/%_.c
$(OBJS):
	$(CC) $(CFLAGS_OPTIM) $(CFLAGS) -fno-pie $^ -o $@

$(DEBUG_OBJS_C): $(BUILD_DIR)/%_debug.o: $(SRC_DIR)/%.c
$(DEBUG_OBJS_BIN): $(BUILD_DIR)/%_debug.o: $(SRC_DIR)/%_.c
$(DEBUG_OBJS):
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) $^ -o $@

$(SRC_DIR)/res/page_.c: $(SRC_DIR)/res/page.html
	cd $(dir $<) && \
	xxd -i $(notdir $<) >$(notdir $@)

$(BUILD_DIR)/res/win/app.coff: $(SRC_DIR)/res/win/app_.rc $(SRC_DIR)/res/win/resource.h
	windres -I$(SRC_DIR) $< -O coff -o $@

$(SRC_DIR)/res/win/app_.rc: $(SRC_DIR)/res/win/app.rc $(VERSION_FILE)
	$(CPP) -I$(SRC_DIR) -P -DVERSION_MAJOR=$(VERSION_MAJOR) -DVERSION_MINOR=$(VERSION_MINOR) -DVERSION_PATCH=$(VERSION_PATCH) -DVERSION=\"$(VERSION)\" $< -o $@

$(DIRS):
	mkdir -p $@

.PHONY: clean debug test check

debug: $(DEBUG_OBJS) $(OTHER_DEPENDENCIES)
	$(CC) $^ $(LDLIBS) -o $(PROGRAM_NAME)

test: $(PROGRAM_NAME)
	@chmod +x tests/run.sh && cd tests && ./run.sh $(PROGRAM_NAME)

check: test

clean:
	$(RM) -r $(BUILD_DIR) $(ALL_DEPENDENCIES) $(SRC_DIR)/res/win/app_.rc
	$(RM) $(PROGRAM_NAME)
