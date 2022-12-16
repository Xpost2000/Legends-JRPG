/*
  There are very few usages of items in the codebase, and the game
  design right now which kind of hurts!
*/
#ifndef ITEM_DEF_C
#define ITEM_DEF_C

#include "entity_stat_block_def.c"

/*
  NOTE: FNV1-A hasn't really failed me yet...
*/
SERIALIZE typedef struct item_id {
    SERIALIZE u32 id_hash; /* may have collisions but with small item count prolly not */
} item_id;

item_id item_id_make(string id_name) {
    item_id result = {};
    result.id_hash = hash_bytes_fnv1a((u8*)id_name.data, id_name.length);
    return result;
}

/* need to avoid having to "dereference items so much... It's more indirection but yeah...." */
bool item_id_equal(item_id a, item_id b) {
    return a.id_hash == b.id_hash;
}

struct item_instance {
    item_id item; 
    s32 count;/* for inventory stacking, as long as they are not equippable  */
};

/* 
   an obese struct. All items are probably just hard coded in-game,
   which is okay in this case.
*/
enum item_flags {
    ITEM_NO_FLAGS = 0,
};

/* these are broad/general types. */
/* The game itself will sort by more specific criteria. */
enum item_type {
    /* do not use for item type please */
    ITEM_SORT_FILTER_ALL,
    ITEM_TYPE_MISC,
    ITEM_TYPE_CONSUMABLE_ITEM, /* party members & player */

    /* these are equippables  */
    ITEM_TYPE_WEAPON,
    ITEM_TYPE_EQUIPMENT,

    ITEM_TYPE_COUNT,
};
/* editor strings */
static string item_type_strings[] = {
    string_literal("(all.)"),
    string_literal("(misc.)"),
    string_literal("(consum.)"),
    string_literal("(weap.)"),
    string_literal("(equip.)"),
    string_literal("(count)"),
};

/* 
   some equipments may take multiple slots.
*/
enum equipment_slot {
    EQUIPMENT_SLOT_FLAG_HEAD    = (0),
    EQUIPMENT_SLOT_FLAG_BODY    = (1),
    EQUIPMENT_SLOT_FLAG_HANDS   = (2),
    EQUIPMENT_SLOT_FLAG_LEGS    = (3),
    EQUIPMENT_SLOT_FLAG_MISC    = (4), /* accessory equips (x2) */
    EQUIPMENT_SLOT_FLAG_WEAPON  = (5), /* accessory equips (x2) */
};

local s32 global_item_icon_frame_counter = 0;
struct item_def {
    string id_name;
    string name;
    string description;

    s32 type;
    s32 health_restoration_value;

    /* override the default type icon */
    /* animation time is going to be the same for all items */
    string icon_name;
    s32    frame_count;

    /* multiplication is always applied in order first. */
    /* though all modifiers are added together first. */
    struct {
        s32 health;
        s32 spell_points;

        Entity_Stat_Block_Base(s32);
    } stats;

    /* armor stat calculations DR */
    s32 defensive_value;

    s32  ability_class_group_id;
    s32  ability_count;
    s32* abilities;

    /* TODO: not flags anymore */
    u8 equipment_slot_flags;

    struct entity_stat_block_modifiers modifiers;

    s32 flags;
    s32 max_stack_value; /* -1 == infinite... */
    s32 gold_value;

    /* 
       I could make a mini DSL to apply some script effects...
       Would allow for items to be described in files... But idk yet.
       
       Have to expose lots of things to a lisp DSL I guess...
    */
    /* ironically a script usage file might be "cheaper?" */
    string script_file_name;
    /* have some code hooks for on combat */
    /* need to have temporary bindings (such as HIT/IT/SELF) for context sensitive hooks */
};

item_id item_get_id(struct item_def* item) {
    return item_id_make(item->id_name);
}

static s32              item_database_count               = 0;
static struct item_def* item_database                     = 0;

local void init_item_icon(s32 index) {
    if (item_database[index].frame_count == 0)
        item_database[index].frame_count = 1;

    if (item_database[index].icon_name.length)
        return;

    string icon_name = string_literal("");
    switch (item_database[index].type) {
        case ITEM_TYPE_MISC: {
            icon_name = string_literal("misc");
        } break;
        case ITEM_TYPE_WEAPON: {
            icon_name = string_literal("weapon");
        } break;
        case ITEM_TYPE_EQUIPMENT: {
            icon_name = string_literal("equipment");
        } break;
        case ITEM_TYPE_CONSUMABLE_ITEM: {
            icon_name = string_literal("consumable");
        } break;
    }
    item_database[index].icon_name = icon_name;
}

static void initialize_items_database(void);
static struct item_def* item_database_find_by_id(item_id id) {
    for (unsigned index = 0; index < item_database_count; ++index) {
        struct item_def* candidate = &item_database[index];
        s32 hash = hash_bytes_fnv1a((u8*) candidate->id_name.data, candidate->id_name.length);

        if (hash == id.id_hash) {
            return candidate;
        }
    }

    return 0;
}

/* NOTE: this should cache for multiple fonts, but all of the engines' fonts are identically sized */
/* but this can easily be expanded for that. */
local f32 item_database_longest_string_name(struct font_cache* font, f32 scale) {
    local bool found_answer = false;
    local f32  answer       = 0;

    if (!found_answer) {
        for (s32 item_index = 0; item_index < item_database_count; ++item_index) {
            f32 width = font_cache_text_width(font, item_database[item_index].name, scale);
            if (width > answer) {
                answer = width;  
            }
        }
    }

    return answer;
} 

/* TODO only for debug reasons I guess */
static bool verify_no_item_id_name_hash_collisions(void) {
    bool bad = false;
#ifndef RELEASE
    for(unsigned i = 0; i < item_database_count; ++i) {
        string first = item_database[i].id_name;

        if (first.length <= 0) {
            continue;
        }

        for (unsigned j = i; j < item_database_count; ++j) {
            if (i == j) {
                continue;
            }
            
            string second = item_database[j].id_name;

            if (second.length <= 0) {
                continue;
            }

            u32 hash_first  = hash_bytes_fnv1a((u8*) first.data, first.length);
            u32 hash_second = hash_bytes_fnv1a((u8*) second.data, second.length);

            if (hash_second == hash_first) {
                _debugprintf("hash collision(\"%.*s\" & \"%.*s\")", first.length, first.data, second.length, second.data);
                bad = true;
            }
        }
    }

#endif
    return !bad;
}

#endif
