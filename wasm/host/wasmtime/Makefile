CC := clang
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude
PKG_CONFIG ?= pkg-config
LIBFFI_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags libffi 2>/dev/null)
LIBFFI_LIBS ?= $(shell $(PKG_CONFIG) --libs libffi 2>/dev/null)
DL_LIBS ?= -ldl
PYTHON ?= python3
HOST_TEST := host_contract_test
HOST_SHARED := liblj_wasmtime_host.so

ifneq ($(strip $(LIBFFI_LIBS)),)
CFLAGS += -DLJ_WASMTIME_ENABLE_LIBFFI=1 $(LIBFFI_CFLAGS)
LDLIBS += $(LIBFFI_LIBS) $(DL_LIBS)
endif

.PHONY: check syntax-check host-contract-check guest-runtime-check clean

check: syntax-check host-contract-check

syntax-check:
	$(CC) $(CFLAGS) -fsyntax-only src/lj_wasmtime_host.c

$(HOST_TEST): tests/host_contract_test.c src/lj_wasmtime_host.c include/lj_wasmtime_host.h
	$(CC) $(CFLAGS) -o $@ tests/host_contract_test.c src/lj_wasmtime_host.c $(LDLIBS)

$(HOST_SHARED): src/lj_wasmtime_host.c include/lj_wasmtime_host.h
	$(CC) $(CFLAGS) -shared -fPIC -o $@ src/lj_wasmtime_host.c $(LDLIBS)

host-contract-check: $(HOST_TEST)
	./$(HOST_TEST)

ifneq ($(strip $(LIBFFI_LIBS)),)
guest-runtime-check: $(HOST_SHARED)
	$(PYTHON) tests/guest_runtime_check.py ./$(HOST_SHARED)
else
guest-runtime-check:
	@echo "SKIP guest-runtime-check: libffi not available"
endif

clean:
	rm -f $(HOST_TEST) $(HOST_SHARED)
