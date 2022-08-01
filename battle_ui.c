enum battle_ui_animation_phase {
    BATTLE_UI_FADE_IN_DARK,
    BATTLE_UI_FLASH_IN_BATTLE_TEXT,
    BATTLE_UI_FADE_OUT,
    BATTLE_UI_MOVE_IN_DETAILS,
    BATTLE_UI_IDLE,
};
struct battle_ui_state {
    f32 timer;
    u32 phase;

    s32 selection;
} global_battle_ui_state;

enum battle_options{
    BATTLE_LOOK,
    BATTLE_MOVE,
    BATTLE_ATTACK,
    BATTLE_ABILITY,
    BATTLE_ITEM,
    BATTLE_DEFEND,
    BATTLE_WAIT,
}
local string battle_menu_main_options[] = {
    string_literal("LOOK"),
    string_literal("MOVE"),
    string_literal("ATTACK"),
    string_literal("ABILITY"),
    string_literal("ITEM"),
    string_literal("DEFEND"),
    string_literal("WAIT"),
};
local string battle_menu_main_option_descriptions[] = {
    string_literal("scout the battle field."),
    string_literal("pick a space to move to."),
    string_literal("use your weapons' basic attack."),
    string_literal("use an ability from your list."),
    string_literal("use an item from your party inventory."),
    string_literal("lose AP on next turn, for higher DEF."),
    string_literal("skip turn and conserve AP."),
};

local void start_combat_ui(void) {
    global_battle_ui_state.phase = 0;
    global_battle_ui_state.timer = 0;
}

local bool is_player_combat_turn(struct game_state* state) {
    struct game_state_combat_state* combat_state            = &state->combat_state;
    struct entity*                  active_combatant_entity = entity_list_dereference_entity(&state->entities, combat_state->participants[combat_state->active_combatant]);
    struct entity*                  player_entity           = entity_list_dereference_entity(&state->entities, player_id);

    if (player_entity == active_combatant_entity)
        return true;

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

    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

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
                case BATTLE_LOOK: {
                    
                } break;
                case BATTLE_MOVE: {
                    
                } break;
                case BATTLE_ATTACK: {
                    
                } break;
                case BATTLE_ABILITY: {
                    
                } break;
                case BATTLE_ITEM: {
                    
                } break;
                case BATTLE_DEFEND: {
                    
                } break;
                case BATTLE_WAIT: {
                    
                } break;
            }
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

    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(15, bottom_y - (16*4)), 36, 2, ui_color);

    /* do context dependent text... */
    { /* option tooltip */
        draw_ui_breathing_text(framebuffer, v2f32(30, bottom_y - (16*4) + 15), normal_font, 2,
                               battle_menu_main_option_descriptions[global_battle_ui_state.selection], 0, modulation_color);
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

            if (is_key_pressed(KEY_RETURN)) {
                struct entity* player_entity   = entity_list_dereference_entity(&state->entities, player_id);
                player_entity->waiting_on_turn = false;
            }
        } break;
    }
}
