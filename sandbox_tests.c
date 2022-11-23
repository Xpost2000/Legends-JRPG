/*
  Isolated code to individually test new things, that I'm not fully integrating quite yet...
  
  Too lazy to compile as a separate project.
*/

void ___dialogue_testing(void) {
    /* game_open_conversation_file(game_state, string_literal("./dlg/intro00.txt")); */
    /* game_open_conversation_file(game_state, string_literal("./dlg/linear_test.txt")); */
}

s32 test_job_number(void* s) {
    s32* i = s;
    (*i) *= 10;
    return 0;
}

static void _thread_testing(void) {
    s32 n[100] = {};
    for (s32 i = 0; i < array_count(n); ++i) n[i] = i+1;
    /* consume this! */
    u64 a = SDL_GetPerformanceCounter();
    {
        for (s64 i = 0; i < array_count(n); ++i)
            thread_pool_add_job(test_job_number, &n[i]);
        for (s64 i = 0; i < array_count(n); ++i)
            thread_pool_add_job(test_job_number, &n[i]);
        for (s64 i = 0; i < array_count(n); ++i)
            thread_pool_add_job(test_job_number, &n[i]);
        for (s64 i = 0; i < array_count(n); ++i)
            thread_pool_add_job(test_job_number, &n[i]);
    }
    thread_pool_synchronize_tasks();
    u64 b = SDL_GetPerformanceCounter();
    _debugprintf("okay finished %llu", b - a);

    _debugprintf("single threaded version of the same test");
    a = SDL_GetPerformanceCounter();
    for (s32 i = 0; i < array_count(n); ++i) n[i] = i+1;
    for (s64 i = 0; i < array_count(n); ++i)
        test_job_number(&n[i]);
    b = SDL_GetPerformanceCounter();
    _debugprintf("okay finished %llu", b - a);
}

static void _sandbox_shop_inventory(void) {
    struct shop_instance s = load_shop_definition(&scratch_arena, string_literal("basic"));
    _debug_print_out_shop_contents(&s);
}

static bool _interpolation_testing(void) {
    f32
        a = 1,
        b = 2;
    const f32
        dt = 1/100.0f;

    /* These values look odd, need to find correct implementations later, or write a curve editor */
#define Interpolation_Loop_Test(fn)             \
    _debugprintf("interpolating " #fn);         \
    for (f32 t = 0; t < 1; t += dt) {           \
        f32 v = fn(1, 2, t);                    \
        _debugprintf("%f", v);                  \
    }

    Interpolation_Loop_Test(lerp_f32);
    Interpolation_Loop_Test(cubic_ease_in_f32);
    Interpolation_Loop_Test(cubic_ease_out_f32);
    Interpolation_Loop_Test(cubic_ease_in_out_f32);
    Interpolation_Loop_Test(quadratic_ease_in_f32);
    Interpolation_Loop_Test(quadratic_ease_out_f32);
    Interpolation_Loop_Test(quadratic_ease_in_out_f32);
    Interpolation_Loop_Test(ease_in_back_f32);
    Interpolation_Loop_Test(ease_out_back_f32);
    Interpolation_Loop_Test(ease_in_out_back_f32);

#undef Interpolation_Loop_Test
    return true;
}

/* no assertive tests I'm eyeballing this */
local void _sandbox_test_byteswap(void) {
    _debugprintf("sandbox endian test");
#define Byteswap_Test(BITS, V)                          \
    do {                                                \
        uint##BITS##_t a = V;                           \
        uint##BITS##_t b = byteswap_u##BITS(a);         \
        _debug_print_bitstring((uint8_t*)&a, BITS/8);   \
        _debugprintf("NUM: %llu", (u64)a);                  \
        _debugprintf("swapped?");                       \
        _debug_print_bitstring((uint8_t*)&b, BITS/8);   \
        _debugprintf("NUM: %llu", (u64)b);                  \
        _debugprintf("next");                           \
    } while(0)

    Byteswap_Test(64, 2);
    Byteswap_Test(32, 2);
    Byteswap_Test(16, 2);
#undef Byteswap_Test
}

static bool sandbox_testing(void) {
#if 0
    _debugprintf("sandbox start");
    /* _thread_testing(); */
    /* _sandbox_shop_inventory(); */
    assertion(_interpolation_testing());
    _sandbox_test_byteswap();
    _debugprintf("sandbox end");
#endif
    return false;
}

/* I don't remember if the game has been initiated yet LOL */
int _game_sandbox_testing(void) {
#if 0
    {
        struct navigation_path path = navigation_path_find(&scratch_arena, &game_state->loaded_area,
                                                        v2f32(7,5), v2f32(7,10));
        _debug_print_navigation_path(&path);
    }
#else
#endif
    return true; 
}
