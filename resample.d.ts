/* eslint-disable */

export declare interface AnimationResampleInstance extends WebAssembly.Instance {
    readonly exports: AnimationResampleExports;
}

declare type ResampleFn = (
    frames: number, frame_stride: number,
    values: number, value_stride: number,
    count: number, tolerance: number
) => number;

declare type ResampleUnknownFn = (
    frames: number, frame_stride: number,
    values: number, value_size: number, value_stride: number,
    count: number, tolerance: number
) => number;

export declare interface AnimationResampleExports extends WebAssembly.Exports {
    readonly memory: WebAssembly.Memory;
    get_heap_ptr(): number;
    stream_continue(
        frames: number, frame_stride: number,
        values: number, value_size: number, value_stride: number,
        count: number
    ): number;
    readonly step_unknown: ResampleUnknownFn;
    readonly lerp_unknown: ResampleUnknownFn;
    readonly onlerp_quat: ResampleFn;
    readonly slerp_quat: ResampleFn;
    readonly lerp_vec4: ResampleFn;
    readonly lerp_vec3: ResampleFn;
    readonly lerp_vec2: ResampleFn;
    readonly lerp_scalar: ResampleFn;
    readonly step_vec4: ResampleFn;
    readonly step_vec3: ResampleFn;
    readonly step_vec2: ResampleFn;
    readonly step_scalar: ResampleFn;
}

declare type AnimationResampleWrapperUnknownFn = (
    frames: Float32Array,
    values: Float32Array,
    elementSize: number,
    tolerance: number
) => {frames: Float32Array, values: Float32Array};

declare type AnimationResampleWrapperFn = (
    frames: Float32Array,
    values: Float32Array,
    tolerance: number
) => {frames: Float32Array, values: Float32Array};

export declare interface AnimationResampleWrapper {
    readonly instance: AnimationResampleInstance;
    module?: WebAssembly.Module;

    readonly step_unknown: AnimationResampleWrapperUnknownFn;
    readonly lerp_unknown: AnimationResampleWrapperUnknownFn;
    readonly onlerp_quat: AnimationResampleWrapperFn;
    readonly slerp_quat: AnimationResampleWrapperFn;
    readonly lerp_vec4: AnimationResampleWrapperFn;
    readonly lerp_vec3: AnimationResampleWrapperFn;
    readonly lerp_vec2: AnimationResampleWrapperFn;
    readonly lerp_scalar: AnimationResampleWrapperFn;
    readonly step_vec4: AnimationResampleWrapperFn;
    readonly step_vec3: AnimationResampleWrapperFn;
    readonly step_vec2: AnimationResampleWrapperFn;
    readonly step_scalar: AnimationResampleWrapperFn;
}
