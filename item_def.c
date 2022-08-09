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
enum equipment_slot_flag_mask {
    EQUIPMENT_SLOT_FLAG_HEAD  = BIT(0),
    EQUIPMENT_SLOT_FLAG_BODY  = BIT(1),
    EQUIPMENT_SLOT_FLAG_HANDS = BIT(2),
    EQUIPMENT_SLOT_FLAG_LEGS  = BIT(3),
    EQUIPMENT_SLOT_FLAG_MISC  = BIT(4), /* accessory equips (x2) */
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
    u8 equipment_slot_flags;

    struct entity_stat_block_modifiers modifiers;

    s32 flags;
    s32 max_stack_value;
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
    item_database[0].id_name                  = string_literal("item_trout_fish_5");
    item_database[0].name                     = string_literal("Dead Trout(?)");
    item_database[0].description              = string_literal("I found this at the bottom of the river. Still edible.");
    item_database[0].type                     = ITEM_TYPE_CONSUMABLE_ITEM;
    item_database[0].health_restoration_value = 5;
    item_database[0].gold_value               = 5;
    item_database[0].max_stack_value          = 20;
    init_item_icon(0);

    item_database[1].id_name                  = string_literal("item_sardine_fish_5");
    item_database[1].name                     = string_literal("Dead Sardine(?)");
    item_database[1].description              = string_literal("... Something about rule number one...");
    item_database[1].type                     = ITEM_TYPE_CONSUMABLE_ITEM;
    item_database[1].health_restoration_value = 5;
    item_database[1].gold_value               = 5;
    item_database[1].max_stack_value          = 20;
    init_item_icon(1);

    item_database[2].id_name                  = string_literal("item_armor_rags");
    item_database[2].name                     = string_literal("Beggers' Rags");
    item_database[2].description              = string_literal("Found these in the street. Smells off...");
    item_database[2].stats.constitution       = 10;
    item_database[2].stats.agility            = -10;
    item_database[2].type                     = ITEM_TYPE_EQUIPMENT;
    item_database[2].equipment_slot_flags     = EQUIPMENT_SLOT_FLAG_BODY;
    item_database[2].modifiers                = entity_stat_block_modifiers_identity;
    item_database[2].gold_value               = 50;
    item_database[2].max_stack_value          = 20;
    init_item_icon(2);
    
    item_database[3].id_name                  = string_literal("item_armor_loincloth");
    item_database[3].name                     = string_literal("Loincloth");
    item_database[3].description              = string_literal("Fashioned from my old bathing towels.");
    item_database[3].type                     = ITEM_TYPE_EQUIPMENT;
    item_database[3].stats.constitution       = 10;
    item_database[3].equipment_slot_flags     = EQUIPMENT_SLOT_FLAG_LEGS;
    item_database[3].modifiers                = entity_stat_block_modifiers_identity;
    item_database[3].gold_value               = 50;
    item_database[3].max_stack_value          = 20;
    init_item_icon(3);

    item_database[4].id_name                  = string_literal("item_armor_bandage_wraps");
    item_database[4].name                     = string_literal("Bandages");
    item_database[4].stats.constitution       = 20;
    item_database[4].stats.agility            = -15;
    item_database[4].description              = string_literal("These are still bloody!");
    item_database[4].type                     = ITEM_TYPE_EQUIPMENT;
    item_database[4].equipment_slot_flags     = EQUIPMENT_SLOT_FLAG_HANDS;
    item_database[4].modifiers                = entity_stat_block_modifiers_identity;
    item_database[4].gold_value               = 50;
    item_database[4].max_stack_value          = 20;
    init_item_icon(4);

    item_database[5].id_name                  = string_literal("item_accessory_wedding_ring");
    item_database[5].name                     = string_literal("Wedding Ring");
    item_database[5].description              = string_literal("Fetches a nice price at a vendor. Sanctity? What's that?");
    item_database[5].type                     = ITEM_TYPE_EQUIPMENT;
    item_database[5].equipment_slot_flags     = EQUIPMENT_SLOT_FLAG_MISC;
    item_database[5].modifiers                = entity_stat_block_modifiers_identity;
    item_database[5].gold_value               = 150;
    item_database[5].max_stack_value          = 20;
    init_item_icon(5);
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
