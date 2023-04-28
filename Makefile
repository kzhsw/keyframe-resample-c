BUILD=build
CFLAGS=-O3 -DNDEBUG -Wall -flto -std=c11
WASMCC?=$(WASI_SDK)/bin/clang
WASIROOT?=$(WASI_SDK)/share/wasi-sysroot

WASM_FLAGS=--target=wasm32-wasi --sysroot=$(WASIROOT) -fno-builtin -ffreestanding -fno-ident -nostartfiles -Wl,--gc-sections,--no-entry,--export-dynamic,--initial-memory=65536,-z,stack-size=8192

WASM_EXPORTS=-Wl,--export=slerp_quat,--export=lerp_vec4,--export=lerp_vec3,--export=lerp_vec2,--export=lerp_scalar,--export=step_vec4,--export=step_vec3,--export=step_vec2,--export=step_scalar,--export=step_unknown,--export=lerp_unknown,--export=stream_continue,--export=get_heap_ptr

js: $(BUILD)/resample_wasm.js $(BUILD)/resample_simd.js

$(BUILD)/resample_wasm.wasm: resample.c
	@mkdir -p $(BUILD)
	$(WASMCC) $^ $(CFLAGS) $(WASM_FLAGS) $(WASM_EXPORTS) -o $@

$(BUILD)/resample_simd.wasm: resample.c
	@mkdir -p $(BUILD)
	$(WASMCC) $^ $(CFLAGS) -msimd128 $(WASM_FLAGS) $(WASM_EXPORTS) -o $@

$(BUILD)/resample_wasm_o4.wasm: $(BUILD)/resample_wasm.wasm
	$(WASM_OPT) -O4 --strip-debug -o $@ $^

$(BUILD)/resample_simd_o4.wasm: $(BUILD)/resample_simd.wasm
	$(WASM_OPT) -O4 --strip-debug --enable-simd -o $@ $^

$(BUILD)/%.js: $(BUILD)/%_o4.wasm
	python3 wasmpack.py wasm $^ > $@

js/meshopt_decoder.js: build/decoder_base.wasm build/decoder_simd.wasm tools/wasmpack.py
	sed -i "s#Built with clang.*#Built with $$($(WASMCC) --version | head -n 1 | sed 's/\s\+(.*//')#" $@
	sed -i "s#\(var wasm_base = \)\".*\";#\\1\"$$(cat build/decoder_base.wasm | python3 tools/wasmpack.py)\";#" $@
	sed -i "s#\(var wasm_simd = \)\".*\";#\\1\"$$(cat build/decoder_simd.wasm | python3 tools/wasmpack.py)\";#" $@

js/meshopt_encoder.js: build/encoder.wasm tools/wasmpack.py
	sed -i "s#Built with clang.*#Built with $$($(WASMCC) --version | head -n 1 | sed 's/\s\+(.*//')#" $@
	sed -i "s#\(var wasm = \)\".*\";#\\1\"$$(cat build/encoder.wasm | python3 tools/wasmpack.py)\";#" $@

js/meshopt_simplifier.js: build/simplifier.wasm tools/wasmpack.py
	sed -i "s#Built with clang.*#Built with $$($(WASMCC) --version | head -n 1 | sed 's/\s\+(.*//')#" $@
	sed -i "s#\(var wasm = \)\".*\";#\\1\"$$(cat build/simplifier.wasm | python3 tools/wasmpack.py)\";#" $@

js/%.module.js: js/%.js
	sed '/UMD-style export/,$$d' <$< >$@
	sed -n "s#\s*module.exports = \(.*\);#export { \\1 };#p" <$< >>$@

clean:
	rm -rf $(BUILD)

.PHONY: all clean js
