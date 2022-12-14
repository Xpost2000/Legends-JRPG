#ifndef V2_DEF_C
#define V2_DEF_C

typedef union {
    struct { f32 x, y; };
    f32 xy[2];
} v2f32;
typedef union {
    struct { s32 x, y; };
    s32 xy[2];
} v2s32;

#define v2f32(X,Y) (v2f32){.x = X, .y = Y}
#define v2s32(X,Y) (v2s32){.x = X, .y = Y}
v2s32 v2f32_to_v2s32(v2f32 a);
v2f32 v2s32_to_v2f32(v2s32 a);

v2f32 v2f32_add(v2f32 a, v2f32 b);
v2f32 v2f32_sub(v2f32 a, v2f32 b);
v2f32 v2f32_scale(v2f32 a, f32 k);
v2f32 v2f32_floor(v2f32 a);
v2f32 v2f32_hadamard(v2f32 a, v2f32 b);
v2f32 v2f32_lerp(v2f32 a, v2f32 b, v2f32 t);
v2f32 v2f32_lerp_scalar(v2f32 a, v2f32 b, f32 t);
f32   v2f32_dot(v2f32 a, v2f32 b);
f32   v2f32_magnitude(v2f32 v);
f32   v2f32_magnitude_sq(v2f32 v);
f32   v2f32_distance(v2f32 a, v2f32 b);
f32   v2f32_distance_sq(v2f32 a, v2f32 b);
v2f32 v2f32_direction(v2f32 a, v2f32 b);
v2f32 v2f32_normalize(v2f32 a);
v2f32 v2f32_direction_from_degree(f32 x);

#endif
