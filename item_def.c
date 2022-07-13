#ifndef ITEM_DEF_C
#define ITEM_DEF_C

/*
  NOTE: FNV1-A hasn't really failed me yet...
*/
typedef struct item_id {
    u32 id_hash; /* may have collisions but with small item count prolly not */
} item_id;

item_id item_id_make(string id_name) {
    item_id result = {};
    result.id_hash = hash_bytes_fnv1a(id_name.data, id_name.length);
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
enum item_type {
    ITEM_TYPE_MISC,
    ITEM_TYPE_CONSUMABLE_ITEM, /* party members & player */
};

struct item_def {
    string id_name;
    string name;
    string description;

    s32 type;

    s32 health_restoration_value;

    s32 flags;
    s32 max_stack_value;
    s32 gold_value;

    /* 
       I could make a mini DSL to apply some script effects...
       Would allow for items to be described in files... But idk yet.
       
       Have to expose lots of things to a lisp DSL I guess...
    */
    /* ironically a script usage file might be "cheaper?" */
};

#define MAX_ITEMS_DATABASE_SIZE (8192)
static struct item_def item_database[MAX_ITEMS_DATABASE_SIZE] = {};

static void initialize_items_database(void) {
    item_database[0].id_name                  = string_literal("item_trout_fish_5");
    item_database[0].name                     = string_literal("Dead Trout(?)");
    item_database[0].description              = string_literal("I found this at the bottom of the river. Still edible.");
    item_database[0].type                     = ITEM_TYPE_CONSUMABLE_ITEM;
    item_database[0].health_restoration_value = 5;
    item_database[0].gold_value               = 5;
    item_database[0].max_stack_value          = 20;

    item_database[1].id_name                  = string_literal("item_sardine_fish_5");
    item_database[1].name                     = string_literal("Dead Sardine(?)");
    item_database[1].description              = string_literal("... Something about rule number one...");
    item_database[1].type                     = ITEM_TYPE_CONSUMABLE_ITEM;
    item_database[1].health_restoration_value = 5;
    item_database[1].gold_value               = 5;
    item_database[1].max_stack_value          = 20;
}

static struct item_def* item_database_find_by_id(item_id id) {
    for (unsigned index = 0; index < MAX_ITEMS_DATABASE_SIZE; ++index) {
        struct item_def* candidate = &item_database[index];
        s32 hash = hash_bytes_fnv1a(candidate->id_name.data, candidate->id_name.length);

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

            u32 hash_first  = hash_bytes_fnv1a(first.data, first.length);
            u32 hash_second = hash_bytes_fnv1a(second.data, second.length);

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
