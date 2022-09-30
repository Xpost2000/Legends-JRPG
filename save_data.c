#include "save_data_def.c"

#define SAVE_RECORDS_PER_SAVE_AREA_RECORD_CHUNK (64)

/* this linked list is fine since we only need to write */
/* and read incredibly sparingly... */

/* delta based saving */
enum save_record_type {
    SAVE_RECORD_TYPE_ENTITY_ENTITY,
    SAVE_RECORD_TYPE_ENTITY_CHEST,
};

struct save_record_entity_chest {
    /* chests will never move. Only write flags */
    u32 target_entity;
    u32 flags;
};

struct save_record {
    u32 type;

    union {
        
    };
};

struct save_area_record_chunk {
    s16 written_entries;
    struct save_record records[SAVE_RECORDS_PER_SAVE_AREA_RECORD_CHUNK];
    struct save_area_record_chunk* next;
};

struct save_area_record_entry {
    u32 map_hash_id; /* pray I don't change hashing algorithms or everything breaks! */
    u32 used;

    struct save_area_record_chunk* first;
    struct save_area_record_chunk* last;

    struct save_area_record_entry* next;
};

struct master_save_record {
    /* common things, such as all game variables */
    /*
      this isn't stored here, we're just writing it to the top of the file header,
      and reading it from there. Just pretend it's here though.
    */

    /* generic entries */
    u32 used_entries;
    struct save_area_record_entry* first;
    struct save_area_record_entry* last;
};

struct master_save_record global_save_data = {};

local struct memory_arena save_arena = {};

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

void apply_save_data(struct game_state* state) {
    
}
