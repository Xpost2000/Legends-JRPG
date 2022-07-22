#include "save_data_def.c"

/* delta based saving */

/*
  This is just a bunch of dynamic arrays
 */
/* all of these are implied to have been prefixed with a type discriminator. */
struct save_area_record_header {
    u32 type;
};

struct save_record_entity_chest {
    /* chests will never move. Only write flags */
    u32 target_entity;
    u32 flags;
};
struct save_area_record_entry {
    u32 map_hash;
    u32 used;
    u8 memory[Kilobyte(512)]; /* these are humungous note! */
};

struct {
    u32 used_entries;
    struct save_area_record_entry* entries[1024];
} global_save_data = {};

static struct memory_arena save_arena = {};

void begin_save_entry(u32 area_hash) {
}

void end_save_entry(u32 area_hash) {
    
}

void apply_save_data(struct game_state* state) {
    
}

void initialize_save_data(void) {
    save_arena = memory_arena_create_from_heap("Save Data Memory", Megabyte(8));
}

void finish_save_data(void) {
    memory_arena_finish(&save_arena);
}
