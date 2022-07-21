/*
  Isolated code to individually test new things, that I'm not fully integrating quite yet...
  
  Too lazy to compile as a separate project.
*/

void ___dialogue_testing(void) {
    /* game_open_conversation_file(game_state, string_literal("./dlg/intro00.txt")); */
    game_open_conversation_file(game_state, string_literal("./dlg/linear_test.txt"));
}

static bool sandbox_testing(void) {
    _debugprintf("sandbox start");
    ___dialogue_testing();
    struct lisp_form code = lisp_read_form(&scratch_arena, string_literal("(hi this is (1 2 3))"));
    _debug_print_out_lisp_code(&code);
    fprintf(stderr, "\n");
    _debugprintf("sandbox end");

    return false;
}
