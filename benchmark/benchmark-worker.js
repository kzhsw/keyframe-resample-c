import {WebIO} from 'https://cdn.skypack.dev/@gltf-transform/core'
import {wasm} from '../build/resample_wasm.esm.js';
import {wasm as simdWasm} from '../build/resample_simd.esm.js';
import {makeWrapper} from '../resample-wrapper.js';
import {resampleFast} from './js/resample-fast.js';
import {resample as resampleOpt} from "./js/resample-opt.js";
import {resample} from "./js/resample-orig.js";

async function benchmarkWasm(simd, memorySize, warmup, run) {
    let buf = new Uint8Array(await fetch('./demo.glb')
        .then(e=>e.arrayBuffer()));
    let io = new WebIO();
    let doc = await io.readBinary(buf);

    const mod = simd ? simdWasm : wasm;
    const {instance} = await WebAssembly.instantiate(mod);
    if (memorySize) {
        instance.exports.memory.grow(memorySize);
    }
    const w = makeWrapper(instance);

    const optionHolder1 = {
        w: w,
        tolerance: 1.1920928955078125e-07,
        weights: false,
        stats: {
            beforeLength: 0,
            beforeFrames: 0,
            timeEscaped: 0,
            afterLength: 0,
            afterFrames: 0,
        }
    };
    // warmup
    for (let i = 0; i < warmup; i++) {
        const doc1 = doc.clone();
        await doc1.transform(resampleFast(optionHolder1));
    }
    const optionHolder = {
        w: w,
        tolerance: 1.1920928955078125e-07,
        weights: false,
        stats: {
            beforeLength: 0,
            beforeFrames: 0,
            timeEscaped: 0,
            afterLength: 0,
            afterFrames: 0,
        }
    };
    // run
    for (let i = 0; i < run; i++) {
        const doc1 = doc.clone();
        await doc1.transform(resampleFast(optionHolder));
    }
    return optionHolder.stats;
}

async function benchmarkJs(optimize, warmup, run) {
    let buf = new Uint8Array(await fetch('./demo.glb')
        .then(e=>e.arrayBuffer()));
    let io = new WebIO();
    let doc = await io.readBinary(buf);

    const resampleFn =
        optimize ? resampleOpt : resample;

    const optionHolder1 = {
        tolerance: 1.1920928955078125e-07,
        stats: {
            beforeLength: 0,
            beforeFrames: 0,
            timeEscaped: 0,
            afterLength: 0,
            afterFrames: 0,
        }
    };
    // warmup
    for (let i = 0; i < warmup; i++) {
        const doc1 = doc.clone();
        await doc1.transform(resampleFn(optionHolder1));
    }
    const optionHolder = {
        tolerance: 1.1920928955078125e-07,
        stats: {
            beforeLength: 0,
            beforeFrames: 0,
            timeEscaped: 0,
            afterLength: 0,
            afterFrames: 0,
        }
    };
    // run
    for (let i = 0; i < run; i++) {
        const doc1 = doc.clone();
        await doc1.transform(resampleFn(optionHolder));
    }
    return optionHolder.stats;
}

async function benchmark(type, memorySize, warmup, run) {
    let stats;
    if (type === 'wasm' || type === 'simd') {
        stats = await benchmarkWasm(type === 'simd', memorySize, warmup, run);
    }
    if (type === 'js' || type === 'jsOpt') {
        stats = await benchmarkJs(type === 'jsOpt', warmup, run);
    }
    return stats;
}

onmessage = async (ev) => {
    let {type, memorySize, warmup, run} = ev.data;
    benchmark(type, memorySize, warmup, run).then(stats => {
        if (stats) {
            postMessage(stats);
        } else {
            postMessage({
                err: 'unknown',
            });
        }
    }, err => {
        console.error(err);
        postMessage({
            err: '' + err,
        });
    });
};
