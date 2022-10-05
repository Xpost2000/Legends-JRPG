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

s32 string_to_input_action(string input_string) {
    for (s32 string_index = 0; string_index < array_count(input_action_strings); ++string_index) {
        if (string_equal(input_string, input_action_strings[string_index])) {
            return string_index;
        }
    }

    return -1;
};

string input_action_to_string(s32 action) {
    if (action >= 0 && action < array_count(input_action_strings))
        return input_action_strings[action];

    return string_literal("INPUT_ACTION_NULL");
}

local bool any_bindings_avaliable(struct input_binding* binding) {
    return  (binding->keys[0].key ||
             binding->keys[1].key ||
             binding->gamepad_button);
}

local s32 binding_modifier_count(struct key_binding* binding) {
    s32 modifier_count = 0;

    if (binding->modifier_flags & KEY_MODIFIER_SHIFT) modifier_count++;
    if (binding->modifier_flags & KEY_MODIFIER_ALT) modifier_count++;
    if (binding->modifier_flags & KEY_MODIFIER_CTRL) modifier_count++;

    return modifier_count;
}

void input_mapper_write_out_controls_to(string filename) {
    /* Okay so my API hasn't generically come up with anything useful for this part of the API... */
    /* Which is actually a surprisingly big oversight, but 90% of my data is pure binary. */
    struct binary_serializer serializer = open_write_file_serializer(filename);

    serialize_format(&serializer, "# INPUT_ACTION GAMEPAD_BUTTON KEY0 KEY1 \n");

    /* nice column tabulation */
    /* since I like it. */
    s32 longest_gamepad_string_length = length_of_longest_string(controller_button_strings, array_count(controller_button_strings));
    s32 longest_action_string_length  = length_of_longest_string(input_action_strings, array_count(input_action_strings));
    s32 longest_key_string_length     = length_of_longest_cstring(keyboard_key_strings, array_count(keyboard_key_strings)) + 5;

    for (s32 action_binding_index = 0; action_binding_index < INPUT_ACTION_COUNT; ++action_binding_index) {
        struct input_binding* current_binding = global_input_mapper.bindings + action_binding_index;

        if (any_bindings_avaliable(current_binding)) {
            char controller_string[64] = {};
            char key_strings[2][64]    = {};

            {
                string controller_string_s = controller_button_to_string(current_binding->gamepad_button);
                string key_string_s[2]     = {
                    keycode_to_string(current_binding->keys[0].key),
                    keycode_to_string(current_binding->keys[1].key),
                };

                snprintf(controller_string, 64, "%.*s", controller_string_s.length, controller_string_s.data);
                {
                    s32 cursor[2] = {};
                    /* key 1 */
                    {
                        s32 modifier_count = binding_modifier_count(&current_binding->keys[0]);
                        s32 written = snprintf(key_strings[0]+cursor[0], 64-cursor[0], "%.*s", key_string_s[0].length, key_string_s[0].data);
                        cursor[0] += written;

                        if (modifier_count > 0) {
                            written = snprintf(key_strings[0]+cursor[0], 64-cursor[0], "+");
                            cursor[0] += written;
                            for (s32 modifier_index = 0; modifier_index < modifier_count; ++modifier_index) {
                                if (current_binding->keys[0].modifier_flags & BIT(modifier_index)) {
                                    written = snprintf(key_strings[0]+cursor[0], 64-cursor[0], "%.*s", modifier_strings[modifier_index].length, modifier_strings[modifier_index].data);
                                    cursor[0] += written;
                                    if (!(modifier_index+1 >= modifier_count)) {
                                        written = snprintf(key_strings[0]+cursor[0], 64-cursor[0], "+");
                                        cursor[0] += written;
                                    } else {
                                        written = snprintf(key_strings[0]+cursor[0], 64-cursor[0], " ");
                                        cursor[0] += written;
                                    }
                                }
                            }
                        }
                    }
                    /* key 2 */
                    {
                        s32 modifier_count = binding_modifier_count(&current_binding->keys[1]);
                        s32 written = snprintf(key_strings[1]+cursor[1], 64-cursor[1], "%.*s", key_string_s[1].length, key_string_s[1].data);
                        cursor[1] += written;

                        if (modifier_count > 0) {
                            written = snprintf(key_strings[1]+cursor[1], 64-cursor[1], "+");
                            cursor[1] += written;
                            for (s32 modifier_index = 0; modifier_index < modifier_count; ++modifier_index) {
                                if (current_binding->keys[1].modifier_flags & BIT(modifier_index)) {
                                    written = snprintf(key_strings[1]+cursor[1], 64-cursor[1], "%.*s", modifier_strings[modifier_index].length, modifier_strings[modifier_index].data);
                                    cursor[1] += written;
                                    if (!(modifier_index+1 >= modifier_count)) {
                                        written = snprintf(key_strings[1]+cursor[1], 64-cursor[1], "+");
                                        cursor[1] += written;
                                    } else {
                                        written = snprintf(key_strings[1]+cursor[1], 64-cursor[1], " ");
                                        cursor[1] += written;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            serialize_format(&serializer, "%s", input_action_strings[action_binding_index].data);
            /* for(s32 space_to_insert = 0; space_to_insert < longest_action_string_length; ++space_to_insert) {serialize_format(&serializer, " ");} */
            /* serialize_format(&serializer, "%s", controller_string); */
            /* for(s32 space_to_insert = 0; space_to_insert < longest_gamepad_string_length; ++space_to_insert) {serialize_format(&serializer, " ");} */
            /* serialize_format(&serializer, "%s", key_strings[0]); */
            /* for(s32 space_to_insert = 0; space_to_insert < longest_key_string_length; ++space_to_insert) {serialize_format(&serializer, " ");} */
            /* serialize_format(&serializer, "%s", key_strings[1]); */
            /* serialize_format(&serializer, "\n"); */
        }
    }

    serializer_finish(&serializer);
}

bool input_mapper_read_controls_from(string filename) {
    return false;
}

void initialize_input_mapper_with_bindings(void) {
    bool successful_read = input_mapper_read_controls_from(string_literal("controls.txt"));

    if (!successful_read) {
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
            set_action_binding_key(INPUT_ACTION_SWITCH_CATEGORY_FORWARDS,     KEY_TAB,       KEY_MODIFIER_NONE,  0);
            set_action_binding_key(INPUT_ACTION_SWITCH_CATEGORY_BACKWARDS,    KEY_TAB,       KEY_MODIFIER_SHIFT, 0);
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

        input_mapper_write_out_controls_to(string_literal("controls.txt"));
    }

}
