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
} global_battle_ui_state;

local void start_combat_ui(void) {
    global_battle_ui_state.phase = 0;
    global_battle_ui_state.timer = 0;
}

local void draw_turn_panel(struct game_state* state, struct software_framebuffer* framebuffer, f32 x, f32 y) {
    struct game_state_combat_state* combat_state = &state->combat_state;

    for (s32 index = 0; index < combat_state->count; ++index) {
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

        const s32 square_size = 32;
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(x, y + index * square_size * 1.3, square_size, square_size), color, BLEND_MODE_ALPHA);
    }
}

local void update_and_render_battle_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache*              font         = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
    struct game_state_combat_state* combat_state = &state->combat_state;

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

            if (global_battle_ui_state.timer >= max_t) {
                global_battle_ui_state.timer = 0;
                global_battle_ui_state.phase = BATTLE_UI_IDLE;
            }

            global_battle_ui_state.timer += dt;
        } break;
        case BATTLE_UI_IDLE: {
            draw_turn_panel(state, framebuffer, 10, 100);
        } break;
    }
}
