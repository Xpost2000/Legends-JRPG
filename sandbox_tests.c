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
    _debugprintf("okay finished %d", b - a);

    _debugprintf("single threaded version of the same test");
    a = SDL_GetPerformanceCounter();
    for (s32 i = 0; i < array_count(n); ++i) n[i] = i+1;
    for (s64 i = 0; i < array_count(n); ++i)
        test_job_number(&n[i]);
    b = SDL_GetPerformanceCounter();
    _debugprintf("okay finished %d", b - a);
}

static void _sandbox_shop_inventory(void) {
    struct shop_instance s = load_shop_definition(&scratch_arena, string_literal("basic"));
    _debug_print_out_shop_contents(&s);
}

static bool sandbox_testing(void) {
    _debugprintf("sandbox start");
    /* _thread_testing(); */
    _sandbox_shop_inventory();
    _debugprintf("sandbox end");
    return false;
}

/* I don't remember if the game has been initiated yet LOL */
int _game_sandbox_testing(void) {
    {
        struct navigation_path path = navigation_path_find(&scratch_arena, &game_state->loaded_area,
                                                        v2f32(7,5), v2f32(7,10));
        _debug_print_navigation_path(&path);
    }
    return true; 
}
