import {PropertyType, Root} from "@gltf-transform/core";
import {createTransform, dedup, isTransformPending} from '@gltf-transform/functions';

const NAME = 'resampleFast';

const RESAMPLE_DEFAULTS = {
    tolerance: 1.1920928955078125e-07,
    // disabled by default
    weights: false,
    // stats: {
    //     beforeLength: 0,
    //     beforeFrames: 0,
    //     timeEscaped: 0,
    //     afterLength: 0,
    //     afterFrames: 0,
    // }
};

/**
 * Reimplementation of gltf-transform's `resample` in wasm, utilizing cglm and simd
 *
 * Resample {@link Animation}s, losslessly deduplicating keyframes to reduce file size. Duplicate
 * keyframes are commonly present in animation 'baked' by the authoring software to apply IK
 * constraints or other software-specific features. Based on THREE.KeyframeTrack.optimize().
 *
 * Example: (0,0,0,0,1,1,1,0,0,0,0,0,0,0) --> (0,0,1,1,0,0)
 */
export function resampleFast(_options = RESAMPLE_DEFAULTS) {
    const options = {...RESAMPLE_DEFAULTS, ..._options};

    return createTransform(NAME, async (document, context) => {
        const accessorsVisited = new Set();
        const srcAccessorCount = document.getRoot().listAccessors().length;
        const logger = document.getLogger();

        let didSkipMorphTargets = false;
        const wrapper = options.wrapper;

        for (const animation of document.getRoot().listAnimations()) {
            // Skip morph targets, see https://github.com/donmccurdy/glTF-Transform/issues/290.
            const samplerTargetPaths = new Map();
            for (const channel of animation.listChannels()) {
                samplerTargetPaths.set(channel.getSampler(), channel.getTargetPath());
            }

            for (const sampler of animation.listSamplers()) {
                const targetPath = samplerTargetPaths.get(sampler);
                if (!options.weights && targetPath === 'weights') {
                    didSkipMorphTargets = true;
                    continue;
                }
                // getXXX in gltf-transform is expensive
                const interpolation = sampler.getInterpolation();
                if (interpolation === 'STEP' || interpolation === 'LINEAR') {
                    accessorsVisited.add(sampler.getInput());
                    accessorsVisited.add(sampler.getOutput());
                    optimize(sampler, targetPath, options, wrapper, logger);
                } else {
                    logger.debug(`${NAME}: Skipped unsupported interpolation ${interpolation}`);
                }
            }
        }

        for (const accessor of Array.from(accessorsVisited.values())) {
            const used = accessor.listParents().some((p) => !(p instanceof Root));
            if (!used) accessor.dispose();
        }

        // Resampling may result in duplicate input or output sampler
        // accessors. Find and remove the duplicates after processing.
        const dstAccessorCount = document.getRoot().listAccessors().length;
        if (dstAccessorCount > srcAccessorCount && !isTransformPending(context, NAME, 'dedup')) {
            await document.transform(dedup({propertyTypes: [PropertyType.ACCESSOR]}));
        }

        if (didSkipMorphTargets) {
            logger.debug(`${NAME}: Skipped optimizing morph target keyframes.`);
        }

        logger.debug(`${NAME}: Complete.`);
    });

}

/**
 * @param {import("@gltf-transform/core").Accessor} accessor
 * @param {import("@gltf-transform/core").AnimationSampler} sampler
 * @param {number} type
 * @param {import("@gltf-transform/core").GLTF.AnimationChannelTargetPath} path
 * @param {Map} samplerTargetPaths
 */
// function shouldClone(
//         accessor, sampler, type,
//         path,
//         samplerTargetPaths
// ) {
//     if (accessor.getSparse()) {
//         return true;
//     }
//     let listParents = accessor.listParents();
//     for (let i = 0, length = listParents.length; i < length; i++) {
//         let p = listParents[i];
//         if (p.propertyType === 'Root' || p === sampler) {
//             continue;
//         }
//         if (p.propertyType !== "AnimationSampler") {
//             return true;
//         }
//         if (type === 0 && p.getInput() !== accessor) {
//             return true;
//         }
//         if (type === 1 && p.getOutput() !== accessor) {
//             return true;
//         }
//         if (sampler.getInterpolation() !== p.getInterpolation()) {
//             return true;
//         }
//         if (path !== samplerTargetPaths.get(p)) {
//             return true;
//         }
//     }
//     return false;
// }

/**
 *
 * @param {import("@gltf-transform/core").AnimationSampler} sampler
 * @param {import("@gltf-transform/core").GLTF.AnimationChannelTargetPath} path
 * @param {typeof RESAMPLE_DEFAULTS} options
 * @param {import('./animation-resample.d.ts').AnimationResampleWrapper} wrapper
 * @param {import("@gltf-transform/core").ILogger} logger
 */
function optimize(
    sampler, path,
    options, wrapper, logger
) {
    let input = sampler.getInput();
    let output = sampler.getOutput();
    let frames = input.getArray();
    let values = output.getArray();
    if (!(frames instanceof Float32Array) || !(values instanceof Float32Array)) {
        logger.warn(`${NAME}: skipping normalized or quantized sampler ${
            sampler.getName()
        } as not supported`);
        return;
    }
    let cloned = false;
    // there might be some conditions where this should not be cloned
    // if (shouldClone(input, sampler, 0, path, samplerTargetPaths) ||
    //         shouldClone(output, sampler, 1, path, samplerTargetPaths)) {
    input = input.clone().setSparse(false);
    output = output.clone().setSparse(false);
    cloned = true;
    // }

    const tolerance = options.tolerance;
    const interpolation = sampler.getInterpolation();

    frames = input.getArray();
    values = output.getArray();
    let result = {frames, values};
    // const beforeLength = frames.byteLength + values.byteLength;
    // const beforeFrames = frames.length;
    // const ts = performance.now();
    if (interpolation === 'LINEAR' && path === 'rotation') {
        result = w.slerp_quat(frames, values, tolerance);
    } else {
        let elementSize = output.getElementSize();
        if (path === 'weights') {
            elementSize = values.length / frames.length;
        }
        // es2015 Number.isInteger
        if (!Number.isInteger(elementSize)) {
            logger.warn(`${
                NAME
            }: skipping sampler ${sampler.getName()} with unsupported element size ${
                elementSize
            }, path=${path}, interpolation=${interpolation}`);
            if (cloned) {
                input.dispose();
                output.dispose();
            }
            return;
        }
        if (interpolation === 'LINEAR') {
            switch (elementSize) {
                case 1:
                    result = w.lerp_scalar(frames, values, tolerance);
                    break;
                case 2:
                    result = w.lerp_vec2(frames, values, tolerance);
                    break;
                case 3:
                    result = w.lerp_vec3(frames, values, tolerance);
                    break;
                case 4:
                    result = w.lerp_vec4(frames, values, tolerance);
                    break;
                default:
                    result = w.lerp_unknown(frames, values, elementSize, tolerance);
                    break;
            }
        } else if (interpolation === 'STEP') {
            switch (elementSize) {
                case 1:
                    result = w.step_scalar(frames, values, tolerance);
                    break;
                case 2:
                    result = w.step_vec2(frames, values, tolerance);
                    break;
                case 3:
                    result = w.step_vec3(frames, values, tolerance);
                    break;
                case 4:
                    result = w.step_vec4(frames, values, tolerance);
                    break;
                default:
                    result = w.step_unknown(frames, values, elementSize, tolerance);
                    break;
            }
        } else {
            logger.warn(`${
                NAME
            }: skipping sampler ${sampler.getName()} with unsupported interpolation ${
                interpolation
            }, path=${path}, elementSize=${elementSize}`);
        }
    }
    // const timeEscaped = performance.now() - ts;
    // const afterLength = result.frames.byteLength + result.values.byteLength;
    // const afterFrames = result.frames.length;
    // stats.afterFrames += afterFrames;
    // stats.afterLength += afterLength;
    // stats.timeEscaped += timeEscaped;
    // stats.beforeLength += beforeLength;
    // stats.beforeFrames += beforeFrames;
    // If the sampler was optimized, truncate and save the results. If not, clean up.
    if (result.frames.length !== input.getCount()) {
        input.setArray(result.frames);
        output.setArray(result.values);
        sampler.setInput(input);
        sampler.setOutput(output);
    } else if (cloned) {
        input.dispose();
        output.dispose();
    }
}
