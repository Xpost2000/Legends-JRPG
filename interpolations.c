s32 lerp_s32(s32 a, s32 b, f32 normalized_t) {
    return a * (1 - normalized_t) + (b * normalized_t);
}

f32 lerp_f32(f32 a, f32 b, f32 normalized_t) {
    return a * (1 - normalized_t) + (b * normalized_t);
}

f32 cubic_ease_in_f32(f32 a, f32 b, f32 normalized_t) {
    return (b - a) * (normalized_t * normalized_t * normalized_t) + a;
}

f32 cubic_ease_out_f32(f32 a, f32 b, f32 normalized_t) {
    normalized_t -= 1;
    return (b - a) * (normalized_t * normalized_t * normalized_t) + a;
}

f32 quadratic_ease_in_f32(f32 a, f32 b, f32 normalized_t) {
    return (b - a) * (normalized_t * normalized_t) + a;
}

f32 quadratic_ease_out_f32(f32 a, f32 b, f32 normalized_t) {
    return -(b - a) * ((normalized_t * normalized_t) - 2.0) + a;
}
