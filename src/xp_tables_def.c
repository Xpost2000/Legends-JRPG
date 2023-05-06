#ifndef XP_TABLE_DEF_C
#define XP_TABLE_DEF_C

/*
  XP tables and other leveling related details,

  such as stat-growth per registered character. All additive.

  The XP table is global for the entire game.
*/

#define MAX_POSSIBLE_LEVELS_IN_ENGINE (99)

struct xp_table_entry {
    s32 xp;
};

#define MAX_ABILITIES_TO_LEARN (16)
struct stat_growth_table_entry {
    /* additional format data */
    s32 abilities_count;
    s32 abilities_to_learn[16];
    s32 health;
    s32 magic;
    Entity_Stat_Block_Base(s32);
};

/* NOTE: one indexed list */
struct xp_table {
    s32 count;
    struct xp_table_entry levels[MAX_POSSIBLE_LEVELS_IN_ENGINE+1];
};

/* TODO: add ability reward? or special status effect award? */
struct stat_growth_table {
    char entity_base_id[ENTITY_BASENAME_LENGTH_MAX];
    s32  count;
    struct stat_growth_table_entry growths[MAX_POSSIBLE_LEVELS_IN_ENGINE+1];
};

struct xp_table xp_table_empty(void);
struct stat_growth_table stat_table_for(string base_id);

void                      initialize_progression_tables(struct memory_arena* arena);
struct xp_table*          game_experience_table(void);
struct stat_growth_table* find_progression_table_for(string base_id);

#endif
