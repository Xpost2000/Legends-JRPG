#include "save_data_def.c"
#define CURRENT_SAVE_RECORD_VERSION (1)
#define SAVE_RECORDS_PER_SAVE_AREA_RECORD_CHUNK (64)

/* this linked list is fine since we only need to write */
/* and read incredibly sparingly... */

/* delta based saving */
enum save_record_type {
    SAVE_RECORD_TYPE_NIL,
    SAVE_RECORD_TYPE_ENTITY_ENTITY,
    SAVE_RECORD_TYPE_ENTITY_CHEST,
    SAVE_RECORD_TYPE_COUNT
};

local string save_record_type_strings[] = {
    [SAVE_RECORD_TYPE_NIL]           = string_literal("RECORD_NIL"),
    [SAVE_RECORD_TYPE_ENTITY_ENTITY] = string_literal("RECORD_ENTITY"),
    [SAVE_RECORD_TYPE_ENTITY_CHEST]  = string_literal("RECORD_CHEST"),
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
/*
  FORMAT basically
  --------------
  version: s32
  area_name: string[32]
  area_desc: string[64]
  inventory: entity_actor_inventory[0..item_count]
  player: xy, statblock, equipped items

  other_permenant_entities?

  save_record_data_begins
  save_record_data_ends

  weather? Should be set on load frankly... (on-enter scripts are evoked)
  disallow saving during combat, no combat state
  --------------
*/

/* I don't really care about versioning right now since I'm using this for experimental purposes to make any progress on the save system. */
void game_serialize_save(struct binary_serializer* serializer) {
    u32 save_version = CURRENT_SAVE_RECORD_VERSION;
    serialize_u32(serializer, &save_version);

    /* need to write this from the game itself, TODO: need to think of where to store names or just don't? */
    char area_name_DUMMY[SAVE_SLOT_WIDGET_SAVE_NAME_LENGTH]  = "BLACKROOT FOREST";
    char area_desc_DUMMY[SAVE_SLOT_WIDGET_DESCRIPTOR_LENGTH] = "ACT1: A job to finish.";

    serialize_bytes(serializer, area_name_DUMMY, SAVE_SLOT_WIDGET_SAVE_NAME_LENGTH);
    serialize_bytes(serializer, area_desc_DUMMY, SAVE_SLOT_WIDGET_DESCRIPTOR_LENGTH);

    serialize_s32(serializer, &game_state->inventory.item_count);
    serialize_bytes(serializer, game_state->inventory.items, sizeof(*game_state->inventory.items) * game_state->inventory.item_count);

    {
        struct entity* player = game_get_player(game_state);

        serialize_f32(serializer, &player->position.x);
        serialize_f32(serializer, &player->position.y);

        serialize_bytes(serializer, player->equip_slots, sizeof(player->equip_slots));
        serialize_s32(serializer, &player->health.value);
        serialize_s32(serializer, &player->magic.value);
        Serialize_Structure(serializer, player->stat_block);
        /* May need to serialize abilities later??? */
    }
}

void try_to_apply_record_entry(struct save_record* record, struct game_state* state);
void apply_save_data(struct game_state* state) {
    /* would be nice to write an iterator structure but it's not needed for this. */    
    u32 hashed_area_name = hash_bytes_fnv1a((u8*)state->loaded_area_name, cstring_length(state->loaded_area_name));
    bool found_matching_region = false;

    struct save_area_record_entry* area_entry = global_save_data.first;
    {
        while (area_entry && !found_matching_region) {
            if (area_entry->map_hash_id == hashed_area_name) {
                found_matching_region = true;
            } else {
                area_entry = area_entry->next;
            }
        }
    }

    if (found_matching_region) {
        struct save_area_record_chunk* chunk_entries = area_entry->first;

        /* figure out how to apply everything */
        while (chunk_entries) {
            for (s32 entry_index = 0; entry_index < chunk_entries->written_entries; ++entry_index) {
                struct save_record* current_record = chunk_entries->records + entry_index;
                try_to_apply_record_entry(current_record, state);
            }

            chunk_entries = chunk_entries->next;
        }
    }
}

void try_to_apply_record_entry(struct save_record* record, struct game_state* state) {
    _debugprintf("RECORD TYPE: %.*s", save_record_type_strings[record->type].length, save_record_type_strings[record->type].data);
    switch (record->type) {
        default: {
    _debugprintf("UNHANDLED RECORD TYPE: %.*s", save_record_type_strings[record->type].length, save_record_type_strings[record->type].data);
        } break;
    }
}
