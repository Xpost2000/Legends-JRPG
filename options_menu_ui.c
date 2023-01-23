enum options_menu_phases {
    OPTIONS_MENU_PHASE_CLOSE,
    OPTIONS_MENU_PHASE_OPEN,
};

struct options_menu_state {
    s32 phase;
};

struct options_menu_state options_menu_state;

void options_menu_open(void) {
    options_menu_state.phase = OPTIONS_MENU_PHASE_OPEN;
}

void options_menu_close(void) {
    options_menu_state.phase = OPTIONS_MENU_PHASE_CLOSE;
}

s32 do_options_menu(struct software_framebuffer* framebuffer, f32 dt) {
    switch (options_menu_state.phase) {
        case OPTIONS_MENU_PHASE_OPEN: {
            
        } break;
        case OPTIONS_MENU_PHASE_CLOSE: {
            return OPTIONS_MENU_PROCESS_ID_EXIT;
        } break;
    }
    return OPTIONS_MENU_PROCESS_ID_OKAY;
}
