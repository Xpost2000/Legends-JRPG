void set_action_binding_gamepad(s32 action, s32 gamepad_button) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;
    binding->gamepad_button = gamepad_button;
}

void set_action_binding_key(s32 action, s32 key, s32 slot) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;
    binding->key[slot] = key;
}

bool is_action_down(s32 action) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;

    bool result = false;

    for (s32 key_index = 0; key_index < MAX_SIMULATANEOUS_INPUT_BINDINGS; ++key_index) {
        if (binding->key[key_index]) {
            result |= is_key_down(binding->key[key_index]);
        }
    }

    if (binding->gamepad_button) {
        struct game_controller* gamepad = get_gamepad(0);
        result |= controller_button_down(gamepad, binding->gamepad_button);
    }

    return result;
}

bool is_action_down_with_repeat(s32 action) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;

    bool result = false;

    for (s32 key_index = 0; key_index < MAX_SIMULATANEOUS_INPUT_BINDINGS; ++key_index) {
        if (binding->key[key_index]) {
            result |= is_key_down_with_repeat(binding->key[key_index]);
        }
    }

    if (binding->gamepad_button) {
        struct game_controller* gamepad = get_gamepad(0);
        result |= controller_button_down_with_repeat(gamepad, binding->gamepad_button);
    }

    return result;
    
}

bool is_action_pressed(s32 action) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;
    
    bool result = false;

    for (s32 key_index = 0; key_index < MAX_SIMULATANEOUS_INPUT_BINDINGS; ++key_index) {
        if (binding->key[key_index]) {
            result |= is_key_pressed(binding->key[key_index]);
        }
    }

    if (binding->gamepad_button) {
        struct game_controller* gamepad = get_gamepad(0);
        result |= controller_button_pressed(gamepad, binding->gamepad_button);
    }

    return result;
}

void initialize_input_mapper_with_bindings(void) {
    {
        set_action_binding_key(INPUT_ACTION_MOVE_UP,        KEY_UP,        0);
        set_action_binding_key(INPUT_ACTION_MOVE_DOWN,      KEY_DOWN,      0);
        set_action_binding_key(INPUT_ACTION_MOVE_LEFT,      KEY_LEFT,      0);
        set_action_binding_key(INPUT_ACTION_MOVE_RIGHT,     KEY_RIGHT,     0);
        set_action_binding_key(INPUT_ACTION_MOVE_UP,        KEY_W,         1);
        set_action_binding_key(INPUT_ACTION_MOVE_DOWN,      KEY_S,         1);
        set_action_binding_key(INPUT_ACTION_MOVE_LEFT,      KEY_A,         1);
        set_action_binding_key(INPUT_ACTION_MOVE_RIGHT,     KEY_D,         1);
        set_action_binding_key(INPUT_ACTION_PAUSE,          KEY_ESCAPE,    0);
        set_action_binding_key(INPUT_ACTION_CONFIRMATION,   KEY_RETURN,    0);
        set_action_binding_key(INPUT_ACTION_CANCEL,         KEY_ESCAPE,    0);
        set_action_binding_key(INPUT_ACTION_CANCEL,         KEY_BACKSPACE, 1);
    }
    {
        set_action_binding_gamepad(INPUT_ACTION_MOVE_UP,    DPAD_UP);
        set_action_binding_gamepad(INPUT_ACTION_MOVE_DOWN,  DPAD_DOWN);
        set_action_binding_gamepad(INPUT_ACTION_MOVE_LEFT,  DPAD_LEFT);
        set_action_binding_gamepad(INPUT_ACTION_MOVE_RIGHT, DPAD_RIGHT);
    }
}
