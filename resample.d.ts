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

declare const enum GltfComponentType {
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
}

export declare interface AnimationResampleExports extends WebAssembly.Exports {
    readonly memory: WebAssembly.Memory;
    get_heap_ptr(): number;
    stream_continue(
        frames: number, frame_stride: number,
        values: number, value_size: number, value_stride: number,
        count: number
    ): number;
    normalize(
        ptr: number,
        size: number, stride: number, count: number,
        component_type: number
    ): number;
    denormalize(
        ptr: number,
        size: number, stride: number, count: number,
        component_type: number
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


/**
 * Internal representing supported typed array classes
 * @hidden
 */
export declare type TypedArray = Float32Array | Uint16Array | Uint8Array | Int16Array | Int8Array;


declare type AnimationResampleWrapperUnknownFn = <T extends TypedArray>(
    frames: T,
    values: T,
    elementSize: number,
    tolerance: number,
    normalize?: GltfComponentType | number
) => {frames: T, values: T};

declare type AnimationResampleWrapperFn = <T extends TypedArray>(
    frames: T,
    values: T,
    tolerance: number,
    normalize?: GltfComponentType | number
) => {frames: T, values: T};

export declare interface AnimationResampleWrapper {
    readonly instance: AnimationResampleInstance;

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
