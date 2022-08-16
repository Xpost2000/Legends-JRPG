enum shopping_ui_animation_phase {
    SHOPPING_UI_ANIMATION_PHASE_FADE_IN,
    SHOPPING_UI_ANIMATION_PHASE_SLIDE_IN_SHOPPING,
    SHOPPING_UI_ANIMATION_PHASE_IDLE,
    SHOPPING_UI_ANIMATION_PHASE_SLIDE_OUT_SHOPPING,
    SHOPPING_UI_ANIMATION_PHASE_PHASE_FADE_OUT,
};

#define MAX_CART_ENTRIES (1024)
struct {
    s32 phase;

    s32 shopping_page_filter;
    s32 shopping_item_index;

    /* remap the indices to match the current page */
    s16 cart_entry_count[MAX_CART_ENTRIES];

    /* also want to visibly animate coinage stuff */
    struct {
        s32 current;
        s32 target;

        f32 timer;
    } coin_visual;

    f32 timer;
} shopping_ui = {};

local void shopping_ui_begin(void) {
    _debugprintf("hi shopper");
    shopping_ui.phase = SHOPPING_UI_ANIMATION_PHASE_FADE_IN;
}

local void game_display_and_update_shop_ui(struct software_framebuffer* framebuffer, f32 dt)  {
    switch (shopping_ui.phase) {
        case SHOPPING_UI_ANIMATION_PHASE_FADE_IN: {
            
        } break;
        case SHOPPING_UI_ANIMATION_PHASE_SLIDE_IN_SHOPPING: {
            
        } break;
        case SHOPPING_UI_ANIMATION_PHASE_IDLE: {
            
        } break;
        case SHOPPING_UI_ANIMATION_PHASE_SLIDE_OUT_SHOPPING: {
            
        } break;
        case SHOPPING_UI_ANIMATION_PHASE_PHASE_FADE_OUT: {
            
        } break;
    }
}
