# keyframe-resample-c
 Resamples and optimizes keyframe data in pure C. 

## Building

```bash
make WASI_SDK=~/wasi-sdk-20.0 WASM_OPT=~/binaryen-version_112/bin/wasm-opt
```

See [make.yml](.github/workflows/make.yml) for more detail.

## Performance

Online [benchmark](https://kzhsw.github.io/keyframe-resample-c/benchmark/benchmark.html) is available, code at [here](./benchmark).
