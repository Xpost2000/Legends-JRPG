#ifndef COMMON_UI_DEF_C
#define COMMON_UI_DEF_C

/*
  COMMON GAME_UI implementations
  NOTE: semi-standardized UI widgets for the game's UIs.

  With very basic layout.

  This is not a full UI system, it's just a small collection of widgets so I can quickly implement mouse support.
  By and large most of the UI is still hand-written because the only widgets I can discern are buttons, and windows.

  Most other widgets are specialized or otherwise purely visual.
*/
local struct font_cache* common_ui_text_normal;
local struct font_cache* common_ui_text_highlighted;
local struct font_cache* common_ui_text_focused;

struct common_ui_layout { /* NOTE: vertical layout for now, since all the game stuff uses that */
    f32 x;
    f32 y;
};

struct common_ui_layout common_ui_vertical_layout(f32 x, f32 y) {
    return (struct common_ui_layout) {
        .x = x,
        .y = y,
    };
}

void common_ui_init(void);

/* NOTE: all UI is assumed to happen on an identity transformation */
/* Ideally I'd also push render commands for this but whatever */
enum common_ui_button_flags {
    COMMON_UI_BUTTON_FLAGS_NONE      = 0,
    COMMON_UI_BUTTON_FLAGS_DISABLED  = BIT(0),
    COMMON_UI_BUTTON_FLAGS_BREATHING = BIT(1),
};
bool common_ui_button(struct common_ui_layout* layout, struct software_framebuffer* framebuffer, string text, f32 scale, s32 button_id, s32* selected_id, u32 flags);
enum common_ui_visual_slider_flags {
    COMMON_UI_VISUAL_SLIDER_FLAGS_NONE = 0,
    COMMON_UI_VISUAL_SLIDER_FLAGS_LOTSOFOPTIONS = BIT(0),
};

/*               ||                     */
/* | OPTION1 | OPTION2 | OPTION_N | ... */
void common_ui_visual_slider(struct common_ui_layout* layout, struct software_framebuffer* framebuffer, f32 scale, string* strings, s32 count, s32* option_ptr, s32 slider_id, s32* selected_id, u32 flags);
/* COMMON GAME_UI implementations end */

enum common_ui_checkbox_flags {
    COMMON_UI_CHECKBOX_FLAGS_NONE = 0,
};
void common_ui_checkbox(struct common_ui_layout* layout, struct software_framebuffer* framebuffer, s32 checkbox_id, s32* selected_id, bool* option_ptr, u32 flags);
void common_ui_f32_slider(struct common_ui_layout* layout, struct software_framebuffer* framebuffer, f32 width, s32* selected_id, s32 slider_id, f32* ptr, f32 min_bound, f32 max_bound, f32 step);

#endif
