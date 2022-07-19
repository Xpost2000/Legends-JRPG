/* lex and tokenize some text I guess */
/* basically we run some "load-time" lisp to construct a tree that fits the limited set of features
   that are supported in-engine. Hopefully pretty easy. */
void game_open_conversation_file(struct game_state* state, string filename) {
    _debugprintf("opening file: %.*s", filename.length, filename.data);
}
