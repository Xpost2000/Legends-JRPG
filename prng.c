/* bad LCG */
#define RANDOM_MAX ((1 << 31) - 1)
struct random_state {
    s32 constant;
    s32 multiplier;
    s32 state;
    s32 seed;
    s32 modulus;
};

#define random_state_ext(CONSTANT, MULTIPLIER, SEED, MODULUS) \
    (struct random_state) {                                   \  
          .constant   = CONSTANT,                             \
          .multiplier = MULTIPLIER,                           \
          .seed       = SEED,                                 \
          .state      = SEED,                                 \
          .modulus    = MODULUS,                              \
    }

#define random_state_ex(constant, multiplier) random_state_ext(constant, multiplier, time(NULL), (RANDOM_MAX))
#define random_state() random_state_ex(12345, 1123515245)

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
    return (random_state_next(state) % (maximum - minimum)) + minimum;
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
