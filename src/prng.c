/* bad LCG */
#include "prng_def.c"

s32 random_state_next(struct random_state* state) {
    state->state = (state->multiplier * state->state + state->constant);
    state->state %= state->modulus;

    if (state->state < 0) state->state *= -1;

    return state->state;
}

void random_state_reset(struct random_state* state) {
    state->state = state->seed;
}

f32 random_float(struct random_state* state) {
    return (float)(random_state_next(state)) / (float)RANDOM_MAX;
}

s32 random_ranged_integer(struct random_state* state, s32 minimum, s32 maximum) {
    if (maximum == minimum) return minimum;
    return (random_state_next(state) % (maximum - minimum)) + minimum;
}

f32 random_ranged_float(struct random_state* state, f32 a, f32 b) {
    return lerp_f32(a, b, random_float(state)); 
}

/* generic weighted random, provide list of weights and use the index to find your answer. */
s32 random_weighted_selection(struct random_state* random, f32* weights, u64 item_count) {
    f32 weight_sum = 0.0f;

    for (unsigned index = 0; index < item_count; ++index) {
        weight_sum += weights[index];
    }

    f32 weight_threshold = random_float(random) * weight_sum;

    for (unsigned index = 0; index < item_count; ++index) {
        if (weight_threshold <= weights[index]) {
            return index;
        }

        weight_threshold -= weights[index];
    }

    return 0;
}
