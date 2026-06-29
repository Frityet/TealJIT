CC := clang
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude
HOST_TEST := host_contract_test

.PHONY: check syntax-check host-contract-check clean

check: syntax-check host-contract-check

syntax-check:
	$(CC) $(CFLAGS) -fsyntax-only src/lj_wasmtime_host.c

$(HOST_TEST): tests/host_contract_test.c src/lj_wasmtime_host.c include/lj_wasmtime_host.h
	$(CC) $(CFLAGS) -o $@ tests/host_contract_test.c src/lj_wasmtime_host.c

host-contract-check: $(HOST_TEST)
	./$(HOST_TEST)

clean:
	rm -f $(HOST_TEST)
