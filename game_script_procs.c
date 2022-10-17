#define GAME_LISP_FUNCTION(name) struct lisp_form name ## __script_proc (struct memory_arena* arena, struct game_state* state, struct lisp_form* arguments, s32 argument_count)


/*
  (follow_path GSO_HANDLE PathForm)
  
  PathForm -> '(left/down/up/right ...)
  PathForm -> '(start-x start-y) '(end-x end-y)
 */
GAME_LISP_FUNCTION(FOLLOW_PATH) {
    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    assertion(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "Whoops! Don't know how to make non-entities follow paths!");

    if (argument_count < 2) {
        return LISP_nil;
    }

    v2f32* path       = 0;
    s32    path_count = 0;

    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    assertion(target_entity && "no entity?");

    if (argument_count == 2) {
        /* manually provided path form */
        path = memory_arena_push(&scratch_arena, 0);

        struct lisp_form* defined_path = &arguments[1];
        assertion(defined_path->type == LISP_FORM_LIST && "this is not a path I guess.");

        f32 x_cursor = target_entity->position.x/TILE_UNIT_SIZE;
        f32 y_cursor = target_entity->position.y/TILE_UNIT_SIZE;

        memory_arena_push(&scratch_arena, sizeof(v2f32));
        path[path_count].x = x_cursor;
        path[path_count++].y = y_cursor;

        /* A sort of RLE to make the path look less stupid */
#define Compress_Path(direction)                                        \
        s32 count = 0;                                                  \
        for (; direction_index < defined_path->list.count; ++direction_index) { \
            sym = lisp_list_nth(defined_path, direction_index);         \
            if (lisp_form_symbol_matching(*sym, string_literal(direction))) count++; \
            else {direction_index -= 1; break;}                         \
        }                                                               \
        

        for (s32 direction_index = 0; direction_index < defined_path->list.count; ++direction_index) {
            struct lisp_form* sym = lisp_list_nth(defined_path, direction_index);

            if (lisp_form_symbol_matching(*sym, string_literal("right"))) {
                Compress_Path("right");
                x_cursor += 1 * count;
            } else if (lisp_form_symbol_matching(*sym, string_literal("left"))) {
                Compress_Path("left");
                x_cursor -= 1 * count;
            } else if (lisp_form_symbol_matching(*sym, string_literal("down"))) {
                Compress_Path("down");
                y_cursor += 1 * count;
            } else if (lisp_form_symbol_matching(*sym, string_literal("up"))) {
                Compress_Path("up");
                y_cursor -= 1 * count;
            }

            memory_arena_push(&scratch_arena, sizeof(v2f32));
            path[path_count].x = x_cursor;
            path[path_count++].y = y_cursor;
        }

#undef Compress_Path
    } else if (argument_count == 3) {
        /* TODO: revise the usage of this. Might look correct. */
        v2f32 start;
        v2f32 end;

        lisp_form_get_v2f32(arguments[1], &start);
        lisp_form_get_v2f32(arguments[2], &end);

        struct navigation_path path_results = navigation_path_find(&scratch_arena, &state->loaded_area, start, end);
        path       = path_results.points;
        path_count = path_results.count;
    }

    entity_combat_submit_movement_action(target_entity, path, path_count);

    if (running_script_index != NO_SCRIPT) {
        struct game_script_script_instance* script_instance    = running_scripts + running_script_index;
        struct game_script_execution_state* current_stackframe = script_instance->stackframe + (script_instance->execution_stack_depth-1);
        current_stackframe->awaiters.entity_id = ptr.entity_id;
    }

    return LISP_nil;
}

GAME_LISP_FUNCTION(ENTITY_LOOK_AT) {
    /* The only use case right now is looking at other entities */
    if (argument_count < 2) {
        _debugprintf("entity_look_at requires two arguments");
        return LISP_nil;
    }
    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    assertion(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "This only works on entities!");
    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    assertion(target_entity && "no entity?");

    if (lisp_form_symbol_matching(arguments[1], string_literal("left"))) {
        target_entity->facing_direction = DIRECTION_LEFT;
    } else if (lisp_form_symbol_matching(arguments[1], string_literal("right"))) {
        target_entity->facing_direction = DIRECTION_RIGHT;
    } else if (lisp_form_symbol_matching(arguments[1], string_literal("down"))) {
        target_entity->facing_direction = DIRECTION_DOWN;
    } else if (lisp_form_symbol_matching(arguments[1], string_literal("up"))) {
        target_entity->facing_direction = DIRECTION_UP;
    } else {
        struct game_script_typed_ptr ptr2 = game_script_object_handle_decode(arguments[1]);
        assertion(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "This only works on entities!");
        struct entity* target_entity2 = game_dereference_entity(state, ptr2.entity_id);
        assertion(target_entity2 && "no entity?");

        entity_look_at(target_entity, target_entity2->position);
    }

    return LISP_nil;
}

GAME_LISP_FUNCTION(ENTITY_SPEAK) {
    if (argument_count < 2) {
        _debugprintf("entity_speak requires two arguments");
        return LISP_nil;
    }
    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    assertion(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "This only works on entities!");
    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    assertion(target_entity && "no entity?");

    string text;
    if (lisp_form_get_string(arguments[1], &text)) {
        passive_speaking_dialogue_push(ptr.entity_id, text, MENU_FONT_COLOR_GOLD);
    } else {
        _debugprintf("entity speak needs a string!");
    }
    return LISP_nil;
}

GAME_LISP_FUNCTION(KILL_ALL) {
    /* or any player entity rather, so we'll check for friendly */
    /*
      NOTE:
      need a better way to iterate all combat enemies. Could build specific iterator based off of combat state

      Consider it during cleanup time. Or don't.
    */
    struct game_state_combat_state* combat_state = &state->combat_state;

    struct entity* active_combatant_entity = game_dereference_entity(state, combat_state->participants[combat_state->active_combatant]);

    for (s32 index = combat_state->active_combatant; index < combat_state->count; ++index) {
        {
            struct entity* entity = game_dereference_entity(state, combat_state->participants[index]);

            if (entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
                continue;
            } else {
                if (entity->ai.flags & ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER) {
                    entity_do_physical_hurt(entity, 99999);
                    battle_notify_killed_entity(combat_state->participants[index]);
                }
            }
        }
    }

    return LISP_nil;
}

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

GAME_LISP_FUNCTION(OPEN_SHOP) {
    if (argument_count == 1) {
        string shop_name = {};
        assert(lisp_form_get_string(arguments[0], &shop_name));
        game_begin_shopping(shop_name);
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
GAME_LISP_FUNCTION(GAME_INPUT_ENABLED) {
    return lisp_form_produce_truthy_value_form(!disable_game_input);
}
GAME_LISP_FUNCTION(GAME_DISABLE_INPUT) {
    disable_game_input = true;
    return lisp_form_produce_truthy_value_form(!disable_game_input);
}
GAME_LISP_FUNCTION(GAME_ENABLE_INPUT) {
    disable_game_input = false;
    return lisp_form_produce_truthy_value_form(!disable_game_input);
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

GAME_LISP_FUNCTION(GAME_OPEN_CONVERSATION) {
    assertion(argument_count >= 1 && "need at least one argument");

    string conversation_path_partial = {};
    lisp_form_get_string(arguments[0], &conversation_path_partial);
    string conversation_path = format_temp_s("dlg/%.*s.txt", conversation_path_partial.length, conversation_path_partial.data);
    game_open_conversation_file(state, conversation_path);

    if (argument_count == 2) {
        struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[1]);
        struct entity* to_speak = game_dereference_entity(state, ptr.entity_id);

        v2f32 focus_point = to_speak->position;
        camera_set_point_to_interpolate(&state->camera, focus_point);
    }

return LISP_nil;
}

GAME_LISP_FUNCTION(GAME_START_FIGHT_WITH) {
    for (unsigned argument_index = 0; argument_index < argument_count; ++argument_index) {
        struct game_script_typed_ptr ptr       = game_script_object_handle_decode(arguments[argument_index]);
        struct entity*               to_fight  = game_dereference_entity(state, ptr.entity_id);
        to_fight->ai.flags                    |= ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER;
    }
    return LISP_nil;
}

GAME_LISP_FUNCTION(START_CUTSCENE) {
    assertion(argument_count >= 1 && "need at least one argument");

    string cutscene_path = {};
    lisp_form_get_string(arguments[0], &cutscene_path);
    cutscene_open(cutscene_path);
    
    return LISP_nil;
}
GAME_LISP_FUNCTION(END_CUTSCENE) {
    cutscene_stop();
    return LISP_nil;
}

GAME_LISP_FUNCTION(CUTSCENE_LOAD_AREA) {
    string cutscene_area_path = {};
    lisp_form_get_string(arguments[0], &cutscene_area_path);
    cutscene_load_area(cutscene_area_path);

    return LISP_nil;
}

GAME_LISP_FUNCTION(CUTSCENE_UNLOAD_AREA) {
    cutscene_unload_area();
    return LISP_nil;
}

#undef GAME_LISP_FUNCTION

#define STRINGIFY(x) #x
#define GAME_LISP_FUNCTION(NAME) (struct game_script_function_builtin) { .name = #NAME, .function = NAME ## __script_proc }
#undef STRINGIFY
/* this can and should be metaprogrammed. */
static struct game_script_function_builtin script_function_table[] = {
    GAME_LISP_FUNCTION(DLG_NEXT),
    GAME_LISP_FUNCTION(OBJ_ACTIVATIONS),
    GAME_LISP_FUNCTION(GAME_START_RAIN),
    GAME_LISP_FUNCTION(ENTITY_LOOK_AT),
    GAME_LISP_FUNCTION(GAME_STOP_RAIN),
    GAME_LISP_FUNCTION(GAME_START_SNOW),
    GAME_LISP_FUNCTION(FOLLOW_PATH),
    GAME_LISP_FUNCTION(ENTITY_SPEAK),
    GAME_LISP_FUNCTION(GAME_STOP_SNOW),
    GAME_LISP_FUNCTION(GAME_IS_WEATHER),
    GAME_LISP_FUNCTION(GAME_SET_ENVIRONMENT_COLORS),
    GAME_LISP_FUNCTION(GAME_MESSAGE_QUEUE),
    GAME_LISP_FUNCTION(KILL_ALL),
    GAME_LISP_FUNCTION(GAME_OPEN_CONVERSATION),
    GAME_LISP_FUNCTION(GAME_ENABLE_INPUT),
    GAME_LISP_FUNCTION(GAME_DISABLE_INPUT),
    GAME_LISP_FUNCTION(GAME_INPUT_ENABLED),
    GAME_LISP_FUNCTION(GAME_SET_REGION_NAME),
    GAME_LISP_FUNCTION(OPEN_SHOP),
    GAME_LISP_FUNCTION(GAME_START_FIGHT_WITH),
    GAME_LISP_FUNCTION(START_CUTSCENE),
    GAME_LISP_FUNCTION(END_CUTSCENE),
    GAME_LISP_FUNCTION(CUTSCENE_LOAD_AREA),
    GAME_LISP_FUNCTION(CUTSCENE_UNLOAD_AREA)
    /*
      ALL CUTSCENE ACTIONS WILL BE PREFIXED, SINCE THE CUTSCENE SYSTEM RUNS GENERALIZED GAME SCRIPT,
      it's kind of easy to accidently break something this way!

      I might contextually change functions based on cutscene-ness but who knows?
    */
};
