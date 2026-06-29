CC ?= cc
EMCC ?= emcc
LLVM_OBJDUMP ?= llvm-objdump
WASM_VALIDATE ?= wasm-validate
ROOT := ../..
SRC := $(ROOT)/src
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -I$(SRC)
WASM64_CFLAGS ?= -sMEMORY64=1 -std=c99 -Wall -Wextra -Werror -I$(SRC)
LDLIBS ?= -lm -ldl
OUT ?= /tmp/lj-wasm-nyi-trace.wasm
OUT_IR ?= /tmp/lj-wasm-ir-trace.wasm
WASM64_CHECK_DIR ?= /tmp/lj-wasm64-support-check
WASM64_SKIP_SRC :=
WASM64_CORE_SRC := $(filter-out $(WASM64_SKIP_SRC),$(notdir $(wildcard $(SRC)/*.c)))

.PHONY: check trace-module-check trace-ir-module-check wasm64-support-check clean

wasm_nyi_trace_module: wasm_nyi_trace_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_nyi_trace_module.c $(SRC)/libluajit.a $(LDLIBS)

wasm_trace_ir_module: wasm_trace_ir_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_trace_ir_module.c $(SRC)/libluajit.a $(LDLIBS)

check: trace-module-check trace-ir-module-check wasm64-support-check

trace-module-check: wasm_nyi_trace_module
	./wasm_nyi_trace_module > $(OUT)
	test `wc -c < $(OUT)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT) >/dev/null
	$(WASM_VALIDATE) $(OUT)

trace-ir-module-check: wasm_trace_ir_module
	./wasm_trace_ir_module > $(OUT_IR)
	test `wc -c < $(OUT_IR)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT_IR) >/dev/null
	$(WASM_VALIDATE) $(OUT_IR)

wasm64-support-check:
	mkdir -p $(WASM64_CHECK_DIR)
	for file in $(WASM64_CORE_SRC); do \
	  extra=; \
	  if test "$$file" = ljamalg.c; then extra="-Wno-unused-function"; fi; \
	  $(EMCC) $(WASM64_CFLAGS) $$extra -c $(SRC)/$$file -o $(WASM64_CHECK_DIR)/$${file%.c}.o; \
	done

clean:
	rm -f wasm_nyi_trace_module wasm_trace_ir_module
