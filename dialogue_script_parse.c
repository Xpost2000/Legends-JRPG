/*
 * TODO error checking
 * I guess most dialogue should be pretty linear so we can do a lot of tricks to change things
 * with area scripts and what not.
 */

local void dialogue_node_evaluate_code(struct memory_arena* arena, struct conversation* conversation, u32 node_index, string code_string) {
    struct conversation_node* node = conversation->nodes + node_index;

    struct lisp_form code = lisp_read_form(arena, code_string);
    assertion(code.type == LISP_FORM_LIST);

    struct lisp_list* list = &code.list;
    {
        struct lisp_form first = list->forms[0];

        if (lisp_form_symbol_matching(first, string_literal("choices"))) {
            _debugprintf("Dialogue choice spotted!");
            /* we can build our stuff here pretty easily... */
            for (unsigned choice_index = 0; choice_index < list->count-1; ++choice_index) {
                struct conversation_choice* new_choice = &node->choices[node->choice_count++];
                _debugprintf("ALLOC CHOICE");
                _debug_print_out_lisp_code(&code);

                struct lisp_form* choice_form = &list->forms[choice_index+1];
                /* should be a list... Hope it is! */

                /* I mean I should really be error checking, but I'm too lazy to do that right now */
                string choice_text = choice_form->list.forms[0].string;
                new_choice->text = choice_text;
                if (choice_form->list.count > 1) {
                    struct lisp_form* action_form = &choice_form->list.forms[1];
                    {
                        struct lisp_form* action_form_first = &action_form->list.forms[0];

                        if (lisp_form_symbol_matching(*action_form_first, string_literal("bye"))) {
                            new_choice->target = 0;
                        } else if (lisp_form_symbol_matching(*action_form_first, string_literal("next"))) {
                            s32 offset = 2;

                            if (action_form->list.count > 1) {
                                struct lisp_form* action_form_second = &action_form->list.forms[1];
                                if (action_form_second->type == LISP_FORM_NUMBER) {
                                    if (action_form_second->is_real == false) {
                                        offset += action_form_second->integer;
                                    } else {
                                        _debugprintf("found a (next) instruction but bad second arg");
                                    }
                                } else {
                                    _debugprintf("found a (next) instruction but bad second arg");
                                }
                            }

                            new_choice->target = node_index + offset;
                        } else if (lisp_form_symbol_matching(*action_form_first, string_literal("goto"))) {
                            if (action_form->list.count > 1) {
                                struct lisp_form* action_form_second = &action_form->list.forms[1];

                                if (action_form_second->type == LISP_FORM_NUMBER) {
                                    if (action_form_second->is_real == false) {
                                        new_choice->target = action_form_second->integer;
                                    } else {
                                        _debugprintf("found a (goto) instruction but bad second arg");
                                    }
                                } else {
                                    _debugprintf("found a (goto) instruction but bad second arg");
                                }
                            } else {
                                _debugprintf("found a (goto) instruction but bad second arg");
                            }
                        } else if (lisp_form_symbol_matching(*action_form_first, string_literal("do"))) {
                            _debugprintf("do block spotted for dialogue choice. Saving code to evaluate on demand!");
                            /* NOTE
                               we need to rebuild a string out of this probably... It's a bit difficult to find where 
                               the code may have originally been referenced.
                           
                               However, I could also just reparse it quickly (I have to reparse either way... It's a bit more work),
                               and just jump to the form instead of saving the specific form.

                               Should still be fast since it's a one time thing.
                            */
                            new_choice->script_code = code_string;
                        }
                    }
                } else {
                    _debugprintf("okay default choices!");
                    new_choice->target = node_index + 2;
                }
            }
        } else if (lisp_form_symbol_matching(first, string_literal("do"))) {
            /* just save the whole do and parse from there. */
            _debugprintf("do block spotted! Saving code to evaluate on demand.");
            node->script_code = code_string;
        } else if (lisp_form_symbol_matching(first, string_literal("bye"))) { /* hacky but okay, need to have subset of none actionable things */
            node->target = 0;
        }
    }
}

/*
  NOTE: This essentially operates "line" mode
*/
local void parse_and_compose_dialogue(struct game_state* state, struct lexer* lexer_state, s32* starting_node) {
    struct lexer_token determiner_token = lexer_peek_token(lexer_state);

    if (determiner_token.type == TOKEN_TYPE_LIST) {
        _debugprintf("DETERMINE_START?");
        /* try to hope it's a determiner form */
        struct lisp_form  code            = lisp_read_form(&scratch_arena, determiner_token.str);
        struct lisp_form* DETERMINE_START = lisp_list_nth(&code, 0);
        if (lisp_form_symbol_matching(*DETERMINE_START,string_literal("determine-start"))) {
            _debugprintf("all good boss!");
            struct lisp_form  body            = lisp_list_sliced(code, 1, -1);

            if (body.list.count != 1) {
                _debugprintf("This could be trouble!");
            }

            struct lisp_form winning_start;
            for (s32 body_form_index = 0; body_form_index < 1; ++body_form_index) {
                _debugprintf("Evaluating:");
                _debug_print_out_lisp_code(&body.list.forms[body_form_index]);
                winning_start = game_script_evaluate_form(&scratch_arena, state, &body.list.forms[body_form_index]);
                _debugprintf("DONNNNNNNNNNE");
            }

            s32 new_start;
            if (!lisp_form_get_s32(winning_start, &new_start)) {
                _debugprintf("that's bad! The last form evaled was not an index!");
                _debugprintf("Got back\n");
                _debug_print_out_lisp_code(&winning_start);
            } else {
                *starting_node = new_start;
                _debugprintf("Figured out the new start should be: %d\n", *starting_node);
            }
        }
        lexer_next_token(lexer_state);
    } else if (determiner_token.type == TOKEN_TYPE_NONE) {
        _debugprintf("EOF? (%d v. %d)", lexer_state->current_read_index, lexer_state->buffer.length);
        /* try to eat remaining state to finish off the lexer */
        lexer_next_token(lexer_state);
    } else {
        /* no error checking */
        struct lexer_token speaker_name  = lexer_next_token(lexer_state);
        struct lexer_token colon         = lexer_next_token(lexer_state);
        struct lexer_token dialogue_line = lexer_next_token(lexer_state);
        /* try to peek and see if we find an arrow */
        struct lexer_token maybe_arrow   = lexer_peek_token(lexer_state);

        struct conversation* conversation = &state->current_conversation;

        if (speaker_name.type == TOKEN_TYPE_STRING && colon.type == TOKEN_TYPE_COLON && dialogue_line.type == TOKEN_TYPE_STRING) {
            struct conversation_node* new_node = &conversation->nodes[conversation->node_count++];
            _debugprintf("allocating new node: (%d)", conversation->node_count);

            new_node->speaker_name = speaker_name.str;
            new_node->text         = dialogue_line.str;
            new_node->choice_count = 0;
            new_node->target       = conversation->node_count+1;

            if (lexer_token_is_symbol_matching(maybe_arrow, string_literal("=>"))) {
                lexer_next_token(lexer_state);
                _debugprintf("has arrow... Look for lisp code and parse it into instructions!");
                struct lexer_token lisp_code = lexer_next_token(lexer_state);

                if (lisp_code.type != TOKEN_TYPE_LIST) {
                    _debugprintf("So this is a problem");
                    _debug_print_token(lisp_code);
                    goto error;
                } else {
                    /* not top-level */
                    /* 
                       I mean theoretically I could interpret all of this and not have the structure... 
                   
                       Maybe consider that at some point?
                    */
                    /* build code */
                    dialogue_node_evaluate_code(&state->conversation_arena, conversation, conversation->node_count-1, lisp_code.str);
                }
            } else {
                _debugprintf("linear dialogue");
                if (lexer_token_is_null(maybe_arrow)) {
                    lexer_next_token(lexer_state);
                    new_node->target = 0;
                    _debugprintf("Might be the end.");
                }
            }
        } else {
        error:
            _debugprintf("dialogue error, cannot read for some reason... Not sure why right now");
            _debug_print_token(speaker_name);
            _debug_print_token(colon);
            _debug_print_token(dialogue_line);
            state->is_conversation_active             = false;
        }

    }
}

local void game_open_conversation_file(struct game_state* state, string filename) {
    memory_arena_clear_bottom(&state->conversation_arena);
    memory_arena_clear_top(&state->conversation_arena);
    _debugprintf("opening file: %.*s", filename.length, filename.data);
    /* 
       NOTE:
       However we don't have stack capacity on the arena. It's just double ended linear for now.
       this can technically be marked on the arena as a stack item. 
       
       NOTE:
       keep this in memory until the conversation is done.
    */

    state->is_conversation_active             = true;
    start_dialogue_ui();
    state->before_conversation_camera         = state->camera; 
    struct conversation* conversation = &state->current_conversation;
    conversation->node_count = 0;
    state->current_conversation_node_id       = 0;
    state->currently_selected_dialogue_choice = 0;
    state->conversation_file_buffer           = read_entire_file(memory_arena_allocator(&state->conversation_arena), filename);
    struct lexer lexer_state                  = {.buffer = file_buffer_as_string(&state->conversation_file_buffer),};

    s32 starting_node = 1;
    while (!lexer_done(&lexer_state)) {
        parse_and_compose_dialogue(state, &lexer_state, &starting_node);
        _debugprintf("next line\n");
    }

    void dialogue_ui_set_target_node(u32);
    dialogue_ui_set_target_node(starting_node);
}
