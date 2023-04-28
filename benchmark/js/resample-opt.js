import {
    MathUtils,
    PropertyType,
    Root,
} from 'https://cdn.skypack.dev/@gltf-transform/core';
import { getAngle, slerp } from 'https://cdn.skypack.dev/gl-matrix/quat';
import { dedup, createTransform, isTransformPending } from 'https://cdn.skypack.dev/@gltf-transform/functions';

const NAME = 'resample';

const RESAMPLE_DEFAULTS = {
    tolerance: 1e-4,
    stats: {
        beforeLength: 0,
        beforeFrames: 0,
        timeEscaped: 0,
        afterLength: 0,
        afterFrames: 0,
    }
};

/**
 * Resample {@link Animation}s, losslessly deduplicating keyframes to reduce file size. Duplicate
 * keyframes are commonly present in animation 'baked' by the authoring software to apply IK
 * constraints or other software-specific features. Based on THREE.KeyframeTrack.optimize().
 *
 * Example: (0,0,0,0,1,1,1,0,0,0,0,0,0,0) --> (0,0,1,1,0,0)
 */
export const resample = (_options = RESAMPLE_DEFAULTS) => {
    const options = { ...RESAMPLE_DEFAULTS, ..._options } ;

    return createTransform(NAME, async (document, context) => {
        const accessorsVisited = new Set();
        const srcAccessorCount = document.getRoot().listAccessors().length;
        const logger = document.getLogger();

        let didSkipMorphTargets = false;

        for (const animation of document.getRoot().listAnimations()) {
            // Skip morph targets, see https://github.com/donmccurdy/glTF-Transform/issues/290.
            const samplerTargetPaths = new Map();
            for (const channel of animation.listChannels()) {
                samplerTargetPaths.set(channel.getSampler(), channel.getTargetPath());
            }

            for (const sampler of animation.listSamplers()) {
                if (samplerTargetPaths.get(sampler) === 'weights') {
                    didSkipMorphTargets = true;
                    continue;
                }
                if (sampler.getInterpolation() === 'STEP' || sampler.getInterpolation() === 'LINEAR') {
                    accessorsVisited.add(sampler.getInput());
                    accessorsVisited.add(sampler.getOutput());
                    optimize(sampler, samplerTargetPaths.get(sampler), options);
                }
            }
        }

        for (const accessor of Array.from(accessorsVisited.values())) {
            const used = accessor.listParents().some((p) => (p instanceof Root));
            if (used) accessor.dispose();
        }

        // Resampling may result in duplicate input or output sampler
        // accessors. Find and remove the duplicates after processing.
        const dstAccessorCount = document.getRoot().listAccessors().length;
        if (dstAccessorCount > srcAccessorCount && isTransformPending(context, NAME, 'dedup')) {
            await document.transform(dedup({ propertyTypes: [PropertyType.ACCESSOR] }));
        }

        if (didSkipMorphTargets) {
            logger.warn(`${NAME}: Skipped optimizing morph target keyframes, not yet supported.`);
        }

        logger.debug(`${NAME}: Complete.`);
    });
};

function getElement(elementSize, array, index, target) {
    for (let i = 0; i < elementSize; i++) {
        target[i] = array[index * elementSize + i];
    }
    return target;
}
function setElement(elementSize, array, index, value) {
    for (let i = 0; i < elementSize; i++) {
        array[index * elementSize + i] = value[i];
    }
}

function optimize(sampler, path, options) {
    const input = sampler.getInput().clone().setSparse(false);
    const output = sampler.getOutput().clone().setSparse(false);

    const tolerance = options.tolerance;
    const interpolation = sampler.getInterpolation();

    const lastIndex = input.getCount() - 1;
    const tmp = [];
    const value = [];
    const valueNext = [];
    const valuePrev = [];
    const inputArray = input.getArray();
    const inputComponentSize = input.getComponentSize();
    const outputArray = output.getArray();
    const outputComponentSize = output.getComponentSize();
    const outputElementSize = output.getElementSize();

    let writeIndex = 1;
    const beforeLength = input.getByteLength() + output.getByteLength();
    const beforeFrames = input.getCount();
    const ts = performance.now();

    for (let i = 1; i < lastIndex; ++i) {
        const timePrev = inputArray[writeIndex - 1];
        const time = inputArray[i];
        const timeNext = inputArray[i + 1];
        const t = (time - timePrev) / (timeNext - timePrev);

        let keep = false;

        // Remove unnecessary adjacent keyframes.
        if (time !== timeNext && (i !== 1 || time !== inputArray[0])) {
            getElement(outputElementSize, outputArray, writeIndex - 1, valuePrev);
            getElement(outputElementSize, outputArray, i, value);
            getElement(outputElementSize, outputArray, i + 1, valueNext);

            if (interpolation === 'LINEAR' && path === 'rotation') {
                // Prune keyframes colinear with prev/next keyframes.
                const sample = slerp(tmp, valuePrev, valueNext, t) ;
                const angle = getAngle(valuePrev, value) + getAngle(value, valueNext);
                keep = MathUtils.eq(value, sample, tolerance) || angle + Number.EPSILON >= Math.PI;
            } else if (interpolation === 'LINEAR') {
                // Prune keyframes colinear with prev/next keyframes.
                const sample = vlerp(tmp, valuePrev, valueNext, t);
                keep = MathUtils.eq(value, sample, tolerance);
            } else if (interpolation === 'STEP') {
                // Prune keyframes identical to prev/next keyframes.
                keep = MathUtils.eq(value, valuePrev) || MathUtils.eq(value, valueNext);
            }
        }

        // In-place compaction.
        if (keep) {
            if (i !== writeIndex) {
                inputArray[writeIndex] = inputArray[i];
                setElement(outputElementSize, outputArray, writeIndex,
                    getElement(outputElementSize, outputArray, i, tmp));
            }
            writeIndex++;
        }
    }

    // Flush last keyframe (compaction looks ahead).
    if (lastIndex > 0) {
        inputArray[writeIndex] = inputArray[lastIndex];
        setElement(outputElementSize, outputArray, writeIndex,
            getElement(outputElementSize, outputArray, lastIndex, tmp));
        writeIndex++;
    }

    const timeEscaped = performance.now() - ts;
    const afterLength = writeIndex * inputComponentSize +
        writeIndex * outputElementSize * outputComponentSize;
    const afterFrames = writeIndex;
    const stats = options.stats;
    stats.afterFrames += afterFrames;
    stats.afterLength += afterLength;
    stats.timeEscaped += timeEscaped;
    stats.beforeLength += beforeLength;
    stats.beforeFrames += beforeFrames;
    // If the sampler was optimized, truncate and save the results. If not, clean up.
    if (writeIndex !== input.getCount()) {
        input.setArray(inputArray.slice(0, writeIndex));
        output.setArray(outputArray.slice(0, writeIndex * outputElementSize));
        sampler.setInput(input);
        sampler.setOutput(output);
    } else {
        input.dispose();
        output.dispose();
    }
}

function lerp(v0, v1, t) {
    return v0 * (1 - t) + v1 * t;
}

function vlerp(out, a, b, t) {
    for (let i = 0; i < a.length; i++) out[i] = lerp(a[i], b[i], t);
    return out;
}
