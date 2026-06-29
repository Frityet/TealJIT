CC := clang
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude

.PHONY: check
check:
	$(CC) $(CFLAGS) -fsyntax-only src/lj_wasmtime_host.c
