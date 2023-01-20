/*
  NOTE:

  The credits should include a mode7 preview of the world (and various other screen caps of the world),
  but it's okay...

  Obviously, most of this can't be done until the game is finished, but this serves as a nicely flashy thing.

  This should also not change obviously too much... Since it looks nice, and it's a credits screen anyways.

  I'm putting effort in the animation here since I like stuff that animates nicely, and also so it looks nice enough
  that I will never want to change it anyways.
*/

#define CREDITS_TEXT_TYPING_SPEED (0.1275)

enum credits_state_phases {
    CREDITS_PHASE_FADE_IN_TITLE,
    CREDITS_PHASE_SLIDE_TITLE_UP,

    CREDITS_PHASE_MAIN_ANIMATION,

    CREDITS_PHASE_SLIDE_TITLE_DOWN,
    CREDITS_PHASE_FADE_OUT_TITLE,
    CREDITS_PHASE_SEE_YOU_NEXT_TIME,
    CREDITS_PHASE_SEE_YOU_NEXT_TIME_FADE_OUT,
};

local string see_you_next_time_phase_strings[] = {
    string_literal("THIS ISN'T THE END YET."),
    string_literal("SEE YOU NEXT TIME."),
    string_literal(":)"),
};

struct see_you_next_time_phase_data {
    s32 category_string_index;
    s32 credit_string_index;
    s32 type_down;
};

enum credits_main_animation_phases {
    CREDITS_MAIN_ANIMATION_PHASE_FADE_IN_CATEGORY,
    CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_UP,
    CREDITS_MAIN_ANIMATION_PHASE_TYPE_IN_CREDITS,
    CREDITS_MAIN_ANIMATION_PHASE_TYPE_OUT_CREDITS,
    CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_DOWN,
    CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_OUT_OF_FRAME,
};

struct credits_main_animation_phase_data {
    s32 subphase;
    s32 current_credit_line_index;
};

struct credits_state {
    s32 phase;
    s32 typing_characters_shown;
    f32 timer;

    struct see_you_next_time_phase_data      see_you_next_time;
    struct credits_main_animation_phase_data main_animation;

    /*
      NOTE:
      This is for the debug mode so I can time how long a music track should be for the
      credits
    */
#ifndef RELEASE
    f32 total_elapsed_time;
#endif
};

local struct credits_state credits_state = {};

void enter_credits(void) {
    setup_credits_mode();
    set_game_screen_mode(GAME_SCREEN_CREDITS);
}

void setup_credits_mode(void) {
    zero_memory(&credits_state, sizeof(credits_state));
}

local void _credits_set_phase(s32 phase) {
#ifndef RELEASE
    f32 elapsed_time = credits_state.total_elapsed_time;
#endif
    zero_memory(&credits_state, sizeof(credits_state));
    credits_state.phase                   = phase;
#ifndef RELEASE
    credits_state.total_elapsed_time      = elapsed_time;
#endif
}

local void _draw_credits_game_title(struct software_framebuffer* framebuffer, f32 y, f32 alpha) {
    struct font_cache* heading_font  = game_get_font(MENU_FONT_COLOR_GOLD);
    f32                scale         = 4;
    f32                content_width = font_cache_text_width(heading_font, game_title, scale);

    software_framebuffer_draw_text(framebuffer, heading_font, scale, v2f32(SCREEN_WIDTH/2 - content_width/2, y), game_title, color32f32(1,1,1,alpha), BLEND_MODE_ALPHA);
}

/* this is moved into a function since it's more complicated logic than the other stuff */
struct credit_lines {string* list; s32 count;};

local string credits_categories[] = {
    string_literal("SCENARIO BY"),
    string_literal("LEVEL DESIGN"),
    string_literal("GAME DESIGN"),
    string_literal("PROGRAMMING"),
    string_literal("GRAPHICS"),
    string_literal("SOUND EFFECTS"),
    string_literal("MUSIC"),
    string_literal("Q&A"),
    string_literal("SPECIAL THANKS"),
};

/*
  for now credits are hardcoded into the engine executable. Will resolve to fill this out from a data file later,
  but it's not essential.
*/

local string writing_credits[] = {
    string_literal("Jerry Zhu"),
};
local string level_design_credits[] = {
    string_literal("Jerry Zhu"),
};
local string game_design_credits[] = {
    string_literal("Jerry Zhu"),
    string_literal("With inspiration from Square's Tactics RPGs"),
    string_literal("With inspiration from Nippon Ichi's Disgaea series"),
};
local string programming_credits[] = {
    string_literal("Jerry Zhu"),
};
local string graphics_credits[] = {
    string_literal("Jerry Zhu"),
    string_literal("gnsh from OpenGameArt for the font"),
};
local string sound_effects_credits[] = {
    string_literal("Jerry Zhu with the aid of SFXR"),
};
local string music_credits[] = {
    string_literal("no music yet"),
};
local string quality_assurance_credits[] = {
    string_literal("no QA yet"),
};
local string special_thanks_credits[] = {
    string_literal("All of my friends that support me.")
};

#define new_credit_line(me) (struct credit_lines) {.list = me, .count = array_count(me)},

local struct credit_lines credits_lines[] = {
    new_credit_line(writing_credits)
    new_credit_line(level_design_credits)
    new_credit_line(game_design_credits)
    new_credit_line(programming_credits)
    new_credit_line(graphics_credits)
    new_credit_line(sound_effects_credits)
    new_credit_line(music_credits)
    new_credit_line(quality_assurance_credits)
    new_credit_line(special_thanks_credits)
};

#undef new_credit_line

local void update_and_render_credits_phase_main_animation(struct software_framebuffer* framebuffer, f32 dt, f32 bottom_of_title_y) {
    struct credits_main_animation_phase_data* main_animation = &credits_state.main_animation;

    struct font_cache* heading_font  = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* subtitle_font = game_get_font(MENU_FONT_COLOR_ORANGE);
    struct font_cache* content_font  = game_get_font(MENU_FONT_COLOR_STEEL);

    switch (main_animation->phase) { 
        case CREDITS_MAIN_ANIMATION_PHASE_FADE_IN_CATEGORY: {
            
        } break;

        case CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_DOWN: 
        case CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_UP: {
        
        } break;

        case CREDITS_MAIN_ANIMATION_PHASE_TYPE_IN_CREDITS:
        case CREDITS_MAIN_ANIMATION_PHASE_TYPE_OUT_CREDITS: {
            
        } break;

        case CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_OUT_OF_FRAME: {
        
        } break;
    }
}

local void update_and_render_credits_see_you_next_time(struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* content_font  = game_get_font(MENU_FONT_COLOR_STEEL);

    const string base_string    = see_you_next_time_phase_strings[credits_state.see_you_next_time.string_index];
    bool         is_last_string  = credits_state.see_you_next_time.string_index == array_count(see_you_next_time_phase_strings)-1;
    string       to_show        = string_slice(base_string, 0, credits_state.typing_characters_shown);
    f32          scale          = 4;

    if (is_last_string) {
        scale = 8;
    }

    f32 content_width  = font_cache_text_width(content_font, to_show, scale);
    f32 content_height = font_cache_text_height(content_font) * scale;

    /* would like to have more lines to type. */
    if (credits_state.see_you_next_time.type_down == 0) {
        if (credits_state.typing_characters_shown < base_string.length) {
            if (credits_state.timer >= CREDITS_TEXT_TYPING_SPEED) {
                credits_state.typing_characters_shown += 1;
                credits_state.timer                    = 0;
            } else {
                credits_state.timer += dt;
            }
        } else {
            if (credits_state.timer >= 1.0) {
                credits_state.see_you_next_time.type_down = 1;
                credits_state.timer                       = 0;
            } else {
            }
        }
    } else {
        if (credits_state.typing_characters_shown > 0) {
            if (!is_last_string) {
                if (credits_state.timer >= CREDITS_TEXT_TYPING_SPEED) {
                    credits_state.typing_characters_shown -= 1;
                    credits_state.timer                    = 0;
                } else {
                    credits_state.timer += dt;
                }
            } else {
                if (credits_state.timer >= 0.75) {
                    credits_state.typing_characters_shown -= 1;
                    credits_state.timer                    = 0;
                } else {
                    credits_state.timer += dt;
                }
            }
        } else {
            if (is_last_string) {
                if (credits_state.timer >= 10.5) {
                    /* finish credits */
#ifndef RELEASE
                    _debugprintf("CREDITS TOTAL TIME: %3.3f seconds", credits_state.total_elapsed_time);
#endif
                    set_game_screen_mode(last_screen_mode);
                }
            } else {
                if (credits_state.timer >= 1.0) {
                    credits_state.see_you_next_time.string_index += 1;
                    credits_state.see_you_next_time.type_down     = 0;
                    credits_state.timer = 0;
                }
            }
        }
    }

    software_framebuffer_draw_text(framebuffer, content_font, scale, v2f32(SCREEN_WIDTH/2 - content_width/2, SCREEN_HEIGHT/2 - content_height/2), to_show, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
}

void update_and_render_credits_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    software_framebuffer_clear_buffer(framebuffer, color32u8(0,0,0,255));
    credits_state.total_elapsed_time += dt;

    struct font_cache* heading_font  = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* subtitle_font = game_get_font(MENU_FONT_COLOR_ORANGE);
    struct font_cache* content_font  = game_get_font(MENU_FONT_COLOR_STEEL);

    f32                base_font_height = font_cache_text_height(heading_font);

    switch (credits_state.phase) {
        case CREDITS_PHASE_FADE_OUT_TITLE:
        case CREDITS_PHASE_FADE_IN_TITLE: {
            const f32 MAX_PHASE_T   = 2.0;
            const f32 MAX_FADE_IN_T = MAX_PHASE_T - 1.0;
            f32       alpha         = clamp_f32(credits_state.timer / MAX_FADE_IN_T, 0, 1);

            s32 next_phase = CREDITS_PHASE_SLIDE_TITLE_UP;

            if (credits_state.phase == CREDITS_PHASE_FADE_OUT_TITLE) {
                alpha      = 1 - alpha;
                next_phase = CREDITS_PHASE_SLIDE_TITLE_DOWN;
            }

            _draw_credits_game_title(framebuffer, SCREEN_HEIGHT/2 - base_font_height, alpha);

            if (credits_state.timer >= MAX_PHASE_T) {
                _credits_set_phase(next_phase);
                return;
            }
        } break;

        case CREDITS_PHASE_SLIDE_TITLE_DOWN:
        case CREDITS_PHASE_SLIDE_TITLE_UP: {
            const f32 MAX_PHASE_T = 1.25;
            const f32 MAX_LERP_T  = MAX_PHASE_T - 0.55;
            f32       alpha       = clamp_f32(credits_state.timer / MAX_LERP_T, 0, 1);

            s32 next_phase = CREDITS_PHASE_MAIN_ANIMATION;
            if (credits_state.phase == CREDITS_PHASE_SLIDE_TITLE_DOWN) {
                alpha      = 1 - alpha;
                next_phase = CREDITS_PHASE_SEE_YOU_NEXT_TIME;
            }

            f32       y           = lerp_f32(SCREEN_HEIGHT/2 - base_font_height, 10, alpha);

            _draw_credits_game_title(framebuffer, y, 1.0);

            if (credits_state.timer >= MAX_PHASE_T) {
                _credits_set_phase(next_phase);
                return;
            }
        } break;

        case CREDITS_PHASE_MAIN_ANIMATION: {
            f32 y = SCREEN_HEIGHT/2 - base_font_height;
            _draw_credits_game_title(framebuffer, y, 1.0);
            update_and_render_credits_phase_main_animation(framebuffer, dt, y);
        } break;

        case CREDITS_PHASE_SEE_YOU_NEXT_TIME: { /* maybe add a guy that jumps or something to be adorable */
            update_and_render_credits_see_you_next_time(framebuffer, dt);
        } break;
    }
    credits_state.timer += dt;
}
