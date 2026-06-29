CC := clang
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude
PKG_CONFIG ?= pkg-config
LIBFFI_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags libffi 2>/dev/null)
LIBFFI_LIBS ?= $(shell $(PKG_CONFIG) --libs libffi 2>/dev/null)
DL_LIBS ?= -ldl
HOST_TEST := host_contract_test

ifneq ($(strip $(LIBFFI_LIBS)),)
CFLAGS += -DLJ_WASMTIME_ENABLE_LIBFFI=1 $(LIBFFI_CFLAGS)
LDLIBS += $(LIBFFI_LIBS) $(DL_LIBS)
endif

.PHONY: check syntax-check host-contract-check clean

check: syntax-check host-contract-check

syntax-check:
	$(CC) $(CFLAGS) -fsyntax-only src/lj_wasmtime_host.c

$(HOST_TEST): tests/host_contract_test.c src/lj_wasmtime_host.c include/lj_wasmtime_host.h
	$(CC) $(CFLAGS) -o $@ tests/host_contract_test.c src/lj_wasmtime_host.c $(LDLIBS)

host-contract-check: $(HOST_TEST)
	./$(HOST_TEST)

clean:
	rm -f $(HOST_TEST)
