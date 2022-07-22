/* area change code. */

/* TODO: I don't have an idea of world regions yet, but here it is for later. This will likely not change */
/* NOTE: I assume that screen fade has already happened from here! */
enum region_zone_animation_phases {
    REGION_ZONE_ANIMATION_PHASE_NONE,
    REGION_ZONE_ANIMATION_PHASE_LINGER, 
    REGION_ZONE_ANIMATION_PHASE_FADE_IN_TEXT,
    REGION_ZONE_ANIMATION_PHASE_LINGER_TEXT,
    /* NOTE: I want a more elaborate animation. However I also want this to be possible tonight. So more elaborate is in the future. */
    REGION_ZONE_ANIMATION_PHASE_FADE_AWAY,
};

local f32  region_zone_animation_timer  = 0.0f;
local s32  region_zone_animation_phase  = 0;

local bool region_zone_animation_block_input = false;

void initialize_region_zone_change(void) {
    region_zone_animation_timer       = 1.5f;
    region_zone_animation_phase       = REGION_ZONE_ANIMATION_PHASE_LINGER;
    region_zone_animation_block_input = true;
    
}

void update_and_render_region_zone_change(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* font = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
    switch (region_zone_animation_phase) {
        case REGION_ZONE_ANIMATION_PHASE_LINGER: {
            software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0,255), BLEND_MODE_ALPHA);
            if (region_zone_animation_timer < 0.0) {
                region_zone_animation_phase++;
            }
            region_zone_animation_timer -= dt;
        } break;
        case REGION_ZONE_ANIMATION_PHASE_FADE_IN_TEXT: {
            software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0,255), BLEND_MODE_ALPHA);

            f32 text_alpha = region_zone_animation_timer / 1.2f;
            if (text_alpha > 1.0) text_alpha = 1.0;
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 6, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), string_from_cstring(state->current_region_name), color32f32(1,1,1,text_alpha), BLEND_MODE_ALPHA);

            if (region_zone_animation_timer > 1.9f) {
                f32 text_alpha = (region_zone_animation_timer-1.9f) / 0.6f;
                if (text_alpha > 1.0) text_alpha = 1.0;

                software_framebuffer_draw_text_bounds_centered(framebuffer, font, 4, rectangle_f32(0,80,SCREEN_WIDTH,SCREEN_HEIGHT+80), string_from_cstring(state->current_region_subtitle), color32f32(1,1,1,text_alpha), BLEND_MODE_ALPHA);
            }

            if (region_zone_animation_timer > 2.5f) {
                region_zone_animation_timer = 1.5f;
                region_zone_animation_phase++;
            }

            region_zone_animation_timer += dt;
        } break;
        case REGION_ZONE_ANIMATION_PHASE_LINGER_TEXT: {
            software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0,255), BLEND_MODE_ALPHA);
            if (region_zone_animation_timer < 0.0) {
                region_zone_animation_phase++;
            }
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 6, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), string_from_cstring(state->current_region_name), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 4, rectangle_f32(0,80,SCREEN_WIDTH,SCREEN_HEIGHT+80), string_from_cstring(state->current_region_subtitle), color32f32(1,1,1,1), BLEND_MODE_ALPHA);
            region_zone_animation_timer -= dt;
        } break;
        case REGION_ZONE_ANIMATION_PHASE_FADE_AWAY: {

            f32 text_alpha = region_zone_animation_timer / 1.2f;
            f32 text_alpha2 = 0;
            f32 background_alpha = 0;
            if (text_alpha > 1.0) text_alpha = 1.0;
            if (region_zone_animation_timer > 1.9f) {
                text_alpha2 = (region_zone_animation_timer-1.9f) / 0.6f;
                if (text_alpha2 > 1.0) text_alpha2 = 1.0;

                if (region_zone_animation_timer > 2.3f) {
                    background_alpha = (region_zone_animation_timer-2.3f) / 0.6f;
                    if (background_alpha > 1.0) background_alpha = 1.0f;
                }
            }

            software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0,255 * (1-background_alpha)), BLEND_MODE_ALPHA);
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 6, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), string_from_cstring(state->current_region_name), color32f32(1,1,1,1-text_alpha), BLEND_MODE_ALPHA);
            software_framebuffer_draw_text_bounds_centered(framebuffer, font, 4, rectangle_f32(0,80,SCREEN_WIDTH,SCREEN_HEIGHT+80), string_from_cstring(state->current_region_subtitle), color32f32(1,1,1,1-text_alpha2), BLEND_MODE_ALPHA);

            if (region_zone_animation_timer > 2.9f) {
                region_zone_animation_block_input = false;
                region_zone_animation_phase       = REGION_ZONE_ANIMATION_PHASE_NONE;
            }

            region_zone_animation_timer += dt;
        } break;
    }

}
