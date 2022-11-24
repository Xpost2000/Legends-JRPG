#ifndef PAUSE_MENU_UI_DEF_C
#define PAUSE_MENU_UI_DEF_C

enum ui_pause_menu_animation_state{
    UI_PAUSE_MENU_TRANSITION_IN,
    UI_PAUSE_MENU_NO_ANIM,
    UI_PAUSE_MENU_TRANSITION_CLOSING,
};

enum ui_pause_menu_sub_menu_state {
    UI_PAUSE_MENU_SUB_MENU_STATE_NONE      = 0,
    UI_PAUSE_MENU_SUB_MENU_STATE_INVENTORY = 1,
    UI_PAUSE_MENU_SUB_MENU_STATE_EQUIPMENT = 2,
    UI_PAUSE_MENU_SUB_MENU_STATE_OPTIONS   = 3,
};

/* share this for all types of similar menu types. */
/* could do for a renaming. */
struct ui_pause_menu {
    u8  animation_state; /* 0 = OPENING, 1 = NONE, 2 = CLOSING */
    f32 transition_t;
    s32 selection;

    /* NOTE we can use this field instead of that weird thing in the editor_state */
    /* other than the fact that the editor is actually ripping this state from the game state */
    /* in reality, I guess this should actually just be a global variable LOL. Or otherwise shared state */
    /* since preferably the game_state is almost exclusively only serializing stuff. */
    s32 last_sub_menu_state;
    s32 sub_menu_state;
    /* reserved space */
    f32 shift_t[128];
};

enum ui_pause_menu_gameplay_options {
    PAUSE_MENU_RESUME,
    PAUSE_MENU_PARTY_EQUIPMENT,
    PAUSE_MENU_PARTY_ITEMS,
    PAUSE_MENU_OPTIONS,
    PAUSE_MENU_QUIT,
    PAUSE_MENU_RETURN_TO_DESKTOP,
    PAUSE_MENU_OPTION_COUNT,
};
static string ui_pause_menu_strings[] = {
    [PAUSE_MENU_RESUME]            = string_literal("RESUME"),
    [PAUSE_MENU_PARTY_EQUIPMENT]   = string_literal("PARTY"),
    [PAUSE_MENU_PARTY_ITEMS]       = string_literal("ITEMS"),
    [PAUSE_MENU_OPTIONS]           = string_literal("OPTIONS"),
    [PAUSE_MENU_QUIT]              = string_literal("QUIT"),
    [PAUSE_MENU_RETURN_TO_DESKTOP] = string_literal("RETURN TO DESKTOP"),
};
static string ui_pause_editor_menu_strings[] = {
    string_literal("RESUME"),
    string_literal("TEST"),
    string_literal("SAVE"),
    string_literal("LOAD"),
    string_literal("OPTIONS"),
    string_literal("QUIT"),
};

local void open_pause_menu(void);
local void close_pause_menu(void);
local void reexpose_pause_menu_options(void);
local void pause_menu_transition_to(u32 submenu);

#endif
