#ifndef INPUT_MAPPER_DEF_C

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

string input_action_strings[] = {
    [INPUT_ACTION_NULL]                      = string_literal("INPUT_ACTION_NULL"),
    [INPUT_ACTION_MOVE_UP]                   = string_literal("ACTION_MOVE_UP"),
    [INPUT_ACTION_MOVE_DOWN]                 = string_literal("ACTION_MOVE_DOWN"),
    [INPUT_ACTION_MOVE_LEFT]                 = string_literal("ACTION_MOVE_LEFT"),
    [INPUT_ACTION_MOVE_RIGHT]                = string_literal("ACTION_MOVE_RIGHT"),
    [INPUT_ACTION_PAUSE]                     = string_literal("ACTION_PAUSE"),
    [INPUT_ACTION_CONFIRMATION]              = string_literal("ACTION_CONFIRM"),
    [INPUT_ACTION_CANCEL]                    = string_literal("ACTION_CANCEL"),
    [INPUT_ACTION_SWITCH_CATEGORY_FORWARDS]  = string_literal("ACTION_SWITCH_CATEGORY_FORWARD"),
    [INPUT_ACTION_SWITCH_CATEGORY_BACKWARDS] = string_literal("ACTION_SWITCH_CATEGORY_BACKWARD"),
};

enum required_modifiers_flags {
    KEY_MODIFIER_NONE  = 0,
    KEY_MODIFIER_SHIFT = BIT(0),
    KEY_MODIFIER_ALT   = BIT(1),
    KEY_MODIFIER_CTRL  = BIT(2),
};
string modifier_strings[3] = {
    string_literal("Shift"),
    string_literal("Alt"),
    string_literal("Ctrl"),
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

void input_mapper_write_out_controls_to(string filename);
bool input_mapper_read_controls_from(string filename);

#endif
