/* game scripting language things */
/* here we evaluate the lisp script to do things! */

/*
  Special form NOTE:
  
  (no-wait (FORM))
  Will consume a single form and execute it without trying to wait for it's completion, this is for some things
  
  
  All actions are done in parallel if possible. (might be buggy with message queue, but the intent
  is that you use it for cutscene scripting like path finding.)
*/

/* evaluate a singular form at a time */
typedef struct lisp_form (*game_script_function)(struct memory_arena*, struct game_state*, struct lisp_form*, s32);

struct game_script_function_builtin {
    char* name;
    game_script_function function;
};

struct game_script_execution_state {
    s32 current_form_index;
    struct lisp_form body;
};

#define GAME_SCRIPT_EXECUTION_STATE_STACK_SIZE (128) /* hopefully this doesn't stack overflow */
local s32  game_script_execution_stack_depth                                                            = 0;
local struct game_script_execution_state game_script_stackframe[GAME_SCRIPT_EXECUTION_STATE_STACK_SIZE] = {};
local bool queued_script_already                                                                        = false;

void game_script_push_stackframe(struct lisp_form f) {
    assertion(game_script_execution_stack_depth < GAME_SCRIPT_EXECUTION_STATE_STACK_SIZE && "game_script stackoverflow!");
    game_script_stackframe[game_script_execution_stack_depth].current_form_index = 0;
    game_script_stackframe[game_script_execution_stack_depth++].body             = f;
}

void game_script_enqueue_form_to_execute(struct lisp_form f) {
    assertion(queued_script_already == false && "We do not support multiple scripts... Yet! Haven't thought of an answer yet.");
    queued_script_already                              = true;
    game_script_push_stackframe(f);
}

/* considering this is so patternized, is it worth writing a mini metaprogramming thing that just automates this? Who knows */

#define GAME_LISP_FUNCTION(name) struct lisp_form name ## __script_proc (struct memory_arena* arena, struct game_state* state, struct lisp_form* arguments, s32 argument_count)

GAME_LISP_FUNCTION(OBJ_ACTIVATIONS) {
    /* expect an object handle. Not checking right now */

    if (argument_count == 1) {
        struct game_script_typed_ptr object_ptr = game_script_object_handle_decode(arguments[0]);

        switch (object_ptr.type) {
            case GAME_SCRIPT_TARGET_TRIGGER: {
                struct trigger* trigger = (struct trigger*)object_ptr.ptr;
                return lisp_form_integer(trigger->activations);
            } break;
                bad_case;
        }
    }

    return LISP_nil;
}
GAME_LISP_FUNCTION(DLG_NEXT) {
    _debugprintf("NOT IMPLEMENTED YET!");
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_START_RAIN) {
    _debugprintf("game_start_rain script");
    weather_start_rain(state);
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_STOP_RAIN) {
    _debugprintf("game_stop_rain script");
    weather_stop_rain(state);
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_START_SNOW) {
    _debugprintf("game_start_snow script");
    weather_start_snow(state);
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_STOP_SNOW) {
    _debugprintf("game_stop_snow script");
    weather_stop_snow(state);
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_SET_REGION_NAME) {
    _debugprintf("set region name");
    struct lisp_form* str_name = &arguments[0];
    struct lisp_form* str_sub_name = &arguments[1];

    string area_name     = {};
    string area_sub_name = {};

    area_name = str_name->string;
    if (argument_count == 2) {
        area_sub_name = str_sub_name->string;
    }
    /* no check */
    game_attempt_to_change_area_name(state, string_literal("Does not work yet!"), string_literal("The development zone"));
    return LISP_nil;
}

GAME_LISP_FUNCTION(GAME_SET_ENVIRONMENT_COLORS) {
    if (argument_count == 1) {
        struct lisp_form* argument = &arguments[0];

        if (argument->type == LISP_FORM_SYMBOL) {
            if (lisp_form_symbol_matching(*argument, string_literal("night"))) {
                global_color_grading_filter = COLOR_GRADING_NIGHT;
            } else if (lisp_form_symbol_matching(*argument, string_literal("dawn"))) {
                global_color_grading_filter = COLOR_GRADING_DAWN;
            } else if (lisp_form_symbol_matching(*argument, string_literal("midnight"))) {
                global_color_grading_filter = COLOR_GRADING_DARKEST;
            } else if (lisp_form_symbol_matching(*argument, string_literal("day"))) {
                global_color_grading_filter = COLOR_GRADING_DAY;
            }
        } else {
            struct lisp_form* r = lisp_list_nth(argument, 0);
            struct lisp_form* g = lisp_list_nth(argument, 1);
            struct lisp_form* b = lisp_list_nth(argument, 2);
            struct lisp_form* a = lisp_list_nth(argument, 3);

            f32 color_r;
            f32 color_g;
            f32 color_b;
            f32 color_a;

            if (!(r && lisp_form_get_f32(*r, &color_r))) color_r = 255.0f;
            if (!(g && lisp_form_get_f32(*g, &color_g))) color_g = 255.0f;
            if (!(b && lisp_form_get_f32(*b, &color_b))) color_b = 255.0f;
            if (!(a && lisp_form_get_f32(*a, &color_a))) color_a = 255.0f;

            global_color_grading_filter.r = color_r;
            global_color_grading_filter.g = color_g;
            global_color_grading_filter.b = color_b;
            global_color_grading_filter.a = color_a;
        }
    }

    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_MESSAGE_QUEUE) {
    if (argument_count == 1) {
        struct lisp_form* argument = &arguments[0];

        if (argument->type == LISP_FORM_STRING) {
            game_message_queue(argument->string);
        }
    }

    return LISP_nil;
}

#undef GAME_LISP_FUNCTION

#define STRINGIFY(x) #x
#define GAME_LISP_FUNCTION(NAME) (struct game_script_function_builtin) { .name = #NAME, .function = NAME ## __script_proc }
#undef STRINGIFY
static struct game_script_function_builtin script_function_table[] = {
    GAME_LISP_FUNCTION(DLG_NEXT),
    GAME_LISP_FUNCTION(OBJ_ACTIVATIONS),
    GAME_LISP_FUNCTION(GAME_START_RAIN),
    GAME_LISP_FUNCTION(GAME_STOP_RAIN),
    GAME_LISP_FUNCTION(GAME_START_SNOW),
    GAME_LISP_FUNCTION(GAME_STOP_SNOW),
    GAME_LISP_FUNCTION(GAME_SET_ENVIRONMENT_COLORS),
    GAME_LISP_FUNCTION(GAME_MESSAGE_QUEUE),
    GAME_LISP_FUNCTION(GAME_SET_REGION_NAME),
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

/* Why is this separate? Must've been smoking something. Undo this later. */
bool handle_builtin_game_script_functions(struct memory_arena* arena, struct game_state* state, struct lisp_form* form, struct lisp_form* result) {
    /* math */
    struct lisp_form first_form = form->list.forms[0];
    {
        s32 integer_accumulator = 0;
        f32 real_accumulator    = 0;
        bool should_be_real = false;

        bool is_mathematical_operator = false;

        if (lisp_form_symbol_matching(first_form, string_literal("+"))) {
            is_mathematical_operator = true;
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
            is_mathematical_operator = true;
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
            is_mathematical_operator = true;
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
        } else {
            *result = lisp_form_integer(integer_accumulator);
        }

        if (is_mathematical_operator) {
            return true;
        }
    }
    /* boolean logic (or and and) (equal) */
    /* I don't need this to be ultra robust, I'll leave it as undefined behavior if you're not using boolean evaluable conditions */
    {
        bool evaled_boolean_result = false;
        bool is_boolean_operator   = false;

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
            is_boolean_operator = true;
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
            is_boolean_operator = true;
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
            is_boolean_operator = true;
        }

        if (evaled_boolean_result) {
            *result = LISP_t;
        } else {
            *result = LISP_nil;
        }

        if (is_boolean_operator) return true;
    }

    {
        if (lisp_form_symbol_matching(first_form, string_literal("print"))) {
            printf("printing: %d forms\n", form->list.count - 1);
            for (s32 index = 1; index < form->list.count; ++index) {
                struct lisp_form child_evaled = game_script_evaluate_form(arena, state, form->list.forms+index);
                lisp_form_output(&child_evaled);
                printf(" ");
            }
            printf("\n");

            *result = LISP_nil;
            return true;
        }
    }
    /* hopefully a list */
    return false;
}

bool game_script_object_handle_matches_object(struct lisp_form object_handle, s32 type, s32 id) {
    assertion(object_handle.type == LISP_FORM_LIST && "lisp object handle is not a list??? Oh no");
    assertion(object_handle.list.count == 2 && "lisp representations of object handles should be like (TYPE ID)");

    struct lisp_form* type_discriminator_form = lisp_list_nth(&object_handle, 0);
    struct lisp_form* id_form                 = lisp_list_nth(&object_handle, 1);

    s32 real_id = 0;

    if (!lisp_form_get_s32(*id_form, &real_id)) {
        return false;
    }

    string id_to_match = entity_game_script_target_type_name[type];

    if (lisp_form_symbol_matching(*type_discriminator_form, id_to_match)) {
        if (real_id == id) {
            return true;
        }
    }

    return false;
}


struct lisp_form game_script_evaluate_form(struct memory_arena* arena, struct game_state* state, struct lisp_form* form) {
    _debugprintf("begin");
    if (form) {
        if (form->quoted) {
            _debugprintf("quoted eval");
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
            } else if (lisp_form_symbol_matching(form->list.forms[0], string_literal("progn"))) {
                _debugprintf("Found a progn");
                struct lisp_form result = {};

                for (s32 index = 1; index < form->list.count; ++index) {
                    result = game_script_evaluate_form(arena, state, form->list.forms+index);
                }

                return result;
            } else {
                struct lisp_form result = LISP_nil;
                /* NOTE: 
                   builtins technically evalute their children on their own time... 
                   I should change this...
                */

                if (!handle_builtin_game_script_functions(arena, state, form, &result)) {
                    _debugprintf("calling from function table");
                    game_script_function function = lookup_script_function(form->list.forms[0].string);

                    if (function) {
                        s32 argument_count = 0;
                        struct lisp_form* evaluated_params = 0;

                        if (form->list.count == 1) {
                            argument_count   = 0;
                            evaluated_params = 0;
                        } else {
                            argument_count   = form->list.count-1;
                            evaluated_params = memory_arena_push(arena, argument_count * sizeof(*form->list.forms));

                            for (s32 index = 0; index < argument_count; ++index) {
                                _debugprintf("evaluating arg");
                                evaluated_params[index] = game_script_evaluate_form(arena, state, form->list.forms + (index+1));
                            }
                        }

                        result = function(arena, state, evaluated_params, argument_count);
                    } else {
                        _debugprintf("(no function called \"%.*s\")", form->list.forms[0].string.length, form->list.forms[0].string.data);
                    }
                } else {
                    _debugprintf("built in handled");
                }

                return result;
            }
        } else {
            _debugprintf("self evaluating");
            if (form->type == LISP_FORM_SYMBOL) {
                /* look up in the game variable list */
                return game_script_look_up_dictionary_value(*form);
            } else {
                return (*form);
            }
        }
    } else {
        _debugprintf("no form?");
        return LISP_nil;
    }

    _debugprintf("end");
    return LISP_nil;
}

struct game_script_typed_ptr game_script_object_handle_decode(struct lisp_form object_handle) {
    assertion(object_handle.type == LISP_FORM_LIST && "lisp object handle is not a list??? Oh no");
    assertion(object_handle.list.count == 2 && "lisp representations of object handles should be like (TYPE ID)");
    struct game_script_typed_ptr result = {};

    struct lisp_form* type_discriminator_form = lisp_list_nth(&object_handle, 0);
    struct lisp_form* id_form                 = lisp_list_nth(&object_handle, 1);

    s32 type_id = 0;
    s32 real_id = 0;

    if (!lisp_form_get_s32(*id_form, &real_id)) {
        /* TODO: needs better erroring */
        return result;
    }

    for (s32 index = 0; index < array_count(entity_game_script_target_type_name); ++index) {
        if (lisp_form_symbol_matching(*type_discriminator_form, entity_game_script_target_type_name[index])) {
            type_id = index;
            break;
        }
    }

    result.type = type_id;

    {
        struct level_area* area = &game_state->loaded_area;
        switch (type_id) {
            case GAME_SCRIPT_TARGET_TRIGGER: {
                result.ptr = area->script_triggers + real_id;
            } break;
            case GAME_SCRIPT_TARGET_ENTITY: {
                result.ptr = game_state->entities.entities + real_id;
            } break;
            case GAME_SCRIPT_TARGET_CHEST: {
                result.ptr = area->chests + real_id;
            } break;
        }
    }

    return result;
}

/* This assumes form_to_wait_on has already begun it's action. */
bool game_script_waiting_on_form(struct lisp_form* form_to_wait_on) {
    /* always a list */

    struct lisp_form* first = lisp_list_nth(form_to_wait_on, 0);

    /* builtins */
    if (lisp_form_symbol_matching(*first, string_literal("if"))) {
        struct lisp_form* condition    = lisp_list_get_child(form_to_wait_on, 1);
        struct lisp_form* true_branch  = lisp_list_get_child(form_to_wait_on, 2);
        struct lisp_form* false_branch = lisp_list_get_child(form_to_wait_on, 3);

        /* crying in *side_effects*. Lots of bugs waiting to happen. */
        struct lisp_form evaluated = game_script_evaluate_form(&scratch_arena, game_state, condition);

        if (evaluated.type != LISP_FORM_NIL) {
            return game_script_waiting_on_form(true_branch);
        } else {
            if (false_branch)
                return game_script_waiting_on_form(false_branch);
            else
                return true;
        }
    } else if (lisp_form_symbol_matching(*first, string_literal("progn"))) {
        return game_script_waiting_on_form(lisp_list_nth(form_to_wait_on, -1));
    }

    if (lisp_form_symbol_matching(*first, string_literal("game_message_queue"))) {
        if (global_popup_state.message_count == 0) {
            return true;
        } else {
            return false;
        }
    }

    return true;
}

/*
  if it's a scalar (normal value), we'll assume it's the same as receiving a finished signal.
  
  if we get a (!game_script_coroutine_result_object! *status* *real-value*)
  
  TODO: does not handle multiple script enqueues yet...
*/
void game_script_execute_awaiting_scripts(struct memory_arena* arena, struct game_state* state, f32 dt) {
    if (game_script_execution_stack_depth <= 0) {
        /* allow queueing next script */
        queued_script_already = false;
    } else {
        /* execute current script with a stackframe. */
        struct game_script_execution_state* current_stackframe = game_script_stackframe + (game_script_execution_stack_depth-1);

        /* finished stackframe. pop off */
        if (current_stackframe->current_form_index >= current_stackframe->body.list.count) {
            game_script_execution_stack_depth -= 1;
        } else {
            /* as I cannot currently think of a real coroutine-isque implementation */
            /* I'm going to hard code all the "resumes" */

            /*
              It's probably because I don't have the structure for a coroutine, as I'm trying
              to really simulate synchronous state.
              
              IE: I don't ever yield.
             */

            bool allow_advancement = true;
            if (current_stackframe->current_form_index > 1) {
                /* 
                   so we're going to manually code the waits by just checking what the previous
                   form was. Which is the simplest possible method. Because I use multiple stackframes,
                   this is actually still going to work the way I want it to. It just won't be *elegant*,
                   but I don't give a crap right now, I got a small headache trying to think of a short way to hammer
                   in coroutines and trying to read the lua source didn't super help.
                   
                   I'm not that smart okay :(, I'm trying to make a game anyways, not a scripting language.
                */

                struct lisp_form* last_form = current_stackframe->body.list.forms + (current_stackframe->current_form_index-1);
                allow_advancement = game_script_waiting_on_form(last_form);
            }

            if (allow_advancement) {
                /* sort of custom eval */
                struct lisp_form* first = lisp_list_nth(&current_stackframe->body, 0);

                if (lisp_form_symbol_matching(*first, string_literal("if"))) {
                    struct lisp_form* condition    = lisp_list_get_child(&current_stackframe->body, 1);
                    struct lisp_form* true_branch  = lisp_list_get_child(&current_stackframe->body, 2);
                    struct lisp_form* false_branch = lisp_list_get_child(&current_stackframe->body, 3);

                    /* crying in *side_effects*. Lots of bugs waiting to happen. */
                    struct lisp_form evaluated = game_script_evaluate_form(&scratch_arena, game_state, condition);

                    if (evaluated.type != LISP_FORM_NIL) {
                        game_script_push_stackframe(*true_branch);
                    } else {
                        if (false_branch)
                            game_script_push_stackframe(*false_branch);
                    }
                } else if (lisp_form_symbol_matching(*first, string_literal("progn"))) {
                    game_script_push_stackframe(lisp_list_sliced(current_stackframe->body, 1, -1));
                } else {
                    game_script_evaluate_form(&scratch_arena, game_state, current_stackframe->body.list.forms + current_stackframe->current_form_index);
                }

                current_stackframe->current_form_index += 1;
            }
        }
    }
}
