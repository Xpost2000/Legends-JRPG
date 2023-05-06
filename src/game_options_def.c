#ifndef GAME_OPTIONS_DEF
#define GAME_OPTIONS_DEF

/*
  Other data such as timing data will go here.

  Or multiplier tables will also go here
*/

local string difficulty_setting_strings[] = {
    string_literal("SIMPLE"),
    string_literal("NORMAL"),
    string_literal("TACTICIAN"),
};

local string message_setting_strings[] = {
    string_literal("SLOW"),
    string_literal("NORMAL"),
    string_literal("FAST"),
    string_literal("INSTANT"),
};

local string ui_theme_setting_strings[] = {
    string_literal("AZURE"),
    string_literal("DEMON"),
    string_literal("ROYAL"),
};


/*
  TODO: none of this is used yet.

*/
struct game_options {
    /* NOTE: should be in a save file. */
    s32 difficulty;
    s32 messagespeed;
    s32 ui_theme;

    f32 music_volume;
    f32 sound_volume;

    /* NOTE: can be saved outside of savefile. */
    bool fullscreen;
    s32 screen_width;
    s32 screen_height;

    s32 resolution_index; /* Use this to find the filled resolution */
};

local struct game_options global_game_options = {};

local void game_options_load_from_file(string where);
local void game_options_write_to_file(string where);
local void game_apply_options(void);

#endif
