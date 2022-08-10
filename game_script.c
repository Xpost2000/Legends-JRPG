/*
  Game script,

  a lisp like stack based monster. The implementation don't look so great but this does work so I'm
  okay with it.
  
  There are lots of possible edge cases, that are not handled. Since this is a lisp and everything
  IS an expression, you can theoretically do lots of things (by putting somethings where you shouldn't...). However unlike a lisp we don't have
  a nicer debugger, so if you do something undefined, something might explode...
*/

/*
  NOTE: I don't really like the waiting implementation.
  
  I would ideally like to implement real continuations (and I'm partly through, as I think I have enough state to
  implement a form of continuations.)
  
  However it's probably a waste to do so, as I'm more looking for a limited usage of continuations like an await/async
  thing.
  
  I cheated and looked at Godot for some ideas, but I suspect it's simpler for them as Godot only has a couple of built-in nodes,
  and otherwise communicates exclusively through Godot objects. It also has a signal system which might make their await system
  a bit nicer.
  
  I don't have that.
  
  I'll just manually write waiting code for stuff I need then, it's not that big of a deal, though it dampers things somewhat...
 */

/* this is a pool, so that when we have multiple running scripts, we can have multiple waiting scripts. */
/* nevermind that fact that only one script runs at a time... */
#define MAX_ACTIVE_GAME_SCRIPT_WAITING_TIMERS (512)
struct game_script_wait_timer {
    f32 time;
    u8  active;
};

struct game_script_wait_timer game_script_timer_objects[MAX_ACTIVE_GAME_SCRIPT_WAITING_TIMERS] = {};

s32 game_script_create_timer(f32 time) {
    s32 free_index = 0;

    for (free_index; free_index < array_count(game_script_timer_objects); ++free_index) {
        struct game_script_wait_timer* timer = game_script_timer_objects + free_index;

        if (!timer->active) {
            timer->active = true;
            timer->time   = time;
            return free_index;
        }
    }

    return -1;
}

bool game_script_is_timer_done(s32 index) {
    struct game_script_wait_timer* timer = game_script_timer_objects + index;

    if (!timer->active)      return true;
    if (timer->time <= 0.0f) return true;

    return false;
}

void game_script_finish_timer(s32 index) {
    struct game_script_wait_timer* timer = game_script_timer_objects + index;
    timer->active = false;
}

typedef struct lisp_form (*game_script_function)(struct memory_arena*, struct game_state*, struct lisp_form*, s32);
struct game_script_function_builtin {
    char* name;
    game_script_function function;
};


/**/
#define GAME_SCRIPT_EXECUTION_STATE_STACK_SIZE (128) /* hopefully this doesn't stack overflow */
#define MAX_GAME_SCRIPT_RUNNING_SCRIPTS        (256)

struct game_script_execution_state {
    s32 current_form_index;

    struct {
        s32 timer_id;
    } awaiters;

    struct lisp_form body;
};

struct game_script_script_instance {
    struct game_script_execution_state stackframe[GAME_SCRIPT_EXECUTION_STATE_STACK_SIZE];
    s32 execution_stack_depth;

    bool active;

    struct lisp_form _scratch_list_wrapper;
};

local struct game_script_script_instance* running_scripts;

struct game_script_script_instance* new_script_instance(void) {
    for (s32 free_index = 0; free_index < MAX_GAME_SCRIPT_RUNNING_SCRIPTS; ++free_index) {
        struct game_script_script_instance* script = free_index + running_scripts;

        if (!script->active) {
            script->active = true;
            return script;
        }
    }

    return 0;
}

void game_script_initialize(struct memory_arena* arena) {
    running_scripts = memory_arena_push(arena, sizeof(*running_scripts) * MAX_GAME_SCRIPT_RUNNING_SCRIPTS);

    /* used for wrapping conditional clauses. As the execute scripts function wants lists of forms to execute. */
    for (s32 index = 0; index < MAX_GAME_SCRIPT_RUNNING_SCRIPTS; ++index) {
        running_scripts[index]._scratch_list_wrapper.type       = LISP_FORM_LIST;
        running_scripts[index]._scratch_list_wrapper.list.forms = memory_arena_push(arena, sizeof(struct lisp_form));
        running_scripts[index]._scratch_list_wrapper.list.count = 1;
    }
}

void game_script_deinitialize(void) {
    
}

void game_script_instance_push_stackframe(struct game_script_script_instance* script_state, struct lisp_form f) {
    assertion(script_state->execution_stack_depth < GAME_SCRIPT_EXECUTION_STATE_STACK_SIZE && "game_script stackoverflow!");
    script_state->stackframe[script_state->execution_stack_depth].current_form_index = 0;
    script_state->stackframe[script_state->execution_stack_depth].awaiters.timer_id  = -1;
    script_state->stackframe[script_state->execution_stack_depth++].body             = f;
}

void game_script_enqueue_form_to_execute(struct lisp_form f) {
    struct game_script_script_instance* new_script = new_script_instance();
    assertion(new_script && "ran out of script instances to handle!");
    game_script_instance_push_stackframe(new_script, f);
}

/* considering this is so patternized, is it worth writing a mini metaprogramming thing that just automates this? Who knows */

#define GAME_LISP_FUNCTION(name) struct lisp_form name ## __script_proc (struct memory_arena* arena, struct game_state* state, struct lisp_form* arguments, s32 argument_count)

GAME_LISP_FUNCTION(OBJ_ACTIVATIONS) {
    /* expect an object handle. Not checking right now */

    _debugprintf("Hi I was called with : %d args", argument_count);
    if (argument_count == 1) {
        struct game_script_typed_ptr object_ptr = game_script_object_handle_decode(arguments[0]);

        switch (object_ptr.type) {
            case GAME_SCRIPT_TARGET_TRIGGER: {
                struct trigger* trigger = (struct trigger*)object_ptr.ptr;
                _debugprintf("requested activation # for trigger: (%d)", trigger->activations);
                return lisp_form_integer(trigger->activations);
            } break;
                bad_case;
        }
    }

    return LISP_nil;
}
GAME_LISP_FUNCTION(random) {
    /* expect an object handle. Not checking right now */
    if (argument_count == 1) {
        if (arguments[0].type == LISP_FORM_NUMBER) {
            if (arguments[0].is_real) {
                f32 arg = 1.0f;

                lisp_form_get_f32(arguments[0], &arg);
                f32 return_value = random_float(&state->rng) * arg;

                return lisp_form_real(return_value);
            } else {
                s32 arg = 0;

                lisp_form_get_s32(arguments[0], &arg);
                s32 return_value = random_ranged_integer(&state->rng, 0, arg);

                return lisp_form_integer(return_value);
            }
        } else {
            return LISP_nil;
        }
    }

    return LISP_nil;
}
GAME_LISP_FUNCTION(DLG_NEXT) {
    _debugprintf("NOT IMPLEMENTED YET!");
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_IS_WEATHER) {
    struct lisp_form weather_type = arguments[0];
    if (lisp_form_symbol_matching(weather_type, string_literal("snow"))) {
        return lisp_form_produce_truthy_value_form(state->weather.features & WEATHER_SNOW);
    } else if (lisp_form_symbol_matching(weather_type, string_literal("rain"))) {
        return lisp_form_produce_truthy_value_form(state->weather.features & WEATHER_RAIN);
    } else if (lisp_form_symbol_matching(weather_type, string_literal("fog"))) {
        return lisp_form_produce_truthy_value_form(state->weather.features & WEATHER_FOGGY);
    } else if (lisp_form_symbol_matching(weather_type, string_literal("storm"))) {
        return lisp_form_produce_truthy_value_form(state->weather.features & WEATHER_STORMY);
    }

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
    struct lisp_form* str_name     = &arguments[0];
    struct lisp_form* str_sub_name = &arguments[1];

    string area_name     = string_literal("");
    string area_sub_name = string_literal("");

    area_name = str_name->string;
    if (argument_count == 2) {
        area_sub_name = str_sub_name->string;
    }
    _debugprintf("set_region name");
    _debug_print_out_lisp_code(str_name);
    _debug_print_out_lisp_code(str_sub_name);
    /* no check */
    game_attempt_to_change_area_name(state, area_name, area_sub_name);
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
    GAME_LISP_FUNCTION(GAME_IS_WEATHER),
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

        /* don't wait, make sure nothing bad happens okay? */
        if (lisp_form_symbol_matching(first_form, string_literal("wait"))) {
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

struct lisp_form game_script_evaluate_form(struct memory_arena* arena, struct game_state* state, struct lisp_form* form);

/* not checking if any of these forms were what you say they are. No identity theft please */
struct lisp_form game_script_evaluate_if_and_return_used_branch(struct memory_arena* arena, struct game_state* state, struct lisp_form* if_form) {
    struct lisp_form* condition    = lisp_list_get_child(if_form, 1);
    struct lisp_form* true_branch  = lisp_list_get_child(if_form, 2);
    struct lisp_form* false_branch = lisp_list_get_child(if_form, 3);

    struct lisp_form evaluated = game_script_evaluate_form(arena, state, condition);
    if (evaluated.type != LISP_FORM_NIL) {
        return *true_branch;
    } else {
        return *false_branch;
    }
}

struct lisp_form game_script_evaluate_cond_and_return_used_branch(struct memory_arena* arena, struct game_state* state, struct lisp_form* cond_form) {
    for (s32 index = 1; index < cond_form->list.count; ++index) {
        struct lisp_form* current_form = cond_form->list.forms + index;
        {
            struct lisp_form* condition_clause           = current_form->list.forms + 0;
            struct lisp_form  evaluated_condition_clause = game_script_evaluate_form(arena, state, condition_clause);

            struct lisp_form condition_clause_action     = lisp_list_sliced(*current_form, 1, -1);
            if (evaluated_condition_clause.type != LISP_FORM_NIL) {
                return condition_clause_action;
            }
        }
    }

    return LISP_nil;
}

struct lisp_form game_script_evaluate_case_and_return_used_branch(struct memory_arena* arena, struct game_state* state, struct lisp_form* case_form) {
    struct lisp_form* condition = lisp_list_get_child(case_form, 1);
    struct lisp_form  evaluated = game_script_evaluate_form(arena, state, condition);

    for (s32 index = 2; index < case_form->list.count; ++index) {
        struct lisp_form* current_form = case_form->list.forms + index;
        {
            struct lisp_form* condition_clause           = current_form->list.forms + 0;
            struct lisp_form  evaluated_condition_clause = game_script_evaluate_form(arena, state, condition_clause);

            struct lisp_form condition_clause_action     = lisp_list_sliced(*current_form, 1, -1);
            if (evaluated_condition_clause.type == LISP_FORM_T || lisp_form_check_equality(evaluated_condition_clause, evaluated)) {
                return condition_clause_action;
            }
        }
    }

    return LISP_nil;
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
                struct lisp_form used_branch = game_script_evaluate_if_and_return_used_branch(arena, state, form);
                return game_script_evaluate_form(arena, state, &used_branch);
            } else if (lisp_form_symbol_matching(form->list.forms[0], string_literal("case"))) {
                struct lisp_form used_branch = game_script_evaluate_case_and_return_used_branch(arena, state, form);
                return game_script_evaluate_form(arena, state, &used_branch);
            } else if (lisp_form_symbol_matching(form->list.forms[0], string_literal("cond"))) {
                struct lisp_form used_branch = game_script_evaluate_cond_and_return_used_branch(arena, state, form);
                return game_script_evaluate_form(arena, state, &used_branch);
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
bool game_script_waiting_on_form(struct game_script_script_instance* script_state, struct lisp_form* form_to_wait_on) {
    /* always a list */

    struct lisp_form* first = lisp_list_nth(form_to_wait_on, 0);

    if (first) {
        /* builtins */
        if (lisp_form_symbol_matching(*first, string_literal("if"))) {
            /* crying in *side_effects*. Lots of bugs waiting to happen. */
            struct lisp_form evaluated = game_script_evaluate_if_and_return_used_branch(&scratch_arena, game_state, form_to_wait_on);
            return game_script_waiting_on_form(script_state, &evaluated);
        } else if (lisp_form_symbol_matching(*first, string_literal("case"))) {
            struct lisp_form evaluated = game_script_evaluate_case_and_return_used_branch(&scratch_arena, game_state, form_to_wait_on);
            return game_script_waiting_on_form(script_state, &evaluated);
        } else if (lisp_form_symbol_matching(*first, string_literal("cond"))) {
            struct lisp_form evaluated = game_script_evaluate_cond_and_return_used_branch(&scratch_arena, game_state, form_to_wait_on);
            return game_script_waiting_on_form(script_state, &evaluated);
        } else if (lisp_form_symbol_matching(*first, string_literal("progn"))) {
            return game_script_waiting_on_form(script_state, lisp_list_nth(form_to_wait_on, -1));
        } else if (lisp_form_symbol_matching(*first, string_literal("wait"))) {
            struct game_script_execution_state* current_stackframe = script_state->stackframe + (script_state->execution_stack_depth-1);

            if (current_stackframe->awaiters.timer_id != -1) {
                if (game_script_is_timer_done(current_stackframe->awaiters.timer_id)) {
                    game_script_finish_timer(current_stackframe->awaiters.timer_id);
                    return true;
                }

                return false;
            }

            return true;
        }

        if (lisp_form_symbol_matching(*first, string_literal("game_message_queue"))) {
            if (global_popup_state.message_count == 0) {
                return true;
            } else {
                return false;
            }
        }
    }

    return true;
}

/*
  if it's a scalar (normal value), we'll assume it's the same as receiving a finished signal.
  
  if we get a (!game_script_coroutine_result_object! *status* *real-value*)
  
  TODO: does not handle multiple script enqueues yet... (is this even needed? Hope not.)
*/
void game_script_execute_awaiting_scripts(struct memory_arena* arena, struct game_state* state, f32 dt) {
    for (s32 script_instance_index = 0; script_instance_index < MAX_GAME_SCRIPT_RUNNING_SCRIPTS; ++script_instance_index) {
        struct game_script_script_instance* script_instance = running_scripts + script_instance_index;

        if (!script_instance->active)
            continue;

        if (script_instance->execution_stack_depth <= 0) {
            script_instance->active = false;
        } else {
            struct game_script_execution_state* current_stackframe = script_instance->stackframe + (script_instance->execution_stack_depth-1);
            _debugprintf("current stack frame(script: %d) (%d)", script_instance_index, script_instance->execution_stack_depth);

            if (current_stackframe->current_form_index >= current_stackframe->body.list.count) {
                script_instance->execution_stack_depth -= 1;
            } else {
                /*
                  It's probably because I don't have the structure for a coroutine, as I'm trying
                  to really simulate synchronous state.
              
                  IE: I don't ever yield.
                */

                bool allow_advancement = true;
                if (current_stackframe->current_form_index >= 1) {
                    /* 
                       so we're going to manually code the waits by just checking what the previous
                       form was. Which is the simplest possible method. Because I use multiple stackframes,
                       this is actually still going to work the way I want it to. It just won't be *elegant*,
                       but I don't give a crap right now, I got a small headache trying to think of a short way to hammer
                       in coroutines and trying to read the lua source didn't super help.
                   
                       I'm not that smart okay :(, I'm trying to make a game anyways, not a scripting language.
                    */

                    struct lisp_form* last_form = lisp_list_nth(&current_stackframe->body, current_stackframe->current_form_index-1);
                    allow_advancement = game_script_waiting_on_form(script_instance, last_form);
                }
            

                if (allow_advancement) {
                    /* sort of custom eval */
                    struct lisp_form* current_form = lisp_list_nth(&current_stackframe->body, current_stackframe->current_form_index);
                    struct lisp_form* first        = lisp_list_nth(current_form, 0);

                    if (first) {
                        if (lisp_form_symbol_matching(*first, string_literal("if"))) {
                            struct lisp_form used_branch = game_script_evaluate_if_and_return_used_branch(&scratch_arena, game_state, current_form);
                            script_instance->_scratch_list_wrapper.list.forms[0] = used_branch;
                            game_script_instance_push_stackframe(script_instance, script_instance->_scratch_list_wrapper);
                        } else if (lisp_form_symbol_matching(*first, string_literal("case"))) {
                            struct lisp_form used_branch = game_script_evaluate_case_and_return_used_branch(&scratch_arena, game_state, current_form);
                            game_script_instance_push_stackframe(script_instance, used_branch);
                        } else if (lisp_form_symbol_matching(*first, string_literal("cond"))) {
                            struct lisp_form used_branch = game_script_evaluate_cond_and_return_used_branch(&scratch_arena, game_state, current_form);
                            game_script_instance_push_stackframe(script_instance, used_branch);
                        } else if (lisp_form_symbol_matching(*first, string_literal("progn"))) {
                            game_script_instance_push_stackframe(script_instance, lisp_list_sliced(*current_form, 1, -1));
                        } else {
                            /* wait is not evaluated. It's  */
                            if (lisp_form_symbol_matching(*first, string_literal("wait"))) {
                                /* 
                                   NOTE: it's really more rather the entire execution state is waiting, not
                                   just the stackframe... Oh whatever
                                */
                                f32 time_limit = 1.0f;
                                struct lisp_form* wait_param = lisp_list_nth(current_form, 1);

                                if (wait_param && lisp_form_get_f32(*wait_param, &time_limit));
                                current_stackframe->awaiters.timer_id = game_script_create_timer(time_limit);
                            } else {
                                _debugprintf("executing?");
                                _debug_print_out_lisp_code(current_form);
                                game_script_evaluate_form(&scratch_arena, game_state, current_form);
                            }
                        }
                    } else {
                        _debugprintf("cannot execute");
                        _debug_print_out_lisp_code(current_form);
                        /* no execute */
                    }

                    current_stackframe->current_form_index += 1;
                }
            }
        }
    }
}

void game_script_run_all_timers(f32 dt) {
    for (s32 timer_index = 0; timer_index < MAX_ACTIVE_GAME_SCRIPT_WAITING_TIMERS; ++timer_index) {
        struct game_script_wait_timer* timer  = game_script_timer_objects + timer_index;
        timer->time                          -= dt;
    }
}
