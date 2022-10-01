void set_action_binding_gamepad(s32 action, s32 gamepad_button) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;
    binding->gamepad_button = gamepad_button;
}

void set_action_binding_key(s32 action, s32 key, s32 modifier_flags, s32 slot) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;
    binding->keys[slot].key            = key;
    binding->keys[slot].modifier_flags = modifier_flags;
}

local bool are_required_modifiers_down_for_key(struct key_binding key) {
    bool result = true;

    if (key.modifier_flags & KEY_MODIFIER_SHIFT) {
        result &= is_key_down(KEY_SHIFT);
    }
    if (key.modifier_flags & KEY_MODIFIER_ALT) {
        result &= is_key_down(KEY_ALT);
    }
    if (key.modifier_flags & KEY_MODIFIER_CTRL) {
        result &= is_key_down(KEY_CTRL);
    }

    return result;
}

bool is_action_down(s32 action) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "this is bad for some reason");
    struct input_binding* binding = global_input_mapper.bindings + action;

    bool result = false;

    for (s32 key_index = 0; key_index < MAX_SIMULATANEOUS_INPUT_BINDINGS; ++key_index) {
        if (binding->keys[key_index].key && are_required_modifiers_down_for_key(binding->keys[key_index])) {
            result |= is_key_down(binding->keys[key_index].key);
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
        if (binding->keys[key_index].key && are_required_modifiers_down_for_key(binding->keys[key_index])) {
            result |= is_key_down_with_repeat(binding->keys[key_index].key);
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
        if (binding->keys[key_index].key && are_required_modifiers_down_for_key(binding->keys[key_index])) {
            result |= is_key_pressed(binding->keys[key_index].key);
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
        set_action_binding_key(INPUT_ACTION_MOVE_UP,                      KEY_UP,        KEY_MODIFIER_NONE,  0);
        set_action_binding_key(INPUT_ACTION_MOVE_DOWN,                    KEY_DOWN,      KEY_MODIFIER_NONE,  0);
        set_action_binding_key(INPUT_ACTION_MOVE_LEFT,                    KEY_LEFT,      KEY_MODIFIER_NONE,  0);
        set_action_binding_key(INPUT_ACTION_MOVE_RIGHT,                   KEY_RIGHT,     KEY_MODIFIER_NONE,  0);
        set_action_binding_key(INPUT_ACTION_MOVE_UP,                      KEY_W,         KEY_MODIFIER_NONE,  1);
        set_action_binding_key(INPUT_ACTION_MOVE_DOWN,                    KEY_S,         KEY_MODIFIER_NONE,  1);
        set_action_binding_key(INPUT_ACTION_MOVE_LEFT,                    KEY_A,         KEY_MODIFIER_NONE,  1);
        set_action_binding_key(INPUT_ACTION_MOVE_RIGHT,                   KEY_D,         KEY_MODIFIER_NONE,  1);
        set_action_binding_key(INPUT_ACTION_PAUSE,                        KEY_ESCAPE,    KEY_MODIFIER_NONE,  0);
        set_action_binding_key(INPUT_ACTION_CONFIRMATION,                 KEY_RETURN,    KEY_MODIFIER_NONE,  0);
        set_action_binding_key(INPUT_ACTION_CANCEL,                       KEY_ESCAPE,    KEY_MODIFIER_NONE,  0);
        set_action_binding_key(INPUT_ACTION_CANCEL,                       KEY_BACKSPACE, KEY_MODIFIER_NONE,  1);
        set_action_binding_key(INPUT_ACTION_SWITCH_CATEGORY_FORWARDS,     KEY_TAB,       KEY_MODIFIER_NONE,  1);
        set_action_binding_key(INPUT_ACTION_SWITCH_CATEGORY_BACKWARDS,    KEY_TAB,       KEY_MODIFIER_SHIFT, 1);
    }
    {
        set_action_binding_gamepad(INPUT_ACTION_MOVE_UP,                          DPAD_UP);
        set_action_binding_gamepad(INPUT_ACTION_MOVE_DOWN,                        DPAD_DOWN);
        set_action_binding_gamepad(INPUT_ACTION_MOVE_LEFT,                        DPAD_LEFT);
        set_action_binding_gamepad(INPUT_ACTION_MOVE_RIGHT,                       DPAD_RIGHT);
        set_action_binding_gamepad(INPUT_ACTION_CONFIRMATION,                     BUTTON_A);
        set_action_binding_gamepad(INPUT_ACTION_CANCEL,                           BUTTON_B);
        set_action_binding_gamepad(INPUT_ACTION_PAUSE,                            BUTTON_START);
        set_action_binding_gamepad(INPUT_ACTION_SWITCH_CATEGORY_FORWARDS,         BUTTON_RB);
        set_action_binding_gamepad(INPUT_ACTION_SWITCH_CATEGORY_BACKWARDS,        BUTTON_LB);
    }
}
