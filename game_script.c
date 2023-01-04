#define NO_SCRIPT (-1)
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

        /* 
           while I should theoretically be waiting for an entity
           to do the action I request, we're only waiting until an
           entity is no longer doing an action.
           
           I'll remedy this with a cutscene_setup proc which cancels
           all current actions for entities.
        */
        entity_id entity_id;
    } awaiters;

    struct lisp_form body;
};

struct game_script_script_instance {
    struct game_script_execution_state stackframe[GAME_SCRIPT_EXECUTION_STATE_STACK_SIZE];
    s32 execution_stack_depth;
    s32 id;

    bool active;

    struct lisp_form bindings[CONTEXT_BINDING_ID_COUNT];
    /* I could check for nil, but this is cheaper and easier to write. */
    bool             bindings_present[CONTEXT_BINDING_ID_COUNT];

    struct lisp_form _scratch_list_wrapper;
};

local struct game_script_script_instance* running_scripts;
/* HACK: for reference with certain functions that behave differently when not in an eval environment. */
local s32                                 running_script_index = NO_SCRIPT;

void game_script_instance_set_contextual_binding(struct game_script_script_instance* instance, s32 context_binding_id, struct lisp_form new_value) {
    if (new_value.type == LISP_FORM_NIL) {
        instance->bindings_present[context_binding_id] = false;
    } else {
        instance->bindings_present[context_binding_id] = true;
    }

    instance->bindings[context_binding_id] = new_value;
}

void game_script_instance_clear_contextual_bindings(struct game_script_script_instance* instance) {
    for (s32 binding_id = 0; binding_id < array_count(instance->bindings_present); ++binding_id) {
        instance->bindings_present[binding_id] = false;
    }
}

struct game_script_script_instance* new_script_instance(s32 enqueue_id) {
    if (enqueue_id != DO_NOT_CARE) {
        for (s32 script_index = 0; script_index < MAX_GAME_SCRIPT_RUNNING_SCRIPTS; ++script_index) {
            struct game_script_script_instance* script = script_index + running_scripts;

            if (script->active && script->id == enqueue_id) {
#if 0
                _debugprintf("Shall not enqueue! Script of the same kind is already running! (%d)", enqueue_id);
#endif
                return NULL;
            }
        }
    }

    for (s32 free_index = 0; free_index < MAX_GAME_SCRIPT_RUNNING_SCRIPTS; ++free_index) {
        struct game_script_script_instance* script = free_index + running_scripts;

        if (!script->active) {
            script->active = true;
            script->id     = enqueue_id;
            return script;
        }
    }

    assertion(false && "Run out of script instances!");
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
    script_state->stackframe[script_state->execution_stack_depth].awaiters.entity_id = (entity_id){};
    script_state->stackframe[script_state->execution_stack_depth++].body             = f;
    _debug_print_out_lisp_code(&f);
}

struct game_script_script_instance* game_script_enqueue_form_to_execute_ex(struct lisp_form f, s32 enqueue_id) {
    struct game_script_script_instance* new_script = new_script_instance(enqueue_id);

    if (new_script) {
        game_script_instance_push_stackframe(new_script, f);
        _debugprintf("enqueued");
        _debug_print_out_lisp_code(&f);
        return new_script;
    }

    return NULL;
}

struct game_script_script_instance* game_script_enqueue_form_to_execute(struct lisp_form f) {
    return game_script_enqueue_form_to_execute_ex(f, DO_NOT_CARE);
}

/* considering this is so patternized, is it worth writing a mini metaprogramming thing that just automates this? Who knows */
/* NOTE: Not type checking fyi, while the game is moddable, making it fault tolerant is really annoying. So it will crash. */
/* only used to pretty print required argument checks, which is the only "safe-check" that I do */
local string numeric_strings_table_to_nineteen[] = {
    string_literal("zero"),
    string_literal("one"),
    string_literal("two"),
    string_literal("three"),
    string_literal("four"),
    string_literal("five"),
    string_literal("six"),
    string_literal("seven"),
    string_literal("eight"),
    string_literal("nine"),
    string_literal("ten"),
    string_literal("eleven"),
    string_literal("twelve"),
    string_literal("thirteen"),
    string_literal("fourteen"),
    string_literal("fifteen"),
    string_literal("sixteen"),
    string_literal("seventeen"),
    string_literal("eighteen"),
    string_literal("nineteen"),
};
char* _script_error_number_to_string(s32 n) {
    if (n >= array_count(numeric_strings_table_to_nineteen)) {
        return "[too high!]";
    }

    return numeric_strings_table_to_nineteen[n].data;
}

/*
  Macro might need to be evolved at some point to be more fault tolerant...
  Or we might need a "script" error log, to show up during debug mode so I can actually
  figure out what the hell is wrong
*/
#define Script_Error(FUNCTION, ERRORSTRING, rest...)            \
    do {                                                        \
        _debugprintf(#FUNCTION " :error: " ERRORSTRING, ##rest); \
        return LISP_nil;                                        \
    } while(0); 

#define Required_Minimum_Argument_Count(FUNCTION, COUNT)                        \
    do {                                                                \
        if (argument_count < COUNT) {                                   \
            Script_Error(FUNCTION, " requires %s arguments!", _script_error_number_to_string(COUNT)); \
        }                                                               \
    } while(0);
#define Required_Argument_Count(FUNCTION, COUNT)                        \
    do {                                                                \
        if (argument_count < COUNT) {                                   \
            Script_Error(FUNCTION, " requires %s arguments!", _script_error_number_to_string(COUNT)); \
        } else if (argument_count > COUNT) {                            \
            Script_Error(FUNCTION, " has %s too many arguments!", _script_error_number_to_string(argument_count - COUNT)); \
        }                                                               \
    } while(0);
#define Fatal_Script_Error(x) assertion(x)
/*
  (follow_path GSO_HANDLE PathForm)
  
  PathForm -> '(left/down/up/right ...)
  PathForm -> '(start-x start-y) '(end-x end-y)
*/
#include "game_script_procs.c"

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

bool game_script_look_up_contextual_binding(struct lisp_form name, struct lisp_form* output) {
    if (running_script_index == NO_SCRIPT) { /* contextual binding only makes sense with a queued script instance which can store it's own context. */
        return false;
    }

    struct game_script_script_instance* current_running_script = &running_scripts[running_script_index];

    {
        if (lisp_form_symbol_matching(name, string_literal("self"))           && current_running_script->bindings_present[CONTEXT_BINDING_SELF]) {
            *output = current_running_script->bindings[CONTEXT_BINDING_SELF];
            return true;
        } else if (lisp_form_symbol_matching(name, string_literal("toucher")) && current_running_script->bindings_present[CONTEXT_BINDING_TOUCHER]) {
            *output = current_running_script->bindings[CONTEXT_BINDING_TOUCHER];
            return true;
        } else if (lisp_form_symbol_matching(name, string_literal("hitter"))  && current_running_script->bindings_present[CONTEXT_BINDING_HITTER]) {
            *output = current_running_script->bindings[CONTEXT_BINDING_HITTER];
            return true;
        }
    }

    return false;
}

bool game_script_look_up_dictionary_value(struct lisp_form name, struct lisp_form* output) {
    struct game_variable* variable = lookup_game_variable(name.string, false);

    if (variable) {
        *output = lisp_form_integer(variable->value);
        return true;
    } else {
        *output = LISP_nil;
        return false;
    }
}

/* TODO should allow us to flexibly change types though. */
struct lisp_form game_script_set_dictionary_value(struct lisp_form name, struct lisp_form value) {
    struct game_variable* variable = lookup_game_variable(name.string, false);

    if (!variable) {
        variable = lookup_game_variable(name.string, true);
    }

    lisp_form_get_s32(value, &variable->value);

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
        } else if (lisp_form_symbol_matching(first_form, string_literal("equal")) || lisp_form_symbol_matching(first_form, string_literal("="))) {
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
        } else if (lisp_form_symbol_matching(first_form, string_literal("not"))) {
            assertion(form->list.count == 2 && "NOT is a unary operator");
            struct lisp_form child_first = game_script_evaluate_form(arena, state, form->list.forms + 1);
            if (child_first.type == LISP_FORM_NIL || (child_first.type == LISP_FORM_LIST && child_first.list.count == 0)) {
                evaled_boolean_result = true;
            } else {
                evaled_boolean_result = false;
            }
            is_boolean_operator = true;
        } else if (
            lisp_form_symbol_matching(first_form, string_literal("<")) ||
            lisp_form_symbol_matching(first_form, string_literal(">")) ||
            lisp_form_symbol_matching(first_form, string_literal("<=")) ||
            lisp_form_symbol_matching(first_form, string_literal(">="))
        ) {
            assertion(form->list.count == 3 && "COMPARISONS ARE BINARY");
            struct lisp_form child_first  = game_script_evaluate_form(arena, state, form->list.forms + 1);
            struct lisp_form child_second = game_script_evaluate_form(arena, state, form->list.forms + 2);

            /* TODO: For now these are integer only comparisons */
            s32 child_first_v = 0;
            s32 child_second_v = 0;
            lisp_form_get_s32(child_first, &child_first_v);
            lisp_form_get_s32(child_second, &child_second_v);

            if (lisp_form_symbol_matching(first_form, string_literal("<"))) {
                evaled_boolean_result = child_first_v < child_second_v;
            } else if (lisp_form_symbol_matching(first_form, string_literal(">"))) {
                evaled_boolean_result = child_first_v > child_second_v;
            } else if (lisp_form_symbol_matching(first_form, string_literal("<="))) {
                evaled_boolean_result = child_first_v <= child_second_v;
            } else if (lisp_form_symbol_matching(first_form, string_literal(">="))) {
                evaled_boolean_result = child_first_v >= child_second_v;
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
        if (false_branch) {
            return *false_branch;
        } else {
            return LISP_nil;
        }
    }
}

struct lisp_form game_script_evaluate_cond_and_return_used_branch(struct memory_arena* arena, struct game_state* state, struct lisp_form* cond_form) {
    for (s32 index = 1; index < cond_form->list.count; ++index) {
        struct lisp_form* current_form = cond_form->list.forms + index;
        {
            struct lisp_form* condition_clause           = current_form->list.forms + 0;
            _debugprintf("COND");
            struct lisp_form  evaluated_condition_clause = game_script_evaluate_form(arena, state, condition_clause);
            _debug_print_out_lisp_code(&evaluated_condition_clause);

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
            struct lisp_form* condition_clause           = current_form->list.forms;
            struct lisp_form  evaluated_condition_clause = game_script_evaluate_form(arena, state, condition_clause);

            struct lisp_form condition_clause_action     = lisp_list_sliced(*current_form, 1, -1);
            if (evaluated_condition_clause.type == LISP_FORM_T || lisp_form_check_equality(evaluated_condition_clause, evaluated)) {
                _debugprintf("CHECK\n");
                _debug_print_out_lisp_code(condition);
                _debug_print_out_lisp_code(&evaluated);
                _debug_print_out_lisp_code(&evaluated_condition_clause);
                _debugprintf("good case\n");
                _debug_print_out_lisp_code(&condition_clause_action);
                return condition_clause_action;
            } else {
                _debugprintf("CHECK\n");
                _debug_print_out_lisp_code(condition);
                _debug_print_out_lisp_code(&evaluated);
                _debug_print_out_lisp_code(&evaluated_condition_clause);
                _debugprintf("bad case\n");
            }
        }
    }

    return LISP_nil;
}

struct lisp_form game_script_evaluate_form(struct memory_arena* arena, struct game_state* state, struct lisp_form* form) {
    _debugprintf("begin");
    _debug_print_out_lisp_code(form);
    if (form) {
        if (form->quoted) {
            _debugprintf("quoted eval");
            return (*form);
        }

        /* evalute for standard forms, like if mostly... Actually only if */
        if (form->type == LISP_FORM_LIST) {
            _debugprintf("LIST FORM TO EVAL");
            _debug_print_out_lisp_code(form);
            if (lisp_form_symbol_matching(form->list.forms[0], string_literal("if"))) {
                struct lisp_form used_branch = game_script_evaluate_if_and_return_used_branch(arena, state, form);
                return game_script_evaluate_form(arena, state, &used_branch);
            } else if (lisp_form_symbol_matching(form->list.forms[0], string_literal("case"))) {
                struct lisp_form used_branch = game_script_evaluate_case_and_return_used_branch(arena, state, form);
                _debug_print_out_lisp_code(&used_branch);
                /* evaluate branch like progn... */
                struct lisp_form last;
                for (s32 index = 0; index < used_branch.list.count; ++index) {
                    last = game_script_evaluate_form(arena, state, &used_branch.list.forms[index]);
                }
                return last;
            } else if (lisp_form_symbol_matching(form->list.forms[0], string_literal("cond"))) {
                struct lisp_form used_branch = game_script_evaluate_cond_and_return_used_branch(arena, state, form);
                /* evaluate branch like progn... */
                struct lisp_form last;
                for (s32 index = 0; index < used_branch.list.count; ++index) {
                    last = game_script_evaluate_form(arena, state, &used_branch.list.forms[index]);
                }
                return last;
            } else if (lisp_form_symbol_matching(form->list.forms[0], string_literal("progn"))) {
                _debugprintf("Found a progn");
                struct lisp_form result = {};

                for (s32 index = 1; index < form->list.count; ++index) {
                    result = game_script_evaluate_form(arena, state, form->list.forms+index);
                }

                return result;
            } else if (lisp_form_symbol_matching(form->list.forms[0], string_literal("setvar"))) {
                /* no error checking :/ */
                struct lisp_form* variable_name = &form->list.forms[1];
                struct lisp_form* expression    = &form->list.forms[2];
                struct lisp_form evaluated = game_script_evaluate_form(arena, state, expression);

                s32 s32_result;
                if (lisp_form_get_s32(evaluated, &s32_result)) {
                    string var_name_as_string;
                    if (lisp_form_get_string(*variable_name, &var_name_as_string)) {
                        struct game_variable* var = lookup_game_variable(var_name_as_string, true);
                        if (!var) {
                            _debugprintf("this is an error!");
                        } else {
                            var->value = s32_result;
                            _debugprintf("Successfully set var");
                        }
                    }
                } else {
                    _debugprintf("please use an integer!");
                }
            } else if (lisp_form_symbol_matching(form->list.forms[0], string_literal("incvar"))) {
                struct lisp_form* variable_name = &form->list.forms[1];
                string var_name_as_string;
                if (lisp_form_get_string(*variable_name, &var_name_as_string)) {
                    struct game_variable* var = lookup_game_variable(var_name_as_string, false);
                    if (!var) {
                        _debugprintf("this is an error!");
                    } else {
                        var->value += 1;
                    }
                }
            } else if (lisp_form_symbol_matching(form->list.forms[0], string_literal("decvar"))) {
                struct lisp_form* variable_name = &form->list.forms[1];
                string var_name_as_string;
                if (lisp_form_get_string(*variable_name, &var_name_as_string)) {
                    struct game_variable* var = lookup_game_variable(var_name_as_string, false);
                    if (!var) {
                        _debugprintf("this is an error!");
                    } else {
                        var->value -= 1;
                    }
                }
            } else {
                struct lisp_form result = LISP_nil;
                /* NOTE: 
                   builtins technically evalute their children on their own time... 
                   I should change this...
                */

                if (!handle_builtin_game_script_functions(arena, state, form, &result)) {
                    _debugprintf("calling from function table");
                    game_script_function function = lookup_script_function(form->list.forms[0].string);
                    _debugprintf("Calling function %.*s", form->list.forms[0].string.length, form->list.forms[0].string.data);

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
                struct lisp_form result = LISP_nil;

                _debug_print_out_lisp_code(form);
                if (!game_script_look_up_contextual_binding(*form, &result)) {
                    _debugprintf("failed to find contextual binding.");
                    if (!game_script_look_up_dictionary_value(*form, &result)) {
                        _debugprintf("failed to find dictionary variable value? (%.*s)", form->string.length, form->string.data);
                    }
                }

                return result;
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
#if 0
    assertion(object_handle.list.count == 2 && "lisp representations of object handles should be like (TYPE ID)");
#endif
    struct game_script_typed_ptr result = {};
#if 0 
    _debugprintf("Decoding object handle: ");
    _debug_print_out_lisp_code(&object_handle);
#endif

    struct lisp_form* type_discriminator_form = lisp_list_nth(&object_handle, 0);
    struct lisp_form* id_form                 = lisp_list_nth(&object_handle, 1);

    s32 type_id = 0;
    s32 real_id = 0;


    bool found_any = false;
    for (s32 index = 0; index < array_count(entity_game_script_target_type_name); ++index) {
        if (lisp_form_symbol_matching(*type_discriminator_form, entity_game_script_target_type_name[index])) {
            type_id   = index;
            found_any = true;
            break;
        }
    }

    assertion(found_any && "Unknown script handle... Crashing, fix the script!");

    result.type = type_id;

    {
        struct level_area* area = &game_state->loaded_area;
        switch (type_id) {
            case GAME_SCRIPT_TARGET_LIGHT: {
                if (!lisp_form_get_s32(*id_form, &real_id)) {
                    return result;
                }
                result.ptr = area->lights + real_id;
            } break;
            case GAME_SCRIPT_TARGET_TRANSITION_TRIGGER: {
                if (!lisp_form_get_s32(*id_form, &real_id)) {
                    return result;
                }
                result.ptr = area->trigger_level_transitions + real_id;
            } break;
            case GAME_SCRIPT_TARGET_TRIGGER: {
                if (!lisp_form_get_s32(*id_form, &real_id)) {
                    return result;
                }
                result.ptr = area->script_triggers + real_id;
            } break;
            case GAME_SCRIPT_TARGET_ENTITY: {
                /*
                  TODO:
                  Update this for multiple Party Members.

                  For the player form

                  accept the following forms

                  (entity player INDEX)    ;; This is obvious...
                  (entity player BASENAME) ;; All entities *should* have unique basenames in the player party
                */
                if (lisp_form_symbol_matching(*id_form, string_literal("player"))) {
                    _debugprintf("found player");
                    switch (object_handle.list.count) {
                        case 2: {
                            result.entity_id = game_state->party_members[game_state->leader_index]; /* this is the "active player leader" */
                        } break;
                        case 3: {
#if 0
                            struct lisp_form* third_argument = lisp_list_nth(&object_handle, 2);
                            s32    player_index       = 0;
                            string player_basename_id = {};
                            if (lisp_form_get_s32(*third_argument, &player_index)) {
                                result.entity_id = ; /* party member index */
                            } else {
                                result.entity_id = ; /* party member find based off of basename */
                            }
#else
                            unimplemented("Multiple party member support");
#endif
                        } break;
                            bad_case;
                    }
                } else {
                    string script_name_string;

                    if (lisp_form_get_s32(*id_form, &real_id)) {
                        _debugprintf("looking up id?");
                        if (real_id < GAME_MAX_PERMENANT_ENTITIES) {
                            result.entity_id = entity_list_get_id(&game_state->permenant_entities, real_id);
                        } else {
                            real_id -= GAME_MAX_PERMENANT_ENTITIES;
                            result.entity_id = entity_list_get_id(&game_state->loaded_area.entities, real_id);
                        }
                    } else if (lisp_form_get_string(*id_form, &script_name_string)) {
#if 0
                        _debugprintf("Looking up string name?");
#endif

                        if (cutscene_viewing_separate_area()) {
                            result.entity_id = entity_list_find_entity_id_with_scriptname(&cutscene_view_area()->entities, script_name_string);
                        } else {
                            result.entity_id = entity_list_find_entity_id_with_scriptname(&game_state->permenant_entities, script_name_string);

                            if (result.entity_id.index == 0) {
                                result.entity_id = entity_list_find_entity_id_with_scriptname(&game_state->loaded_area.entities, script_name_string);
                            }
                        }

                        if (result.entity_id.index == 0) {
                            _debugprintf("No entity with scriptname \"%.*s\" found.", script_name_string.length, script_name_string.data);
                        }
                    } else {
                        assertion(false && "Invalid id type for entity");
                    }
                }
                return result;
            } break;
            case GAME_SCRIPT_TARGET_SAVEPOINT: {
                if (!lisp_form_get_s32(*id_form, &real_id)) {
                    return result;
                }
                result.ptr = area->savepoints + real_id;
                return result;
            } break;
            case GAME_SCRIPT_TARGET_CHEST: {
                if (!lisp_form_get_s32(*id_form, &real_id)) {
                    return result;
                }
                result.ptr = area->chests + real_id;
                return result;
            } break;
            case GAME_SCRIPT_TARGET_SCRIPTABLE_LAYER: {
                if (!lisp_form_get_s32(*id_form, &real_id)) {
                    return result;
                }
                _debugprintf("Script layer : %d", real_id);
                result.ptr = area->scriptable_layer_properties + real_id;
                return result;
            } break;
        }
    }

    return result;
}

/* This assumes form_to_wait_on has already begun it's action. */
bool game_script_waiting_on_form(struct game_script_script_instance* script_state, struct lisp_form* form_to_wait_on) {
    /* always a list */
    struct game_script_execution_state* current_stackframe = script_state->stackframe + (script_state->execution_stack_depth-1);

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
            if (current_stackframe->awaiters.timer_id != -1) {
                if (game_script_is_timer_done(current_stackframe->awaiters.timer_id)) {
                    game_script_finish_timer(current_stackframe->awaiters.timer_id);
                    return true;
                }

                return false;
            }

            return true;
        }

        if (lisp_form_symbol_matching(*first, string_literal("follow_path"))) {
            struct entity* waiting_ent = game_dereference_entity(game_state, current_stackframe->awaiters.entity_id);

            if (waiting_ent) {
                if (waiting_ent->ai.current_action == ENTITY_ACTION_NONE) {
                    return true;
                } else {
                    return false;
                }
            }
        }

        if (lisp_form_symbol_matching(*first, string_literal("game_message_queue"))) {
            if (global_popup_state.message_count == 0) {
                return true;
            } else {
                return false;
            }
        }

        if (lisp_form_symbol_matching(*first, string_literal("game_open_conversation"))) {
            if (game_state->is_conversation_active) {
                return false;
            } else {
                return true;
            }
        }
    }

    return true;
}

void game_script_clear_all_awaited_scripts(void) {
    for (s32 free_index = 0; free_index < MAX_GAME_SCRIPT_RUNNING_SCRIPTS; ++free_index) {
        struct game_script_script_instance* script = free_index + running_scripts;
        script->active = false;
    }
}

void game_script_execute_awaiting_scripts(struct memory_arena* arena, struct game_state* state, f32 dt) {
    running_script_index = -1;

    for (s32 script_instance_index = 0; script_instance_index < MAX_GAME_SCRIPT_RUNNING_SCRIPTS; ++script_instance_index) {
        struct game_script_script_instance* script_instance = running_scripts + script_instance_index;
        running_script_index = script_instance_index;

        if (!script_instance->active)
            continue;

        if (script_instance->execution_stack_depth <= 0) {
            game_script_instance_clear_contextual_bindings(script_instance);
            script_instance->active = false;

            /* BAD? */
            /* if (cutscene_active()) { */
            /*     _debugprintf("Cutscene ending"); */
            /*     cutscene_stop(); */
            /* } */
        } else {
            struct game_script_execution_state* current_stackframe = script_instance->stackframe + (script_instance->execution_stack_depth-1);
#if 0
            _debugprintf("current stack frame(script: %d) (%d)", script_instance_index, script_instance->execution_stack_depth);
            _debug_print_out_lisp_code(&current_stackframe->body);
#endif

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
#if 0
                    _debugprintf("Can I advance? %d", allow_advancement);
#endif
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
                                /*
                                  TODO:
                                  handle cutscene specific forms such as "together" which are
                                  like progns but each executing together and waiting for a sum finish...
                                */
                                _debugprintf("executing?");
                                _debug_print_out_lisp_code(current_form);
                                game_script_evaluate_form(&scratch_arena, game_state, current_form);
                            }
                        }
                    } else {
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
