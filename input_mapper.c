void set_action_binding_gamepad(s32 action, s32 gamepad_button) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;
    binding->gamepad_button = gamepad_button;
}

void set_action_binding_key(s32 action, s32 key) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;
    binding->key = key;
}

bool is_action_down(s32 action) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;

    bool result = false;

    if (binding->key) {
        result |= is_key_down(binding->key);
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

    if (binding->key) {
        result |= is_key_down_with_repeat(binding->key);
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

    if (binding->key) {
        result |= is_key_pressed(binding->key);
    }

    if (binding->gamepad_button) {
        struct game_controller* gamepad = get_gamepad(0);
        result |= controller_button_pressed(gamepad, binding->gamepad_button);
    }

    return result;
}
