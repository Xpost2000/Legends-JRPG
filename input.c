#include "input_def.c"

struct {
    struct input_state current_state;
    struct input_state last_state;

    bool keys_receiving_events[KEY_COUNT];
} global_input = {};
local struct game_controller global_controllers[4];

void register_key_down(s32 keyid) {
    /*
      I literally don't know anywhere better to put this text editing specific
      code, and I always just throw it like here.
    */
    if (keyid == KEY_BACKSPACE) {
        if (global_input.current_state.editing_text && global_input.current_state.text_edit_cursor > 0) {
            global_input.current_state.text[--global_input.current_state.text_edit_cursor] = 0;
        }
    }
    global_input.current_state.keys[keyid]    = true;
    global_input.keys_receiving_events[keyid] = true;
}

void register_key_up(s32 keyid) {
    global_input.current_state.keys[keyid] = false;
}

void register_mouse_position(s32 x, s32 y) {
    global_input.current_state.mouse_x = x;
    global_input.current_state.mouse_y = y;
}

void register_mouse_wheel(s32 x, s32 y) {
    global_input.current_state.mouse_wheel_relative_x = x;
    global_input.current_state.mouse_wheel_relative_y = y;
}

void register_mouse_button(s32 button_id, bool state) {
    assertion((button_id >= 0 && button_id < MOUSE_BUTTON_COUNT) && "wtf?");
    global_input.current_state.mouse_buttons[button_id] = state;
}

void get_mouse_location(s32* mx, s32* my) {
    safe_assignment(mx) = global_input.current_state.mouse_x;
    safe_assignment(my) = global_input.current_state.mouse_y;
}

void get_mouse_buttons(bool* left, bool* middle, bool* right) {
    safe_assignment(left)   = global_input.current_state.mouse_buttons[MOUSE_BUTTON_LEFT];
    safe_assignment(middle) = global_input.current_state.mouse_buttons[MOUSE_BUTTON_MIDDLE];
    safe_assignment(right)  = global_input.current_state.mouse_buttons[MOUSE_BUTTON_RIGHT];
}

bool is_key_down_with_repeat(s32 keyid) {
    assertion(keyid < KEY_COUNT && "invalid key id?");
    return global_input.keys_receiving_events[keyid];
}

bool is_key_down(s32 keyid) {
    assertion(keyid < KEY_COUNT && "invalid key id?");
    return global_input.current_state.keys[keyid];
}

bool is_key_pressed(s32 keyid) {
    bool current_keystate = global_input.current_state.keys[keyid];
    bool last_keystate = global_input.last_state.keys[keyid];

    if (current_keystate == last_keystate) {
        return false;
    }

    if (current_keystate == true && last_keystate == false) {
        return true;
    }

    return false;
}

bool controller_button_pressed(struct game_controller* controller, u8 button_id) {
    bool last    = controller->last_buttons[button_id];
    bool current = controller->buttons[button_id];

    if (last == current) {
        return false;
    } else if (current == true && last == false) {
        return true;
    }

    return false;
}

struct game_controller* get_gamepad(s32 index) {
    assertion(index >= 0 && index < array_count(global_controllers) && "Bad controller index");
    return global_controllers + index;
}

void begin_input_frame(void) {
    /* nope */
}

void end_input_frame(void) {
    global_input.last_state = global_input.current_state;

    for (unsigned index = 0; index < array_count(global_controllers); ++index) {
        struct game_controller* controller = global_controllers + index;

        {
            controller->last_triggers    = controller->triggers;
            controller->last_left_stick  = controller->last_left_stick;
            controller->last_right_stick = controller->last_right_stick;
            memcpy(controller->last_buttons, controller->buttons, sizeof(controller->buttons));
        }
    }

    global_input.current_state.mouse_wheel_relative_x = 0;
    global_input.current_state.mouse_wheel_relative_y = 0;

    zero_array(global_input.keys_receiving_events);
}

void start_text_edit(char* target, size_t length) {
    if (!global_input.current_state.editing_text) {
        zero_array(global_input.current_state.text);
        if (target) memcpy(global_input.current_state.text, target, length);
        global_input.current_state.text_edit_cursor = length;
        global_input.current_state.editing_text = true;
    }
}

void end_text_edit(char* target_buffer, size_t target_buffer_size) {
    if (target_buffer && target_buffer_size) {
        memcpy(target_buffer, global_input.current_state.text, target_buffer_size);
        target_buffer[target_buffer_size] = 0;
    }

    zero_array(global_input.current_state.text);
    global_input.current_state.editing_text = false;
}

void send_text_input(char* text, size_t text_length) {
    for (size_t index = 0; index < text_length && global_input.current_state.text_edit_cursor < 1024; ++index) {
        global_input.current_state.text[global_input.current_state.text_edit_cursor++] = text[index];
    }
}

bool is_editing_text(void) {
    return global_input.current_state.editing_text;
}

char* current_text_buffer(void) {
    return global_input.current_state.text;
}

bool any_key_down(void) {
    for (unsigned index = 0; index < array_count(global_input.current_state.keys); ++index) {
        if (!global_input.last_state.keys[index] && global_input.current_state.keys[index]) {
            return true;
        }
    }

    return false;
}

bool controller_any_button_down(struct game_controller* controller) {
    for (unsigned index = 0; index < array_count(controller->buttons); ++index) {
        if (!controller->last_buttons[index] && controller->buttons[index]) {
            return true;
        }
    }

    return false;
}

v2f32 mouse_location(void) {
    s32 mouse_positions[2];
    get_mouse_location(mouse_positions, mouse_positions+1);

    return v2f32(mouse_positions[0], mouse_positions[1]);
}

s32 mouse_wheel_x(void) {
    return global_input.current_state.mouse_wheel_relative_x;
}

s32 mouse_wheel_y(void) {
    return global_input.current_state.mouse_wheel_relative_y;
}

bool is_mouse_wheel_up(void) {
    s32 y = mouse_wheel_y();

    if (y > 0) {
        return true;
    }

    return false;
}

bool is_mouse_wheel_down(void) {
    s32 y = mouse_wheel_y();

    if (y < 0) {
        return true;
    }

    return false;
}
