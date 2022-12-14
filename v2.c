#include "v2_def.c"

v2f32 v2f32_add(v2f32 a, v2f32 b) {
    return v2f32(a.x + b.x, a.y + b.y); 
}

v2f32 v2f32_sub(v2f32 a, v2f32 b) {
    return v2f32(a.x - b.x, a.y - b.y); 
}

v2f32 v2f32_hadamard(v2f32 a, v2f32 b) {
    return v2f32(a.x * b.x, a.y * b.y); 
}

v2f32 v2f32_lerp(v2f32 a, v2f32 b, v2f32 t) {
    return v2f32(lerp_f32(a.x, b.x, t.x), lerp_f32(a.y, b.y, t.y)); 
}
v2f32 v2f32_lerp_scalar(v2f32 a, v2f32 b, f32 t) {
    return v2f32_lerp(a, b, v2f32(t, t));
}

v2f32 v2f32_scale(v2f32 a, f32 k) {
    return v2f32(a.x * k, a.y * k); 
}

f32 v2f32_dot(v2f32 a, v2f32 b) {
    return (a.x * b.x) + (a.y * b.y);
}

f32 v2f32_magnitude_sq(v2f32 v) {
    return v2f32_dot(v,v);
}

f32 v2f32_distance_sq(v2f32 a, v2f32 b) {
    v2f32 difference  = v2f32_sub(b, a);
    f32   distance_sq = difference.x * difference.x + difference.y * difference.y;

    return distance_sq;
}

f32 v2f32_distance(v2f32 a, v2f32 b) {
    return sqrtf(v2f32_distance_sq(a,b));
}

f32 v2f32_magnitude(v2f32 v) {
    return sqrtf(v2f32_magnitude_sq(v));
}

v2f32 v2f32_floor(v2f32 a) {
    return v2f32(floorf(a.x), floorf(a.y));
}

v2f32 v2f32_normalize(v2f32 a) {
    f32 magnitude = v2f32_magnitude(a);

    return v2f32(a.x / magnitude, a.y / magnitude);
}

v2f32 v2f32_direction(v2f32 a, v2f32 b) {
    v2f32 delta = v2f32_sub(b, a);
    return v2f32_normalize(delta);
}

v2s32 v2f32_to_v2s32(v2f32 a) {
    return v2s32(a.x, a.y);
}
v2f32 v2s32_to_v2f32(v2s32 a) {
    return v2f32(a.x, a.y);
}

v2f32 v2f32_direction_from_degree(f32 x) {
    return v2f32(cosf(degree_to_radians(x)), sinf(degree_to_radians(x)));
}

v2f32 v2f32_perpendicular(v2f32 a) {
    return v2f32(-a.y, a.x); 
}
