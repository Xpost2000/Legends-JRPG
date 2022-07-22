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

local void update_and_render_battle_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    switch (global_battle_ui_state.phase) {
        case BATTLE_UI_FADE_IN_DARK: {
            
        } break;
        case BATTLE_UI_FADE_IN_BATTLE_TEXT: {
            
        } break;
        case BATTLE_UI_FADE_OUT: {
            
        } break;
        case BATTLE_UI_MOVE_IN_DETAILS: {
            
        } break;
        case BATTLE_UI_IDLE: {
            
        } break;
    }
}
