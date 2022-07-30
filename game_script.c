/* game scripting language things */
/* here we evaluate the lisp script to do things! */

/* evaluate a singular form at a time */
typedef struct lisp_form (*game_script_function)(struct memory_arena*, struct game_state*, struct lisp_form*, s32);

struct game_script_function_builtin {
    char* name;
    game_script_function function;
};

#define GAME_LISP_FUNCTION(name) struct lisp_form name ## __script_proc (struct memory_arena* arena, struct game_state* state, struct lisp_form* arguments, s32 argument_count)

GAME_LISP_FUNCTION(DLG_NEXT) {
    _debugprintf("NOT IMPLEMENTED YET!");
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_START_RAIN) {
    weather_start_rain(state);
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_STOP_RAIN) {
    weather_stop_rain(state);
    return LISP_nil;
}

#undef GAME_LISP_FUNCTION

#define STRINGIFY(x) #x
#define GAME_LISP_FUNCTION(NAME) (struct game_script_function_builtin) { .name = #NAME, .function = NAME ## __script_proc }
#undef STRINGIFY
static struct game_script_function_builtin script_function_table[] = {
    GAME_LISP_FUNCTION(DLG_NEXT),
    GAME_LISP_FUNCTION(GAME_START_RAIN),
    GAME_LISP_FUNCTION(GAME_STOP_RAIN)
};

local game_script_function lookup_script_function(string name) {
    for (unsigned index = 0; index < array_count(script_function_table); ++index) {
        struct game_script_function_builtin* current_builtin = script_function_table + index; 

        if (string_equal_case_insensitive(string_from_cstring(current_builtin->name), name)) {
            return current_builtin->function;
        }
    }

    return 0;
}
#undef GAME_LISP_FUNCTION

struct game_variable* lookup_game_variable(string name) {
    struct game_variables* variables = &game_state->variables;

    for (s32 index = 0; index < variables->count; ++index) {
        struct game_variable* variable = variables->variables + index;

        if (string_equal(name, string_from_cstring(variable->name))) {
            return variable;
        }
    }

    struct game_variable* new_variable = &variables->variables[variables->count++];
    cstring_copy(name.data, new_variable->name, array_count(new_variable->name));
    new_variable->float_value   = 0;
    new_variable->integer_value = 0;
    return new_variable;
}

struct lisp_form game_script_look_up_dictionary_value(struct lisp_form name) {
    struct game_variable* variable = lookup_game_variable(name.string);

    if (variable->is_float) {
        return lisp_form_real(variable->float_value);
    } else {
        return lisp_form_integer(variable->integer_value);
    }

    return LISP_nil;
}

/* TODO should allow us to flexibly change types though. */
/* struct lisp_form game_script_set_dictionary_value(struct lisp_form name, struct lisp_form value) { */
/* } */

/* forward decls, functions recursively call each other */
struct lisp_form game_script_evaluate_form(struct memory_arena* arena, struct game_state* state, struct lisp_form* form);
bool handle_builtin_game_script_functions(struct memory_arena* arena, struct game_state* state, struct lisp_form* to_evaluate, struct lisp_form* result);

bool handle_builtin_game_script_functions(struct memory_arena* arena, struct game_state* state, struct lisp_form* form, struct lisp_form* result) {
    /* math */
    struct lisp_form first_form = form->list.forms[0];
    {
        s32 integer_accumulator = 0;
        f32 real_accumulator    = 0;
        bool should_be_real = false;

        if (lisp_form_symbol_matching(first_form, string_literal("+"))) {
            for (s32 index = 1; index < form->list.count; ++index) {
                struct lisp_form* child = form->list.forms + index;
                struct lisp_form child_evaled = game_script_evaluate_form(arena, state, child);

                if (child_evaled.type == LISP_FORM_NUMBER) {
                    if (child_evaled.is_real) {
                        should_be_real = true;
                        real_accumulator += child_evaled.real;
                    } else {
                        if (should_be_real) {
                            real_accumulator += child_evaled.real;
                        } else {
                            integer_accumulator += child_evaled.integer;
                        }
                    }
                } else {
                    return false; /* bad eval */
                }
            }
        } else if (lisp_form_symbol_matching(first_form, string_literal("-"))) {
            for (s32 index = 1; index < form->list.count; ++index) {
                struct lisp_form* child = form->list.forms + index;
                struct lisp_form child_evaled = game_script_evaluate_form(arena, state, child);

                if (child_evaled.type == LISP_FORM_NUMBER) {
                    if (child_evaled.is_real) {
                        should_be_real = true;
                        real_accumulator -= child_evaled.real;
                    } else {
                        if (should_be_real) {
                            real_accumulator -= child_evaled.real;
                        } else {
                            integer_accumulator -= child_evaled.integer;
                        }
                    }
                } else {
                    return false; /* bad eval */
                }
            }
        } else if (lisp_form_symbol_matching(first_form, string_literal("*"))) {
            for (s32 index = 1; index < form->list.count; ++index) {
                struct lisp_form* child = form->list.forms + index;
                struct lisp_form child_evaled = game_script_evaluate_form(arena, state, child);

                if (child_evaled.type == LISP_FORM_NUMBER) {
                    if (child_evaled.is_real) {
                        should_be_real = true;
                        real_accumulator *= child_evaled.real;
                    } else {
                        if (should_be_real) {
                            real_accumulator *= child_evaled.real;
                        } else {
                            integer_accumulator *= child_evaled.integer;
                        }
                    }
                } else {
                    return false; /* bad eval */
                }
            }
        } else if (lisp_form_symbol_matching(first_form, string_literal("/"))) {
            for (s32 index = 1; index < form->list.count; ++index) {
                struct lisp_form* child = form->list.forms + index;
                struct lisp_form child_evaled = game_script_evaluate_form(arena, state, child);

                if (child_evaled.type == LISP_FORM_NUMBER) {
                    if (child_evaled.is_real) {
                        should_be_real = true;
                        real_accumulator /= child_evaled.real;
                    } else {
                        if (should_be_real) {
                            real_accumulator /= child_evaled.real;
                        } else {
                            integer_accumulator /= child_evaled.integer;
                        }
                    }
                } else {
                    return false; /* bad eval */
                }
            }
        }

        if (should_be_real) {
            *result = lisp_form_real(real_accumulator);
            return true;
        } else {
            *result = lisp_form_integer(integer_accumulator);
            return true;
        }
    }
    /* hopefully a list */
    return false;
}

struct lisp_form game_script_evaluate_form(struct memory_arena* arena, struct game_state* state, struct lisp_form* form) {
    if (form) {
        if (form->quoted) {
            return (*form);
        }

        /* evalute for standard forms, like if mostly... Actually only if */
        if (form->type == LISP_FORM_LIST) {
            if (lisp_form_symbol_matching(form->list.forms[0], string_literal("if"))) {
                struct lisp_form* condition    = lisp_list_get_child(form, 1);
                struct lisp_form* true_branch  = lisp_list_get_child(form, 2);
                struct lisp_form* false_branch = lisp_list_get_child(form, 3);

                struct lisp_form evaluated = game_script_evaluate_form(arena, state, condition);
                if (evaluated.type != LISP_FORM_NIL) {
                    return game_script_evaluate_form(arena, state, true_branch);
                } else {
                    return game_script_evaluate_form(arena, state, false_branch);
                }
            } else {
                struct lisp_form result = LISP_nil;

                if (!handle_builtin_game_script_functions(arena, state, form, &result)) {
                    game_script_function function = lookup_script_function(form->list.forms[0].string);

                    if (function) {
                        result = function(arena, state, form->list.forms + 1, form->list.count-1);   
                    } else {
                        _debugprintf("(no function called \"%.*s\")", form->list.forms[0].string.length, form->list.forms[0].string.data);
                    }
                }

                return result;
            }
        } else {
            if (form->type == LISP_FORM_SYMBOL) {
                /* look up in the game variable list */
                return game_script_look_up_dictionary_value(*form);
            } else {
                return (*form);
            }
        }
    } else {
        return LISP_nil;
    }

    return LISP_nil;
}
