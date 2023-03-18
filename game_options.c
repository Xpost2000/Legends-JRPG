local void game_options_load_from_file(string where) {
    _debugprintf("TODO: this is not done! But I don't want to crash!");
}

local void game_options_write_to_file(string where) {
    _debugprintf("TODO: this is not done! But I don't want to crash!");
}

local void game_apply_options(void) {
    struct screen_resolution resolution = queried_screen_resolutions[global_game_options.resolution_index];

    global_game_options.screen_width = resolution.w;
    global_game_options.screen_height = resolution.h;

    set_fullscreen(global_game_options.fullscreen);
    change_resolution(resolution.w, resolution.h);
}
