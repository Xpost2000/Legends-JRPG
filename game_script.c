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

struct game_variable* lookup_game_variable(string name, bool create_when_not_found) {
    struct game_variables* variables = &game_state->variables;

    for (s32 index = 0; index < variables->count; ++index) {
        struct game_variable* variable = variables->variables + index;

        if (string_equal(name, string_from_cstring(variable->name))) {
            return variable;
        }
    }

    if (create_when_not_found) {
        struct game_variable* new_variable = &variables->variables[variables->count++];

        cstring_copy(name.data, new_variable->name, array_count(new_variable->name));
        new_variable->float_value   = 0;
        new_variable->integer_value = 0;

        return new_variable;
    }
    return NULL;
}

struct lisp_form game_script_look_up_dictionary_value(struct lisp_form name) {
    struct game_variable* variable = lookup_game_variable(name.string, false);

    if (variable) {
        if (variable->is_float) {
            return lisp_form_real(variable->float_value);
        } else {
            return lisp_form_integer(variable->integer_value);
        }
    }

    return LISP_nil;
}

/* TODO should allow us to flexibly change types though. */
struct lisp_form game_script_set_dictionary_value(struct lisp_form name, struct lisp_form value) {
    struct game_variable* variable = lookup_game_variable(name.string, false);

    if (!variable) {
        variable = lookup_game_variable(name.string, true);

        /* there is a weird type system. Consider if I just want all integers? Probably. but who knows */
        if (value.type == LISP_FORM_NUMBER) {
            if (value.is_real) {
                variable->is_float = true;
            } else {
                variable->is_float = false;
            }
        }
    }

    if (variable->is_float) {
        lisp_form_get_f32(value, &variable->float_value);
    } else {
        lisp_form_get_s32(value, &variable->integer_value);
    }

    return value;
}

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
    /* boolean logic (or and and) (equal) */
    /* I don't need this to be ultra robust, I'll leave it as undefined behavior if you're not using boolean evaluable conditions */
    {
        bool evaled_boolean_result = false;

        if (lisp_form_symbol_matching(first_form, string_literal("and"))) {
            evaled_boolean_result = true;
            for (s32 index = 1; index < form->list.count && evaled_boolean_result; ++index) {
                struct lisp_form* child = form->list.forms + index;
                struct lisp_form child_evaled = game_script_evaluate_form(arena, state, child);

                if (child_evaled.type == LISP_FORM_T) {
                    continue;
                } else {
                    /* short circuit */
                    evaled_boolean_result = false;
                }
            }
        } else if (lisp_form_symbol_matching(first_form, string_literal("or"))) {
            evaled_boolean_result = false;
            for (s32 index = 1; index < form->list.count && !evaled_boolean_result; ++index) {
                struct lisp_form* child = form->list.forms + index;
                struct lisp_form child_evaled = game_script_evaluate_form(arena, state, child);

                if (child_evaled.type == LISP_FORM_T) {
                    /* short circuit */
                    evaled_boolean_result = true;
                } else {
                    continue;
                }
            }
        } else if (lisp_form_symbol_matching(first_form, string_literal("equal"))) {
            /* allow checking lots of equals */
            if (form->list.count >= 2) {
                evaled_boolean_result = false;

                struct lisp_form child_first  = game_script_evaluate_form(arena, state, form->list.forms + 1);
                {
                    struct lisp_form child_second = game_script_evaluate_form(arena, state, form->list.forms + 2);
                    evaled_boolean_result = lisp_form_check_equality(child_first, child_second);
                }

                for (s32 index = 3; index < form->list.count && !evaled_boolean_result; ++index) {
                    struct lisp_form* child = form->list.forms + index;
                    struct lisp_form child_evaled = game_script_evaluate_form(arena, state, child);

                    /* if we know the first two are equal, just check against any of the already good ones. */
                    evaled_boolean_result &= lisp_form_check_equality(child_first, child_evaled);
                }
            } else {
                return false;
            }
        }

        if (evaled_boolean_result) {
            *result = LISP_t;
        } else {
            *result = LISP_nil;
        }

        return true;
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
