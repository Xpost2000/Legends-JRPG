# Legends RPG: Save System Excerpt

The save system is generally evolving (because the game gets more complex), although
this sample is the main crux save system, with other auxiliary functions being implemented on top
for specific versions.

This is the "dirtiest" of the code samples because it's more deeply intwined with the engine's memory allocation
patterns and game objects.

Following the engine's theme of **no dynamic allocation at runtime**, the save system also qualifies as "runtime" and
does not dynamically allocate at all (outside of the initial fixed storage buffer.)

The save system instead works with chunks of save records which are stitched together on save time. On save load time, the
linearized save entries are siphoned back into chunks.

This causes the implementation to look a little "weird" because of the chunking, however the interface makes this a non-issue
(the saving usage code is just ```register_entity_type(pointer to object)``` so there is no leaky abstraction.)

Besides, the allure of "no memory allocation" is very attractive.

## Code
```c
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
    */
    _debugprintf("area entry count: %d", area_entry_count);

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
                        for (s32 record_index = 0; record_index < entry_chunk->written_entries; ++record_index) {
                            save_serialize_record_entry(entry_chunk, record_index, serializer, save_version);
                            written += 1;
                        }
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
```
