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
    /*
      conceivably chests... should really only be looted or not...
     */
    u32 target_entity;
};

struct save_record {
    u32 type;

    union {
        struct save_record_entity_chest chest_record;
    };
};

struct save_area_record_chunk {
    s16 written_entries;
    struct save_record records[SAVE_RECORDS_PER_SAVE_AREA_RECORD_CHUNK];
    struct save_area_record_chunk* next;
};

struct save_area_record_entry {
    u32 map_hash_id; /* pray I don't change hashing algorithms or everything breaks! */

    struct save_area_record_chunk* first;
    struct save_area_record_chunk* last;

    struct save_area_record_entry* next;
};

local s32 save_area_record_entry_record_count(struct save_area_record_entry* entry) {
    s32                            count = 0;
    struct save_area_record_chunk* chunk = entry->first;

    while (chunk) {
        count += chunk->written_entries;
        chunk = chunk->next;
    }

    return count;
}

struct master_save_record {
    /* common things, such as all game variables */
    /*
      this isn't stored here, we're just writing it to the top of the file header,
      and reading it from there. Just pretend it's here though.
    */

    /* generic entries */
    struct save_area_record_entry* first;
    struct save_area_record_entry* last;
};

struct master_save_record global_save_data = {};

local s32 save_area_record_entry_count(void) {
    s32                            count = 0; 
    struct save_area_record_entry* entry = global_save_data.first;

    while (entry) {
        count+=1;
        entry = entry->next;
    }

    return count;
}


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
    struct binary_serializer write_serializer = open_write_file_serializer(filename_from_saveslot_id(save_slot_id));
    game_serialize_save(&write_serializer);
    serializer_finish(&write_serializer);
}

void game_load_from_save_slot(s32 save_slot_id) {
    assertion(save_slot_id >= 0 && save_slot_id < GAME_MAX_SAVE_SLOTS);

    /* clear both arenas and start from zero. */
    {
        zero_memory(&global_save_data, sizeof(global_save_data));
        memory_arena_clear_top(&save_arena);
        memory_arena_clear_bottom(&save_arena);
    }

    struct binary_serializer read_serializer = open_read_file_serializer(filename_from_saveslot_id(save_slot_id));
    game_serialize_save(&read_serializer);
    apply_save_data(game_state);
    serializer_finish(&read_serializer);
}

struct save_area_record_entry* save_record_allocate_area_entry_chunk(void) {
    struct save_area_record_entry* new_entry = memory_arena_push(&save_arena, sizeof(*new_entry));

    if (!global_save_data.last) {
        global_save_data.first = global_save_data.last = new_entry;
    } else {
        global_save_data.last->next = new_entry;
        global_save_data.last       = new_entry;
    }

    zero_memory(new_entry, sizeof(*new_entry));
    return new_entry;
}

struct save_area_record_chunk* save_area_record_chunk_allocate_entry_chunk(struct save_area_record_entry* area_entry) {
    struct save_area_record_chunk* new_record_chunk = memory_arena_push(&save_arena, sizeof(*new_record_chunk));

    if (!area_entry->last) {
        area_entry->first = area_entry->last = new_record_chunk;
    } else {
        area_entry->last->next = new_record_chunk;
        area_entry->last       = new_record_chunk;
    }

    zero_memory(new_record_chunk, sizeof(*new_record_chunk));
    return new_record_chunk;
}

struct save_record* save_area_record_entry_allocate_record(struct save_area_record_entry* area_entry) {
    if (!area_entry->last) {
        save_area_record_chunk_allocate_entry_chunk(area_entry);
    }

    if (area_entry->last->written_entries >= SAVE_RECORDS_PER_SAVE_AREA_RECORD_CHUNK) {
        save_area_record_chunk_allocate_entry_chunk(area_entry);
    }

    struct save_record* new_record = &area_entry->last->records[area_entry->last->written_entries++];
    return new_record;
}

struct save_area_record_entry* save_record_area_entry_find_or_allocate(string entryname) {
    u32 hashed_name = hash_bytes_fnv1a((u8*)entryname.data, entryname.length);
    {
        struct save_area_record_entry* cursor = global_save_data.first;

        while (cursor) {
            if (cursor->map_hash_id == hashed_name) {
                _debugprintf("found match for \"%.*s\"", entryname.length, entryname.data);
                return cursor;
            }
        }
    }

    _debugprintf("no matching area record... creating for \"%.*s\"", entryname.length, entryname.data);

    struct save_area_record_entry* new_entry = save_record_allocate_area_entry_chunk();
    new_entry->map_hash_id = hashed_name;
    return new_entry;
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
    serialize_bytes(serializer, game_state->loaded_area_name, sizeof(game_state->loaded_area_name));

    if (serializer->mode == BINARY_SERIALIZER_READ) {
        load_level_from_file(game_state, string_from_cstring(game_state->loaded_area_name));
    }

    serialize_s32(serializer, &game_state->inventory.item_count);
    serialize_bytes(serializer, game_state->inventory.items, sizeof(*game_state->inventory.items) * game_state->inventory.item_count);

    {
        struct entity* player = game_get_player(game_state);

        serialize_f32(serializer, &player->position.x);
        serialize_f32(serializer, &player->position.y);

        _debugprintf("PLAYER AT <%f, %f>", player->position.x, player->position.y);

        serialize_bytes(serializer, player->equip_slots, sizeof(player->equip_slots));
        serialize_s32(serializer, &player->health.value);
        serialize_s32(serializer, &player->magic.value);
        Serialize_Structure(serializer, player->stat_block);
        /* May need to serialize abilities later??? */
    }

    /* so this part isn't so fun since it varies a little because we may have to allocate, so we have to branch the code here... */
    s32 area_entry_count = save_area_record_entry_count();
    serialize_s32(serializer, &area_entry_count);

    /*
      this is version dependent! Because I use a union to store it!

      While events are theoretically streams (because I don't care about random access,
      since I want all of them at once),

      it **is** possible to future proof it by simply making it a stream of data (of varying lengths),
      as the individual record types are extremely unlikely to change (and even then, it 's way easier to
      control version variances between individual record types as opposed to the whole size of the save record,

      that might be some... Uh, shit I forgot the rest of what I wanted to type, got distracted by texting on Discord.)
    */

    if (serializer->mode == BINARY_SERIALIZER_WRITE) {
        /* we're going to write them as if they were flat entries */ 
        struct save_area_record_entry* current_area_entry = global_save_data.first;

        for (s32 area_entry_index = 0; area_entry_index < area_entry_count; ++area_entry_index) {
            s32 record_entries = save_area_record_entry_record_count(current_area_entry);
            _debugprintf("Current area entry has %d entries to write", record_entries);

            serialize_u32(serializer, &current_area_entry->map_hash_id);
            serialize_s32(serializer, &record_entries);

            {
                struct save_area_record_chunk* entry_chunk = current_area_entry->first;

                s32 written = 0;
                while (entry_chunk) {
                    serialize_bytes(serializer, entry_chunk->records, sizeof(*entry_chunk->records) * entry_chunk->written_entries);
                    written += entry_chunk->written_entries;
                    _debugprintf("Current entry chunk wrote: %d entries (%d total)", entry_chunk->written_entries, written);

                    entry_chunk = entry_chunk->next;
                }

                _debugprintf("Done writing area entry, advancing...");
                assertion(written == record_entries && "Hmm, this doesn't smell so good to me.");
            }

            current_area_entry = current_area_entry->next;
        }
    } else {
        for (s32 area_entry_index = 0; area_entry_index < area_entry_count; ++area_entry_index) {
            struct save_area_record_entry* current_area_entry = save_record_allocate_area_entry_chunk();
            s32                            record_entries     = save_area_record_entry_record_count(current_area_entry);

            serialize_u32(serializer, &current_area_entry->map_hash_id);
            serialize_s32(serializer, &record_entries);

            {
                s32 expected_chunks_to_write = record_entries / SAVE_RECORDS_PER_SAVE_AREA_RECORD_CHUNK + 1;
                s32 consumed_chunks = 0;

                _debugprintf("Read mode of save file, have to write %d chunks (%d total entries)", expected_chunks_to_write, record_entries);

                s32 original_record_entries = record_entries;
                for (s32 chunk_to_write = 0; chunk_to_write < expected_chunks_to_write; ++chunk_to_write) {
                    struct save_area_record_chunk* entry_chunk = save_area_record_chunk_allocate_entry_chunk(current_area_entry);

                    s32 amount_to_consume = record_entries;
                    if (amount_to_consume >= SAVE_RECORDS_PER_SAVE_AREA_RECORD_CHUNK) {
                        amount_to_consume = SAVE_RECORDS_PER_SAVE_AREA_RECORD_CHUNK;
                    }

                    serialize_bytes(serializer, entry_chunk->records, sizeof(*entry_chunk->records) * amount_to_consume);
                    entry_chunk->written_entries = amount_to_consume;
                    record_entries -= amount_to_consume;
                    consumed_chunks += 1;
                }

                assertion(save_area_record_entry_record_count(current_area_entry) == original_record_entries && "Okay, that's bad...");
            }
        }
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
                _debugprintf("Found matching region!");
            } else {
                area_entry = area_entry->next;
            }
        }
    }

    if (found_matching_region) {
        struct save_area_record_chunk* chunk_entries = area_entry->first;

        /* figure out how to apply everything */
        while (chunk_entries) {
            _debugprintf("Iteration of chunk entries (%d entries in chunk)", chunk_entries->written_entries);
            for (s32 entry_index = 0; entry_index < chunk_entries->written_entries; ++entry_index) {
                struct save_record* current_record = chunk_entries->records + entry_index;
                try_to_apply_record_entry(current_record, state);
            }

            chunk_entries = chunk_entries->next;
        }
    } else {
        _debugprintf("Hmm? Not in the correct region I guess");
    }
}

void try_to_apply_record_entry(struct save_record* record, struct game_state* state) {
    _debugprintf("RECORD TYPE: %.*s", save_record_type_strings[record->type].length, save_record_type_strings[record->type].data);
    struct level_area* area = &state->loaded_area;

    switch (record->type) {
        case SAVE_RECORD_TYPE_ENTITY_CHEST: {
            struct save_record_entity_chest* chest_record = &record->chest_record;
            area->chests[chest_record->target_entity].flags |= ENTITY_CHEST_FLAGS_UNLOCKED;
            _debugprintf("opening chest: %d\n", chest_record->target_entity);
        } break;
        default: {
            _debugprintf("UNHANDLED RECORD TYPE: %.*s", save_record_type_strings[record->type].length, save_record_type_strings[record->type].data);
        } break;
    }
}

/* SAVE DATA REGISTER ENTRY CODE */
local struct save_area_record_entry* current_area_save_entry(void) {
    struct save_area_record_entry* area_entry = save_record_area_entry_find_or_allocate(string_from_cstring(game_state->loaded_area_name));
    return area_entry;
}

void save_data_register_chest_looted(u32 chest_id) {
    struct save_area_record_entry* area       = current_area_save_entry();
    struct save_record*            new_record = save_area_record_entry_allocate_record(area);
    new_record->type                          = SAVE_RECORD_TYPE_ENTITY_CHEST;
    new_record->chest_record.target_entity    = chest_id;
}
