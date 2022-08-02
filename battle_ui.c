enum battle_ui_animation_phase {
    BATTLE_UI_FADE_IN_DARK,
    BATTLE_UI_FLASH_IN_BATTLE_TEXT,
    BATTLE_UI_FADE_OUT,
    BATTLE_UI_MOVE_IN_DETAILS,
    BATTLE_UI_IDLE,
};

enum battle_ui_submodes {
    BATTLE_UI_SUBMODE_NONE,
    BATTLE_UI_SUBMODE_LOOKING,
    BATTLE_UI_SUBMODE_MOVING,
    BATTLE_UI_SUBMODE_ATTACKING,
    BATTLE_UI_SUBMODE_USING_ABILITY,
    BATTLE_UI_SUBMODE_USING_ITEM,
    /* the rest are not submodes, and are just registered actions. */
    /* NOTE action point system. There is no cancelling. */
};
struct battle_ui_state {
    f32 timer;
    u32 phase;

    s32 selection;

    /* Can be animated? */
    s32 submode;

    bool remembered_original_camera_position;
    v2f32 prelooking_mode_camera_position;

    s32 movement_start_x;
    s32 movement_start_y;
    s32 movement_end_x;
    s32 movement_end_y;

    /* wasteful? Maybe. Easy? Yes. */
    s32   max_remembered_path_points_count;
    v2f32 max_remembered_path_points[256];
} global_battle_ui_state;

enum battle_options{
    BATTLE_LOOK,
    BATTLE_MOVE,
    BATTLE_ATTACK,
    BATTLE_ABILITY,
    BATTLE_ITEM,
    BATTLE_DEFEND,
    BATTLE_WAIT,
};
local string battle_menu_main_options[] = {
    string_literal("LOOK"),
    string_literal("MOVE"),
    string_literal("ATTACK"),
    string_literal("ABILITY"),
    string_literal("ITEM"),
    string_literal("DEFEND"),
    string_literal("END TURN"),
};
local string battle_menu_main_option_descriptions[] = {
    string_literal("scout the battle field."),
    string_literal("pick a space to move to."),
    string_literal("use your weapons' basic attack."),
    string_literal("use an ability from your list."),
    string_literal("use an item from your party inventory."),
    string_literal("finish turn, lose AP on next turn for higher DEF."),
    string_literal("finish turn, conserving AP."),
};

local void start_combat_ui(void) {
    global_battle_ui_state.phase = 0;
    global_battle_ui_state.timer = 0;
}

local bool is_player_combat_turn(struct game_state* state) {
    struct game_state_combat_state* combat_state            = &state->combat_state;
    struct entity*                  active_combatant_entity = entity_list_dereference_entity(&state->entities, combat_state->participants[combat_state->active_combatant]);
    struct entity*                  player_entity           = entity_list_dereference_entity(&state->entities, player_id);

    if (player_entity == active_combatant_entity) {
        /* check if player entity can do anything */
        if (player_entity->ai.current_action == 0)
            return true;

        return false;
    }

    return false;
}

local void draw_turn_panel(struct game_state* state, struct software_framebuffer* framebuffer, f32 x, f32 y) {
    struct game_state_combat_state* combat_state = &state->combat_state;

    struct entity* active_combatant_entity = entity_list_dereference_entity(&state->entities, combat_state->participants[combat_state->active_combatant]);
    bool player_turn = is_player_combat_turn(state);

    for (s32 index = combat_state->active_combatant; index < combat_state->count; ++index) {
        union color32u8 color = color32u8(128, 128, 128, 255);

        {
            struct entity* entity = entity_list_dereference_entity(&state->entities, combat_state->participants[index]);
            if (entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
                color = color32u8(0, 255, 0, 255);
            } else {
                if (entity->ai.flags & ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER) {
                    color = color32u8(255, 0, 0, 255);
                }
            }
        }

        if (!player_turn) {
            color.r >>= 1;
            color.g >>= 1;
            color.b >>= 1;
        }

        const s32 square_size = 32;
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(x, y + (index - combat_state->active_combatant) * square_size * 1.3, square_size, square_size), color, BLEND_MODE_ALPHA);
    }
}
#define UI_BATTLE_COLOR (color32f32(34/255.0f, 37/255.0f, 143/255.0f, 1.0))

local void do_battle_selection_menu(struct game_state* state, struct software_framebuffer* framebuffer, f32 x, f32 y, bool allow_input) {
    struct game_state_combat_state* combat_state = &state->combat_state;

    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    if (is_key_pressed(KEY_BACKSPACE)) {
        if (global_battle_ui_state.submode != BATTLE_UI_SUBMODE_NONE) {
            global_battle_ui_state.submode                             = BATTLE_UI_SUBMODE_NONE;

            if (global_battle_ui_state.remembered_original_camera_position) {
                global_battle_ui_state.remembered_original_camera_position = false;
                state->camera.xy                                           = global_battle_ui_state.prelooking_mode_camera_position;
            }

            global_battle_ui_state.max_remembered_path_points_count = 0;

            level_area_clear_movement_visibility_map(&state->loaded_area);
            _debugprintf("restore to previous menu state");
        }
    }

    switch (global_battle_ui_state.submode) {
        case BATTLE_UI_SUBMODE_NONE: {
            union color32f32 modulation_color = color32f32_WHITE;
            union color32f32 ui_color         = UI_BATTLE_COLOR;

            if (!allow_input) {
                modulation_color = color32f32(0.5, 0.5, 0.5, 0.5);   
                ui_color.a = 0.5;
            }

            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x, y), 8, 14, ui_color);
            for (unsigned index = 0; index < array_count(battle_menu_main_options); ++index) {
                struct font_cache* painting_font = normal_font;

                if (index == global_battle_ui_state.selection) {
                    painting_font = highlighted_font;
                }

                draw_ui_breathing_text(framebuffer, v2f32(x + 20, y + 15 + index * 32), painting_font,
                                       2, battle_menu_main_options[index], index, modulation_color);
            }

            if (allow_input) {
                s32 options_count = array_count(battle_menu_main_options);
                if (is_key_down_with_repeat(KEY_DOWN)) {
                    global_battle_ui_state.selection++;

                    if (global_battle_ui_state.selection >= options_count) {
                        global_battle_ui_state.selection = 0;
                    }
                }
                else if (is_key_down_with_repeat(KEY_UP)) {
                    global_battle_ui_state.selection--;

                    if (global_battle_ui_state.selection < 0) {
                        global_battle_ui_state.selection = options_count-1;
                    }
                }

                if (is_key_pressed(KEY_RETURN)) {
                    switch (global_battle_ui_state.selection) {
                        /* NOTE: No ability is expected to really reach outside of your view... Hence the need for a separate view command */
                        case BATTLE_LOOK: {
                            global_battle_ui_state.submode = BATTLE_UI_SUBMODE_LOOKING;
                        } break;
                        case BATTLE_MOVE: {
                            global_battle_ui_state.submode = BATTLE_UI_SUBMODE_MOVING;
                            
                            struct entity* active_combatant_entity = entity_list_dereference_entity(&state->entities, combat_state->participants[combat_state->active_combatant]);
                            level_area_build_movement_visibility_map(&scratch_arena, &state->loaded_area, active_combatant_entity->position.x / TILE_UNIT_SIZE, active_combatant_entity->position.y / TILE_UNIT_SIZE, 6);

                            global_battle_ui_state.movement_start_x = active_combatant_entity->position.x / TILE_UNIT_SIZE;
                            global_battle_ui_state.movement_start_y = active_combatant_entity->position.y / TILE_UNIT_SIZE;
                            global_battle_ui_state.movement_end_x   = active_combatant_entity->position.x / TILE_UNIT_SIZE;
                            global_battle_ui_state.movement_end_y   = active_combatant_entity->position.y / TILE_UNIT_SIZE;
                        } break;
                        case BATTLE_ATTACK: {
                            global_battle_ui_state.submode = BATTLE_UI_SUBMODE_ATTACKING;
                        } break;
                        case BATTLE_ABILITY: {
                            /* global_battle_ui_state.submode = BATTLE_UI_SUBMODE_USING_ABILITY; */
                            _debugprintf("TODO: using abilities!");
                        } break;
                        case BATTLE_ITEM: {
                            /* global_battle_ui_state.submode = BATTLE_UI_SUBMODE_USING_ITEM; */
                            _debugprintf("TODO: using items!");
                        } break;
                        case BATTLE_DEFEND: {
                            _debugprintf("TODO: using defending!");
                        } break;
                        case BATTLE_WAIT: {
                            _debugprintf("TODO: using waiting!");
                        } break;
                    }
                }
            }
        } break;

        case BATTLE_UI_SUBMODE_MOVING: {
            struct level_area* area = &state->loaded_area;

            s32 proposed_y = global_battle_ui_state.movement_end_y;
            s32 proposed_x = global_battle_ui_state.movement_end_x;

            bool should_find_new_path = false;

            {
                if (is_key_pressed(KEY_W)) {
                    _debugprintf("nani?");
                    proposed_y--;
                } else if (is_key_pressed(KEY_S)) {
                    _debugprintf("nani?");
                    proposed_y++;
                }

                if (area->combat_movement_visibility_map[(proposed_y - area->navigation_data.min_y) * area->navigation_data.width + (proposed_x - area->navigation_data.min_x)]) {
                    if (proposed_y != global_battle_ui_state.movement_end_y) {
                        _debugprintf("okay, new y is fine");
                        should_find_new_path = true;
                    }

                    global_battle_ui_state.movement_end_y = proposed_y;
                }
            }

            {
                if (is_key_pressed(KEY_A)) {
                    _debugprintf("nani?");
                    proposed_x--;
                } else if (is_key_pressed(KEY_D)) {
                    _debugprintf("nani?");
                    proposed_x++;
                }

                if (area->combat_movement_visibility_map[(proposed_y - area->navigation_data.min_y) * area->navigation_data.width + (proposed_x - area->navigation_data.min_x)]) {
                    if (proposed_x != global_battle_ui_state.movement_end_x) {
                        _debugprintf("okay, new x is fine");
                        should_find_new_path = true;
                    }

                    global_battle_ui_state.movement_end_x = proposed_x;
                }
            }

            if (should_find_new_path) {
                struct navigation_path new_path = navigation_path_find(&scratch_arena, area,
                                                                       v2f32(global_battle_ui_state.movement_start_x, global_battle_ui_state.movement_start_y),
                                                                       v2f32(global_battle_ui_state.movement_end_x, global_battle_ui_state.movement_end_y));
                s32 minimum_count = 0;

                if (new_path.count < array_count(global_battle_ui_state.max_remembered_path_points)) {
                    minimum_count = new_path.count;
                } else {
                    minimum_count = array_count(global_battle_ui_state.max_remembered_path_points);
                }

                global_battle_ui_state.max_remembered_path_points_count = minimum_count;

                for (s32 index = 0; index < minimum_count; ++index) {
                    global_battle_ui_state.max_remembered_path_points[index] = new_path.points[index];
                }
            }

            /* would like to do action points but not right now. */

            if (is_key_pressed(KEY_RETURN)) {
                /* submit movement */
                struct entity* active_combatant_entity = entity_list_dereference_entity(&state->entities, combat_state->participants[combat_state->active_combatant]);
                entity_combat_submit_movement_action(active_combatant_entity, global_battle_ui_state.max_remembered_path_points, global_battle_ui_state.max_remembered_path_points_count);
                global_battle_ui_state.max_remembered_path_points_count = 0;
                global_battle_ui_state.submode = BATTLE_UI_SUBMODE_NONE;
                level_area_clear_movement_visibility_map(&state->loaded_area);
            }
            
        } break;

        case BATTLE_UI_SUBMODE_LOOKING: {
            /**/
            {
                if (!global_battle_ui_state.remembered_original_camera_position) {
                    global_battle_ui_state.remembered_original_camera_position = true;
                    global_battle_ui_state.prelooking_mode_camera_position     = state->camera.xy;
                } 

                /* mode should allow some basic analysis of opponents. */
                /* NOTE: (I want this to darken everything except for the selected entities...) */
                /* this is going to cause lots of hackery */
                f32 cursor_square_size = 4;
                software_framebuffer_draw_quad(framebuffer, rectangle_f32(framebuffer->width/2-cursor_square_size/2, framebuffer->height/2-cursor_square_size/2, cursor_square_size, cursor_square_size), color32u8_WHITE, BLEND_MODE_NONE);
            }
        } break;
    }
}

local void update_game_camera_combat(struct game_state* state, f32 dt) {
    if (global_battle_ui_state.submode == BATTLE_UI_SUBMODE_LOOKING) {
        const f32 CAMERA_VELOCITY = 150;

        if (is_key_down(KEY_W)) {
            state->camera.xy.y -= CAMERA_VELOCITY * dt;
        } else if (is_key_down(KEY_S)) {
            state->camera.xy.y += CAMERA_VELOCITY * dt;
        }

        if (is_key_down(KEY_A)) {
            state->camera.xy.x -= CAMERA_VELOCITY * dt;
        } else if (is_key_down(KEY_D)) {
            state->camera.xy.x += CAMERA_VELOCITY * dt;
        }
    }
}

local string analyze_entity_and_display_tooltip(struct memory_arena* arena, v2f32 cursor_at, struct game_state* state) {
    struct entity* target_entity_to_analyze = 0;

    struct rectangle_f32 cursor_rect = rectangle_f32(cursor_at.x, cursor_at.y, 8, 8);

    for (s32 index = 0; index < state->entities.capacity && target_entity_to_analyze == 0; ++index) {
        struct entity* current_entity = state->entities.entities + index;

        if (current_entity->flags & ENTITY_FLAGS_ACTIVE) {
            struct rectangle_f32 entity_rectangle = entity_rectangle_collision_bounds(current_entity);

            if (rectangle_f32_intersect(entity_rectangle, cursor_rect)) {
                target_entity_to_analyze = current_entity;
            }
        }
    }

    if (!target_entity_to_analyze) {
        return string_null;
    }

    if (!(target_entity_to_analyze->flags & ENTITY_FLAGS_ALIVE)) {
        char* r = format_temp("%.*s : is dead.", target_entity_to_analyze->name.length, target_entity_to_analyze->name.data);
        return string_from_cstring(r);
    }
    else {
        if (target_entity_to_analyze == game_get_player(state)) {
            return string_literal("This is you.");
        } else {
            char* r = format_temp("%.*s : HP : %d/%d, %d/%d/%d/%d/%d/%d",
                                  target_entity_to_analyze->name.length, target_entity_to_analyze->name.data,
                                  target_entity_to_analyze->health.value, target_entity_to_analyze->health.max,
                                  target_entity_to_analyze->stat_block.vigor,
                                  target_entity_to_analyze->stat_block.strength,
                                  target_entity_to_analyze->stat_block.agility,
                                  target_entity_to_analyze->stat_block.speed,
                                  target_entity_to_analyze->stat_block.intelligence,
                                  target_entity_to_analyze->stat_block.luck);

            return string_from_cstring(r);
        }
    }
}

local void draw_battle_tooltips(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt, f32 bottom_y, bool player_turn) {
    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);

    union color32f32 modulation_color = color32f32_WHITE;
    union color32f32 ui_color         = UI_BATTLE_COLOR;

    if (!player_turn) {
        modulation_color = color32f32(0.5, 0.5, 0.5, 0.5);   
        ui_color.a = 0.5;
    }

    string tip = {};

    /* do context dependent text... */
    switch (global_battle_ui_state.submode) {
        case BATTLE_UI_SUBMODE_LOOKING: {
            tip = analyze_entity_and_display_tooltip(&scratch_arena,
                                                     v2f32(state->camera.xy.x - 4, state->camera.xy.y - 4),
                                                     state);
        } break;
        case BATTLE_UI_SUBMODE_NONE: {
            tip = battle_menu_main_option_descriptions[global_battle_ui_state.selection];
        } break;
    }

    if (tip.length) {
        draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(15, bottom_y - (16*4)), 36, 2, ui_color);
        draw_ui_breathing_text(framebuffer, v2f32(30, bottom_y - (16*4) + 15), normal_font, 2, tip, 0, modulation_color);
    }

    if (!player_turn) return;
}

local void update_and_render_battle_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache*              font         = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
    struct game_state_combat_state* combat_state = &state->combat_state;

    /* pixels */
    const f32 BATTLE_SELECTIONS_WIDTH = 16 * 2 * 5;

    switch (global_battle_ui_state.phase) {
        case BATTLE_UI_FADE_IN_DARK: {
            const f32 max_t = 0.75;
            f32 t = global_battle_ui_state.timer / max_t;
            if (t > 1.0) t = 1.0;

            software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0, 128 * t), BLEND_MODE_ALPHA);

            global_battle_ui_state.timer += dt;

            if (global_battle_ui_state.timer >= max_t) {
                global_battle_ui_state.timer = 0;
                global_battle_ui_state.phase = BATTLE_UI_FLASH_IN_BATTLE_TEXT;
            }
        } break;
        case BATTLE_UI_FLASH_IN_BATTLE_TEXT: {
            const f32 max_t = 0.45;
            f32 t = global_battle_ui_state.timer / max_t;
            if (t > 1.0) t = 1.0;

            software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0, 128), BLEND_MODE_ALPHA);

            f32 x_cursor = lerp_f32(-300, 0, t);
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 4, rectangle_f32(x_cursor, 0, SCREEN_WIDTH, SCREEN_HEIGHT), string_literal("BATTLE ENGAGED!"), color32f32(1,1,1,1), BLEND_MODE_ALPHA);

            /* while this happens we should also snap all entities to grid points */

            global_battle_ui_state.timer += dt;

            if (global_battle_ui_state.timer >= (max_t+0.85)) {
                global_battle_ui_state.timer = 0;
                global_battle_ui_state.phase = BATTLE_UI_FADE_OUT;
            }
        } break;
        case BATTLE_UI_FADE_OUT: {
            const f32 max_t = 1;
            f32 t = global_battle_ui_state.timer / max_t;
            if (t > 1.0) t = 1.0;

            software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0, 128 * (1.0 - t)), BLEND_MODE_ALPHA);
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 4, rectangle_f32(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT), string_literal("BATTLE ENGAGED!"), color32f32(1,1,1, (1.0 - t)), BLEND_MODE_ALPHA);

            global_battle_ui_state.timer += dt;

            if (global_battle_ui_state.timer >= (max_t)) {
                global_battle_ui_state.timer = 0;
                global_battle_ui_state.phase = BATTLE_UI_MOVE_IN_DETAILS;
            }
        } break;
        case BATTLE_UI_MOVE_IN_DETAILS: {
            /* move in battle UI in a fancy way. */ 

            /* need more thought for this... */
            /* turn panel (left) */
            const f32 max_t = 0.4;
            f32 t = global_battle_ui_state.timer / max_t;

            {
                f32 x_cursor = lerp_f32(-80, 0, t);
                draw_turn_panel(state, framebuffer, 10 + x_cursor, 100);
            }
            {
                f32 x_cursor = lerp_f32(framebuffer->width + BATTLE_SELECTIONS_WIDTH, framebuffer->width - BATTLE_SELECTIONS_WIDTH + 15, t);
                do_battle_selection_menu(state, framebuffer, x_cursor, 100, false);
            }

            if (global_battle_ui_state.timer >= max_t) {
                global_battle_ui_state.timer = 0;
                global_battle_ui_state.phase = BATTLE_UI_IDLE;
            }

            global_battle_ui_state.timer += dt;
        } break;
        case BATTLE_UI_IDLE: {
            draw_turn_panel(state, framebuffer, 10, 100);
            bool is_player_turn = is_player_combat_turn(state);

            do_battle_selection_menu(state, framebuffer, framebuffer->width - BATTLE_SELECTIONS_WIDTH + 15, 100, is_player_turn);
            draw_battle_tooltips(state, framebuffer, dt, framebuffer->height, is_player_turn);

            if (global_battle_ui_state.submode == BATTLE_UI_SUBMODE_NONE) {
            }
        } break;
    }
}

local void render_combat_area_information(struct game_state* state, struct render_commands* commands, struct level_area* area) {
    struct level_area_navigation_map* navigation_map = &area->navigation_data;
    s32                               map_width               = navigation_map->width;
    s32                               map_height              = navigation_map->height;

    for (s32 y_cursor = 0; y_cursor < map_height; ++y_cursor) {
        for (s32 x_cursor = 0; x_cursor < map_width; ++x_cursor) {
            if (area->combat_movement_visibility_map[y_cursor * map_width + x_cursor]) {
                render_commands_push_quad(commands, rectangle_f32((x_cursor + navigation_map->min_x) * TILE_UNIT_SIZE, (y_cursor + navigation_map->min_y) * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE), color32u8(0, 0, 255, 128), BLEND_MODE_ALPHA);
            }
        }
    }

    if (global_battle_ui_state.submode == BATTLE_UI_SUBMODE_MOVING) {
        render_commands_push_quad(commands, rectangle_f32(global_battle_ui_state.movement_end_x * TILE_UNIT_SIZE, global_battle_ui_state.movement_end_y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE), color32u8(255, 0, 0, 128), BLEND_MODE_ALPHA);
    }

    /* draw nav path */
    {
        for (s32 index = 0; index < global_battle_ui_state.max_remembered_path_points_count; ++index) {
            v2f32 point = global_battle_ui_state.max_remembered_path_points[index];
            render_commands_push_quad(commands, rectangle_f32(point.x * TILE_UNIT_SIZE, point.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE), color32u8(255, 0, 255, 128), BLEND_MODE_ALPHA);
        }
    }
}
