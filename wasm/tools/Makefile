CC ?= cc
EMCC ?= emcc
LLVM_OBJDUMP ?= llvm-objdump
ROOT := ../..
SRC := $(ROOT)/src
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -I$(SRC)
WASM64_CFLAGS ?= -sMEMORY64=1 -std=c99 -Wall -Wextra -Werror -I$(SRC)
LDLIBS ?= -lm -ldl
OUT ?= /tmp/lj-wasm-nyi-trace.wasm
WASM64_CHECK_DIR ?= /tmp/lj-wasm64-support-check
# The amalgam currently follows lj_asm.c into the missing WASM assembler.
WASM64_SKIP_SRC := ljamalg.c lj_asm.c
WASM64_CORE_SRC := $(filter-out $(WASM64_SKIP_SRC),$(notdir $(wildcard $(SRC)/*.c)))

.PHONY: check trace-module-check wasm64-support-check clean

wasm_nyi_trace_module: wasm_nyi_trace_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_nyi_trace_module.c $(SRC)/libluajit.a $(LDLIBS)

check: trace-module-check wasm64-support-check

trace-module-check: wasm_nyi_trace_module
	./wasm_nyi_trace_module > $(OUT)
	test `wc -c < $(OUT)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT) >/dev/null

wasm64-support-check:
	mkdir -p $(WASM64_CHECK_DIR)
	for file in $(WASM64_CORE_SRC); do \
	  $(EMCC) $(WASM64_CFLAGS) -c $(SRC)/$$file -o $(WASM64_CHECK_DIR)/$${file%.c}.o; \
	done

clean:
	rm -f wasm_nyi_trace_module
