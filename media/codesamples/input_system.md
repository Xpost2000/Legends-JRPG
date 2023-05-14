# Legends RPG: Input System

The higher level input system for legends is simple but useful for preventing the engine from
tying down inputs to specific keybindings.

## Main Interface
```c
struct key_binding {
    s32 key;
    s32 modifier_flags;
};
struct input_binding {
    struct key_binding keys[MAX_SIMULATANEOUS_INPUT_BINDINGS];
    s32 gamepad_button;
};

struct input_mapper_state {
    struct input_binding bindings[INPUT_ACTION_COUNT];
};

void initialize_input_mapper_with_bindings(void);

void set_action_binding_gamepad(s32 action, s32 gamepad_button);
void set_action_binding_key(s32 action, s32 key, s32 modifier_flags, s32 slot);

bool is_action_down(s32 action);
bool is_action_down_with_repeat(s32 action);
bool is_action_pressed(s32 action);

void input_mapper_write_out_controls_to(string filename);
bool input_mapper_read_controls_from(string filename);
```

The input mapper's main high level interface appears like this, and allows for remapping an input key or gamepad button
into a higher level game "action".

The engine does not support mapping more "analog" inputs such as joysticks or mouse inputs, however in my experience those are
generally very special cases and don't typically change. Joysticks have always been movement inputs with the extent of customization
being southpaw bindings.

This is a set of hardcoded game actions, and is specified through an enum. There are as many bindings as there are 
input action enumerations.

```c
enum input_action {
    INPUT_ACTION_NULL,
    INPUT_ACTION_MOVE_UP,
    INPUT_ACTION_MOVE_DOWN,
    INPUT_ACTION_MOVE_LEFT,
    INPUT_ACTION_MOVE_RIGHT,
    INPUT_ACTION_PAUSE,
    INPUT_ACTION_CONFIRMATION,
    INPUT_ACTION_CANCEL,
    /* mainly for the shop, can't think of any other reason to bind this otherwise... */
    INPUT_ACTION_SWITCH_CATEGORY_BACKWARDS,
    INPUT_ACTION_SWITCH_CATEGORY_FORWARDS,
    INPUT_ACTION_COUNT,
};
```

You can easily specify more input actions by adding to the enum.

As the interface indicates, it is possible to read input actions from a bindings file that looks as such:

```
# INPUT_ACTION                  GAMEPAD_BUTTON       KEY0            KEY1 
ACTION_MOVE_UP                  GAMEPAD_DPAD_UP    Arrow_Up          W
ACTION_MOVE_DOWN                GAMEPAD_DPAD_DOWN  Arrow_Down        S
ACTION_MOVE_LEFT                GAMEPAD_DPAD_LEFT  Arrow_Left        A
ACTION_MOVE_RIGHT               GAMEPAD_DPAD_RIGHT Arrow_Right       D
ACTION_PAUSE                    GAMEPAD_START      Escape            KEY_NONE
ACTION_CONFIRM                  GAMEPAD_A          Return            KEY_NONE
ACTION_CANCEL                   GAMEPAD_B          Backspace         KEY_NONE
ACTION_SWITCH_CATEGORY_BACKWARD GAMEPAD_LB         Tab+Shift         KEY_NONE
ACTION_SWITCH_CATEGORY_FORWARD  GAMEPAD_RB         Tab               KEY_NONE
```

The game will automatically generate a configuration file, and all fields are not optional. Modifiers can be appended as ```KEY+(MODIFIER)```

## Implementation 

Here is the implementation of the input_mapper interface.

**NOTE: The input mapper supports multiple two keys per action, which requires a linear search.***


```c
void set_action_binding_gamepad(s32 action, s32 gamepad_button) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "Invalid action. Out of bounds!");
    struct input_binding* binding = global_input_mapper.bindings + action;
    binding->gamepad_button = gamepad_button;
}

void set_action_binding_key(s32 action, s32 key, s32 modifier_flags, s32 slot) {
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "Invalid action. Out of bounds!");
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
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "Invalid action. Out of bounds!");
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
    assertion(action >= 0 && action < INPUT_ACTION_COUNT && "Invalid action. Out of bounds!");
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
```

## Implementation of Serialization

Here is the implementation of the serialization of the input configuration to a text file.

```c
void input_mapper_write_out_controls_to(string filename) {
    struct binary_serializer serializer = open_write_file_serializer(filename);

    serialize_format(&serializer, "# INPUT_ACTION GAMEPAD_BUTTON KEY0 KEY1 \n");

    /* I tend to like columns/tabular code so this is a personal "formatter". */
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
            for(s32 space_to_insert = 0; space_to_insert < (longest_action_string_length - input_action_strings[action_binding_index].length)+1; ++space_to_insert) {serialize_format(&serializer, " ");}
            serialize_format(&serializer, "%s", controller_string);
            for(s32 space_to_insert = 0; space_to_insert < (longest_gamepad_string_length - cstring_length(controller_string))+1; ++space_to_insert) {serialize_format(&serializer, " ");}
            serialize_format(&serializer, "%s", key_strings[0]);
            for(s32 space_to_insert = 0; space_to_insert < (longest_key_string_length - cstring_length(key_strings[0]))+1; ++space_to_insert) {serialize_format(&serializer, " ");}
            serialize_format(&serializer, "%s", key_strings[1]);
            serialize_format(&serializer, "\n");
        }
    }

    serializer_finish(&serializer);
}

bool input_mapper_read_controls_from(string filename) {
    bool successful = true;

    if (file_exists(filename)) {
        /*
          Reusing lexer from the lisp reader, since it happens to work fine
         */
        struct file_buffer data_file_buffer = read_entire_file(memory_arena_allocator(&scratch_arena), filename);
        struct lexer lexer_state            = {.buffer = file_buffer_as_string(&data_file_buffer)};

        _debugprintf("filebuffer?");
        _debugprintf("%.*s (%d)", (int)data_file_buffer.length, data_file_buffer.buffer, lexer_state.current_read_index);

        {
            while (!lexer_done(&lexer_state)) {
                struct lexer_token action_id                  = lexer_next_token(&lexer_state);
                struct lexer_token action_controller_binding  = lexer_next_token(&lexer_state);
                struct lexer_token action_keybinding[2]       = {lexer_next_token(&lexer_state), lexer_next_token(&lexer_state)};

                if (action_id.type == TOKEN_TYPE_NONE ||
                    action_controller_binding.type == TOKEN_TYPE_NONE ||
                    action_keybinding[0].type == TOKEN_TYPE_NONE ||
                    action_keybinding[1].type == TOKEN_TYPE_NONE) {
                    _debugprintf("Malformed controls file?");
                    _debugprintf("action_id:");
                    _debug_print_token(action_id);
                    _debugprintf("action_controller_binding:");
                    _debug_print_token(action_controller_binding);
                    _debugprintf("action_keybinding[0]:");
                    _debug_print_token(action_keybinding[0]);
                    _debugprintf("action_keybinding[1]:");
                    _debug_print_token(action_keybinding[1]);
                    successful = false;
                } else {
                    _debug_print_token(action_keybinding[0]);
                    _debug_print_token(action_keybinding[1]);
                    /* okay all tokens are good, probably*/
                    s32 action_binding_slot       = string_to_input_action(action_id.str);
                    s32 action_controller_binding_id = string_to_controller_button(action_controller_binding.str);

                    struct input_binding* binding = global_input_mapper.bindings + action_binding_slot;
                    binding->gamepad_button       = action_controller_binding_id;

                    /* reading key strings plus modifiers */
                    {
                        struct string_array strings = string_split(&scratch_arena, action_keybinding[0].str, '+');
                        /* core string */
                        {
                            binding->keys[0].key = string_to_keyid(strings.strings[0]);
                        }

                        for (s32 modifier_index = 1; modifier_index < strings.count; ++modifier_index) {
                            string modifier = strings.strings[modifier_index];

                            if (string_equal(modifier, modifier_strings[0])) {
                                binding->keys[0].modifier_flags |= KEY_MODIFIER_SHIFT;
                            }
                            if (string_equal(modifier, modifier_strings[1])) {
                                binding->keys[0].modifier_flags |= KEY_MODIFIER_ALT;
                            }
                            if (string_equal(modifier, modifier_strings[2])) {
                                binding->keys[0].modifier_flags |= KEY_MODIFIER_CTRL;
                            }
                        }
                    }
                    {
                        struct string_array strings = string_split(&scratch_arena, action_keybinding[1].str, '+');
                        /* core string */
                        {
                            binding->keys[1].key = string_to_keyid(strings.strings[0]);
                        }

                        for (s32 modifier_index = 1; modifier_index < strings.count; ++modifier_index) {
                            string modifier = strings.strings[modifier_index];

                            if (string_equal(modifier, modifier_strings[0])) {
                                binding->keys[1].modifier_flags |= KEY_MODIFIER_SHIFT;
                            }
                            if (string_equal(modifier, modifier_strings[1])) {
                                binding->keys[1].modifier_flags |= KEY_MODIFIER_ALT;
                            }
                            if (string_equal(modifier, modifier_strings[2])) {
                                binding->keys[1].modifier_flags |= KEY_MODIFIER_CTRL;
                            }
                        }
                    }

                    if (lexer_peek_token(&lexer_state).type == TOKEN_TYPE_NONE) {
                        _debugprintf("Next token is EOF?!");
                        lexer_next_token(&lexer_state);
                    }
                }
            }
        }

        file_buffer_free(&data_file_buffer);
    } else {
        successful = false;
    }

    return successful;
}
```
