#define GAME_LISP_FUNCTION(name) struct lisp_form name ## __script_proc (struct memory_arena* arena, struct game_state* state, struct lisp_form* arguments, s32 argument_count)

GAME_LISP_FUNCTION(FOLLOW_PATH) {
    Required_Argument_Count(FOLLOW_PATH, 2)

    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    Fatal_Script_Error(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "Whoops! Don't know how to make non-entities follow paths!");

    v2f32* path       = 0;
    s32    path_count = 0;

    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    assertion(target_entity && "no entity?");

    if (argument_count == 2) {
        /* manually provided path form */
        path = memory_arena_push(&scratch_arena, 0);

        struct lisp_form* defined_path = &arguments[1];
        Fatal_Script_Error(defined_path->type == LISP_FORM_LIST && "this is not a path I guess.");

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
    Required_Argument_Count(ENTITY_LOOK_AT, 2);

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
/* Should remove these ENTITY_ prefixes so I can get a more consistent "script language" but it's whatever right now */
GAME_LISP_FUNCTION(ENTITY_IS_DEAD) {
    Required_Argument_Count(ENTITY_IS_DEAD, 1);

    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    assertion(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "This only works on entities!");
    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    assertion(target_entity && "no entity?");

    return lisp_form_produce_truthy_value_form(entity_is_dead(target_entity));
}

GAME_LISP_FUNCTION(ENTITY_GET_HEALTH_PERCENT) {
    Required_Argument_Count(ENTITY_GET_HEALTH_PERCENT, 1);

    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    assertion(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "This only works on entities!");
    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    assertion(target_entity && "no entity?");

    return lisp_form_real(entity_health_percentage(target_entity));
}

GAME_LISP_FUNCTION(ENTITY_SPEAK) {
    Required_Argument_Count(ENTITY_SPEAK, 2);

    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    Fatal_Script_Error(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "This only works on entities!");
    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    Fatal_Script_Error(target_entity && "no entity?");

    string text;
    if (lisp_form_get_string(arguments[1], &text)) {
        passive_speaking_dialogue_push(ptr.entity_id, text, MENU_FONT_COLOR_GOLD);
    } else {
        _debugprintf("entity speak needs a string!");
    }
    return LISP_nil;
}

GAME_LISP_FUNCTION(KILL_ALL) {
    Required_Argument_Count(KILL_ALL, 0);
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
    Required_Argument_Count(OBJ_ACTIVATIONS, 1);
    struct game_script_typed_ptr object_ptr = game_script_object_handle_decode(arguments[0]);

    switch (object_ptr.type) {
        case GAME_SCRIPT_TARGET_TRIGGER: {
            struct trigger* trigger = (struct trigger*)object_ptr.ptr;
            _debugprintf("requested activation # for trigger: (%d)", trigger->activations);
            return lisp_form_integer(trigger->activations);
        } break;
            bad_case;
    }

    return LISP_nil;
}

GAME_LISP_FUNCTION(OPEN_SHOP) {
    Required_Argument_Count(OPEN_SHOP, 1);

    string shop_name = {};
    assert(lisp_form_get_string(arguments[0], &shop_name));
    game_begin_shopping(shop_name);

    return LISP_nil;
}

GAME_LISP_FUNCTION(random) {
    /* expect an object handle. Not checking right now */
    Required_Argument_Count(RANDOM, 1);
    
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

    return LISP_nil;
}
GAME_LISP_FUNCTION(DLG_NEXT) {
    _debugprintf("NOT IMPLEMENTED YET!");
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_IS_WEATHER) {
    Required_Argument_Count(GAME_IS_WEATHER, 1);

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
    Required_Argument_Count(GAME_START_RAIN, 0);
    _debugprintf("game_start_rain script");
    weather_start_rain(state);
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_STOP_RAIN) {
    Required_Argument_Count(GAME_STOP_RAIN, 0);
    _debugprintf("game_stop_rain script");
    weather_stop_rain(state);
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_START_SNOW) {
    Required_Argument_Count(GAME_START_SNOW, 0);
    _debugprintf("game_start_snow script");
    weather_start_snow(state);
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_STOP_SNOW) {
    Required_Argument_Count(GAME_STOP_SNOW, 0);
    _debugprintf("game_stop_snow script");
    weather_stop_snow(state);
    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_SET_REGION_NAME) {
    Required_Argument_Count(GAME_SET_REGION_NAME, 2);
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
    Required_Argument_Count(GAME_SET_ENVIRONMENT_COLORS, 1);

    struct lisp_form* argument = &arguments[0];

    if (argument->type == LISP_FORM_SYMBOL) {
        if (lisp_form_symbol_matching(*argument, string_literal("night"))) {
            game_set_time_color(2);
        } else if (lisp_form_symbol_matching(*argument, string_literal("dawn"))) {
            game_set_time_color(1);
        } else if (lisp_form_symbol_matching(*argument, string_literal("midnight"))) {
            game_set_time_color(3);
        } else if (lisp_form_symbol_matching(*argument, string_literal("day"))) {
            game_set_time_color(0);
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

    return LISP_nil;
}
GAME_LISP_FUNCTION(GAME_INPUT_ENABLED) {
    Required_Argument_Count(GAME_INPUT_ENABLED, 0);
    return lisp_form_produce_truthy_value_form(!disable_game_input);
}
GAME_LISP_FUNCTION(GAME_DISABLE_INPUT) {
    Required_Argument_Count(GAME_DISABLE_INPUT, 0);
    disable_game_input = true;
    return lisp_form_produce_truthy_value_form(!disable_game_input);
}
GAME_LISP_FUNCTION(GAME_ENABLE_INPUT) {
    Required_Argument_Count(GAME_ENABLE_INPUT, 0);
    disable_game_input = false;
    return lisp_form_produce_truthy_value_form(!disable_game_input);
}
GAME_LISP_FUNCTION(GAME_MESSAGE_QUEUE) {
    Required_Argument_Count(GAME_MESSAGE_QUEUE, 1);

    struct lisp_form* argument = &arguments[0];

    if (argument->type == LISP_FORM_STRING) {
        game_message_queue(argument->string);
    }

    return LISP_nil;
}

GAME_LISP_FUNCTION(GAME_OPEN_CONVERSATION) {
    Required_Minimum_Argument_Count(GAME_OPEN_CONVERSATION, 1);

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
    Required_Minimum_Argument_Count(GAME_START_FIGHT_WITH, 1);

    for (unsigned argument_index = 0; argument_index < argument_count; ++argument_index) {
        struct game_script_typed_ptr ptr       = game_script_object_handle_decode(arguments[argument_index]);
        struct entity*               to_fight  = game_dereference_entity(state, ptr.entity_id);
        to_fight->ai.flags                    |= ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER;
    }
    return LISP_nil;
}

GAME_LISP_FUNCTION(START_CUTSCENE) {
    Required_Argument_Count(START_CUTSCENE, 1);

    string cutscene_path = {};
    lisp_form_get_string(arguments[0], &cutscene_path);
    cutscene_open(cutscene_path);
    
    return LISP_nil;
}
GAME_LISP_FUNCTION(END_CUTSCENE) {
    Required_Argument_Count(END_CUTSCENE, 0);
    cutscene_stop();
    return LISP_nil;
}

GAME_LISP_FUNCTION(CUTSCENE_LOAD_AREA) {
    Required_Argument_Count(CUTSCENE_LOAD_AREA, 1);

    string cutscene_area_path = {};
    lisp_form_get_string(arguments[0], &cutscene_area_path);
    cutscene_load_area(cutscene_area_path);

    return LISP_nil;
}

GAME_LISP_FUNCTION(CUTSCENE_UNLOAD_AREA) {
    cutscene_unload_area();
    return LISP_nil;
}
GAME_LISP_FUNCTION(CAMERA_PUTAT_FORCE) {
    Required_Argument_Count(CAMERA_PUTAT_FORCE, 1);

    assertion(lisp_form_get_f32(arguments[0].list.forms[0], &game_state->camera.xy.x) && "camera x fail?");
    assertion(lisp_form_get_f32(arguments[0].list.forms[1], &game_state->camera.xy.y) && "camera y fail?");
    _debugprintf("%f, %f\n", game_state->camera.xy.x, game_state->camera.xy.y);

    return LISP_nil;
}

/*
  ARG0: AreaName
  ARG1: FacingDirection (of main character) { up, left, right, down, retain }
  ARG2(?): Where (x, y): if null, use default spawn area
 */
GAME_LISP_FUNCTION(GAME_LOAD_AREA) {
    Required_Minimum_Argument_Count(GAME_LOAD_AREA, 2);

    bool  use_spawn      = true;
    v2f32 spawn_location = {};

    if (argument_count == 3) {
        use_spawn = false;
        lisp_form_get_v2f32(arguments[2], &spawn_location);
    }

    string direction_string;
    string level_path;
    if(!lisp_form_get_string(arguments[0], &level_path)) {
        Fatal_Script_Error(!"Bad level path!");
    } else {
        _debugprintf("LEVEL PATH: %.*s", level_path.length, level_path.data);
    }

    if (!lisp_form_get_string(arguments[1], &direction_string)) {
        Fatal_Script_Error(!"Bad direction string!?");
    }

    u8 direction = facing_direction_from_string(direction_string);
    load_level_from_file_with_setup(game_state, level_path);

    /* * get all player entities */
    struct entity* player = game_get_player(game_state);
    if (!use_spawn) {
        player->position    = spawn_location;
        player->position.x *= TILE_UNIT_SIZE;
        player->position.y *= TILE_UNIT_SIZE;
    }

    if (direction != DIRECTION_RETAINED) {
        player->facing_direction = direction;
    }

    return LISP_nil;
}

/* NOTE: ENTITY INVENTORY RELATED STUFF DOESN'T COUNT CHESTS! (YET?) */
GAME_LISP_FUNCTION(ENTITY_REMOVE_ITEM) {
    Required_Minimum_Argument_Count(ENTITY_REMOVE_ITEM, 2);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct entity*               entity = game_dereference_entity(state, ptr.entity_id);

    string object_name;
    if (!lisp_form_get_string(arguments[1], &object_name)) {
        Fatal_Script_Error(!"Bad item name");
    }

    s32 item_count = 1;

    if (argument_count == 3) {
        lisp_form_get_s32(arguments[2], &item_count);
    }

    struct entity_inventory* inventory_target = (struct entity_inventory*)&entity->inventory;
    int                      inventory_limits = MAX_ACTOR_AVALIABLE_ITEMS;

    if (game_entity_is_party_member(entity)) {
        inventory_target = (struct entity_inventory*)&game_state->inventory;
        inventory_limits = MAX_PARTY_ITEMS;
    }

    if (item_count == -1) {
        entity_inventory_remove_item_by_name(inventory_target, object_name, true);
    } else {
        for (unsigned removed = 0; removed < item_count; ++removed) {
            entity_inventory_remove_item_by_name(inventory_target, object_name, false);
        }
    }

    return LISP_nil;
}

GAME_LISP_FUNCTION(ENTITY_REMOVE_ABILITY) {
    Required_Argument_Count(ENTITY_REMOVE_ABILITY, 2);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct entity*               entity = game_dereference_entity(state, ptr.entity_id);

    string object_name;
    if (!lisp_form_get_string(arguments[1], &object_name)) {
        Fatal_Script_Error(!"Bad ability name");
    }

    entity_remove_ability_by_name(entity, object_name);
    return LISP_nil;
}

GAME_LISP_FUNCTION(ENTITY_ADD_ITEM) {
    Required_Minimum_Argument_Count(ENTITY_REMOVE_ITEM, 2);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct entity*               entity = game_dereference_entity(state, ptr.entity_id);

    string object_name;
    if (!lisp_form_get_string(arguments[1], &object_name)) {
        Fatal_Script_Error(!"Bad item name");
    }

    s32 item_count = 1;

    if (argument_count == 3) {
        lisp_form_get_s32(arguments[2], &item_count);
    }

    struct entity_inventory* inventory_target = (struct entity_inventory*)&entity->inventory;
    int                      inventory_limits = MAX_ACTOR_AVALIABLE_ITEMS;

    if (game_entity_is_party_member(entity)) {
        inventory_target = (struct entity_inventory*)&game_state->inventory;
        inventory_limits = MAX_PARTY_ITEMS;
    }

    item_id added_item_id = item_id_make(object_name);
    for (unsigned added = 0; added < item_count; ++added) {
        entity_inventory_add(inventory_target, inventory_limits, added_item_id);
    }

    return LISP_nil;
}

GAME_LISP_FUNCTION(ENTITY_ADD_ABILITY) {
    Required_Argument_Count(ENTITY_ADD_ABILITY, 2);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct entity*               entity = game_dereference_entity(state, ptr.entity_id);

    string object_name = {};
    if (!lisp_form_get_string(arguments[1], &object_name)) {
        Fatal_Script_Error(!"Bad ability name");
    }

    entity_add_ability_by_name(entity, object_name);
    return LISP_nil;
}

GAME_LISP_FUNCTION(ENTITY_HAS_ITEM) {
    Required_Argument_Count(ENTITY_HAS_ITEM, 2);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct entity*               entity = game_dereference_entity(state, ptr.entity_id);

    string object_name = {};
    if (!lisp_form_get_string(arguments[1], &object_name)) {
        Fatal_Script_Error(!"Bad item name");
    }

    struct entity_inventory* inventory_target = (struct entity_inventory*)&entity->inventory;
    int                      inventory_limits = MAX_ACTOR_AVALIABLE_ITEMS;

    return lisp_form_produce_truthy_value_form(entity_inventory_has_item(inventory_target, item_id_make(object_name)));
}

GAME_LISP_FUNCTION(ENTITY_GET_ITEM_COUNT) {
    Required_Argument_Count(ENTITY_GET_ITEM_COUNT, 2);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct entity*               entity = game_dereference_entity(state, ptr.entity_id);

    string object_name;
    if (!lisp_form_get_string(arguments[1], &object_name)) {
        Fatal_Script_Error(!"Bad item name");
    }

    struct entity_inventory* inventory_target = (struct entity_inventory*)&entity->inventory;
    int                      inventory_limits = MAX_ACTOR_AVALIABLE_ITEMS;

    return lisp_form_integer(entity_inventory_count_instances_of(inventory_target, object_name));
}

/* TODO: This should be a blocking function! But I haven't really used it so who's to say it should be? */
GAME_LISP_FUNCTION(GAME_DO_TRANSITION) {
    /* ID, DIR, COLOR, DELAY, LENGTH */
    Required_Argument_Count(GAME_DO_TRANSITION, 5);
    struct lisp_form* transition_id        = &arguments[0];
    struct lisp_form* transition_direction = &arguments[1];
    struct lisp_form* transition_color     = &arguments[2];
    struct lisp_form* transition_delay     = &arguments[3];
    struct lisp_form* transition_length    = &arguments[4];

    u8 fade_in = 1;

    if (lisp_form_symbol_matching(*transition_direction, string_literal("out"))) {
        fade_in = 0;
    } else if (lisp_form_symbol_matching(*transition_direction, string_literal("in"))) {
        fade_in = 1;
    }

    union color32f32 color;
    {
        Fatal_Script_Error(transition_color->type != LISP_FORM_LIST && "color needs to be a list? (4 or 3 elems)");
        Fatal_Script_Error(transition_color->list.count < 3 && "less than 3 colors?");

        f32 r = 0;
        f32 g = 0;
        f32 b = 0;
        f32 a = 1;

        Fatal_Script_Error(lisp_form_get_f32(transition_color->list.forms[0], &r) && "Could not read color value (r)");
        Fatal_Script_Error(lisp_form_get_f32(transition_color->list.forms[1], &g) && "Could not read color value (g)");
        Fatal_Script_Error(lisp_form_get_f32(transition_color->list.forms[2], &b) && "Could not read color value (b)");

        if (transition_color->list.count == 4) {
            Fatal_Script_Error(lisp_form_get_f32(transition_color->list.forms[3], &a) && "Could not read color value (a)");
        }

        color.r = r;
        color.g = g;
        color.b = b;
        color.a = a;
    }

    f32 delay  = 0;
    f32 length = 0;
    {
        Fatal_Script_Error(lisp_form_get_f32(*transition_delay, &delay) && "Could not read transition delay timer");
        Fatal_Script_Error(lisp_form_get_f32(*transition_length, &length) && "Could not read length timer");
    }

    if (lisp_form_symbol_matching(*transition_id, string_literal("color"))) {
        if (fade_in) {
            do_color_transition_in(color, delay, length);
        } else {
            do_color_transition_out(color, delay, length);
        }
    } else if (lisp_form_symbol_matching(*transition_id, string_literal("horizontal_slide"))) {
        if (fade_in) {
            do_horizontal_slide_in(color, delay, length);
        } else {
            do_horizontal_slide_out(color, delay, length);
        }
    } else if (lisp_form_symbol_matching(*transition_id, string_literal("vertical_slide"))) {
        if (fade_in) {
            do_vertical_slide_in(color, delay, length);
        } else {
            do_vertical_slide_out(color, delay, length);
        }
    } else if (lisp_form_symbol_matching(*transition_id, string_literal("shuteye"))) {
        if (fade_in) {
            do_shuteye_in(color, delay, length);
        } else {
            do_shuteye_out(color, delay, length);
        }
    } else if (lisp_form_symbol_matching(*transition_id, string_literal("curtainclose"))) {
        if (fade_in) {
            do_curtainclose_in(color, delay, length);
        } else {
            do_curtainclose_out(color, delay, length);
        }
    }

    return LISP_nil;
}

GAME_LISP_FUNCTION(CAMERA_TRAUMATIZE) {
    Required_Argument_Count(CAMERA_TRAUMATIZE, 1);

    f32 trauma_value = 0;
    Fatal_Script_Error(lisp_form_get_f32(arguments[0], &trauma_value));

    camera_traumatize(&game_state->camera, trauma_value);
    return LISP_nil;
}

GAME_LISP_FUNCTION(CAMERA_SET_CONSTANT_TRAUMA) {
    Required_Argument_Count(CAMERA_SET_CONSTANT_TRAUMA, 1);

    if (lisp_form_symbol_matching(arguments[0], string_literal("stop"))) {
        game_stop_constantly_traumatizing_camera();
    } else {
        f32 trauma_value = 0;
        Fatal_Script_Error(lisp_form_get_f32(arguments[0], &trauma_value));
        game_apply_constant_camera_trauma_as(trauma_value);
    }
    return LISP_nil;
}

GAME_LISP_FUNCTION(REPEAT) {
    Required_Argument_Count(REPEAT, 2);
    struct lisp_form* thing_to_repeat = &arguments[0];
    struct lisp_form* times_to_repeat = &arguments[1];

    s32 times_to_repeat_integer = 0;
    lisp_form_get_s32(*times_to_repeat, &times_to_repeat_integer);

    struct lisp_form new_form = {};
    new_form.type             = LISP_FORM_LIST;
    new_form.list.forms       = memory_arena_push(&scratch_arena, sizeof(new_form) * times_to_repeat_integer);
    new_form.list.count       = times_to_repeat_integer;

    for (s32 write_index = 0; write_index < times_to_repeat_integer; ++write_index) {
        new_form.list.forms[write_index] = *thing_to_repeat;
    }

    return new_form;
}

/* These prototypes are here so that will metagen.c will catch it*/
#if 1
GAME_LISP_FUNCTION(VIGOR);
GAME_LISP_FUNCTION(STRENGTH);
GAME_LISP_FUNCTION(CONSTITUTION);
GAME_LISP_FUNCTION(WILLPOWER);
GAME_LISP_FUNCTION(AGILITY);
GAME_LISP_FUNCTION(SPEED);
GAME_LISP_FUNCTION(INTELLIGENCE);
GAME_LISP_FUNCTION(LUCK);
GAME_LISP_FUNCTION(SET_VIGOR);
GAME_LISP_FUNCTION(SET_STRENGTH);
GAME_LISP_FUNCTION(SET_CONSTITUTION);
GAME_LISP_FUNCTION(SET_WILLPOWER);
GAME_LISP_FUNCTION(SET_AGILITY);
GAME_LISP_FUNCTION(SET_SPEED);
GAME_LISP_FUNCTION(SET_INTELLIGENCE);
GAME_LISP_FUNCTION(SET_LUCK);
GAME_LISP_FUNCTION(SET_COUNTER);
#endif

#define Define_Stat_Accessors(STAT_NAME, STAT_FIELD)                    \
    GAME_LISP_FUNCTION(STAT_NAME) {                                     \
        Required_Argument_Count(STAT_NAME, 1);  \
        struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]); \
        Fatal_Script_Error(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "Non ENTITY cannot have stats!"); \
        struct entity*               entity = game_dereference_entity(state, ptr.entity_id); \
        s32                          value  = entity->stat_block.STAT_FIELD ; \
        return lisp_form_integer(value);        \
    }                                           \
    GAME_LISP_FUNCTION(SET_ ## STAT_NAME) {     \
        Required_Argument_Count(SET_ ## STAT_NAME, 2); \
        struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]); \
        Fatal_Script_Error(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "Non ENTITY cannot have stats!"); \
        struct entity*               entity = game_dereference_entity(state, ptr.entity_id); \
        s32                          value  = 0; \
        Fatal_Script_Error(lisp_form_get_s32(arguments[1], &value) && "Stat accessor set needs to be a number"); \
        entity->stat_block.STAT_FIELD       = value; \
        return lisp_form_integer(value);        \
    }

Define_Stat_Accessors(VIGOR,        vigor);
Define_Stat_Accessors(STRENGTH,     strength);
Define_Stat_Accessors(CONSTITUTION, constitution);
Define_Stat_Accessors(WILLPOWER,    willpower);
Define_Stat_Accessors(AGILITY,      agility);
Define_Stat_Accessors(SPEED,        speed);
Define_Stat_Accessors(INTELLIGENCE, intelligence);
Define_Stat_Accessors(LUCK,         luck);
Define_Stat_Accessors(COUNTER,      counter);

GAME_LISP_FUNCTION(HEALTH) {
    Required_Argument_Count(HEALTH, 1);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct entity*               entity = game_dereference_entity(state, ptr.entity_id);
    s32                          value  = entity->health.value;
    return lisp_form_integer(value);
}
GAME_LISP_FUNCTION(SET_HEALTH_LIMIT) {
    Required_Argument_Count(SET_HEALTH, 2);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct entity*               entity = game_dereference_entity(state, ptr.entity_id);
    s32                          value  = 0;
    Fatal_Script_Error(lisp_form_get_s32(arguments[1], &value) && "Stat accessor set needs to be a number");
    entity->health.value = value;
    if (entity->health.value > entity->health.max) {
        entity->health.max = entity->health.value;
    }
    (entity_validate_death(entity));
    return lisp_form_integer(value);
}
GAME_LISP_FUNCTION(SET_HEALTH) {
    Required_Argument_Count(SET_HEALTH, 2);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct entity*               entity = game_dereference_entity(state, ptr.entity_id);
    s32                          value  = 0;
    Fatal_Script_Error(lisp_form_get_s32(arguments[1], &value) && "Stat accessor set needs to be a number");
    entity->health.value = value;
    if (entity->health.value > entity->health.max) {
        entity->health.value = entity->health.max;
    }
    (entity_validate_death(entity));
    return lisp_form_integer(value);
}
/* return position as a tile grid unit */
GAME_LISP_FUNCTION(GET_POSITION) {
    Required_Argument_Count(GET_POSITION, 1);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);

    switch (ptr.type) {
        case GAME_SCRIPT_TARGET_ENTITY: {
            struct entity*               entity = game_dereference_entity(state, ptr.entity_id);
            return lisp_form_v2f32(v2f32(entity->position.x / TILE_UNIT_SIZE,
                                         entity->position.y / TILE_UNIT_SIZE));
        } break;
        case GAME_SCRIPT_TARGET_LIGHT: {
            struct light_def* light = ptr.ptr;
            return lisp_form_v2f32(light->position);
        } break;
        case GAME_SCRIPT_TARGET_TRANSITION_TRIGGER: {
            /* ? */
            struct trigger_level_transition* transition = ptr.ptr;
            return lisp_form_v2f32(
                v2f32(
                    transition->bounds.x,
                    transition->bounds.y
                )
            );
        } break;
        case GAME_SCRIPT_TARGET_TRIGGER: {
            /* ? */
            struct trigger* trigger = ptr.ptr;
            return lisp_form_v2f32(
                v2f32(
                    trigger->bounds.x,
                    trigger->bounds.y
                )
            );
        } break;
        case GAME_SCRIPT_TARGET_SAVEPOINT: {
            struct entity_savepoint* savepoint = ptr.ptr;
            return lisp_form_v2f32(savepoint->position);
        } break;
        case GAME_SCRIPT_TARGET_CHEST: {
            struct entity_chest* chest = ptr.ptr;
            return lisp_form_v2f32(chest->position);
        } break;
        case GAME_SCRIPT_TARGET_SCRIPTABLE_LAYER: {
            struct scriptable_tile_layer_property* layer_properties = ptr.ptr;
            return lisp_form_v2f32(
                v2f32(
                    layer_properties->offset_x,
                    layer_properties->offset_y
                )
            );
        } break;
        default: {
            /* unknown type */
            /* ? */
        } break;
    }

    return LISP_nil;
}
GAME_LISP_FUNCTION(SET_POSITION) {
    Required_Argument_Count(SET_POSITION, 2);
    struct game_script_typed_ptr ptr        = game_script_object_handle_decode(arguments[0]);
    struct lisp_form*            where_form = &arguments[1];
    v2f32                        where      = v2f32(0,0);

    Fatal_Script_Error(lisp_form_get_v2f32(*where_form, &where) && "Need where form!");

    switch (ptr.type) {
        case GAME_SCRIPT_TARGET_ENTITY: {
            struct entity*               entity = game_dereference_entity(state, ptr.entity_id);
            entity->position.x = where.x * TILE_UNIT_SIZE;
            entity->position.y = where.y * TILE_UNIT_SIZE;
            return lisp_form_v2f32(v2f32(entity->position.x / TILE_UNIT_SIZE,
                                         entity->position.y / TILE_UNIT_SIZE));
        } break;
        case GAME_SCRIPT_TARGET_LIGHT: {
            struct light_def* light = ptr.ptr;
            light->position = where;
            return lisp_form_v2f32(light->position);
        } break;
        case GAME_SCRIPT_TARGET_TRANSITION_TRIGGER: {
            /* ? */
            struct trigger_level_transition* transition = ptr.ptr;
            transition->bounds.x = where.x;
            transition->bounds.y = where.y;
            return lisp_form_v2f32(
                v2f32(
                    transition->bounds.x,
                    transition->bounds.y
                )
            );
        } break;
        case GAME_SCRIPT_TARGET_TRIGGER: {
            /* ? */
            struct trigger* trigger = ptr.ptr;
            trigger->bounds.x = where.x;
            trigger->bounds.y = where.y;
            return lisp_form_v2f32(
                v2f32(
                    trigger->bounds.x,
                    trigger->bounds.y
                )
            );
        } break;
        case GAME_SCRIPT_TARGET_SAVEPOINT: {
            struct entity_savepoint* savepoint = ptr.ptr;
            savepoint->position = where;
            return lisp_form_v2f32(savepoint->position);
        } break;
        case GAME_SCRIPT_TARGET_CHEST: {
            struct entity_chest* chest = ptr.ptr;
            chest->position = where;
            return lisp_form_v2f32(chest->position);
        } break;
        case GAME_SCRIPT_TARGET_SCRIPTABLE_LAYER: {
            struct scriptable_tile_layer_property* layer_properties = ptr.ptr;
            layer_properties->offset_x = where.x;
            layer_properties->offset_y = where.y;
            return lisp_form_v2f32(
                v2f32(
                    layer_properties->offset_x,
                    layer_properties->offset_y
                )
            );
        } break;
        default: {
            /* unknown type */
            /* ? */
        } break;
    }

    return LISP_nil;
}

GAME_LISP_FUNCTION(SET_SCRIPTABLE_LAYER_ID) {
    Required_Argument_Count(SET_SCRIPTABLE_LAYER_ID, 2);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    struct lisp_form*            arg1   = &arguments[1];
    s32                          new_id = 0;
    Fatal_Script_Error(ptr.type == GAME_SCRIPT_TARGET_SCRIPTABLE_LAYER && "This method only works on script_layers!");
    if (lisp_form_symbol_matching(*arg1, string_literal("object"))) {
        new_id = TILE_LAYER_OBJECT;
    } else if (lisp_form_symbol_matching(*arg1, string_literal("ground"))) {
        new_id = TILE_LAYER_GROUND;
    } else if (lisp_form_symbol_matching(*arg1, string_literal("clutter-decor"))) {
        new_id = TILE_LAYER_CLUTTER_DECOR;
    } else if (lisp_form_symbol_matching(*arg1, string_literal("overhead"))) {
        new_id = TILE_LAYER_OVERHEAD;
    } else if (lisp_form_symbol_matching(*arg1, string_literal("roof"))) {
        new_id = TILE_LAYER_ROOF;
    } else if (lisp_form_symbol_matching(*arg1, string_literal("foreground"))) {
        new_id = TILE_LAYER_FOREGROUND;
    } else {
        Fatal_Script_Error(lisp_form_get_s32(*arg1, &new_id) && "new id must be an integer convertable number!");
    }
    struct scriptable_tile_layer_property* layer_properties = ptr.ptr;
    layer_properties->draw_layer                            = new_id;
    _debugprintf("Set new draw layer id to be: %d(%.*s)", new_id, tile_layer_strings[new_id].length, tile_layer_strings[new_id].data);
    return LISP_nil;
}

GAME_LISP_FUNCTION(NTH) {
    Required_Argument_Count(NTH, 2);
    s32 index = -1;
    struct lisp_form* arg0 = lisp_list_nth(arguments, 0);
    struct lisp_form* arg1 = lisp_list_nth(arguments, 1);
    Fatal_Script_Error(lisp_form_get_s32(*arg1, &index) && "Needs index argument!");

    struct lisp_form* result = lisp_list_nth(arg0, index);
    return *result;
}

GAME_LISP_FUNCTION(NAME) {
    Required_Argument_Count(NAME, 1);
    struct game_script_typed_ptr ptr    = game_script_object_handle_decode(arguments[0]);
    switch (ptr.type) {
        case GAME_SCRIPT_TARGET_ENTITY: {
            struct entity* entity = game_dereference_entity(state, ptr.entity_id);
            return lisp_form_string(entity->name);
        } break;
        default: {
            unimplemented("Not sure how to get the name of htis type of entity yet");
        } break;
    }

    return LISP_nil;
}

GAME_LISP_FUNCTION(SUICIDE) {
    Required_Argument_Count(SUICIDE, 0);
    entity_do_physical_hurt(game_get_player(game_state), 99999);
    return LISP_nil;
}

GAME_LISP_FUNCTION(DO_EXPLOSION) {
    Required_Minimum_Argument_Count(DO_EXPLOSION, 4);
    v2f32 where               = v2f32(0,0);
    f32   radius              = 0;
    s32   effect_explosion_id = 0;
    s32   damage_amount       = 0;
    s32   team_origin         = 0;
    u32   explosion_flags     = 0;

    struct lisp_form* where_form     = &arguments[0];
    struct lisp_form* radius_form    = &arguments[1];
    struct lisp_form* effect_id_form = &arguments[2];
    struct lisp_form* damage_form    = &arguments[2];

    Fatal_Script_Error(lisp_form_get_v2f32(*where_form,   &where) && "Bad where form! (v2f32)");
    Fatal_Script_Error(lisp_form_get_f32(*radius_form,    &radius) && "Bad radius form! (f32)");
    Fatal_Script_Error(lisp_form_get_s32(*effect_id_form, &effect_explosion_id) && "Bad explosion form id!");
    Fatal_Script_Error(lisp_form_get_s32(*damage_form,    &damage_amount) && "Bad damage form amount (s32)");


    game_produce_damaging_explosion(where,
                                    radius,
                                    effect_explosion_id,
                                    damage_amount,
                                    team_origin,
                                    explosion_flags);

    return LISP_nil;
}

GAME_LISP_FUNCTION(RESET_TIMESTEP_SCALE) {
    Required_Argument_Count(RESET_TIMESTEP_SCALE, 0);
    GLOBAL_GAME_TIMESTEP_MODIFIER = 1;
    return LISP_nil;
}

GAME_LISP_FUNCTION(SET_TIMESTEP_SCALE) {
    Required_Argument_Count(SET_TIMESTEP_SCALE, 1);
    lisp_form_get_f32(arguments[0], &GLOBAL_GAME_TIMESTEP_MODIFIER);
    return LISP_nil;
}

GAME_LISP_FUNCTION(ENGINEFORCESAVE) {
    Required_Argument_Count(ENGINEFORCESAVE, 1);
    s32 save_slot = 0;
    lisp_form_get_s32(arguments[0], &save_slot);
    if (save_slot >= GAME_MAX_SAVE_SLOTS) {
        save_slot = GAME_MAX_SAVE_SLOTS-1;
    } else if (save_slot < 0) {
        save_slot = 0;
    }
    game_write_save_slot(save_slot);
    return LISP_nil;
}
GAME_LISP_FUNCTION(ENTITY_FIND_FIRST_ITEM) {
    Required_Argument_Count(ENTITY_FIND_FIRST_ITEM, 2);
    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    Fatal_Script_Error(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "Whoops! Don't know how to make non-entities follow paths!");
    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    assertion(target_entity && "no entity?");

    struct lisp_form string_name_of_item = arguments[1];
    string string_name;
    assertion(lisp_form_get_string(string_name_of_item, &string_name) && "string name of item is not a string");

    struct entity_inventory* inventory = NULL;
    if (game_entity_is_party_member(target_entity)) {
        inventory = (struct entity_inventory*)&game_state->inventory;
    } else {
        inventory = (struct entity_inventory*)&target_entity->inventory;
    }

    s32 index_of_first_item = 0;
    for (s32 item_index = 0; item_index < inventory->count; ++item_index) {
        struct item_instance current_item = inventory->items[item_index];
        struct item_def*     item_base    = item_database_find_by_id(current_item.item);

        if (string_equal(item_base->name, string_name)) {
            index_of_first_item = item_index;
            break;
        }
    }

    return lisp_form_integer(index_of_first_item);
}
/* only from inventory! */
/* NOTE: these are currently untested */
GAME_LISP_FUNCTION(ENTITY_EQUIP_ITEM) {
    Required_Argument_Count(ENTITY_EQUIP_ITEM, 3);
    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    Fatal_Script_Error(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "Whoops! Don't know how to make non-entities follow paths!");
    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    assertion(target_entity && "no entity?");
    struct lisp_form equip_slot_symbol = arguments[1];
    string equip_slot = {};
    assertion(lisp_form_get_string(equip_slot_symbol, &equip_slot) && "entity equip slot string not a string!");
    s32 equip_slot_index = entity_equip_slot_index_from_string(equip_slot);
    assertion(equip_slot_index != -1 && "well that's bad, this is an invalid index for equip slots!");
    struct lisp_form index_of_item = arguments[2];
    s32 index = 0;
    assertion(lisp_form_get_s32(index_of_item, &index) && "entity equip item needs to be an integer");

    struct entity_inventory* inventory_target = (struct entity_inventory*)&target_entity->inventory;
    int                      inventory_limits = MAX_ACTOR_AVALIABLE_ITEMS;

    if (game_entity_is_party_member(target_entity)) {
        inventory_target = (struct entity_inventory*)&game_state->inventory;
        inventory_limits = MAX_PARTY_ITEMS;
    }
    
    entity_inventory_equip_item(inventory_target, inventory_limits, index, equip_slot_index, target_entity);
    return LISP_nil;
}
GAME_LISP_FUNCTION(ENTITY_UNEQUIP_ITEM) {
    Required_Argument_Count(ENTITY_UNEQUIP_ITEM, 1);
    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    Fatal_Script_Error(ptr.type == GAME_SCRIPT_TARGET_ENTITY && "Whoops! Don't know how to make non-entities follow paths!");
    struct entity* target_entity = game_dereference_entity(state, ptr.entity_id);
    assertion(target_entity && "no entity?");
    struct lisp_form equip_slot_symbol = arguments[1];
    string equip_slot = {};
    assertion(lisp_form_get_string(equip_slot_symbol, &equip_slot) && "entity equip slot string not a string!");
    s32 equip_slot_index = entity_equip_slot_index_from_string(equip_slot);
    assertion(equip_slot_index != -1 && "well that's bad, this is an invalid index for equip slots!");

    struct entity_inventory* inventory_target = (struct entity_inventory*)&target_entity->inventory;
    int                      inventory_limits = MAX_ACTOR_AVALIABLE_ITEMS;

    if (game_entity_is_party_member(target_entity)) {
        inventory_target = (struct entity_inventory*)&game_state->inventory;
        inventory_limits = MAX_PARTY_ITEMS;
    }
    
    entity_inventory_unequip_item(inventory_target, inventory_limits, equip_slot_index, target_entity);
    return LISP_nil;
}
GAME_LISP_FUNCTION(HIDE) {
    Required_Argument_Count(HIDE, 1);
    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    switch (ptr.type) {
        case GAME_SCRIPT_TARGET_LIGHT: {
            /* ? */
        } break;
        case GAME_SCRIPT_TARGET_TRANSITION_TRIGGER: {
            /* ? */
        } break;
        case GAME_SCRIPT_TARGET_TRIGGER: {
            /* ? */
        } break;
        case GAME_SCRIPT_TARGET_ENTITY: {
            struct entity* entity = ptr.ptr;
            entity->flags |= ENTITY_FLAGS_HIDDEN;
        } break;
        case GAME_SCRIPT_TARGET_SAVEPOINT: {
            struct entity_savepoint* savepoint = ptr.ptr;
            savepoint->flags |= ENTITY_FLAGS_HIDDEN;
        } break;
        case GAME_SCRIPT_TARGET_CHEST: {
            struct entity_chest* chest = ptr.ptr;
            chest->flags |= ENTITY_FLAGS_HIDDEN;
        } break;
        case GAME_SCRIPT_TARGET_SCRIPTABLE_LAYER: {
            struct scriptable_tile_layer_property* layer_properties = ptr.ptr;
            layer_properties->flags |= SCRIPTABLE_TILE_LAYER_FLAGS_HIDDEN;
        } break;
    }
    return LISP_nil;
}
GAME_LISP_FUNCTION(SHOW) {
    Required_Argument_Count(SHOW, 1);
    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    switch (ptr.type) {
        case GAME_SCRIPT_TARGET_LIGHT: {
            /* ? */
        } break;
        case GAME_SCRIPT_TARGET_TRANSITION_TRIGGER: {
            /* ? */
        } break;
        case GAME_SCRIPT_TARGET_TRIGGER: {
            /* ? */
        } break;
        case GAME_SCRIPT_TARGET_ENTITY: {
            struct entity* entity = ptr.ptr;
            entity->flags &= ~(ENTITY_FLAGS_HIDDEN);
        } break;
        case GAME_SCRIPT_TARGET_SAVEPOINT: {
            struct entity_savepoint* savepoint = ptr.ptr;
            savepoint->flags &= ~(ENTITY_FLAGS_HIDDEN);
        } break;
        case GAME_SCRIPT_TARGET_CHEST: {
            struct entity_chest* chest = ptr.ptr;
            chest->flags &= ~(ENTITY_FLAGS_HIDDEN);
        } break;
        case GAME_SCRIPT_TARGET_SCRIPTABLE_LAYER: {
            struct scriptable_tile_layer_property* layer_properties = ptr.ptr;
            layer_properties->flags &= ~(ENTITY_FLAGS_HIDDEN);
        } break;
    }
    return LISP_nil;
}
GAME_LISP_FUNCTION(TOGGLE_VISIBILITY) {
    Required_Argument_Count(TOGGLE_VISIBILITY, 1);
    struct game_script_typed_ptr ptr = game_script_object_handle_decode(arguments[0]);
    switch (ptr.type) {
        case GAME_SCRIPT_TARGET_LIGHT: {
            /* ? */
        } break;
        case GAME_SCRIPT_TARGET_TRANSITION_TRIGGER: {
            /* ? */
        } break;
        case GAME_SCRIPT_TARGET_TRIGGER: {
            /* ? */
        } break;
        case GAME_SCRIPT_TARGET_ENTITY: {
            struct entity* entity = ptr.ptr;
            entity->flags ^= (ENTITY_FLAGS_HIDDEN);
        } break;
        case GAME_SCRIPT_TARGET_SAVEPOINT: {
            struct entity_savepoint* savepoint = ptr.ptr;
            savepoint->flags ^= (ENTITY_FLAGS_HIDDEN);
        } break;
        case GAME_SCRIPT_TARGET_CHEST: {
            struct entity_chest* chest = ptr.ptr;
            chest->flags ^= (ENTITY_FLAGS_HIDDEN);
        } break;
        case GAME_SCRIPT_TARGET_SCRIPTABLE_LAYER: {
            struct scriptable_tile_layer_property* layer_properties = ptr.ptr;
            layer_properties->flags ^= (ENTITY_FLAGS_HIDDEN);
        } break;
    }
    return LISP_nil;
}

#undef GAME_LISP_FUNCTION

#define STRINGIFY(x) #x
#define GAME_LISP_FUNCTION(NAME) (struct game_script_function_builtin) { .name = #NAME, .function = NAME ## __script_proc }
#undef STRINGIFY
/* this can and should be metaprogrammed. */

#include "generated_game_script_proc_table.c"

#undef Fatal_Script_Error
#undef Script_Error
#undef Required_Argument_Count
