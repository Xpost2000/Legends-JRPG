#ifndef INPUT_DEF_C
#define INPUT_DEF_C

#include "common.c"

enum mouse_button {
    MOUSE_BUTTON_LEFT=1,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_COUNT,
};

enum gamepad_axis {
    GAMEPAD_AXIS_POSITIVE_X=1,
    GAMEPAD_AXIS_NEGATIVE_X,
    GAMEPAD_AXIS_POSITIVE_Y,
    GAMEPAD_AXIS_NEGATIVE_Y,
};

enum keyboard_button {
    KEY_UNKNOWN,
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
    KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
    KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
    KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
    KEY_F11, KEY_F12, KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT,
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_MINUS, KEY_BACKQUOTE, KEY_EQUALS,
    KEY_SEMICOLON, KEY_QUOTE, KEY_COMMA,
    KEY_PERIOD, KEY_RETURN, KEY_BACKSPACE, KEY_ESCAPE,

    KEY_INSERT, KEY_HOME, KEY_PAGEUP, KEY_PAGEDOWN, KEY_DELETE, KEY_END,
    KEY_PRINTSCREEN,

    /*
      I probably don't actually care about mapping these keys.
    */
    KEY_PAUSE, KEY_SCROLL_LOCK, KEY_NUMBER_LOCK,
    KEYPAD_0, KEYPAD_1, KEYPAD_2, KEYPAD_3, KEYPAD_4,
    KEYPAD_5, KEYPAD_6, KEYPAD_7, KEYPAD_8, KEYPAD_9,

    KEYPAD_LEFT, KEYPAD_RIGHT, KEYPAD_UP, KEYPAD_DOWN,
    KEYPAD_ASTERISK, KEYPAD_BACKSLASH,
    KEYPAD_MINUS, KEYPAD_PLUS, KEYPAD_PERIOD,

    KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET,
    KEY_FORWARDSLASH, KEY_BACKSLASH,

    KEY_TAB, KEY_SHIFT,
    KEY_META, KEY_SUPER, KEY_SPACE,

    KEY_CTRL, KEY_ALT,

    KEY_COUNT
};

enum controller_button {
    BUTTON_UNKNOWN,
    BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y,
    BUTTON_RS, BUTTON_LS,
    BUTTON_RB, BUTTON_LB,
    BUTTON_START, BUTTON_BACK,
    DPAD_UP,DPAD_DOWN,DPAD_LEFT,DPAD_RIGHT,
    BUTTON_COUNT,
};

/* TODO: need to produce human readable strings later */
static string controller_button_strings[] = {
    [BUTTON_UNKNOWN] = string_literal("PAD_NONE"),
    [BUTTON_A]       = string_literal("GAMEPAD_A"),
    [BUTTON_B]       = string_literal("GAMEPAD_B"),
    [BUTTON_X]       = string_literal("GAMEPAD_X"),
    [BUTTON_Y]       = string_literal("GAMEPAD_Y"),
    [BUTTON_RS]      = string_literal("GAMEPAD_RS"),
    [BUTTON_LS]      = string_literal("GAMEPAD_LS"),
    [BUTTON_RB]      = string_literal("GAMEPAD_RB"),
    [BUTTON_LB]      = string_literal("GAMEPAD_LB"),
    [BUTTON_START]   = string_literal("GAMEPAD_START"),
    [BUTTON_BACK]    = string_literal("GAMEPAD_BACK"),
    [DPAD_UP]        = string_literal("GAMEPAD_DPAD_UP"),
    [DPAD_DOWN]      = string_literal("GAMEPAD_DPAD_DOWN"),
    [DPAD_LEFT]      = string_literal("GAMEPAD_DPAD_LEFT"),
    [DPAD_RIGHT]     = string_literal("GAMEPAD_DPAD_RIGHT"),
};

string controller_button_to_string(s32 buttonid);
s32    string_to_controller_button(string buttonstring);

struct game_controller_triggers {
    f32 left;
    f32 right;
};
struct game_controller_joystick {
    f32 axes[2];
};
struct game_controller {
    struct game_controller_triggers triggers;
    u8                              buttons[BUTTON_COUNT];
    struct game_controller_joystick left_stick;
    struct game_controller_joystick right_stick;

    struct game_controller_triggers last_triggers;
    u8                              last_buttons[BUTTON_COUNT];
    struct game_controller_joystick last_left_stick;
    struct game_controller_joystick last_right_stick;

    u8                              buttons_that_received_events[BUTTON_COUNT];
    void* _internal_controller_handle;
};

/* 1.0 - 0.0 */
void controller_rumble(struct game_controller* controller, f32 x_magnitude, f32 y_magnitude, u32 ms);
bool controller_button_down(struct game_controller* controller, u8 button_id);
bool controller_button_down_with_repeat(struct game_controller* controller, u8 button_id);
bool controller_button_pressed(struct game_controller* controller, u8 button_id);

/* KEYPAD keys are left out because I have not mapped them yet. */
internal char* keyboard_key_strings[] = {
    [KEY_UNKNOWN] = "Unknown Key?",

    [KEY_A] = "A",
    [KEY_B] = "B",
    [KEY_C] = "C",
    [KEY_D] = "D",
    [KEY_E] = "E",
    [KEY_F] = "F",
    [KEY_G] = "G",
    [KEY_H] = "H",
    [KEY_I] = "I",
    [KEY_J] = "J",
    [KEY_K] = "K",
    [KEY_L] = "L",
    [KEY_M] = "M",
    [KEY_N] = "N",
    [KEY_O] = "O",
    [KEY_P] = "P",
    [KEY_Q] = "Q",
    [KEY_R] = "R",
    [KEY_S] = "S",
    [KEY_T] = "T",
    [KEY_U] = "U",
    [KEY_V] = "V",
    [KEY_W] = "W",
    [KEY_X] = "X",
    [KEY_Y] = "Y",
    [KEY_Z] = "Z",

    [KEY_F1]  = "F1",
    [KEY_F2]  = "F2",
    [KEY_F3]  = "F3",
    [KEY_F4]  = "F4",
    [KEY_F5]  = "F5",
    [KEY_F6]  = "F6",
    [KEY_F7]  = "F7",
    [KEY_F8]  = "F8",
    [KEY_F9]  = "F9",
    [KEY_F10] = "F10",
    [KEY_F11] = "F11",
    [KEY_F12] = "F12",

    [KEY_1]  = "1",
    [KEY_2]  = "2",
    [KEY_3]  = "3",
    [KEY_4]  = "4",
    [KEY_5]  = "5",
    [KEY_6]  = "6",
    [KEY_7]  = "7",
    [KEY_8]  = "8",
    [KEY_9]  = "9",
    [KEY_0]  = "0",

    [KEY_MINUS]     = "-",
    [KEY_BACKQUOTE] = "`",
    [KEY_EQUALS]    = "=",
    [KEY_SEMICOLON] = ";",
    [KEY_QUOTE]     = "\'",
    [KEY_COMMA]     = ",",
    [KEY_PERIOD]    = ".",

    [KEY_RETURN]    = "Return",
    [KEY_BACKSPACE] = "Backspace",
    [KEY_ESCAPE]    = "Escape",

    [KEY_INSERT]   = "Insert",
    [KEY_HOME]     = "Home",
    [KEY_PAGEUP]   = "Page Up",
    [KEY_PAGEDOWN] = "Page Down",
    [KEY_DELETE]   = "Delete",
    [KEY_END]      = "End",

    [KEY_PRINTSCREEN] = "Print Screen",
    [KEY_PAUSE]       = "Pause",
    
    [KEY_SCROLL_LOCK] = "Scroll Lock",
    [KEY_NUMBER_LOCK] = "Number Lock",

    [KEY_LEFT_BRACKET]  = "[",
    [KEY_RIGHT_BRACKET] = "]",

    [KEY_FORWARDSLASH] = "/",
    [KEY_BACKSLASH]    = "\\",

    [KEY_TAB]   = "Tab",
    [KEY_SHIFT] = "Shift",
    [KEY_CTRL]  = "Control",
    [KEY_ALT]   = "Alt",

    [KEY_UP]    = "Arrow_Up",
    [KEY_DOWN]    = "Arrow_Down",
    [KEY_LEFT]    = "Arrow_Left",
    [KEY_RIGHT]    = "Arrow_Right",

    [KEY_SPACE] = "Space",
};

string keycode_to_string(s32 keyid);
s32    string_to_keyid(string string);

struct input_state {
    bool keys[KEY_COUNT];
    s32  mouse_x;
    s32  mouse_y;
    bool mouse_buttons[MOUSE_BUTTON_COUNT];
    s32  mouse_wheel_relative_x;
    s32  mouse_wheel_relative_y;

    bool editing_text;
    s32  text_edit_cursor;
    char text[1024];
};

struct game_controller* get_gamepad(s32 index);

enum {
    CONTROLLER_JOYSTICK_LEFT,
    CONTROLLER_JOYSTICK_RIGHT,
};
local float angle_formed_by_joystick(struct game_controller* controller, s32 which) {
    struct game_controller_joystick target;
    switch (which) {
        case CONTROLLER_JOYSTICK_LEFT: {
            target = controller->left_stick;
        } break;
        case CONTROLLER_JOYSTICK_RIGHT: {
            target = controller->right_stick;
        } break;
    }

    f32 angle = atan2(target.axes[1], target.axes[0]);
    return angle;
}

void register_key_down(s32 keyid);
void register_key_up(s32 keyid);

/* NOTE: oddly enough a platform layer responsibility. */
void register_controller_down(s32 which, s32 button);

void register_mouse_position(s32 x, s32 y);
void register_mouse_wheel(s32 x, s32 y);
void register_mouse_button(s32 button_id, bool state);

/* NOTE: I have no idea when I've cared about the mouse wheel relative x/y but ok */
s32  mouse_wheel_x(void);
s32  mouse_wheel_y(void);
bool is_mouse_wheel_up(void);
bool is_mouse_wheel_down(void);

bool is_key_down(s32 keyid);
/*
  This is to allow for processing of key events without having
  to write an event catcher.
*/
bool is_key_down_with_repeat(s32 keyid);

bool is_key_pressed(s32 keyid);
bool any_key_down(void);
bool controller_any_button_down(struct game_controller* controller);

void get_mouse_location(s32* mx, s32* my);
void get_mouse_buttons(bool* left, bool* middle, bool* right);
bool mouse_left(void);
bool mouse_middle(void);
bool mouse_right(void);
v2f32 mouse_location(void);

void begin_input_frame(void);
void end_input_frame(void);

void start_text_edit(char* target, size_t length);
void end_text_edit(char* target, size_t amount); /*copies all text input s32o target buffer. Not necessarily unicode aware. whoops!*/

void send_text_input(char* text, size_t text_length);
bool is_editing_text(void);
char* current_text_buffer(void);

#include "input_mapper_def.c"

#endif
