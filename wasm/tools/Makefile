CC ?= cc
LLVM_OBJDUMP ?= llvm-objdump
ROOT := ../..
SRC := $(ROOT)/src
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -I$(SRC)
LDLIBS ?= -lm -ldl
OUT ?= /tmp/lj-wasm-nyi-trace.wasm

.PHONY: check clean

wasm_nyi_trace_module: wasm_nyi_trace_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_nyi_trace_module.c $(SRC)/libluajit.a $(LDLIBS)

check: wasm_nyi_trace_module
	./wasm_nyi_trace_module > $(OUT)
	test `wc -c < $(OUT)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT) >/dev/null

clean:
	rm -f wasm_nyi_trace_module
