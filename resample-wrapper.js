/**
 * @param {import('./resample.d.ts').AnimationResampleInstance} instance
 * @return {import('./resample.d.ts').AnimationResampleWrapper}
 */
export function makeWrapper(instance) {
    // heapPtr is aligned with 16 bytes
    const heapPtr = instance.exports.get_heap_ptr();
    // Math.floor((memory.buffer.byteLength - heapPtr) / 4);
    const availableSize = (instance.exports.memory.buffer.byteLength - heapPtr) >> 2;
    const memory = new Float32Array(
            instance.exports.memory.buffer,
            heapPtr, availableSize
    );
    const epsilon = 1.1920928955078125e-07;

    /**
     * @param {number} offset
     * @return {number}
     */
    function wasmPtr(offset) {
        return offset * 4 + heapPtr;
    }

    /**
     *
     * @param {Float32Array} frames
     * @param {Float32Array} values
     * @param {number} tolerance
     * @param {number} elementSize
     * @param {import('./resample').ResampleFn} callWasm
     */
    function resampleInternal(
            frames, values,
            tolerance, elementSize,
            callWasm
    ) {
        const chunkSize = (memory.length / (elementSize + 1)) | 0;
        const valueChunk = elementSize * chunkSize;
        const wasmFrameOffset = 0, wasmValueOffset = chunkSize;
        const wasmFramePtr = wasmPtr(wasmFrameOffset),
                wasmValuePtr = wasmPtr(wasmValueOffset);
        let frameReadOffset = 0, frameWriteOffset = 0,
                valueReadOffset = 0, valueWriteOffset = 0;
        let lastWriteCount = 0;
        let isFirstChunk = true;
        while (frameReadOffset < frames.length) {
            let currChunkSize = chunkSize;
            let currValueChunkSize = valueChunk;
            let offset = 0;
            let currWasmFrameOffset = wasmFrameOffset;
            let currWasmValueOffset = wasmValueOffset;
            if (isFirstChunk) {
                isFirstChunk = false;
            } else {
                offset = instance.exports.stream_continue(
                        wasmFramePtr, 1,
                        wasmValuePtr, elementSize, elementSize,
                        lastWriteCount
                );
                currChunkSize -= offset;
                currValueChunkSize -= offset * elementSize;
                currWasmFrameOffset += offset;
                currWasmValueOffset += offset * elementSize;
                frameWriteOffset -= offset;
                valueWriteOffset -= offset * elementSize;
            }
            if (frameReadOffset + currChunkSize < frames.length) {
                memory.set(
                        frames.subarray(
                                frameReadOffset,
                                frameReadOffset + currChunkSize),
                        currWasmFrameOffset);
                memory.set(
                        values.subarray(
                                valueReadOffset,
                                valueReadOffset + currValueChunkSize),
                        currWasmValueOffset);
            } else {
                // last chunk
                const lastChunkSize = frames.length - frameReadOffset;
                memory.set(
                        frames.subarray(frameReadOffset, frameReadOffset + lastChunkSize),
                        currWasmFrameOffset);
                memory.set(
                        values.subarray(
                                valueReadOffset,
                                valueReadOffset + (lastChunkSize * elementSize)),
                        currWasmValueOffset);
                currChunkSize = lastChunkSize;
            }
            let writeCount = callWasm(
                    wasmFramePtr, 1,
                    wasmValuePtr, elementSize,
                    currChunkSize + offset, tolerance
            );
            if (frameWriteOffset !== frameReadOffset || writeCount !== currChunkSize) {
                // copy only if needed
                frames.set(
                        memory.subarray(
                                wasmFrameOffset,
                                wasmFrameOffset + writeCount),
                        frameWriteOffset);
            }
            if (valueWriteOffset !== valueReadOffset || writeCount !== currChunkSize) {
                // copy only if needed
                values.set(
                        memory.subarray(
                                wasmValueOffset,
                                wasmValueOffset + (writeCount * elementSize)),
                        valueWriteOffset);
            }
            frameReadOffset += currChunkSize;
            valueReadOffset += currValueChunkSize;
            frameWriteOffset += writeCount;
            valueWriteOffset += writeCount * elementSize;
            lastWriteCount = writeCount;
        }
        return {
            frames: frames.subarray(0, frameWriteOffset),
            values: values.subarray(0, valueWriteOffset),
        };
    }

    function resampleFunction(wasmFn, elementSize) {
        /**
         * @param {Float32Array} frames
         * @param {Float32Array} values
         * @param {number} tolerance
         * @return {{frames: Float32Array, values: Float32Array}}
         */
        function resample(frames, values, tolerance) {
            if (!tolerance) tolerance = epsilon;
            return resampleInternal(frames, values, tolerance, elementSize, (
                    frames, frame_stride,
                    values, value_stride,
                    count, tolerance
            ) => instance.exports[wasmFn](
                    frames, frame_stride,
                    values, value_stride,
                    count, tolerance
            ));
        }
        return resample;
    }

    function resampleUnknown(wasmFn) {
        /**
         * @param {Float32Array} frames
         * @param {Float32Array} values
         * @param {number} elementSize
         * @param {number} tolerance
         * @return {{frames: Float32Array, values: Float32Array}}
         */
        function resample(
                frames, values,
                elementSize, tolerance
        ) {
            if (!tolerance) tolerance = epsilon;
            return resampleInternal(frames, values, tolerance, elementSize, (
                    frames, frame_stride,
                    values, value_stride,
                    count, tolerance
            ) => instance.exports[wasmFn](
                    frames, frame_stride,
                    values, value_stride, value_stride,
                    count, tolerance
            ));
        }
        return resample;
    }
    return {
        instance: instance,
        lerp_unknown: resampleUnknown('lerp_unknown'),
        slerp_quat: resampleFunction('slerp_quat', 4),
        lerp_vec4: resampleFunction('lerp_vec4', 4),
        lerp_vec3: resampleFunction('lerp_vec3', 3),
        lerp_vec2: resampleFunction('lerp_vec2', 2),
        lerp_scalar: resampleFunction('lerp_scalar', 1),
        step_unknown: resampleUnknown('step_unknown'),
        step_vec4: resampleFunction('step_vec4', 4),
        step_vec3: resampleFunction('step_vec3', 3),
        step_vec2: resampleFunction('step_vec2', 2),
        step_scalar: resampleFunction('step_scalar', 1),
    };
}
