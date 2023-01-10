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
    STAT_COUNTER,
    STAT_COUNT
};
#define Entity_Stat_Block_Base(TYPE)            \
    union {                                     \
        struct {                                \
            TYPE vigor;                         \
            TYPE strength;                      \
            TYPE constitution;                  \
            TYPE willpower;                     \
            TYPE agility;                       \
            TYPE speed;                         \
            TYPE intelligence;                  \
            TYPE luck;                          \
            TYPE counter;                       \
        };                                      \
        TYPE     values[STAT_COUNT];            \
    }

local string entity_stat_name_strings[] = {
    [STAT_VIGOR]        = string_literal("Vigor"),
    [STAT_STRENGTH]     = string_literal("Strength"),
    [STAT_CONSTITUTION] = string_literal("Constitution"),
    [STAT_WILLPOWER]    = string_literal("Willpower"),
    [STAT_AGILITY]      = string_literal("Agility"),
    [STAT_SPEED]        = string_literal("Speed"),
    [STAT_INTELLIGENCE] = string_literal("Intelligence"),
    [STAT_LUCK]         = string_literal("Luck"),
    [STAT_COUNTER]      = string_literal("Counter"),
};
local string entity_stat_description_strings[] = {
    [STAT_VIGOR]        = string_literal("One's robustness. Determines HP gain on level up."),
    [STAT_STRENGTH]     = string_literal("One's physical power. Determines physical attack."),
    [STAT_CONSTITUTION] = string_literal("One's hardiness. Determines physical defense."),
    [STAT_WILLPOWER]    = string_literal("One's resilience. Determines magical defense."),
    [STAT_AGILITY]      = string_literal("One's deftness. Determines damage reduction."),
    [STAT_SPEED]        = string_literal("One's hastiness. Determines movement, and likelihood to dodge."),
    [STAT_INTELLIGENCE] = string_literal("One's mind. Determines magic attack."),
    [STAT_LUCK]         = string_literal("One's fortune. Helps in the most unexpected ways."),
    [STAT_COUNTER]      = string_literal("One's ability to retaliate. Determines maximum counter-attacks."),
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
        .health                                                     = 100,
        .spell_points                                               = 100,
        .vigor                                                      = 100,
        .strength                                                   = 100,
        .constitution                                               = 100,
        .willpower                                                  = 100,
        .agility                                                    = 100,
        .speed                                                      = 100,
        .intelligence                                               = 100,
        .luck                                                       = 100,
        .counter                                                    = 0,
    };

#endif
