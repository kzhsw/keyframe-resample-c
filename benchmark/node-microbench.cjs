#!/usr/bin/env node

const path = require('node:path');
const { performance } = require('node:perf_hooks');

const root = path.resolve(process.argv[2] || '.');

function requireWasm(relativePath) {
    return require(path.join(root, relativePath)).wasm;
}

async function load(buffer) {
    const {instance} = await WebAssembly.instantiate(buffer);
    return instance.exports;
}

function prepare(exports, framesSrc, valuesSrc) {
    const heapPtr = exports.get_heap_ptr() >>> 2;
    const memory = new Float32Array(exports.memory.buffer);
    const needed = framesSrc.length + valuesSrc.length + 64;
    const available = memory.length - heapPtr;

    if (available < needed) {
        const bytesNeeded = (needed - available) * 4;
        exports.memory.grow(Math.ceil(bytesNeeded / 65536));
    }

    const grownMemory = new Float32Array(exports.memory.buffer);
    const base = exports.get_heap_ptr() >>> 2;
    const framesOffset = base;
    const valuesOffset = framesOffset + framesSrc.length + 16;

    return {
        memory: grownMemory,
        framesOffset,
        valuesOffset,
        framesPtr: framesOffset * 4,
        valuesPtr: valuesOffset * 4,
    };
}

function runCase(name, exports, fnName, frameCount, elementSize, makeValue) {
    const frames = new Float32Array(frameCount);
    const values = new Float32Array(frameCount * elementSize);

    for (let i = 0; i < frameCount; i++) {
        const t = i * 0.0333333333;
        frames[i] = t;
        makeValue(values, i, t);
    }

    const ptrs = prepare(exports, frames, values);
    const iterations = Math.max(20, Math.floor(200000 / frameCount));

    for (let i = 0; i < 5; i++) {
        ptrs.memory.set(frames, ptrs.framesOffset);
        ptrs.memory.set(values, ptrs.valuesOffset);
        exports[fnName](
            ptrs.framesPtr, 1,
            ptrs.valuesPtr, elementSize,
            frameCount, 1e-6);
    }

    const start = performance.now();
    for (let i = 0; i < iterations; i++) {
        ptrs.memory.set(frames, ptrs.framesOffset);
        ptrs.memory.set(values, ptrs.valuesOffset);
        exports[fnName](
            ptrs.framesPtr, 1,
            ptrs.valuesPtr, elementSize,
            frameCount, 1e-6);
    }
    const elapsed = performance.now() - start;

    return {
        name,
        iterations,
        avgUs: (elapsed * 1000) / iterations,
    };
}

function runUnknownCase(name, exports, fnName, frameCount, valueSize, makeValue) {
    const frames = new Float32Array(frameCount);
    const values = new Float32Array(frameCount * valueSize);

    for (let i = 0; i < frameCount; i++) {
        const t = i * 0.0333333333;
        frames[i] = t;
        makeValue(values, i, t);
    }

    const ptrs = prepare(exports, frames, values);
    const iterations = Math.max(20, Math.floor(140000 / frameCount));

    for (let i = 0; i < 5; i++) {
        ptrs.memory.set(frames, ptrs.framesOffset);
        ptrs.memory.set(values, ptrs.valuesOffset);
        exports[fnName](
            ptrs.framesPtr, 1,
            ptrs.valuesPtr, valueSize, valueSize,
            frameCount, 1e-6);
    }

    const start = performance.now();
    for (let i = 0; i < iterations; i++) {
        ptrs.memory.set(frames, ptrs.framesOffset);
        ptrs.memory.set(values, ptrs.valuesOffset);
        exports[fnName](
            ptrs.framesPtr, 1,
            ptrs.valuesPtr, valueSize, valueSize,
            frameCount, 1e-6);
    }
    const elapsed = performance.now() - start;

    return {
        name,
        iterations,
        avgUs: (elapsed * 1000) / iterations,
    };
}

async function main() {
    const wasm = await load(requireWasm('build/resample_wasm.cjs.js'));
    const simd = await load(requireWasm('build/resample_simd.cjs.js'));

    const results = [
        runCase('wasm lerp_scalar', wasm, 'lerp_scalar', 24000, 1, (arr, i, t) => {
            arr[i] = t;
        }),
        runCase('wasm lerp_vec3', wasm, 'lerp_vec3', 24000, 3, (arr, i, t) => {
            const offset = i * 3;
            arr[offset] = t;
            arr[offset + 1] = t * 2;
            arr[offset + 2] = t * 3;
        }),
        runCase('wasm lerp_vec4', wasm, 'lerp_vec4', 24000, 4, (arr, i, t) => {
            const offset = i * 4;
            arr[offset] = t;
            arr[offset + 1] = t * 2;
            arr[offset + 2] = t * 3;
            arr[offset + 3] = t * 4;
        }),
        runUnknownCase('wasm lerp_unknown5', wasm, 'lerp_unknown', 16000, 5, (arr, i, t) => {
            const offset = i * 5;
            arr[offset] = t;
            arr[offset + 1] = t * 2;
            arr[offset + 2] = t * 3;
            arr[offset + 3] = t * 4;
            arr[offset + 4] = t * 5;
        }),
        runCase('simd lerp_vec3', simd, 'lerp_vec3', 24000, 3, (arr, i, t) => {
            const offset = i * 3;
            arr[offset] = t;
            arr[offset + 1] = t * 2;
            arr[offset + 2] = t * 3;
        }),
        runCase('simd lerp_vec4', simd, 'lerp_vec4', 24000, 4, (arr, i, t) => {
            const offset = i * 4;
            arr[offset] = t;
            arr[offset + 1] = t * 2;
            arr[offset + 2] = t * 3;
            arr[offset + 3] = t * 4;
        }),
    ];

    for (const result of results) {
        console.log(
            `${result.name}: avg ${result.avgUs.toFixed(2)} us over ${result.iterations} iterations`);
    }
}

main().catch((error) => {
    console.error(error);
    process.exit(1);
});
