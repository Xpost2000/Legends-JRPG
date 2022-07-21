/*
  Isolated code to individually test new things, that I'm not fully integrating quite yet...
  
  Too lazy to compile as a separate project.
*/

void ___dialogue_testing(void) {
    /* game_open_conversation_file(game_state, string_literal("./dlg/intro00.txt")); */
    game_open_conversation_file(game_state, string_literal("./dlg/linear_test.txt"));
}

static bool sandbox_testing(void) {
    ___dialogue_testing();

    return true;
}
