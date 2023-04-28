# benchmark

This is a simple benchmark using [glTF-Transform](https://github.com/donmccurdy/glTF-Transform).

See it on [github pages](https://kzhsw.github.io/keyframe-resample-c/benchmark/benchmark.html).

* [resample-fast.js](js/resample-fast.js) is an example of using [resample-wrapper.js](../resample-wrapper.js) with [glTF-Transform](https://github.com/donmccurdy/glTF-Transform)..
* [resample-opt.js](js/resample-opt.js) is the optimized resample algorithm from [glTF-Transform#922](https://github.com/donmccurdy/glTF-Transform/issues/922) in pure js.
* [resample-orig.js](js/resample-orig.js) is the resample algorithm from [glTF-Transform@v3.2.0](https://github.com/donmccurdy/glTF-Transform/tree/v3.2.0) ported to js, tried to stay close to the original impl is ts and added performance hooks.
