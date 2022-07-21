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
    return (struct lisp_form) { .type = LISP_FORM_NIL, .string = string_literal("nil") };
}
GAME_LISP_FUNCTION(GAME_START_RAIN) {
    weather_start_rain(state);
    return (struct lisp_form) { .type = LISP_FORM_NIL, .string = string_literal("nil") };
}
GAME_LISP_FUNCTION(GAME_STOP_RAIN) {
    weather_stop_rain(state);
    return (struct lisp_form) { .type = LISP_FORM_NIL, .string = string_literal("nil") };
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

struct lisp_form game_script_evaluate_form(struct memory_arena* arena, struct game_state* state, struct lisp_form* form) {
    if (form) {
        /* evalute for standard forms, like if mostly... Actually only if */
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
            game_script_function function = lookup_script_function(form->list.forms[0].string);
            if (function) {
                return function(arena, state, form->list.forms + 1, form->list.count-1);   
            } else {
                _debugprintf("(no function called \"%.*s\")", form->list.forms[0].string.length, form->list.forms[0].string.data);
            }
        }
    } else {
        return (struct lisp_form) { .type = LISP_FORM_NIL, .string = string_literal("nil") };
    }

    return (struct lisp_form) { .type = LISP_FORM_NIL, .string = string_literal("nil") };
}
