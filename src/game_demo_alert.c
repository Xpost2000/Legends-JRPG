/*
  Excuse to program a small preview screen for releasing as a demo.

  TODO: re-enable later.
*/

enum preview_demo_phase {
    PREVIEW_DEMO_FADE_IN_ALERT,
    PREVIEW_DEMO_HOLD_ALERT,
    PREVIEW_DEMO_FADE_OUT_ALERT,
};

struct {
    s32 phase;
    f32 timer;
} preview_demo_state = {
    .phase = PREVIEW_DEMO_FADE_IN_ALERT,
};

void update_and_render_preview_demo_alert(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    switch (preview_demo_state.phase) {
        case PREVIEW_DEMO_FADE_IN_ALERT: {
            f32 max_t = 0.5f;
            f32 t     = clamp_f32(preview_demo_state.timer / max_t, 0, 1);

            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(0, 0, framebuffer->width, framebuffer->height), game_get_font(MENU_FONT_COLOR_GOLD), 4, string_literal("THIS PROJECT IS IN-PROGRESS"), 4, color32f32(1, 1, 1, t));
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(0, 64, framebuffer->width, framebuffer->height), game_get_font(MENU_FONT_COLOR_GOLD), 4, string_literal("PLEASE PROVIDE FEEDBACK!"), 128, color32f32(1, 1, 1, t));
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(0, 128, framebuffer->width, framebuffer->height), game_get_font(MENU_FONT_COLOR_GOLD), 4, string_literal("THANK YOU!"), 4512, color32f32(1, 1, 1, t));

            if (preview_demo_state.timer > max_t) {
                preview_demo_state.phase = PREVIEW_DEMO_HOLD_ALERT;
                preview_demo_state.timer = 0;
            }
        } break;

        case PREVIEW_DEMO_HOLD_ALERT: {
            f32 max_t = 3.5f;
            f32 t     = clamp_f32(preview_demo_state.timer / max_t, 0, 1);

            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(0, 0, framebuffer->width, framebuffer->height), game_get_font(MENU_FONT_COLOR_GOLD), 4, string_literal("THIS PROJECT IS IN-PROGRESS"), 4, color32f32(1, 1, 1, 1));
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(0, 64, framebuffer->width, framebuffer->height), game_get_font(MENU_FONT_COLOR_GOLD), 4, string_literal("PLEASE PROVIDE FEEDBACK!"), 128, color32f32(1, 1, 1, 1));
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(0, 128, framebuffer->width, framebuffer->height), game_get_font(MENU_FONT_COLOR_GOLD), 4, string_literal("THANK YOU!"), 4512, color32f32(1, 1, 1, 1));

            if (preview_demo_state.timer > max_t) {
                preview_demo_state.phase = PREVIEW_DEMO_FADE_OUT_ALERT;
                preview_demo_state.timer = 0;
            }
        } break;

        case PREVIEW_DEMO_FADE_OUT_ALERT: {
            f32 max_t = 0.75f;
            f32 t     = clamp_f32(preview_demo_state.timer / max_t, 0, 1);

            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(0, 0, framebuffer->width, framebuffer->height), game_get_font(MENU_FONT_COLOR_GOLD), 4, string_literal("THIS PROJECT IS IN-PROGRESS"), 4, color32f32(1, 1, 1, 1-t));
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(0, 64, framebuffer->width, framebuffer->height), game_get_font(MENU_FONT_COLOR_GOLD), 4, string_literal("PLEASE PROVIDE FEEDBACK!"), 128, color32f32(1, 1, 1, 1-t));
            draw_ui_breathing_text_centered(framebuffer, rectangle_f32(0, 128, framebuffer->width, framebuffer->height), game_get_font(MENU_FONT_COLOR_GOLD), 4, string_literal("THANK YOU!"), 4512, color32f32(1, 1, 1, 1-t));

            if (preview_demo_state.timer > max_t) {
                set_game_screen_mode(GAME_SCREEN_MAIN_MENU);
                preview_demo_state.timer = 0;
            }
        } break;
    }

    preview_demo_state.timer += dt;
}
