
local int translate_sdl_scancode(int scancode) {
    local int _sdl_scancode_to_input_keycode_table[255] = {
        // top row
        [SDL_SCANCODE_ESCAPE]    = KEY_ESCAPE,
        [SDL_SCANCODE_1]         = KEY_1,
        [SDL_SCANCODE_2]         = KEY_2,
        [SDL_SCANCODE_3]         = KEY_3,
        [SDL_SCANCODE_4]         = KEY_4,
        [SDL_SCANCODE_5]         = KEY_5,
        [SDL_SCANCODE_6]         = KEY_6,
        [SDL_SCANCODE_7]         = KEY_7,
        [SDL_SCANCODE_8]         = KEY_8,
        [SDL_SCANCODE_9]         = KEY_9,
        [SDL_SCANCODE_0]         = KEY_0,
        [SDL_SCANCODE_MINUS]     = KEY_MINUS,
        [SDL_SCANCODE_EQUALS]    = KEY_EQUALS,
        [SDL_SCANCODE_BACKSPACE] = KEY_BACKSPACE,

        [SDL_SCANCODE_TAB] = KEY_TAB,

        [SDL_SCANCODE_Q]            = KEY_Q,
        [SDL_SCANCODE_W]            = KEY_W,
        [SDL_SCANCODE_E]            = KEY_E,
        [SDL_SCANCODE_R]            = KEY_R,
        [SDL_SCANCODE_T]            = KEY_T,
        [SDL_SCANCODE_Y]            = KEY_Y,
        [SDL_SCANCODE_U]            = KEY_U,
        [SDL_SCANCODE_I]            = KEY_I,
        [SDL_SCANCODE_O]            = KEY_O,
        [SDL_SCANCODE_P]            = KEY_P,
        [SDL_SCANCODE_LEFTBRACKET]  = KEY_LEFT_BRACKET,
        [SDL_SCANCODE_RIGHTBRACKET] = KEY_RIGHT_BRACKET,
        [SDL_SCANCODE_RETURN]       = KEY_RETURN,

        [SDL_SCANCODE_LCTRL] = KEY_CTRL,
        [SDL_SCANCODE_RCTRL] = KEY_CTRL,

        [SDL_SCANCODE_A] = KEY_A,
        [SDL_SCANCODE_S] = KEY_S,
        [SDL_SCANCODE_D] = KEY_D,
        [SDL_SCANCODE_F] = KEY_F,
        [SDL_SCANCODE_G] = KEY_G,
        [SDL_SCANCODE_H] = KEY_H,
        [SDL_SCANCODE_J] = KEY_J,
        [SDL_SCANCODE_K] = KEY_K,
        [SDL_SCANCODE_L] = KEY_L,
        [SDL_SCANCODE_SEMICOLON] = KEY_SEMICOLON,
        [SDL_SCANCODE_APOSTROPHE] = KEY_QUOTE,
        [SDL_SCANCODE_GRAVE] = KEY_BACKQUOTE,

        [SDL_SCANCODE_LSHIFT] = KEY_SHIFT, // left shift
        [SDL_SCANCODE_RSHIFT] = KEY_SHIFT, // left shift
        [SDL_SCANCODE_BACKSLASH] = KEY_BACKSLASH,

        [SDL_SCANCODE_Z]      = KEY_Z,
        [SDL_SCANCODE_X]      = KEY_X,
        [SDL_SCANCODE_C]      = KEY_C,
        [SDL_SCANCODE_V]      = KEY_V,
        [SDL_SCANCODE_B]      = KEY_B,
        [SDL_SCANCODE_N]      = KEY_N,
        [SDL_SCANCODE_M]      = KEY_M,
        [SDL_SCANCODE_COMMA]  = KEY_COMMA,
        [SDL_SCANCODE_PERIOD] = KEY_PERIOD,
        [SDL_SCANCODE_SLASH]  = KEY_FORWARDSLASH,

        [SDL_SCANCODE_PRINTSCREEN] = KEY_PRINTSCREEN,
        [SDL_SCANCODE_LALT]        = KEY_ALT,
        [SDL_SCANCODE_RALT]        = KEY_ALT,
        [SDL_SCANCODE_SPACE]       = KEY_SPACE,

        [SDL_SCANCODE_F1]  = KEY_F1,
        [SDL_SCANCODE_F2]  = KEY_F2,
        [SDL_SCANCODE_F3]  = KEY_F3,
        [SDL_SCANCODE_F4]  = KEY_F4,
        [SDL_SCANCODE_F5]  = KEY_F5,
        [SDL_SCANCODE_F6]  = KEY_F6,
        [SDL_SCANCODE_F7]  = KEY_F7,
        [SDL_SCANCODE_F8]  = KEY_F8,
        [SDL_SCANCODE_F9]  = KEY_F9,
        [SDL_SCANCODE_F10] = KEY_F10,
        [SDL_SCANCODE_F11] = KEY_F11,
        [SDL_SCANCODE_F12] = KEY_F12,

        [SDL_SCANCODE_NUMLOCKCLEAR] = KEY_NUMBER_LOCK,
        [SDL_SCANCODE_SCROLLLOCK]   = KEY_SCROLL_LOCK,


        [SDL_SCANCODE_PAGEUP]   = KEY_PAGEUP,
        [SDL_SCANCODE_HOME]     = KEY_HOME,
        [SDL_SCANCODE_PAGEDOWN] = KEY_PAGEDOWN,
        [SDL_SCANCODE_INSERT]   = KEY_INSERT,
        [SDL_SCANCODE_DELETE]   = KEY_DELETE,
        [SDL_SCANCODE_END]      = KEY_END,

        [SDL_SCANCODE_UP]    = KEY_UP,
        [SDL_SCANCODE_DOWN]  = KEY_DOWN,
        [SDL_SCANCODE_LEFT]  = KEY_LEFT,
        [SDL_SCANCODE_RIGHT] = KEY_RIGHT,
    };

    int mapping = _sdl_scancode_to_input_keycode_table[scancode];
    /* not_really_important_assert(mapping != KEY_UNKNOWN && "Unbound key?"); */
    return mapping;
}
