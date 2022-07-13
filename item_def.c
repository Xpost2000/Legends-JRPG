#ifndef ITEM_DEF_C
#define ITEM_DEF_C

typedef struct item_id {
    string id; /* names are a bit harder to funge up... */
} item_id;

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

#endif
