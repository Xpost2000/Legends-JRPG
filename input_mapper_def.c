#ifndef INPUT_MAPPER_DEF_C

enum input_actions {
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

string input_action_strings[] = {
    string_literal("move_up"),
    string_literal("move_down"),
    string_literal("move_left"),
    string_literal("move_right"),
    string_literal("pause"),
    string_literal("confirm"),
    string_literal("cancel"),
};

enum required_modifiers_flags {
    KEY_MODIFIER_NONE  = 0,
    KEY_MODIFIER_SHIFT = BIT(0),
    KEY_MODIFIER_ALT   = BIT(1),
    KEY_MODIFIER_CTRL  = BIT(2),
};

#define MAX_SIMULATANEOUS_INPUT_BINDINGS (2)
struct key_binding {
    s32 key;
    s32 modifier_flags;
};
struct input_binding {
    struct key_binding keys[MAX_SIMULATANEOUS_INPUT_BINDINGS];
    s32 gamepad_button;
    /* not sure how to factor this into the existing api */
    /* s32 gamepad_axis; */
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

#endif
