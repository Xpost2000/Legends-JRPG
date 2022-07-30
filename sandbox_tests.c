/*
  Isolated code to individually test new things, that I'm not fully integrating quite yet...
  
  Too lazy to compile as a separate project.
*/

void ___dialogue_testing(void) {
    /* game_open_conversation_file(game_state, string_literal("./dlg/intro00.txt")); */
    /* game_open_conversation_file(game_state, string_literal("./dlg/linear_test.txt")); */
}

static bool sandbox_testing(void) {
    _debugprintf("sandbox start");
#if 1
    struct lisp_form code = lisp_read_form(&scratch_arena, string_literal("(not-quoted 'quoted 'sad)"));
    /* struct lisp_form code = lisp_read_form(&scratch_arena, string_literal("'sad")); */
    _debug_print_out_lisp_code(&code);
    fprintf(stderr, "\n");
#endif
    _debugprintf("sandbox end");

    return true;
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
