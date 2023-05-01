#include <string.h>
#include "./cglm/include/cglm/cglm.h"

// #define RESAMPLE_ONLERP_QUAT

CGLM_INLINE bool is_equals_scalar(const float left, const float right, const float tolerance)
{
    return fabsf(left - right) <= tolerance;
}

CGLM_INLINE bool is_equals_vec2(const vec2 left, const vec2 right, const float tolerance)
{
    return fabsf(left[0] - right[0]) <= tolerance &&
           fabsf(left[1] - right[1]) <= tolerance;
}

#if defined(CGLM_SIMD_WASM)

CGLM_INLINE bool is_equals_f32x4(const glmm_128 left, const glmm_128 right, const float tolerance)
{
    glmm_128 a;
    a = glmm_abs(wasm_f32x4_sub(left, right));
    a = wasm_f32x4_le(a, glmm_set1(tolerance));
    return wasm_i32x4_all_true(a);
}

#else

CGLM_INLINE bool is_equals_vec3(const vec3 left, const vec3 right, const float tolerance)
{
    return fabsf(left[0] - right[0]) <= tolerance &&
           fabsf(left[1] - right[1]) <= tolerance &&
           fabsf(left[2] - right[2]) <= tolerance;
}

CGLM_INLINE bool is_equals_vec4(const vec4 left, const vec4 right, const float tolerance)
{
    return fabsf(left[0] - right[0]) <= tolerance &&
           fabsf(left[1] - right[1]) <= tolerance &&
           fabsf(left[2] - right[2]) <= tolerance &&
           fabsf(left[3] - right[3]) <= tolerance;
}

#endif

CGLM_INLINE bool keep_scalar_step(
    const float *left, const float *middle, const float *right,
    const float tolerance)
{
    float t = *middle;
    return !is_equals_scalar(*left, t, tolerance) ||
           !is_equals_scalar(t, *right, tolerance);
}

CGLM_INLINE bool keep_vec2_step(
    const vec2 left, const vec2 middle, const vec2 right,
    const float tolerance)
{
    return !is_equals_vec2(left, middle, tolerance) ||
           !is_equals_vec2(middle, right, tolerance);
}

CGLM_INLINE bool keep_vec3_step(
    vec3 left, vec3 middle, vec3 right,
    const float tolerance)
{
#if defined(CGLM_SIMD_WASM)
    glmm_128 left_v, middle_v, right_v;
    left_v = glmm_load3(left);
    middle_v = glmm_load3(middle);
    right_v = glmm_load3(right);
    // any vec not equals should be kept
    return !is_equals_f32x4(left_v, middle_v, tolerance) ||
           !is_equals_f32x4(middle_v, right_v, tolerance);
#else
    return !is_equals_vec3(left, middle, tolerance) ||
           !is_equals_vec3(middle, right, tolerance);
#endif
}

CGLM_INLINE bool keep_vec4_step(
    const vec4 left, const vec4 middle, const vec4 right,
    const float tolerance)
{
#if defined(CGLM_SIMD_WASM)
    glmm_128 left_v, middle_v, right_v;
    left_v = glmm_load(left);
    middle_v = glmm_load(middle);
    right_v = glmm_load(right);
    // any vec not equals should be kept
    return !is_equals_f32x4(left_v, middle_v, tolerance) ||
           !is_equals_f32x4(middle_v, right_v, tolerance);
#else
    return !is_equals_vec4(left, middle, tolerance) ||
           !is_equals_vec4(middle, right, tolerance);
#endif
}

CGLM_INLINE bool keep_unknown_step(
    float *left, float *middle, float *right,
    const size_t size, const float tolerance)
{
    size_t offset = 0;
    while ((size - offset) >= 4)
    {
        if (keep_vec4_step(
                left + offset, middle + offset, right + offset,
                tolerance))
        {
            return true;
        }
        offset += 4;
    }
    while (size > offset)
    {
        if (keep_scalar_step(
                left + offset, middle + offset, right + offset,
                tolerance))
        {
            return true;
        }
        offset++;
    }
    return false;
}

CGLM_INLINE bool keep_scalar_lerp(
    const float *left, const float *middle, const float *right,
    const float t, const float tolerance)
{
    float v0, v1;
    v0 = *left;
    v1 = *right;
    // from + s * (to - from)
    v0 = v0 + glm_clamp_zo(t) * (v1 - v0);
    return !is_equals_scalar(v0, *middle, tolerance);
}

CGLM_INLINE bool keep_vec2_lerp(
    vec2 left, vec2 middle, vec2 right,
    const float t, const float tolerance)
{
    vec2 lerp_result;
    glm_vec2_lerp(left, right, t, lerp_result);
    return !is_equals_vec2(lerp_result, middle, tolerance);
}

CGLM_INLINE bool keep_vec3_lerp(
    vec3 left, vec3 middle, vec3 right,
    const float t, const float tolerance)
{
#if defined(CGLM_SIMD_WASM)
    glmm_128 left_v, s;
    // the simd lerp
    left_v = glmm_load3(left);
    s = glmm_load3(right);
    s = wasm_f32x4_sub(s, left_v);
    // note that there is no glm_clamp_zo in glm_vec3_lerp
    s = wasm_f32x4_mul(glmm_set1(t), s);
    s = wasm_f32x4_add(left_v, s);
    return !is_equals_f32x4(glmm_load3(middle), s, tolerance);
#else
    vec3 lerp_result;
    glm_vec3_lerp(left, right, t, lerp_result);
    return !is_equals_vec3(lerp_result, middle, tolerance);
#endif
}

CGLM_INLINE bool keep_vec4_lerp(
    vec4 left, vec4 middle, vec4 right,
    const float t, const float tolerance)
{
#if defined(CGLM_SIMD_WASM)
    glmm_128 left_v, s;
    // the simd lerp
    left_v = glmm_load(left);
    s = glmm_load(right);
    s = wasm_f32x4_sub(s, left_v);
    // note that there is no glm_clamp_zo in glm_vec4_lerp
    s = wasm_f32x4_mul(glmm_set1(t), s);
    s = wasm_f32x4_add(left_v, s);
    return !is_equals_f32x4(glmm_load(middle), s, tolerance);
#else
    vec4 lerp_result;
    glm_vec4_lerp(left, right, t, lerp_result);
    return !is_equals_vec4(lerp_result, middle, tolerance);
#endif
}

CGLM_INLINE bool keep_unknown_lerp(
    float *left, float *middle, float *right,
    const size_t size, const float t, const float tolerance)
{
    size_t offset = 0;
    while ((size - offset) >= 4)
    {
        if (keep_vec4_lerp(
                left + offset, middle + offset, right + offset,
                t, tolerance))
        {
            return true;
        }
        offset += 4;
    }
    while (size > offset)
    {
        if (!keep_scalar_lerp(
                left + offset, middle + offset, right + offset,
                t, tolerance))
        {
            return true;
        }
        offset++;
    }
    return false;
}

/**
 * Gets the angular distance between two unit quaternions
 * https://github.com/toji/gl-matrix/blob/master/src/quat.js
 *
 * @param  {versor} a     Origin unit quaternion
 * @param  {versor} b     Destination unit quaternion
 * @return {float}     Angle, in radians, between the two quaternions
 */
CGLM_INLINE float quat_get_angle(versor a, versor b)
{
    float dotproduct = glm_quat_dot(a, b);
    return acosf(2.f * dotproduct * dotproduct - 1.f);
}

/*!
 * @brief interpolates between two quaternions
 *        using spherical linear interpolation (SLERP)
 *
 * @param[in]   from  from
 * @param[in]   to    to
 * @param[in]   t     amout
 * @param[out]  dest  result quaternion
 */
CGLM_INLINE
void
glm_quat_slerp_precise(versor from, versor to, float t, versor dest) {
    CGLM_ALIGN(16) vec4 q1, q2;
    float cosTheta, sinTheta, angle;

    cosTheta = glm_quat_dot(from, to);
    glm_quat_copy(from, q1);

    if (fabsf(cosTheta) >= 1.0f) {
        glm_quat_copy(q1, dest);
        return;
    }

    /* If cosTheta < 0, the interpolation will take the long way around the sphere. */
    /* To fix this, one quat must be negated. */
    if (cosTheta < 0.0f) {
        glm_vec4_negate(q1);
        cosTheta = -cosTheta;
    }

    /* LERP to avoid zero division */
    if (cosTheta > 0.99999f) {
        glm_quat_lerp(from, to, t, dest);
        return;
    }

    /* SLERP */
    angle = acosf(cosTheta);
    sinTheta = sinf(angle);
    glm_vec4_scale(q1, sinf((1.0f - t) * angle), q1);
    glm_vec4_scale(to, sinf(t * angle), q2);

    glm_vec4_add(q1, q2, q1);
    glm_vec4_scale(q1, 1.0f / sinTheta, dest);
}

CGLM_INLINE bool keep_quat_slerp(
    versor left, versor middle, versor right,
    const float t, const float tolerance)
{
    if (quat_get_angle(left, middle) + quat_get_angle(middle, right) + FLT_EPSILON > GLM_PIf)
    {
        return true;
    }
    versor slerp_result;
    // slerp is slow...
    glm_quat_slerp_precise(left, right, t, slerp_result);
#if defined(CGLM_SIMD_WASM)
    return !is_equals_f32x4(glmm_load(slerp_result), glmm_load(middle), tolerance);
#else
    return !is_equals_vec4(slerp_result, middle, tolerance);
#endif
}

#ifdef RESAMPLE_ONLERP_QUAT
CGLM_INLINE bool keep_quat_onlerp(
    versor left, versor middle, versor right,
    float t, float tolerance)
{
    if (quat_get_angle(left, middle) + quat_get_angle(middle, right) + FLT_EPSILON > GLM_PIf)
    {
        return true;
    }
    // Approximating slerp, https://zeux.io/2015/07/23/approximating-slerp/
    // We also handle quaternion double-cover
    float ca = glm_quat_dot(left, right);

    float d = fabsf(ca);
    float A = 1.0904f + d * (-3.2452f + d * (3.55645f - d * 1.43519f));
    float B = 0.848013f + d * (-1.06021f + d * 0.215638f);
    float k = A * (t - 0.5f) * (t - 0.5f) + B;
    float ot = t + t * (t - 0.5f) * (t - 1) * k;

    float t0 = 1 - ot;
    float t1 = ca > 0 ? ot : -ot;

#if defined(CGLM_SIMD_WASM)
    glmm_128 v1, v2, xdot;
    v1 = wasm_f32x4_mul(glmm_load(left), glmm_set1(t0));
    v1 = wasm_f32x4_add(v1, wasm_f32x4_mul(glmm_load(right), glmm_set1(t1)));
    // normalize;
    xdot = glmm_vdot(v1, v1);
    if (wasm_f32x4_extract_lane(xdot, 0) <= 0)
    {
        // GLM_QUAT_IDENTITY_INIT
        v1 = wasm_f32x4_const(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        v1 = wasm_f32x4_div(v1, wasm_f32x4_sqrt(xdot));
    }
    v2 = glmm_load(middle);
    // normalize;
    xdot = glmm_vdot(v2, v2);
    if (wasm_f32x4_extract_lane(xdot, 0) <= 0)
    {
        // GLM_QUAT_IDENTITY_INIT
        v2 = wasm_f32x4_const(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        v2 = wasm_f32x4_div(v2, wasm_f32x4_sqrt(xdot));
    }
    return !is_equals_f32x4(v1, v2, tolerance);
#else
    versor v1, v2;
    v1[0] = left[0] * t0 + right[0] * t1;
    v1[1] = left[1] * t0 + right[1] * t1;
    v1[2] = left[2] * t0 + right[2] * t1;
    v1[3] = left[3] * t0 + right[3] * t1;
    glm_quat_normalize(v1);
    glm_quat_normalize_to(middle, v2);
    return !is_equals_vec4(v1, v2, tolerance);
#endif
}
#endif

#if defined(CGLM_SIMD_WASM)
#define vec3_copy(src, dest) glmm_store3((dest), glmm_load3(src))
#else
#define vec3_copy(src, dest) glm_vec3_copy((src), (dest))
#endif

#define scalar_copy(src, dest) *(dest) = *(src)

/* value_size here should be defined in outer function */
#define unknown_copy(src, dest) memcpy(dest, src, value_size)

#if defined(__wasm__)
extern unsigned char __heap_base;

intptr_t get_heap_ptr()
{
    intptr_t base_ptr = (intptr_t)&__heap_base;
    // align it with 16 bytes
    return (base_ptr + 16) & ~15;
}
#endif

/* copy last 2 frames to the beginning of stream */
size_t stream_continue(
    float *frames, const size_t frame_stride,
    float *values, const size_t value_size, const size_t value_stride,
    const size_t count)
{
    if (count == 0)
    {
        return 0;
    }
    if (count >= 2)
    {
        size_t offset = count - 2;
        for (size_t i = 0; i < 2; i++)
        {
            frames[i * frame_stride] = frames[(offset + i) * frame_stride];
        }
        if (value_size == 4)
        {
            for (size_t i = 0; i < 2; i++)
            {
                glm_vec4_ucopy(&values[(offset + i) * value_stride], &values[i * value_stride]);
            }
        }
        else if (value_size == 3)
        {
            for (size_t i = 0; i < 2; i++)
            {
                vec3_copy(&values[(offset + i) * value_stride], &values[i * value_stride]);
            }
        }
        else if (value_size == 2)
        {
            for (size_t i = 0; i < 2; i++)
            {
                glm_vec2_copy(&values[(offset + i) * value_stride], &values[i * value_stride]);
            }
        }
        else if (value_size == 1)
        {
            for (size_t i = 0; i < 2; i++)
            {
                scalar_copy(&values[(offset + i) * value_stride], &values[i * value_stride]);
            }
        }
        else
        {
            for (size_t i = 0; i < 2; i++)
            {
                unknown_copy(&values[(offset + i) * value_stride], &values[i * value_stride]);
            }
        }
        return 2;
    }
    if (count == 1)
    {
        size_t offset = count - 1;
        frames[0] = frames[(offset)*frame_stride];
        if (value_size == 4)
        {
            glm_vec4_ucopy(&values[offset * value_stride], &values[0]);
        }
        else if (value_size == 3)
        {
            vec3_copy(&values[offset * value_stride], &values[0]);
        }
        else if (value_size == 2)
        {
            glm_vec2_copy(&values[offset * value_stride], &values[0]);
        }
        else if (value_size == 1)
        {
            scalar_copy(&values[offset * value_stride], &values[0]);
        }
        else
        {
            unknown_copy(&values[offset * value_stride], &values[0]);
        }
        return 1;
    }
    return 0;
}

#define resample_finalize(prev_attr, copy_fn)                                   \
    /* Flush last keyframe (compaction looks ahead). */                         \
    if (last_index > 0)                                                         \
    {                                                                           \
        frames[write_index * frame_stride] = frames[last_index * frame_stride]; \
        copy_fn(                                                                \
            &values[last_index * value_stride],                                 \
            &values[write_index * value_stride]);                               \
        write_index++;                                                          \
    }                                                                           \
                                                                                \
    return write_index

size_t step_unknown(
    float *frames, const size_t frame_stride,
    float *values, const size_t value_size, const size_t value_stride,
    const size_t count, const float tolerance)
{
    if (count == 0)
    {
        return 0;
    }
    if (value_size > value_stride)
    {
        return (size_t)-1;
    }
    float first_frame = frames[0];
    size_t write_index = 1;
    size_t last_index = count - 1;

    for (size_t i = 1; i < last_index; ++i)
    {
        float time = frames[i * frame_stride];
        float time_next = frames[(i + 1) * frame_stride];

        bool keep = false;
        if (time != time_next && (i != 1 || time != first_frame))
        {
            keep = keep_unknown_step(
                /* valuePrev */
                &values[(write_index - 1) * value_stride],
                /* value */
                &values[i * value_stride],
                /* valueNext */
                &values[(i + 1) * value_stride],
                value_size, tolerance);
        }

        /* In-place compaction. */
        if (keep)
        {
            if (i != write_index)
            {
                frames[write_index * frame_stride] = frames[i * frame_stride];
                memcpy(
                    &values[write_index * value_stride],
                    &values[i * value_stride], value_size);
            }
            write_index++;
        }
    }

    resample_finalize(unknown_value, unknown_copy);
}

size_t lerp_unknown(
    float *frames, const size_t frame_stride,
    float *values, const size_t value_size, const size_t value_stride,
    const size_t count, const float tolerance)
{
    if (count == 0)
    {
        return 0;
    }
    if (value_size > value_stride)
    {
        return (size_t)-1;
    }
    float first_frame = frames[0];
    size_t write_index = 1;
    size_t last_index = count - 1;

    for (size_t i = 1; i < last_index; ++i)
    {
        float time_prev = frames[(write_index - 1) * frame_stride];
        float time = frames[i * frame_stride];
        float time_next = frames[(i + 1) * frame_stride];

        bool keep = false;
        if (time != time_next && (i != 1 || time != first_frame))
        {
            float t = (time - time_prev) / (time_next - time_prev);
            keep = keep_unknown_lerp(
                /* valuePrev */
                &values[(write_index - 1) * value_stride],
                /* value */
                &values[i * value_stride],
                /* valueNext */
                &values[(i + 1) * value_stride],
                value_size, t, tolerance);
        }

        /* In-place compaction. */
        if (keep)
        {
            if (i != write_index)
            {
                frames[write_index * frame_stride] = frames[i * frame_stride];
                memcpy(
                    &values[write_index * value_stride],
                    &values[i * value_stride], value_size);
            }
            write_index++;
        }
    }

    resample_finalize(unknown_value, unknown_copy);
}

#define resample_step_stream(name, comp_fn, prev_attr, copy_fn)                    \
    size_t name(                                                                   \
        float *frames, const size_t frame_stride,                                  \
        float *values, const size_t value_stride,                                  \
        const size_t count, const float tolerance)                                 \
    {                                                                              \
        if (count == 0)                                                            \
        {                                                                          \
            return 0;                                                              \
        }                                                                          \
        float first_frame = frames[0];                                             \
        size_t write_index = 1;                                                    \
        size_t last_index = count - 1;                                             \
                                                                                   \
        for (size_t i = 1; i < last_index; ++i)                                    \
        {                                                                          \
            float time = frames[i * frame_stride];                                 \
            float time_next = frames[(i + 1) * frame_stride];                      \
                                                                                   \
            bool keep = false;                                                     \
            if (time != time_next && (i != 1 || time != first_frame))              \
            {                                                                      \
                keep = comp_fn(                                                    \
                    &values[(write_index - 1) * value_stride],                     \
                    &values[i * value_stride],                                     \
                    &values[(i + 1) * value_stride],                               \
                    tolerance);                                                    \
            }                                                                      \
                                                                                   \
            /* In-place compaction. */                                             \
            if (keep)                                                              \
            {                                                                      \
                if (i != write_index)                                              \
                {                                                                  \
                    frames[write_index * frame_stride] = frames[i * frame_stride]; \
                    copy_fn(                                                       \
                        &values[i * value_stride],                                 \
                        &values[write_index * value_stride]);                      \
                }                                                                  \
                write_index++;                                                     \
            }                                                                      \
        }                                                                          \
        resample_finalize(prev_attr, copy_fn);                                     \
    }

#define resample_lerp_stream(name, comp_fn, prev_attr, copy_fn)                    \
    size_t name(                                                                   \
        float *frames, const size_t frame_stride,                                  \
        float *values, const size_t value_stride,                                  \
        const size_t count, const float tolerance)                                 \
    {                                                                              \
        if (count == 0)                                                            \
        {                                                                          \
            return count;                                                          \
        }                                                                          \
        float first_frame = frames[0];                                             \
        size_t write_index = 1;                                                    \
        size_t last_index = count - 1;                                             \
                                                                                   \
        for (size_t i = 1; i < last_index; ++i)                                    \
        {                                                                          \
            float time_prev = frames[(write_index - 1) * frame_stride];            \
            float time = frames[i * frame_stride];                                 \
            float time_next = frames[(i + 1) * frame_stride];                      \
                                                                                   \
            bool keep = false;                                                     \
            if (time != time_next && (i != 1 || time != first_frame))              \
            {                                                                      \
                float t = (time - time_prev) / (time_next - time_prev);            \
                keep = comp_fn(                                                    \
                    &values[(write_index - 1) * value_stride],                     \
                    &values[i * value_stride],                                     \
                    &values[(i + 1) * value_stride],                               \
                    t, tolerance);                                                 \
            }                                                                      \
                                                                                   \
            /* In-place compaction. */                                             \
            if (keep)                                                              \
            {                                                                      \
                if (i != write_index)                                              \
                {                                                                  \
                    frames[write_index * frame_stride] = frames[i * frame_stride]; \
                    copy_fn(                                                       \
                        &values[i * value_stride],                                 \
                        &values[write_index * value_stride]);                      \
                }                                                                  \
                write_index++;                                                     \
            }                                                                      \
        }                                                                          \
                                                                                   \
        resample_finalize(prev_attr, copy_fn);                                     \
    }

resample_step_stream(
    step_scalar,
    keep_scalar_step,
    scalar_value,
    scalar_copy);

resample_step_stream(
    step_vec2,
    keep_vec2_step,
    vec2_value,
    glm_vec2_copy);

resample_step_stream(
    step_vec3,
    keep_vec3_step,
    vec3_value,
    vec3_copy);

resample_step_stream(
    step_vec4,
    keep_vec4_step,
    vec4_value,
    glm_vec4_copy);

resample_lerp_stream(
    lerp_scalar,
    keep_scalar_lerp,
    scalar_value,
    scalar_copy);

resample_lerp_stream(
    lerp_vec2,
    keep_vec2_lerp,
    vec2_value,
    glm_vec2_copy);

resample_lerp_stream(
    lerp_vec3,
    keep_vec3_lerp,
    vec3_value,
    vec3_copy);

resample_lerp_stream(
    lerp_vec4,
    keep_vec4_lerp,
    vec4_value,
    glm_vec4_copy);

resample_lerp_stream(
    slerp_quat,
    keep_quat_slerp,
    quat_value,
    glm_quat_copy);

#ifdef RESAMPLE_ONLERP_QUAT
resample_lerp_stream(
    onlerp_quat,
    keep_quat_onlerp,
    quat_value,
    glm_quat_copy);
#endif

#undef vec3_copy
#undef scalar_copy
#undef unknown_copy
#undef resample_finalize
#undef resample_step_stream
#undef resample_lerp_stream

#if defined(__x86_64__)
void test1()
{
    printf("test1 begin--\n");
    float input[] = {1.f, 2.f, 3.f};
    float output[] = {1.f, 2.f, 3.f};
    reset_stream();
    size_t val = resample_lerp_scalar(input, 1, output, 1, 3, FLT_EPSILON);
    intptr_t offset = get_offset_ptr();
    printf("val=%llu, offset=%lld\n", val, offset);
    float inputBuffer[6];
    float outputBuffer[6];
    memcpy(inputBuffer, input, val * sizeof(float));
    memcpy(outputBuffer, output, val * sizeof(float));
    size_t write_index = val;
    float input2[] = {4.f, 5.f, 6.f};
    float output2[] = {4.f, 5.f, 6.f};
    val = resample_lerp_scalar(input2, 1, output2, 1, 3, FLT_EPSILON);
    offset = get_offset_ptr();
    write_index += offset;
    memcpy(inputBuffer + write_index, input2, val * sizeof(float));
    memcpy(outputBuffer + write_index, output2, val * sizeof(float));
    write_index += val;
    printf("val=%llu, offset=%lld, write_index=%lld\n", val, offset, write_index);
    printf("input={");
    for (int i = 0; i < write_index; ++i)
    {
        printf("%f,", inputBuffer[i]);
    }
    printf("}\noutput={");
    for (int i = 0; i < write_index; ++i)
    {
        printf("%f,", outputBuffer[i]);
    }
    printf("}\ntest1 end--\n");
}
void test2()
{
    printf("test2 begin--\n");
    float input[] = {1.f, 2.f, 3.f};
    float output[] = {1.f, 2.f, 3.f};
    reset_stream();
    size_t val = resample_lerp_scalar(input, 1, output, 1, 3, FLT_EPSILON);
    intptr_t offset = get_offset_ptr();
    printf("val=%llu, offset=%lld\n", val, offset);
    float inputBuffer[6];
    float outputBuffer[6];
    memcpy(inputBuffer, input, val * sizeof(float));
    memcpy(outputBuffer, output, val * sizeof(float));
    size_t write_index = val;
    float input2[] = {4.f, 5.f, 6.f};
    float output2[] = {5.f, 5.f, 6.f};
    val = resample_lerp_scalar(input2, 1, output2, 1, 3, FLT_EPSILON);
    offset = get_offset_ptr();
    write_index += offset;
    memcpy(inputBuffer + write_index, input2, val * sizeof(float));
    memcpy(outputBuffer + write_index, output2, val * sizeof(float));
    write_index += val;
    printf("val=%llu, offset=%lld, write_index=%lld\n", val, offset, write_index);
    printf("input={");
    for (int i = 0; i < write_index; ++i)
    {
        printf("%f,", inputBuffer[i]);
    }
    printf("}\noutput={");
    for (int i = 0; i < write_index; ++i)
    {
        printf("%f,", outputBuffer[i]);
    }
    printf("}\ntest2 end--\n");
}

void test_single()
{
    printf("test_single begin--\n");
    float input[] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
    float output[] = {1.f, 2.f, 3.f, 5.f, 5.f, 6.f};
    float inputBuffer[6];
    float outputBuffer[6];
    size_t write_index = 0;
    reset_stream();
    for (int i = 0; i < 6; i++)
    {
        size_t val = resample_lerp_scalar(input + i, 1, output + i, 1, 1, FLT_EPSILON);
        intptr_t offset = get_offset_ptr();
        write_index += offset;
        printf("val=%llu, offset=%lld, write_index=%lld\n", val, offset, write_index);
        for (int j = 0; j < val; j++)
        {
            inputBuffer[write_index + j] = input[i + j];
            outputBuffer[write_index + j] = output[i + j];
            write_index++;
        }
    }
    printf("input={");
    for (int i = 0; i < write_index; ++i)
    {
        printf("%f,", inputBuffer[i]);
    }
    printf("}\noutput={");
    for (int i = 0; i < write_index; ++i)
    {
        printf("%f,", outputBuffer[i]);
    }
    printf("}\ntest_single end--\n");
}

void test_single3()
{
    printf("test_single3 begin--\n");
    float input[] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
    float output[] = {
        1.f, 1.f, 1.f,
        1.f, 1.f, 1.f,
        1.f, 1.f, 1.f,
        1.f, 1.f, 1.f,
        5.f, 5.f, 5.f,
        6.f, 6.f, 6.f};
    float inputBuffer[6];
    float outputBuffer[18];
    size_t write_index = 0;
    reset_stream();
    for (int i = 0; i < 6; i++)
    {
        size_t val = resample_lerp_vec3(input + i, 1, output + i, 3, 1, FLT_EPSILON);
        intptr_t offset = get_offset_ptr();
        write_index += offset;
        printf("val=%llu, offset=%lld, write_index=%lld\n", val, offset, write_index);
        for (int j = 0; j < val; j++)
        {
            inputBuffer[write_index + j] = input[i + j];
            vec3_copy(&output[i * 3 + j * 3], &outputBuffer[write_index * 3 + j * 3]);
            // outputBuffer[write_index + j] = output[i + j];
            write_index++;
        }
    }
    printf("input={");
    for (int i = 0; i < write_index; ++i)
    {
        printf("%f,", inputBuffer[i]);
    }
    printf("}\noutput={");
    for (int i = 0; i < write_index * 3; ++i)
    {
        printf("%f,", outputBuffer[i]);
    }
    printf("}\ntest_single3 end--\n");
}

void test_single3b()
{
    printf("test_single3b begin--\n");
    float input[] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
    float output[] = {
        1.f, 1.f, 1.f,
        1.f, 1.f, 1.f,
        1.f, 1.f, 1.f,
        1.f, 1.f, 1.f,
        5.f, 5.f, 5.f,
        6.f, 6.f, 6.f};
    float inputBuffer[6];
    float outputBuffer[18];
    size_t write_index = 0;
    reset_stream();
    int bs = 3;
    for (int b = 0; b < 6 / bs; b++)
    {
        size_t val = resample_lerp_vec3(input + b * bs, 1, output + (b * bs) * 3, 3, bs, FLT_EPSILON);
        intptr_t offset = get_offset_ptr();
        write_index += offset;
        printf("val=%llu, offset=%lld, write_index=%lld\n", val, offset, write_index);
        for (int j = 0; j < val; j++)
        {
            inputBuffer[write_index] = input[b * bs + j];
            vec3_copy(&output[(b * bs) * 3], &outputBuffer[write_index * 3]);
            // outputBuffer[write_index + j] = output[i + j];
            write_index++;
        }
    }
    printf("input={");
    for (int i = 0; i < write_index; ++i)
    {
        printf("%f,", inputBuffer[i]);
    }
    printf("}\noutput={");
    for (int i = 0; i < write_index * 3; ++i)
    {
        printf("%f,", outputBuffer[i]);
    }
    printf("}\ntest_single3b end--\n");
}

int main(void)
{
    test1();
    test2();
    test_single();
    test_single3();
    test_single3b();
}
#endif
