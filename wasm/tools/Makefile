CC ?= cc
EMCC ?= emcc
LLVM_OBJDUMP ?= llvm-objdump
PYTHON ?= python3
WASM_OBJDUMP ?= wasm-objdump
WASM_VALIDATE ?= wasm-validate
WASM_VALIDATE_FLAGS ?= --enable-memory64
ROOT := ../..
SRC := $(ROOT)/src
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -I$(SRC)
WASM64_CFLAGS ?= -sMEMORY64=1 -std=c99 -Wall -Wextra -Werror -I$(SRC)
LDLIBS ?= -lm -ldl
OUT ?= /tmp/lj-wasm-nyi-trace.wasm
OUT_IR ?= /tmp/lj-wasm-ir-trace.wasm
OUT_NUMERIC ?= /tmp/lj-wasm-numeric-for-trace.wasm
OUT_ARRAY_STORE ?= /tmp/lj-wasm-array-store-trace.wasm
OUT_ARRAY_READ ?= /tmp/lj-wasm-array-read-trace.wasm
OUT_FALLBACK ?= /tmp/lj-wasm-fallback-trace.wasm
WASM64_CHECK_DIR ?= /tmp/lj-wasm64-support-check
WASM64_SKIP_SRC :=
WASM64_CORE_SRC := $(filter-out $(WASM64_SKIP_SRC),$(notdir $(wildcard $(SRC)/*.c)))

.PHONY: check trace-module-check trace-ir-module-check trace-numeric-for-module-check trace-array-store-module-check trace-array-read-module-check trace-runtime-check trace-fallback-module-check trace-enter-contract-check wasm64-support-check clean

wasm_nyi_trace_module: wasm_nyi_trace_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_nyi_trace_module.c $(SRC)/libluajit.a $(LDLIBS)

wasm_trace_ir_module: wasm_trace_ir_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_trace_ir_module.c $(SRC)/libluajit.a $(LDLIBS)

wasm_trace_numeric_for_module: wasm_trace_numeric_for_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_trace_numeric_for_module.c $(SRC)/libluajit.a $(LDLIBS)

wasm_trace_array_store_module: wasm_trace_array_store_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_trace_array_store_module.c $(SRC)/libluajit.a $(LDLIBS)

wasm_trace_array_read_module: wasm_trace_array_read_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_trace_array_read_module.c $(SRC)/libluajit.a $(LDLIBS)

wasm_trace_fallback_module: wasm_trace_fallback_module.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_trace_fallback_module.c $(SRC)/libluajit.a $(LDLIBS)

wasm_trace_enter_contract: wasm_trace_enter_contract.c $(SRC)/libluajit.a
	$(CC) $(CFLAGS) -o $@ wasm_trace_enter_contract.c $(SRC)/libluajit.a $(LDLIBS)

check: trace-module-check trace-ir-module-check trace-numeric-for-module-check trace-array-store-module-check trace-array-read-module-check trace-runtime-check trace-fallback-module-check trace-enter-contract-check wasm64-support-check

trace-module-check: wasm_nyi_trace_module
	./wasm_nyi_trace_module > $(OUT)
	test `wc -c < $(OUT)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT) >/dev/null
	$(WASM_VALIDATE) $(WASM_VALIDATE_FLAGS) $(OUT)

trace-ir-module-check: wasm_trace_ir_module
	./wasm_trace_ir_module > $(OUT_IR)
	test `wc -c < $(OUT_IR)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT_IR) >/dev/null
	$(WASM_VALIDATE) $(WASM_VALIDATE_FLAGS) $(OUT_IR)
	$(WASM_OBJDUMP) -x $(OUT_IR) | grep -q 'i64 <- env.memory'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i32.load'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'f64.load'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'f64.store'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i64.load'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i64.and'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i32.ge_u'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i64.extend_i32_u'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i64.shl'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i64.shr_u'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i64.eq'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i64.lt_u'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i32.wrap_i64'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'i32.trunc_f64_s'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'f64.convert_i32_s'
	$(WASM_OBJDUMP) -d $(OUT_IR) | grep -q 'f64.reinterpret_i64'

trace-numeric-for-module-check: wasm_trace_numeric_for_module
	./wasm_trace_numeric_for_module > $(OUT_NUMERIC)
	test `wc -c < $(OUT_NUMERIC)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT_NUMERIC) >/dev/null
	$(WASM_VALIDATE) $(WASM_VALIDATE_FLAGS) $(OUT_NUMERIC)
	$(WASM_OBJDUMP) -x $(OUT_NUMERIC) | grep -q 'i64 <- env.memory'
	$(WASM_OBJDUMP) -d $(OUT_NUMERIC) | grep -q 'i32.trunc_f64_s'
	$(WASM_OBJDUMP) -d $(OUT_NUMERIC) | grep -q 'f64.convert_i32_s'
	$(WASM_OBJDUMP) -d $(OUT_NUMERIC) | grep -q 'i64.lt_u'
	$(WASM_OBJDUMP) -d $(OUT_NUMERIC) | grep -q 'i64.store'
	$(WASM_OBJDUMP) -d $(OUT_NUMERIC) | grep -q 'f64.store'
	$(WASM_OBJDUMP) -d $(OUT_NUMERIC) | grep -q 'loop'
	$(WASM_OBJDUMP) -d $(OUT_NUMERIC) | grep -q 'i32.const 100'

trace-array-store-module-check: wasm_trace_array_store_module
	./wasm_trace_array_store_module > $(OUT_ARRAY_STORE)
	test `wc -c < $(OUT_ARRAY_STORE)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT_ARRAY_STORE) >/dev/null
	$(WASM_VALIDATE) $(WASM_VALIDATE_FLAGS) $(OUT_ARRAY_STORE)
	$(WASM_OBJDUMP) -x $(OUT_ARRAY_STORE) | grep -q 'i64 <- env.memory'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_STORE) | grep -q 'i32.trunc_f64_s'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_STORE) | grep -q 'f64.convert_i32_s'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_STORE) | grep -q 'f64.store'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_STORE) | grep -q 'i64.and'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_STORE) | grep -q 'i32.ge_u'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_STORE) | grep -q 'i32.const 100'

trace-array-read-module-check: wasm_trace_array_read_module
	./wasm_trace_array_read_module > $(OUT_ARRAY_READ)
	test `wc -c < $(OUT_ARRAY_READ)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT_ARRAY_READ) >/dev/null
	$(WASM_VALIDATE) $(WASM_VALIDATE_FLAGS) $(OUT_ARRAY_READ)
	$(WASM_OBJDUMP) -x $(OUT_ARRAY_READ) | grep -q 'i64 <- env.memory'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_READ) | grep -q 'i32.rem_s'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_READ) | grep -q 'f64.reinterpret_i64'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_READ) | grep -q 'i64.lt_u'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_READ) | grep -q 'i32.ge_u'
	$(WASM_OBJDUMP) -d $(OUT_ARRAY_READ) | grep -q 'i32.const 100'

trace-runtime-check: wasm_trace_numeric_for_module
	./wasm_trace_numeric_for_module > $(OUT_NUMERIC)
	$(PYTHON) wasm_trace_runtime_check.py $(OUT_NUMERIC)

trace-fallback-module-check: wasm_trace_fallback_module
	./wasm_trace_fallback_module > $(OUT_FALLBACK)
	test `wc -c < $(OUT_FALLBACK)` -gt 8
	$(LLVM_OBJDUMP) -h $(OUT_FALLBACK) >/dev/null
	$(WASM_VALIDATE) $(WASM_VALIDATE_FLAGS) $(OUT_FALLBACK)

trace-enter-contract-check: wasm_trace_enter_contract
	./wasm_trace_enter_contract

wasm64-support-check:
	mkdir -p $(WASM64_CHECK_DIR)
	for file in $(WASM64_CORE_SRC); do \
	  extra=; \
	  if test "$$file" = ljamalg.c; then extra="-Wno-unused-function"; fi; \
	  $(EMCC) $(WASM64_CFLAGS) $$extra -c $(SRC)/$$file -o $(WASM64_CHECK_DIR)/$${file%.c}.o; \
	done

clean:
	rm -f wasm_nyi_trace_module wasm_trace_ir_module wasm_trace_numeric_for_module wasm_trace_array_store_module wasm_trace_array_read_module wasm_trace_fallback_module wasm_trace_enter_contract
