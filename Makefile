BUILD=build
CFLAGS=-O3 -DNDEBUG -Wall -flto -std=c11
WASMCC?=$(WASI_SDK)/bin/clang
WASIROOT?=$(WASI_SDK)/share/wasi-sysroot

WASM_FLAGS=--target=wasm32-wasi --sysroot=$(WASIROOT) -fno-builtin -ffreestanding -fno-ident -nostartfiles -Wl,--gc-sections,--no-entry,--export-dynamic,--initial-memory=65536,-z,stack-size=8192

WASM_EXPORTS=-Wl,--export=slerp_quat,--export=lerp_vec4,--export=lerp_vec3,--export=lerp_vec2,--export=lerp_scalar,--export=step_vec4,--export=step_vec3,--export=step_vec2,--export=step_scalar,--export=step_unknown,--export=lerp_unknown,--export=denormalize,--export=normalize,--export=stream_continue,--export=get_heap_ptr

js: $(BUILD)/resample_wasm.esm.js $(BUILD)/resample_simd.esm.js $(BUILD)/resample_wasm.cjs.js $(BUILD)/resample_simd.cjs.js

$(BUILD)/resample_wasm.wasm: resample.c normalize.c
	@mkdir -p $(BUILD)
	$(WASMCC) $^ $(CFLAGS) $(WASM_FLAGS) $(WASM_EXPORTS) -o $@

$(BUILD)/resample_simd.wasm: resample.c normalize.c
	@mkdir -p $(BUILD)
	$(WASMCC) $^ $(CFLAGS) -msimd128 $(WASM_FLAGS) $(WASM_EXPORTS) -o $@

$(BUILD)/resample_wasm_o4.wasm: $(BUILD)/resample_wasm.wasm
	$(WASM_OPT) -O4 --strip-debug -o $@ $^

$(BUILD)/resample_simd_o4.wasm: $(BUILD)/resample_simd.wasm
	$(WASM_OPT) -O4 --strip-debug --enable-simd -o $@ $^

$(BUILD)/%.esm.js: $(BUILD)/%_o4.wasm
	python3 wasmpack.py esm wasm $^ > $@

$(BUILD)/%.cjs.js: $(BUILD)/%_o4.wasm
	python3 wasmpack.py cjs wasm $^ > $@

clean:
	rm -rf $(BUILD)

.PHONY: all clean js
