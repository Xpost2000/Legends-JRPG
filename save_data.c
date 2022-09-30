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
    save_arena = memory_arena_create_from_heap("Save Data Memory", Megabyte(16));
}

void finish_save_data(void) {
    memory_arena_finish(&save_arena);
}

/*
  prune a lot from the game state to figure out a sensible thing... Might be based on the level?
  Who knows?
 */

local string filename_from_saveslot_id(s32 id) {
    return format_temp_s("./saves/sav%02d.sav", id);
}

void game_serialize_save(struct binary_serializer* serializer);

void game_write_save_slot(s32 save_slot_id) {
    assertion(save_slot_id >= 0 && save_slot_id < GAME_MAX_SAVE_SLOTS);
    struct binary_serializer write_serializer = open_read_file_serializer(filename_from_saveslot_id(save_slot_id));
    game_serialize_save(&write_serializer);
}

void game_load_from_save_slot(s32 save_slot_id) {
    assertion(save_slot_id >= 0 && save_slot_id < GAME_MAX_SAVE_SLOTS);
    struct binary_serializer read_serializer = open_read_file_serializer(filename_from_saveslot_id(save_slot_id));
    game_serialize_save(&read_serializer);
}

/* binary friendly format to runtime friendly format. Oh boy... let the games begin. */
void game_serialize_save(struct binary_serializer* serializer) {
    
}
