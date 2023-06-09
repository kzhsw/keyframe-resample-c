#include "./cglm/include/cglm/cglm.h"

#define NORMALIZE_I8_SCALAR 127.f
#define NORMALIZE_U8_SCALAR 255.f
#define NORMALIZE_I16_SCALAR 32767.f
#define NORMALIZE_U16_SCALAR 65535.f

#define DENORMALIZE_I8_SCALAR (1.f / NORMALIZE_I8_SCALAR)
#define DENORMALIZE_U8_SCALAR (1.f / NORMALIZE_U8_SCALAR)
#define DENORMALIZE_I16_SCALAR (1.f / NORMALIZE_I16_SCALAR)
#define DENORMALIZE_U16_SCALAR (1.f / NORMALIZE_U16_SCALAR)

//#define NORMALIZE_SIMD_UNROLL

CGLM_INLINE void normalize_scalar(
        float * ptr,
        const size_t size,
        const size_t stride,
        const size_t count,
        const float scalar
) {
    for (size_t i = 0; i < count; ++i) {
        float * curr_ptr = ptr;
        for (size_t j = 0; j < size; ++j) {
            *curr_ptr = roundf((*curr_ptr) * scalar);
            curr_ptr++;
        }
        ptr += stride;
    }
}

CGLM_INLINE void denormalize_scalar_i(
        float * ptr,
        const size_t size,
        const size_t stride,
        const size_t count,
        const float scalar
) {
    for (size_t i = 0; i < count; ++i) {
        float * curr_ptr = ptr;
        for (size_t j = 0; j < size; ++j) {
            *curr_ptr = fmaxf((*curr_ptr) * (scalar), -1.f);
            curr_ptr++;
        }
        ptr += stride;
    }
}

CGLM_INLINE void denormalize_scalar_u(
        float * ptr,
        const size_t size,
        const size_t stride,
        const size_t count,
        const float scalar
) {
    for (size_t i = 0; i < count; ++i) {
        float * curr_ptr = ptr;
        for (size_t j = 0; j < size; ++j) {
            *curr_ptr = fmaxf((*curr_ptr) * (scalar), -1.f);
            curr_ptr++;
        }
        ptr += stride;
    }
}

#if defined(CGLM_SIMD_WASM)
static inline void normalize_internal(float *ptr, const size_t length, const float scalar)
{
#ifdef NORMALIZE_SIMD_UNROLL
    glmm_128 v1, v2, v3, v4, v5;
#else
    glmm_128 v1, v2;
#endif
    size_t num;
    num = length;
    v1 = wasm_f32x4_splat(scalar);
#ifdef NORMALIZE_SIMD_UNROLL
    // loop unrolling
    while (num > 15)
    {
        v2 = glmm_load(ptr);
        v3 = glmm_load(ptr + 4);
        v4 = glmm_load(ptr + 8);
        v5 = glmm_load(ptr + 12);
        v2 = wasm_f32x4_mul(v2, v1);
        v3 = wasm_f32x4_mul(v3, v1);
        v4 = wasm_f32x4_mul(v4, v1);
        v5 = wasm_f32x4_mul(v5, v1);
        v2 = wasm_f32x4_nearest(v2);
        v3 = wasm_f32x4_nearest(v3);
        v4 = wasm_f32x4_nearest(v4);
        v5 = wasm_f32x4_nearest(v5);
        glmm_store(ptr, v2);
        glmm_store(ptr + 4, v3);
        glmm_store(ptr + 8, v4);
        glmm_store(ptr + 12, v5);
        ptr += 16;
        num -= 16;
    }
#endif
    while (num > 3)
    {
        v2 = glmm_load(ptr);
        v2 = wasm_f32x4_mul(v2, v1);
        v2 = wasm_f32x4_nearest(v2);
        glmm_store(ptr, v2);
        ptr += 4;
        num -= 4;
    }

    while (num > 0)
    {
        *ptr = roundf((*ptr) * scalar);
        ptr++;
        num--;
    }
}

CGLM_INLINE void normalize_vec4(
        float * ptr,
        const size_t stride,
        const size_t count,
        const float scalar
) {
    glmm_128 v1, v2;
    v1 = wasm_f32x4_splat(scalar);
    for (size_t i = 0; i < count; ++i) {
        v2 = glmm_load(ptr);
        v2 = wasm_f32x4_mul(v2, v1);
        v2 = wasm_f32x4_nearest(v2);
        glmm_store(ptr, v2);
        ptr += stride;
    }
}

CGLM_INLINE void normalize_vec3(
        float * ptr,
        const size_t stride,
        const size_t count,
        const float scalar
) {
    glmm_128 v1, v2;
    v1 = wasm_f32x4_splat(scalar);
    for (size_t i = 0; i < count; ++i) {
        v2 = glmm_load3(ptr);
        v2 = wasm_f32x4_mul(v2, v1);
        v2 = wasm_f32x4_nearest(v2);
        glmm_store3(ptr, v2);
        ptr += stride;
    }
}

#define denormalize_max_i(v, min_val) (v) = wasm_f32x4_pmax((v), (min_val))
#define denormalize_max_u(v, min_val)
#define denormalize_scalar_i(scalar, min_val) *ptr = fmaxf((*ptr) * (scalar), min_val)
#define denormalize_scalar_u(scalar, min_val) *ptr = (*ptr) * (scalar)

#ifdef NORMALIZE_SIMD_UNROLL

#define denormalize_fn(name, max_simd_fn, scalar_fn)                             \
    static inline void name(float *ptr, const size_t length, const float scalar) \
    {                                                                            \
        glmm_128 v1, v2, v3, v4, v5, v6;                                         \
        size_t num;                                                              \
        num = length;                                                            \
        v1 = wasm_f32x4_splat(scalar);                                           \
        v2 = wasm_f32x4_const_splat(-1.f);                                       \
        /* loop unrolling */                                                     \
        while (num > 15)                                                         \
        {                                                                        \
            v3 = glmm_load(ptr);                                                 \
            v4 = glmm_load(ptr + 4);                                             \
            v5 = glmm_load(ptr + 8);                                             \
            v6 = glmm_load(ptr + 12);                                            \
            v3 = wasm_f32x4_mul(v3, v1);                                         \
            v4 = wasm_f32x4_mul(v4, v1);                                         \
            v5 = wasm_f32x4_mul(v5, v1);                                         \
            v6 = wasm_f32x4_mul(v6, v1);                                         \
            max_simd_fn(v3, v2);                                                 \
            max_simd_fn(v4, v2);                                                 \
            max_simd_fn(v5, v2);                                                 \
            max_simd_fn(v6, v2);                                                 \
            glmm_store(ptr, v3);                                                 \
            glmm_store(ptr + 4, v4);                                             \
            glmm_store(ptr + 8, v5);                                             \
            glmm_store(ptr + 12, v6);                                            \
            ptr += 16;                                                           \
            num -= 16;                                                           \
        }                                                                        \
                                                                                 \
        while (num > 3)                                                          \
        {                                                                        \
            v3 = glmm_load(ptr);                                                 \
            v3 = wasm_f32x4_mul(v3, v1);                                         \
            max_simd_fn(v3, v2);                                                 \
            glmm_store(ptr, v3);                                                 \
            ptr += 4;                                                            \
            num -= 4;                                                            \
        }                                                                        \
                                                                                 \
        while (num > 0)                                                          \
        {                                                                        \
            scalar_fn(DENORMALIZE_I8_SCALAR, -1.f);                              \
            ptr++;                                                               \
            num--;                                                               \
        }                                                                        \
    }

#else

#define denormalize_fn(name, max_simd_fn, scalar_fn)                             \
    static inline void name(float *ptr, const size_t length, const float scalar) \
    {                                                                            \
        glmm_128 v1, v2, v3;                                                     \
        size_t num;                                                              \
        num = length;                                                            \
        v1 = wasm_f32x4_splat(scalar);                                           \
        v2 = wasm_f32x4_const_splat(-1.f);                                       \
                                                                                 \
        while (num > 3)                                                          \
        {                                                                        \
            v3 = glmm_load(ptr);                                                 \
            v3 = wasm_f32x4_mul(v3, v1);                                         \
            max_simd_fn(v3, v2);                                                 \
            glmm_store(ptr, v3);                                                 \
            ptr += 4;                                                            \
            num -= 4;                                                            \
        }                                                                        \
                                                                                 \
        while (num > 0)                                                          \
        {                                                                        \
            scalar_fn(DENORMALIZE_I8_SCALAR, -1.f);                              \
            ptr++;                                                               \
            num--;                                                               \
        }                                                                        \
    }

#endif

denormalize_fn(denormalize_i, denormalize_max_i, denormalize_scalar_i);
denormalize_fn(denormalize_u, denormalize_max_u, denormalize_scalar_u);

#undef denormalize_fn
#undef denormalize_max_i
#undef denormalize_max_u
#undef denormalize_scalar_i
#undef denormalize_scalar_u

CGLM_INLINE void denormalize_vec4_i(
        float * ptr,
        const size_t stride,
        const size_t count,
        const float scalar
) {
    glmm_128 v1, v2, v3;
    v1 = wasm_f32x4_splat(scalar);
    v2 = wasm_f32x4_const_splat(-1.f);
    for (size_t i = 0; i < count; ++i) {
        v3 = glmm_load(ptr);
        v3 = wasm_f32x4_mul(v3, v1);
        v3 = wasm_f32x4_max(v2, v3);
        glmm_store(ptr, v3);
        ptr += stride;
    }
}

CGLM_INLINE void denormalize_vec4_u(
        float * ptr,
        const size_t stride,
        const size_t count,
        const float scalar
) {
    glmm_128 v1, v2;
    v1 = wasm_f32x4_splat(scalar);
    for (size_t i = 0; i < count; ++i) {
        v2 = glmm_load(ptr);
        v2 = wasm_f32x4_mul(v2, v1);
        glmm_store(ptr, v2);
        ptr += stride;
    }
}


CGLM_INLINE void denormalize_vec3_i(
        float * ptr,
        const size_t stride,
        const size_t count,
        const float scalar
) {
    glmm_128 v1, v2, v3;
    v1 = wasm_f32x4_splat(scalar);
    v2 = wasm_f32x4_const_splat(-1.f);
    for (size_t i = 0; i < count; ++i) {
        v3 = glmm_load3(ptr);
        v3 = wasm_f32x4_mul(v3, v1);
        v3 = wasm_f32x4_max(v2, v3);
        glmm_store3(ptr, v3);
        ptr += stride;
    }
}

CGLM_INLINE void denormalize_vec3_u(
        float * ptr,
        const size_t stride,
        const size_t count,
        const float scalar
) {
    glmm_128 v1, v2;
    v1 = wasm_f32x4_splat(scalar);
    for (size_t i = 0; i < count; ++i) {
        v2 = glmm_load3(ptr);
        v2 = wasm_f32x4_mul(v2, v1);
        glmm_store3(ptr, v2);
        ptr += stride;
    }
}

#else

static inline void normalize_internal(float *ptr, const size_t length, const float scalar)
{
    float v1, v2, v3, v4;
    size_t num;
    num = length;
    // loop unrolling
    while (num > 3)
    {
        v1 = *(ptr);
        v2 = *(ptr + 1);
        v3 = *(ptr + 2);
        v4 = *(ptr + 3);
        v1 = v1 * scalar;
        v2 = v2 * scalar;
        v3 = v3 * scalar;
        v4 = v4 * scalar;
        v1 = roundf(v1);
        v2 = roundf(v2);
        v3 = roundf(v3);
        v4 = roundf(v4);
        *(ptr) = v1;
        *(ptr + 1) = v2;
        *(ptr + 2) = v3;
        *(ptr + 3) = v4;
        ptr += 4;
        num -= 4;
    }

    while (num > 0)
    {
        *ptr = roundf((*ptr) * scalar);
        ptr++;
        num--;
    }
}

static inline void denormalize_u(float *ptr, const size_t length, const float scalar)
{
    float v1, v2, v3, v4;
    size_t num;
    num = length;
    // loop unrolling
    while (num > 3)
    {
        v1 = *(ptr);
        v2 = *(ptr + 1);
        v3 = *(ptr + 2);
        v4 = *(ptr + 3);
        v1 = v1 * scalar;
        v2 = v2 * scalar;
        v3 = v3 * scalar;
        v4 = v4 * scalar;
        *(ptr) = v1;
        *(ptr + 1) = v2;
        *(ptr + 2) = v3;
        *(ptr + 3) = v4;
        ptr += 4;
        num -= 4;
    }

    while (num > 0)
    {
        *ptr = *ptr * scalar;
        ptr++;
        num--;
    }
}

static inline void denormalize_i(float *ptr, const size_t length, const float scalar)
{
    float v1, v2, v3, v4;
    size_t num;
    num = length;
    // loop unrolling
    while (num > 3)
    {
        v1 = *(ptr);
        v2 = *(ptr + 1);
        v3 = *(ptr + 2);
        v4 = *(ptr + 3);
        v1 = v1 * scalar;
        v2 = v2 * scalar;
        v3 = v3 * scalar;
        v4 = v4 * scalar;
        v1 = fmaxf(v1, -1.f);
        v2 = fmaxf(v2, -1.f);
        v3 = fmaxf(v3, -1.f);
        v4 = fmaxf(v4, -1.f);
        *(ptr) = v1;
        *(ptr + 1) = v2;
        *(ptr + 2) = v3;
        *(ptr + 3) = v4;
        ptr += 4;
        num -= 4;
    }

    while (num > 0)
    {
        *ptr = fmaxf(*ptr * scalar, -1.f);
        ptr++;
        num--;
    }
}

#endif

typedef enum component_type
{
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
} component_type_t;

size_t normalize(
        float *ptr,
        const size_t size,
        const size_t stride,
        const size_t count,
        const component_type_t component_type
)
{
    float scalar = 0.f;
    switch (component_type)
    {
    case BYTE:
        scalar = NORMALIZE_I8_SCALAR;
        break;
    case UNSIGNED_BYTE:
        scalar = NORMALIZE_U8_SCALAR;
        break;
    case SHORT:
        scalar = NORMALIZE_I16_SCALAR;
        break;
    case UNSIGNED_SHORT:
        scalar = NORMALIZE_U16_SCALAR;
        break;
    }
    if (scalar > 0.f)
    {
        if (size == stride) {
            size_t length = size * count;
            normalize_internal(ptr, length, scalar);
        }
#if defined(CGLM_SIMD_WASM)
        if (size == 4) {
            normalize_vec4(ptr, stride, count, scalar);
        } else if (size == 3) {
            normalize_vec3(ptr, stride, count, scalar);
        } else {
#endif
            normalize_scalar(ptr, size, stride, count, scalar);
#if defined(CGLM_SIMD_WASM)
        }
#endif
        return size * count;
    }
    // unknown type
    return 0;
}

size_t denormalize(
        float *ptr,
        const size_t size,
        const size_t stride,
        const size_t count,
        const component_type_t component_type
)
{
    size_t length = size * count;
    if (size == stride) {
        switch (component_type) {
            case BYTE:
                denormalize_i(ptr, length, DENORMALIZE_I8_SCALAR);
                return length;
            case UNSIGNED_BYTE:
                denormalize_u(ptr, length, DENORMALIZE_U8_SCALAR);
                return length;
            case SHORT:
                denormalize_i(ptr, length, DENORMALIZE_I16_SCALAR);
                return length;
            case UNSIGNED_SHORT:
                denormalize_u(ptr, length, DENORMALIZE_U16_SCALAR);
                return length;
        }
        // unknown type
        return 0;
    }

#if defined(CGLM_SIMD_WASM)
    if (size == 4) {
        switch (component_type) {
            case BYTE:
                denormalize_vec4_i(ptr, stride, count, DENORMALIZE_I8_SCALAR);
                return length;
            case UNSIGNED_BYTE:
                denormalize_vec4_u(ptr, stride, count, DENORMALIZE_U8_SCALAR);
                return length;
            case SHORT:
                denormalize_vec4_i(ptr, stride, count, DENORMALIZE_I16_SCALAR);
                return length;
            case UNSIGNED_SHORT:
                denormalize_vec4_u(ptr, stride, count, DENORMALIZE_U16_SCALAR);
                return length;
        }
        // unknown type
        return 0;
    } else if (size == 3) {
        switch (component_type) {
            case BYTE:
                denormalize_vec3_i(ptr, stride, count, DENORMALIZE_I8_SCALAR);
                return length;
            case UNSIGNED_BYTE:
                denormalize_vec3_u(ptr, stride, count, DENORMALIZE_U8_SCALAR);
                return length;
            case SHORT:
                denormalize_vec3_i(ptr, stride, count, DENORMALIZE_I16_SCALAR);
                return length;
            case UNSIGNED_SHORT:
                denormalize_vec3_u(ptr, stride, count, DENORMALIZE_U16_SCALAR);
                return length;
        }
        return 0;
    } else {
#endif
        switch (component_type) {
            case BYTE:
                denormalize_scalar_i(ptr, size, stride, count, DENORMALIZE_I8_SCALAR);
                return length;
            case UNSIGNED_BYTE:
                denormalize_scalar_u(ptr, size, stride, count, DENORMALIZE_U8_SCALAR);
                return length;
            case SHORT:
                denormalize_scalar_i(ptr, size, stride, count, DENORMALIZE_I16_SCALAR);
                return length;
            case UNSIGNED_SHORT:
                denormalize_scalar_u(ptr, size, stride, count, DENORMALIZE_U16_SCALAR);
                return length;
        }
        return 0;
#if defined(CGLM_SIMD_WASM)
    }
#endif
    // unknown type
    return 0;
}
