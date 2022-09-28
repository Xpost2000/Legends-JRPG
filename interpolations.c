/* mostly from https://easings.net */
/*
  INCORRECT FUNCTIONS:
  
 */
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

f32 cubic_ease_in_out_f32(f32 a, f32 b, f32 normalized_t) {
    if (normalized_t < 0.5) {
        f32 new_t = 4 * normalized_t * normalized_t * normalized_t;
        normalized_t = new_t;
    } else {
        f32 new_t = pow(-2 * normalized_t + 2, 3) / 2;
        normalized_t = new_t;
    }

    return (b - a) * normalized_t + a;
}

f32 quadratic_ease_in_f32(f32 a, f32 b, f32 normalized_t) {
    return (b - a) * (normalized_t * normalized_t) + a;
}

f32 quadratic_ease_out_f32(f32 a, f32 b, f32 normalized_t) {
    return -(b - a) * ((normalized_t * normalized_t) - 2.0) + a;
}

f32 quadratic_ease_in_out_f32(f32 a, f32 b, f32 normalized_t) {
    if (normalized_t < 0.5) {
        f32 new_t = (2 * normalized_t * normalized_t);
        normalized_t = new_t;
    } else {
        f32 new_t = 1 - pow(-2 * normalized_t + 2, 2) / 2;
        normalized_t = new_t;
    }

    return (b - a) * normalized_t + a;
}

#define EASE_BACK_C1 (1.70158)
#define EASE_BACK_C2 (EASE_BACK_C1*1.525)
#define EASE_BACK_C3 (EASE_BACK_C1+1)
f32 ease_in_back_f32(f32 a, f32 b, f32 normalized_t) {
    f32 t = ((EASE_BACK_C3) * normalized_t * normalized_t * normalized_t )- ((EASE_BACK_C3) * normalized_t * normalized_t);
    return (b - a) * t + a;
}

f32 ease_out_back_f32(f32 a, f32 b, f32 normalized_t) {
    f32 t = 1 + (EASE_BACK_C3) * pow(normalized_t - 1, 3) + (EASE_BACK_C1) * pow(normalized_t - 1, 2);
    return (b - a) * t + a;
}

f32 ease_in_out_back_f32(f32 a, f32 b, f32 normalized_t) {
    if (normalized_t < 0.5) {
        f32 new_t = (pow(2 * normalized_t, 2) * ((EASE_BACK_C2+1) * 2 * normalized_t - EASE_BACK_C2))/2;
        normalized_t = new_t;
    } else {
        f32 new_t = (pow(2 * normalized_t - 2, 2) * ((EASE_BACK_C2 + 1) * ((normalized_t * 2 - 2) + EASE_BACK_C2) + 2))/2;
        normalized_t = new_t;
    }

    return (b - a) * normalized_t + a;
}
