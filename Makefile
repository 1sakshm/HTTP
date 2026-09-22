CC ?= cc
CPPFLAGS := -Iinclude -D_POSIX_C_SOURCE=200809L
WARNINGS := -Wall -Wextra -Wpedantic -Werror
COMMON_CFLAGS := -std=c11 $(WARNINGS)
SOURCES := src/main.c src/server.c src/response.c
DEBUG_TARGET := build/debug/cserve
RELEASE_TARGET := build/release/cserve

.PHONY: all debug release test clean

all: debug

debug: CFLAGS := $(COMMON_CFLAGS) -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
debug: LDFLAGS := -fsanitize=address,undefined
debug: $(DEBUG_TARGET)

release: CFLAGS := $(COMMON_CFLAGS) -O2 -DNDEBUG
release: $(RELEASE_TARGET)

$(DEBUG_TARGET): $(SOURCES) include/server.h include/response.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $@

$(RELEASE_TARGET): $(SOURCES) include/server.h include/response.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $@

test: debug
	sh tests/smoke.sh

clean:
	rm -rf build
