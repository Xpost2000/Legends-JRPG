/*
  I think I may need to restructure this, but I'll really know once
  I try to turn this into a bunch of files.
 */
#ifndef ITEM_DEF_C
#define ITEM_DEF_C

#include "entity_stat_block_def.c"

/*
  NOTE: FNV1-A hasn't really failed me yet...
*/
typedef struct item_id {
    u32 id_hash; /* may have collisions but with small item count prolly not */
} item_id;

item_id item_id_make(string id_name) {
    item_id result = {};
    result.id_hash = hash_bytes_fnv1a((u8*)id_name.data, id_name.length);
    return result;
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
    s32 damage_value;

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

#define MAX_ITEMS_DATABASE_SIZE (8192)
static struct item_def item_database[MAX_ITEMS_DATABASE_SIZE] = {};

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

/* NOTE: need to load this from a file */
static void initialize_items_database(void) {
    s32 id = 0;
    item_database[id].id_name                  = string_literal("item_gold");
    item_database[id].name                     = string_literal("Gold");
    item_database[id].description              = string_literal("What makes the world go round...");
    item_database[id].type                     = ITEM_TYPE_MISC;
    item_database[id].gold_value               = 1;
    item_database[id].max_stack_value          = -1;
    init_item_icon(id);
    id += 1;

    item_database[id].id_name                  = string_literal("item_trout_fish_5");
    item_database[id].name                     = string_literal("Dead Trout(?)");
    item_database[id].description              = string_literal("I found this at the bottom of the river. Still edible.");
    item_database[id].type                     = ITEM_TYPE_CONSUMABLE_ITEM;
    item_database[id].health_restoration_value = 5;
    item_database[id].gold_value               = 5;
    item_database[id].max_stack_value          = 20;
    init_item_icon(id);
    id += 1;

    item_database[id].id_name                  = string_literal("item_sardine_fish_5");
    item_database[id].name                     = string_literal("Dead Sardine(?)");
    item_database[id].description              = string_literal("... Something about rule number one...");
    item_database[id].type                     = ITEM_TYPE_CONSUMABLE_ITEM;
    item_database[id].health_restoration_value = 5;
    item_database[id].gold_value               = 5;
    item_database[id].max_stack_value          = 20;
    init_item_icon(id);
    id += 1;

    item_database[id].id_name                  = string_literal("item_armor_rags");
    item_database[id].name                     = string_literal("Beggers' Rags");
    item_database[id].description              = string_literal("Found these in the street. Smells off...");
    item_database[id].stats.constitution       = 10;
    item_database[id].stats.agility            = -10;
    item_database[id].type                     = ITEM_TYPE_EQUIPMENT;
    item_database[id].equipment_slot_flags     = EQUIPMENT_SLOT_FLAG_BODY;
    item_database[id].modifiers                = entity_stat_block_modifiers_identity;
    item_database[id].gold_value               = 50;
    item_database[id].max_stack_value          = 20;
    init_item_icon(id);
    id += 1;
    
    item_database[id].id_name                  = string_literal("item_armor_loincloth");
    item_database[id].name                     = string_literal("Loincloth");
    item_database[id].description              = string_literal("Fashioned from my old bathing towels.");
    item_database[id].type                     = ITEM_TYPE_EQUIPMENT;
    item_database[id].stats.constitution       = 10;
    item_database[id].equipment_slot_flags     = EQUIPMENT_SLOT_FLAG_LEGS;
    item_database[id].modifiers                = entity_stat_block_modifiers_identity;
    item_database[id].gold_value               = 50;
    item_database[id].max_stack_value          = 20;
    init_item_icon(id);
    id += 1;

    item_database[id].id_name                  = string_literal("item_armor_bandage_wraps");
    item_database[id].name                     = string_literal("Bandages");
    item_database[id].stats.constitution       = 20;
    item_database[id].stats.agility            = -15;
    item_database[id].description              = string_literal("These are still bloody!");
    item_database[id].type                     = ITEM_TYPE_EQUIPMENT;
    item_database[id].equipment_slot_flags     = EQUIPMENT_SLOT_FLAG_HANDS;
    item_database[id].modifiers                = entity_stat_block_modifiers_identity;
    item_database[id].gold_value               = 50;
    item_database[id].max_stack_value          = 20;
    init_item_icon(id);
    id += 1;

    item_database[id].id_name                  = string_literal("item_accessory_wedding_ring");
    item_database[id].name                     = string_literal("Wedding Ring");
    item_database[id].description              = string_literal("Fetches a nice price at a vendor. Sanctity? What's that?");
    item_database[id].type                     = ITEM_TYPE_EQUIPMENT;
    item_database[id].equipment_slot_flags     = EQUIPMENT_SLOT_FLAG_MISC;
    item_database[id].modifiers                = entity_stat_block_modifiers_identity;
    item_database[id].gold_value               = 150;
    item_database[id].max_stack_value          = 20;
    init_item_icon(id);
    id += 1;
}

static struct item_def* item_database_find_by_id(item_id id) {
    for (unsigned index = 0; index < MAX_ITEMS_DATABASE_SIZE; ++index) {
        struct item_def* candidate = &item_database[index];
        s32 hash = hash_bytes_fnv1a((u8*) candidate->id_name.data, candidate->id_name.length);

        if (hash == id.id_hash) {
            return candidate;
        }
    }

    return 0;
}

/* TODO only for debug reasons I guess */
static bool verify_no_item_id_name_hash_collisions(void) {
    bool bad = false;
#ifndef RELEASE
    for(unsigned i = 0; i < MAX_ITEMS_DATABASE_SIZE; ++i) {
        string first = item_database[i].id_name;

        if (first.length <= 0) {
            continue;
        }

        for (unsigned j = i; j < MAX_ITEMS_DATABASE_SIZE; ++j) {
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
