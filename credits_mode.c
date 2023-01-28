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
#define CREDITS_TEXT_TYPING_SPEED1 (0.1005)

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
    s32 string_index;
    s32 type_down;
};

enum credits_main_animation_phases {
    CREDITS_MAIN_ANIMATION_PHASE_FADE_IN_CATEGORY,
    CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_UP,
    CREDITS_MAIN_ANIMATION_PHASE_TYPE_CREDITS,
    CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_DOWN,
    CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_OUT_OF_FRAME,
};

struct credits_main_animation_phase_data {
    s32 phase;
    s32 current_category_index;
    s32 current_credit_line_index;
    s32 type_in_credits;
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

bool     loaded_credits_music = false;
sound_id credits_music  = {};
void enter_credits(void) {
    setup_credits_mode();
    set_game_screen_mode(GAME_SCREEN_CREDITS);
    play_sound(credits_music);
}

void setup_credits_mode(void) {
    if (!loaded_credits_music) {
        loaded_credits_music = true;
        credits_music        = load_sound(string_literal(GAME_DEFAULT_RESOURCE_PATH "snds/vesperiatestcredits.ogg"), true);
    }
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
#ifdef EXPERIMENTAL_320
    f32                scale         = 2;
#else
    f32                scale         = 4;
#endif
    f32                content_width = font_cache_text_width(heading_font, game_title, scale);

    software_framebuffer_draw_text(framebuffer, heading_font, scale, v2f32(SCREEN_WIDTH/2 - content_width/2, y), game_title, color32f32(1,1,1,alpha), BLEND_MODE_ALPHA);
}

/* this is moved into a function since it's more complicated logic than the other stuff */
struct credit_lines {string* list; s32 count;};

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
};
local string programming_credits[] = {
    string_literal("Jerry Zhu"),
};
local string graphics_credits[] = {
    string_literal("Jerry Zhu"),
    string_literal("Font from OpenGameArt"),
};
local string sound_effects_credits[] = {
    string_literal("Jerry Zhu"),
};
local string music_credits[] = {
    string_literal("Tales of Vesperia(credits)"),
};
local string quality_assurance_credits[] = {
    string_literal("no QA yet"),
};
local string special_thanks_credits[] = {
    string_literal("My friends.")
};
local string inspired_credits[] = {
    string_literal("Final Fantasy Tactics"),
    string_literal("Disgaea"),
};

#define new_credit_line(me) (struct credit_lines) {.list = me, .count = array_count(me)},

local struct credit_lines credits_lines[] = {
    new_credit_line(writing_credits)
    new_credit_line(level_design_credits)
    new_credit_line(game_design_credits)
    new_credit_line(inspired_credits)
    new_credit_line(programming_credits)
    new_credit_line(graphics_credits)
    new_credit_line(sound_effects_credits)
    new_credit_line(music_credits)
    new_credit_line(quality_assurance_credits)
    new_credit_line(special_thanks_credits)
};

#undef new_credit_line

local string credits_categories[] = {
    string_literal("STORY / SCENARIO"),
    string_literal("LEVEL DESIGN"),
    string_literal("GAME DESIGN"),
    string_literal("INSPIRED BY"),
    string_literal("PROGRAMMING"),
    string_literal("GRAPHICS"),
    string_literal("SOUND EFFECTS"),
    string_literal("MUSIC"),
    string_literal("Q&A"),
    string_literal("SPECIAL THANKS"),
};

local void _credits_draw_category_title(struct software_framebuffer* framebuffer, struct font_cache* font, f32 time_for_slide, f32 y, s32 index, f32 alpha) {
    string title         = credits_categories[index];
#ifdef EXPERIMENTAL_320
    f32    scale         = 2;
#else
    f32    scale         = 4;
#endif
    f32    content_width = font_cache_text_width(font, title, scale);
    f32    x             = lerp_f32(SCREEN_WIDTH/2 - content_width/2, -content_width*1.2, time_for_slide);
    software_framebuffer_draw_text(framebuffer, font, scale, v2f32(x, y+50), title, color32f32(1,1,1,alpha), BLEND_MODE_ALPHA);
}

local void update_and_render_credits_phase_main_animation(struct software_framebuffer* framebuffer, f32 dt, f32 bottom_of_title_y) {
    struct credits_main_animation_phase_data* main_animation = &credits_state.main_animation;

    struct font_cache* heading_font     = game_get_font(MENU_FONT_COLOR_GOLD);
    struct font_cache* subtitle_font    = game_get_font(MENU_FONT_COLOR_ORANGE);
    struct font_cache* content_font     = game_get_font(MENU_FONT_COLOR_STEEL);
    f32                base_font_height = font_cache_text_height(heading_font);

    switch (main_animation->phase) { 
        case CREDITS_MAIN_ANIMATION_PHASE_FADE_IN_CATEGORY: {
            f32 MAX_PHASE_T = 1.0f;
            f32 MAX_FADE_T  = MAX_PHASE_T-0.5;

            f32 alpha = clamp_f32(credits_state.timer / MAX_FADE_T, 0, 1);
            _credits_draw_category_title(framebuffer, subtitle_font, 0, SCREEN_HEIGHT/2-base_font_height, main_animation->current_category_index, alpha);

            if (credits_state.timer >= MAX_PHASE_T) {
                credits_state.timer   = 0;
                main_animation->phase = CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_UP;
            }
        } break;

        case CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_DOWN: 
        case CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_UP: {
            f32 MAX_PHASE_T = 1.0f;
            f32 MAX_SLIDE_T  = MAX_PHASE_T-0.5;

            f32 alpha = clamp_f32(credits_state.timer / MAX_SLIDE_T, 0, 1);

            s32 next_phase = CREDITS_MAIN_ANIMATION_PHASE_TYPE_CREDITS;

            if (main_animation->phase == CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_DOWN) {
                next_phase = CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_OUT_OF_FRAME;
                alpha = 1.0 - alpha;
            }

            f32 y = lerp_f32(SCREEN_HEIGHT/2-base_font_height, bottom_of_title_y+base_font_height, alpha);
            _credits_draw_category_title(framebuffer, subtitle_font, 0, y, main_animation->current_category_index, 1.0);

            if (credits_state.timer >= MAX_PHASE_T) {
                credits_state.timer   = 0;
                main_animation->phase = next_phase;
                return;
            }
        } break;

        case CREDITS_MAIN_ANIMATION_PHASE_TYPE_CREDITS: {
            f32 y_for_category = bottom_of_title_y+base_font_height;
            _credits_draw_category_title(framebuffer, subtitle_font, 0, y_for_category, main_animation->current_category_index, 1.0);


            if (main_animation->current_credit_line_index < (credits_lines[main_animation->current_category_index].count)) {
                string base_string = credits_lines[main_animation->current_category_index].list[main_animation->current_credit_line_index];
                string to_show     = string_slice(base_string, 0, credits_state.typing_characters_shown);

                /* NOTE: I want the animation to "erase" all the lines, and that requires a slight rework of this logic. It's not too difficult but not something I want to do right now. */
                if (main_animation->type_in_credits == 0) {
                    if (credits_state.typing_characters_shown < base_string.length) {
                        if (credits_state.timer >= CREDITS_TEXT_TYPING_SPEED1) {
                            credits_state.timer = 0;
                            credits_state.typing_characters_shown += 1;
                        } else {
                        }
                    } else {
                        if (credits_state.timer >= 1.5) {
                            credits_state.timer             = 0;
                            main_animation->type_in_credits = 1;
                        }
                    }
                } else {
                    if (credits_state.typing_characters_shown > 0) {
                        if (credits_state.timer >= CREDITS_TEXT_TYPING_SPEED1) {
                            credits_state.timer = 0;
                            credits_state.typing_characters_shown -= 1;
                        } else {
                        }
                    } else {
                        if (credits_state.timer >= 0.25) {
                            credits_state.timer                        = 0;
                            main_animation->type_in_credits            = 0;
                            main_animation->current_credit_line_index += 1;
                            credits_state.typing_characters_shown    = 0;
                        }
                    }
                }

                {
                    #ifdef EXPERIMENTAL_320
                    f32 content_scale  = 2;
                    #else
                    f32 content_scale  = 4;
                    #endif
                    f32 content_height = font_cache_text_height(content_font) * content_scale;
                    {
                        f32 y = SCREEN_HEIGHT/2 - content_height/2;
                        f32 content_width  = font_cache_text_width(content_font, base_string, content_scale);
                        software_framebuffer_draw_text(framebuffer, content_font, content_scale, v2f32(SCREEN_WIDTH/2 - content_width/2, y), to_show, color32f32(1,1,1,1), BLEND_MODE_ALPHA);
                    }
                }
            } else {
                main_animation->phase = CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_DOWN;
                credits_state.timer = 0;
            }
        } break;

        case CREDITS_MAIN_ANIMATION_PHASE_SLIDE_CATEGORY_OUT_OF_FRAME: {
            f32 MAX_PHASE_T = 1.0f;
            f32 MAX_SLIDE_T  = MAX_PHASE_T-0.5;

            f32 alpha = clamp_f32(credits_state.timer / MAX_SLIDE_T, 0, 1);
            _credits_draw_category_title(framebuffer, subtitle_font, alpha, SCREEN_HEIGHT/2-base_font_height, main_animation->current_category_index, 1.0);

            if (credits_state.timer >= MAX_PHASE_T) {
                credits_state.timer                        = 0;
                main_animation->current_category_index    += 1;
                main_animation->current_credit_line_index  = 0;
                if (main_animation->current_category_index >= array_count(credits_categories)) {
                    credits_state.phase = CREDITS_PHASE_SLIDE_TITLE_DOWN;
                } else {
                    main_animation->phase = CREDITS_MAIN_ANIMATION_PHASE_FADE_IN_CATEGORY;
                }
                return;
            }
        } break;
    }
}

local void update_and_render_credits_see_you_next_time(struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache* content_font  = game_get_font(MENU_FONT_COLOR_STEEL);

    const string base_string    = see_you_next_time_phase_strings[credits_state.see_you_next_time.string_index];
    bool         is_last_string = credits_state.see_you_next_time.string_index == array_count(see_you_next_time_phase_strings)-1;
    string       to_show        = string_slice(base_string, 0, credits_state.typing_characters_shown);
    #ifdef EXPERIMENTAL_320
    f32          scale          = 2;
    #else
    f32          scale          = 4;
    #endif

    if (is_last_string) {
    #ifdef EXPERIMENTAL_320
        scale = 4;
    #else
        scale = 8;
    #endif
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
                }
            } else {
                if (credits_state.timer >= 0.75) {
                    credits_state.typing_characters_shown -= 1;
                    credits_state.timer                    = 0;
                } else {
                }
            }
        } else {
            if (is_last_string) {
                if (credits_state.timer >= 8.5) {
                    /* finish credits */
#ifndef RELEASE
                    _debugprintf("CREDITS TOTAL TIME: %3.3f seconds", credits_state.total_elapsed_time);
#endif
                    stop_music();
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
#ifndef RELEASE
    credits_state.total_elapsed_time += dt;
#endif

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
            f32 y = 10;
            _draw_credits_game_title(framebuffer, y, 1.0);
            update_and_render_credits_phase_main_animation(framebuffer, dt, y);
        } break;

        case CREDITS_PHASE_SEE_YOU_NEXT_TIME: { /* maybe add a guy that jumps or something to be adorable */
            update_and_render_credits_see_you_next_time(framebuffer, dt);
        } break;
    }
    credits_state.timer += dt;
}
