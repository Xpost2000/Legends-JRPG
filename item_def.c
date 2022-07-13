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
struct item_def {
    string id_name;
    string name;
    string description;

    s32 flags;
    s32 max_stack_value;
    s32 gold_value;

    /* 
       I could make a mini DSL to apply some script effects...
       Would allow for items to be described in files... But idk yet.
    */
};

#define MAX_ITEMS_DATABASE_SIZE (8192)
static struct item_def item_database[MAX_ITEMS_DATABASE_SIZE] = {};

static void initialize_items_database(void) {
    
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

        for (unsigned j = 0; j < MAX_ITEMS_DATABASE_SIZE; ++j) {
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
