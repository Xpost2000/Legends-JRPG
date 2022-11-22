#include "save_data_def.c"
#define CURRENT_SAVE_RECORD_VERSION (3)
/*
  NOTE: it may be nice to build an iterator type for these nested things...

  Save record versions are done to accomodate new save record data updates

  VERSION1&2:
      Basically initial versions.
      (broken!)
  VERSION3: (new forward format)
      size discriminator
      Entity record
         - Position
         - Health
*/
#define SAVE_RECORDS_PER_SAVE_AREA_RECORD_CHUNK (64)

/*
  Metaprogramming would genuinely be nice for a lot of this savefile business.
  Oh well.
*/

/* this linked list is fine since we only need to write */
/* and read incredibly sparingly... */

/*
  Ideally I would've made an iterator interface which would allow for less painful read/write code,
  but honestly it doesn't matter too much right now.
*/

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

enum save_record_entity_field_flags {
    SAVE_RECORD_ENTITY_FIELD_FLAGS_NONE         = 0,
    SAVE_RECORD_ENTITY_FIELD_FLAGS_HEALTH       = BIT(0),
    SAVE_RECORD_ENTITY_FIELD_FLAGS_POSITION     = BIT(1),
    SAVE_RECORD_ENTITY_FIELD_FLAGS_DIRECTION    = BIT(2),
    SAVE_RECORD_ENTITY_FIELD_FLAGS_ENTITY_FLAGS = BIT(3),
};
struct save_record_entity_entity {
    entity_id id;
    u32       entity_field_flags; /* intepret whether to read the fields, we save them regardless though */
    u32       entity_flags      ; /* flags stored on the entity itself. */
    v2f32     position;
    s32       health;
    u8        direction;
};

struct save_record {
    u32 type;
    /*
      NOTE:
      byte size of record_size,
      under the assumption that I will generally only add fields (without
      changing the size of or moving older fields, which is a fair assumption.),

      this decision should make the savefiles forward compatible with all engine versions
      after today.

      [At least the save records part.]
    */
    s32 record_size;

    union {
        struct save_record_entity_chest  chest_record;
        struct save_record_entity_entity entity_record;
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
#ifndef RELEASE
    save_arena = memory_arena_create_from_heap("Save Data Memory", Megabyte(16));
#else
    save_arena = memory_arena_push_sub_arena(&game_arena, Megabyte(4));
#endif
}

void finish_save_data(void) {
#ifndef RELEASE
    memory_arena_finish(&save_arena);
#endif
}

/*
  prune a lot from the game state to figure out a sensible thing... Might be based on the level?
  Who knows?
 */

string filename_from_saveslot_id(s32 id) {
    return format_temp_s(GAME_DEFAULT_SAVE_PATH "/sav%02d.sav", id);
}

void game_serialize_save(struct binary_serializer* serializer);

struct save_data_description get_save_data_description(s32 save_id) {
    struct save_data_description result = {};

    string filename = filename_from_saveslot_id(save_id);
    if (file_exists(filename)) {
        result.good = true;

        /* partial version of serialization routine only for the "header" */
        struct binary_serializer read_serializer = open_read_file_serializer(filename_from_saveslot_id(save_id));
        {
            u32 save_version = CURRENT_SAVE_RECORD_VERSION;
            serialize_u32(&read_serializer, &save_version);
            serialize_bytes(&read_serializer, result.name, SAVE_SLOT_WIDGET_SAVE_NAME_LENGTH);
            serialize_bytes(&read_serializer, result.descriptor, SAVE_SLOT_WIDGET_DESCRIPTOR_LENGTH);
            serialize_s64(&read_serializer, &result.timestamp);
            _debugprintf("timestamp read as: %lld", result.timestamp);
        }
        serializer_finish(&read_serializer);
    }

    return result;
}

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
    new_record->record_size        = sizeof(*new_record);
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
            cursor = cursor->next;
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

local void save_serialize_record_entry(struct save_area_record_chunk* entry_chunk, s32 record_to_consume, struct binary_serializer* serializer, s32 save_version) {
    struct save_record* record_to_serialize = entry_chunk->records + record_to_consume; 

    switch (save_version) {
        case 1:
        case 2: { /* This was before the backwards compatibility changes in Version 3 */
            assertion(!"This is a prerelease save file. I have no idea why you have one of these!");
        } break;
            /* The default case is meant to generally be forward compatible with everything in the future... */
            /* If it fails to work, I can analyze the specific structures and drop down */
        case CURRENT_SAVE_RECORD_VERSION: {
        default:
            if (save_version != CURRENT_SAVE_RECORD_VERSION) {
                s32 current_size_of_save_record_structure = sizeof(struct save_record);

                serialize_u32(serializer, &record_to_serialize->type);
                serialize_s32(serializer, &record_to_serialize->record_size);

                s32 already_read = sizeof(s32) + sizeof(u32);

                s32 record_size                        = record_to_serialize->record_size-already_read;
                current_size_of_save_record_structure -= already_read;

                if (record_size >= current_size_of_save_record_structure) {
                    u8 garbage_bytes_to_read[4096];
                    serialize_bytes(serializer, record_to_serialize, current_size_of_save_record_structure);
                    s32 remaining_read_amount = record_size - current_size_of_save_record_structure;
                    serialize_bytes(serializer, garbage_bytes_to_read, remaining_read_amount);
                } else {
                    serialize_bytes(serializer, record_to_serialize, record_size);
                }
            } else {
                /* serialize_bytes(serializer, entry_chunk->records, sizeof(*entry_chunk->records) * amount_to_consume); */
                serialize_bytes(serializer, record_to_serialize, sizeof(*record_to_serialize));
            }
        } break;
    }
}

/* I don't really care about versioning right now since I'm using this for experimental purposes to make any progress on the save system. */
void game_serialize_save(struct binary_serializer* serializer) {
    u32 save_version = CURRENT_SAVE_RECORD_VERSION;
    serialize_u32(serializer, &save_version);

    /* need to write this from the game itself, TODO: need to think of where to store names or just don't? */
    char area_name_DUMMY[SAVE_SLOT_WIDGET_SAVE_NAME_LENGTH]  = "THE DRUNKEN DRAGON";
    char area_desc_DUMMY[SAVE_SLOT_WIDGET_DESCRIPTOR_LENGTH] = "A GATHERING OF HEROES.";

    serialize_bytes(serializer, area_name_DUMMY, SAVE_SLOT_WIDGET_SAVE_NAME_LENGTH);
    serialize_bytes(serializer, area_desc_DUMMY, SAVE_SLOT_WIDGET_DESCRIPTOR_LENGTH);
    {
        s64 timestamp = system_get_current_time();
        _debugprintf("Wrote time as: %lld vs. %lld", timestamp, system_get_current_time());
        serialize_s64(serializer, &timestamp);
    }
    serialize_bytes(serializer, game_state->loaded_area_name, sizeof(game_state->loaded_area_name));

    if (serializer->mode == BINARY_SERIALIZER_READ) {
        /* eh... Probably shouldn't be here */
        load_level_from_file(game_state, string_from_cstring(game_state->loaded_area_name));
    }

    serialize_s32(serializer, &game_state->inventory.item_count);
    serialize_bytes(serializer, game_state->inventory.items, sizeof(*game_state->inventory.items) * game_state->inventory.item_count);

    {
        struct entity* player = game_get_player(game_state);

        serialize_f32(serializer, &player->position.x);
        serialize_f32(serializer, &player->position.y);
        if (save_version > 1) {
            serialize_u8(serializer, &player->facing_direction);
            serialize_bytes(serializer, player->equip_slots, ENTITY_EQUIP_SLOT_INDEX_COUNT * sizeof(item_id));
            /* prevent odd states happening on game load */
            serialize_s32(serializer, &player->interacted_script_trigger_write_index);
            serialize_bytes(serializer, player->interacted_script_trigger_ids, 32 * sizeof(s32));
        }

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

        { /* SAVE RECORDS */
            struct save_area_record_entry* current_area_entry = global_save_data.first;

            for (s32 area_entry_index = 0; area_entry_index < area_entry_count; ++area_entry_index) {
                s32 record_entries = save_area_record_entry_record_count(current_area_entry);
                _debugprintf("Current area entry has %d entries to write", record_entries);

                serialize_u32(serializer, &current_area_entry->map_hash_id);
                serialize_s32(serializer, &record_entries);

                {
                    struct save_area_record_chunk* entry_chunk = current_area_entry->first;

                    /* NOTE: when writing I don't really care about how I serialize, just blit the direct image. */
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
        }
        { /* GAME_ VARIABLES/FLAGS */
            if (save_version > 1) {
                s32 game_variables_total = game_variables_count_all();
                serialize_s32(serializer, &game_variables_total);

                s32 written = 0;
                struct game_variable_chunk* cursor = game_state->variables.first;

                while (cursor) {
                    for (s32 current_variable_index = 0; current_variable_index < cursor->variable_count; ++current_variable_index) {
                        struct game_variable* var = cursor->variables + current_variable_index;
                        serialize_bytes(serializer, var->name, MAX_GAME_VARIABLE_NAME_LENGTH);
                        serialize_s32(serializer, &var->value);
                        _debugprintf("wrote game var: \"%s\": value: %d", var->name, var->value);
                        written += 1;
                    }
                    cursor = cursor->next;
                }

                assertion(written == game_variables_total && "Weird, the written amount of game variables is different than the counted amount...");
            }
        }
    } else {
        {/* SAVE RECORDS */
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

                        for (s32 record_to_consume = 0; record_to_consume < amount_to_consume; ++record_to_consume) {
                            save_serialize_record_entry(entry_chunk, record_to_consume, serializer, save_version);
                        }
                        
                        /* serialize_bytes(serializer, entry_chunk->records, sizeof(*entry_chunk->records) * amount_to_consume); */

                        entry_chunk->written_entries = amount_to_consume;
                        record_entries -= amount_to_consume;
                        consumed_chunks += 1;
                    }

                    assertion(save_area_record_entry_record_count(current_area_entry) == original_record_entries && "Okay, that's bad...");
                }
            }
        }

        /* thankfully this is way simpler than the other one since it kind of just does itself */
        { /* GAME_ VARIABLES/FLAGS */
            /* this is not read in chunks, though it probably should be later on! Although area save records might be expected to grow much bigger later? */
            s32 game_variables_total = 0;
            serialize_s32(serializer, &game_variables_total);

            for (s32 game_variable_index = 0; game_variable_index < game_variables_total; ++game_variable_index) {
                char game_var_name[MAX_GAME_VARIABLE_NAME_LENGTH];
                int  game_var_value;
                serialize_bytes(serializer, game_var_name, MAX_GAME_VARIABLE_NAME_LENGTH);
                serialize_s32(serializer, &game_var_value);

                game_variable_set(string_from_cstring(game_var_name), game_var_value);
                _debugprintf("read game var: \"%s\": value: %d", game_var_name, game_var_value);
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

local void apply_save_record_chest_entry(struct save_record_entity_chest* chest_record, struct game_state* state) {
    struct level_area* area = &state->loaded_area;
    area->chests[chest_record->target_entity].flags |= ENTITY_CHEST_FLAGS_UNLOCKED;
    _debugprintf("opening chest: %d\n", chest_record->target_entity);
}

local void apply_save_record_entity_entry(struct save_record_entity_entity* entity_record, struct game_state* state) {
    struct entity* entity_object = game_dereference_entity(state, entity_record->id);

    u32 field_flags_to_read = entity_record->entity_field_flags;

    if (field_flags_to_read & SAVE_RECORD_ENTITY_FIELD_FLAGS_HEALTH) {
        entity_object->health.value = entity_record->health;
    }

    if (field_flags_to_read & SAVE_RECORD_ENTITY_FIELD_FLAGS_POSITION) {
        entity_object->position = entity_record->position;
    }

    if (field_flags_to_read & SAVE_RECORD_ENTITY_FIELD_FLAGS_DIRECTION) {
        entity_object->facing_direction = entity_record->direction;
    }

    if (field_flags_to_read & SAVE_RECORD_ENTITY_FIELD_FLAGS_ENTITY_FLAGS) {
        entity_object->flags = entity_record->entity_flags;
    }
}

void try_to_apply_record_entry(struct save_record* record, struct game_state* state) {
    _debugprintf("RECORD TYPE: %.*s", save_record_type_strings[record->type].length, save_record_type_strings[record->type].data);

    switch (record->type) {
        case SAVE_RECORD_TYPE_ENTITY_CHEST: {
            struct save_record_entity_chest* chest_record = &record->chest_record;
            apply_save_record_chest_entry(chest_record, state);
        } break;
        case SAVE_RECORD_TYPE_ENTITY_ENTITY: {
            struct save_record_entity_entity* entity_record = &record->entity_record;
            apply_save_record_entity_entry(entity_record, state);
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

local struct save_record* save_data_existing_chest_record(u32 chest_id) {
    struct save_area_record_entry* area = current_area_save_entry();

    for (struct save_area_record_chunk* record_chunk = area->first; record_chunk; record_chunk = record_chunk->next) {
        for (s32 record_index = 0; record_index < record_chunk->written_entries; ++record_index) {
            struct save_record* current_record = &record_chunk->records[record_index];

            if (current_record->type == SAVE_RECORD_TYPE_ENTITY_CHEST) {
                if (current_record->chest_record.target_entity == chest_id) {
                    return current_record;
                }
            }
        }
    }

    return NULL;
}

void save_data_register_chest_looted(u32 chest_id) {
    struct save_area_record_entry* area       = current_area_save_entry();
    struct save_record*            new_record = save_data_existing_chest_record(chest_id);

    if (!new_record) {
        new_record = save_area_record_entry_allocate_record(area);
    }

    new_record->type                          = SAVE_RECORD_TYPE_ENTITY_CHEST;
    new_record->chest_record.target_entity    = chest_id;
}

local struct save_record* save_data_existing_entity_record(entity_id id) {
    struct save_area_record_entry* area = current_area_save_entry();

    for (struct save_area_record_chunk* record_chunk = area->first; record_chunk; record_chunk = record_chunk->next) {
        for (s32 record_index = 0; record_index < record_chunk->written_entries; ++record_index) {
            struct save_record* current_record = &record_chunk->records[record_index];

            if (current_record->type == SAVE_RECORD_TYPE_ENTITY_ENTITY) {
                if (entity_id_equal(current_record->entity_record.id, id)) {
                    return current_record;
                }
            }
        }
    }

    return NULL;
}

void save_data_register_entity(entity_id id) {
    _debugprintf("register entity save data");
    struct save_area_record_entry* area          = current_area_save_entry();
    struct entity*                 entity_object = game_dereference_entity(game_state, id);

    if (entity_object->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
        return;
    }

    struct save_record* new_record = save_data_existing_entity_record(id);
    if (!new_record) {
        _debugprintf("alloced new save record for entity");
        new_record = save_area_record_entry_allocate_record(area);
    } else {
        _debugprintf("found existing save record for entity");
    }

    new_record->type                                = SAVE_RECORD_TYPE_ENTITY_ENTITY;
    struct save_record_entity_entity* entity_record = &new_record->entity_record;

    entity_record->entity_field_flags = BIT(0) | BIT(1) | BIT(2) | BIT(3);
    entity_record->id                 = id;
    entity_record->entity_flags       = entity_object->flags;
    entity_record->position           = entity_object->position;
    entity_record->health             = entity_object->health.value;
    entity_record->direction          = entity_object->facing_direction;
}
