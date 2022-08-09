#ifndef ENTITY_STAT_BLOCK_DEF_C
#define ENTITY_STAT_BLOCK_DEF_C

enum stat_id {
    STAT_VIGOR,
    STAT_STRENGTH,
    STAT_CONSTITUTION,
    STAT_WILLPOWER,
    STAT_AGILITY,
    STAT_SPEED,
    STAT_INTELLIGENCE,
    STAT_LUCK,
    STAT_COUNT
};
#define Entity_Stat_Block_Base(TYPE) \
    union {                          \
        struct {                     \
            TYPE vigor;              \
            TYPE strength;           \
            TYPE constitution;       \
            TYPE willpower;          \
            TYPE agility;            \
            TYPE speed;              \
            TYPE intelligence;       \
            TYPE luck;               \
        };                           \
        TYPE values[8];              \
    }

local string entity_stat_name_strings[] = {
    string_literal("Vigor"),
    string_literal("Strength"),
    string_literal("Constitution"),
    string_literal("Willpower"),
    string_literal("Agility"),
    string_literal("Speed"),
    string_literal("Intelligence"),
    string_literal("Luck"),
};
local string entity_stat_description_strings[] = {
    string_literal("One's robustness. Determines initial HP and HP gain on level up."),
    string_literal("One's physical power. Determines physical attack."),
    string_literal("One's hardiness. Determines physical defense."),
    string_literal("One's resilience. Determines magical defense."),
    string_literal("One's deftness. Determines damage reduction."),
    string_literal("One's hastiness. Determines movement, and likelihood to dodge."),
    string_literal("One's mind. Determines magic attack."),
    string_literal("One's fortune. Helps in the most unexpected ways."),
};

struct entity_stat_block {
    s32 level;

    Entity_Stat_Block_Base(s32);

    s32 experience;
    s32 xp_value;
};


static struct entity_stat_block entity_stat_block_identity = (struct entity_stat_block) {.level=1, .values[0]=5,.values[1]=5,.values[2]=5,.values[3]=5,.values[4]=5,.values[5]=5,.values[6]=5,.values[7]=5,.experience=0,.xp_value=0};

struct entity_stat_block_modifiers {
    f32 health;
    f32 spell_points;
    Entity_Stat_Block_Base(f32);
};

static struct entity_stat_block_modifiers entity_stat_block_modifiers_identity =
    (struct entity_stat_block_modifiers)
    {
        .health                                                     = 1,
        .spell_points                                               = 1,
        .vigor                                                      = 1,
        .strength                                                   = 1,
        .constitution                                               = 1,
        .willpower                                                  = 1,
        .agility                                                    = 1,
        .speed                                                      = 1,
        .intelligence                                               = 1,
        .luck                                                       = 1,
    };

#endif
