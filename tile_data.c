/* I think we'll probably change this a lot more later... But anyways... */
image_id get_tile_image_id(struct tile_data_definition* tile_def) {
    string frame_path       = tile_def->frames[tile_def->frame_index];
    /*
      NOTE: Almost everything expects Cstrings and these strings originate from the file data which we
      don't copy. But just have pointers into...
    */
    /* UGGHHHHH FUCK EMSCRIPTEN FOR DOING THIS! */
    /* string sanitized_string = string_from_cstring(format_temp("%.*s", frame_path.length, frame_path.data)); */
    string sanitized_string = string_clone(&scratch_arena, frame_path);
    return graphics_assets_get_image_by_filepath(&graphics_assets, sanitized_string);
}

/* memory is read at start up */
/* this is kind of expensive since the entire file is kept in memory just to keep the strings. */
/* I mean it's a small amount of memory relative to the whole thing so it's something to think about. */
/* Anyways this is not really possible to hotreload. Oh well. */
/* NOTE: This is because all gamescript strings are intended to be read-only and this happens to be easy to do. */
struct file_buffer tile_data_source_file = {};

/*
  NOTE: For future projects should experiment with different allocation patterns.

  IE: In debug mode, I should always use dynamic allocators for everything just so I can just hotswap things or hotreload stuff whenever possible.
  In release mode replace everything with arenas, (IE I have a lot of debug only paths... Right now I'm coding everything in a nearly release mode style
  path which is fine and all but this is kind of annoying to do sometimes...)
*/
static void initialize_static_table_data(void) {
    tile_data_source_file       = read_entire_file(memory_arena_allocator(&game_arena), string_literal("./res/tile_data.txt"));
    /* NOTE: it would be cleaner to copy all strings and own them directly here. It's not too much of a change but the lisp form reader makes certain assumptions. */
    struct lisp_list file_forms = lisp_read_string_into_forms(&game_arena, file_buffer_as_string(&tile_data_source_file));

    tile_table_data_count = file_forms.count;
    tile_table_data = memory_arena_push(&game_arena, sizeof(*tile_table_data) * tile_table_data_count);

    /* TODO error checking? */
    for (s32 index = 0; index < file_forms.count; ++index) {
        struct lisp_form*            file_list_form     = file_forms.forms + index;
        struct tile_data_definition* current_tile_entry = tile_table_data + index;

        {
            zero_struct(*current_tile_entry);
        }

        {
            struct lisp_form* name_string = lisp_list_nth(file_list_form, 0);
            lisp_form_get_string(*name_string, &current_tile_entry->name);

            _debugprintf("NAME STRING: %.*s", name_string->string.length, name_string->string.data);

            for (s32 remaining_form_index = 1; remaining_form_index < file_list_form->list.count; ++remaining_form_index) {
                struct lisp_form* current_subform = lisp_list_nth(file_list_form, remaining_form_index);

                {
                    struct lisp_form* form_name      = lisp_list_nth(current_subform, 0);
                    struct lisp_form  form_arguments = lisp_list_sliced(*current_subform, 1, -1);

                    if (lisp_form_symbol_matching(*form_name, string_literal("frames"))) {
                        s32 frame_count                 = form_arguments.list.count;
                        current_tile_entry->frame_count = frame_count;

                        for (s32 frame_index = 0; frame_index < frame_count; ++frame_index) {
                            struct lisp_form* current_frame_path = lisp_list_nth(&form_arguments, frame_index);
                            _debugprintf("FRAME PATH STRING: %.*s", current_frame_path->string.length, current_frame_path->string.data);
                            lisp_form_get_string(*current_frame_path, &current_tile_entry->frames[frame_index]);
                        }
                    } else if (lisp_form_symbol_matching(*form_name, string_literal("flags"))) {
                        s32 flags = TILE_DATA_FLAGS_NONE;

                        for (s32 flag_index = 0; flag_index < form_arguments.list.count; ++flag_index) {
                            struct lisp_form* f = lisp_list_nth(&form_arguments, flag_index);

                            if (lisp_form_symbol_matching(*f, string_literal("solid"))) {
                                flags |= TILE_DATA_FLAGS_SOLID;
                            }
                        }

                        current_tile_entry->flags = flags;
                    } else if (lisp_form_symbol_matching(*form_name, string_literal("time-until-next-frame"))) {
                        struct lisp_form* arg0 = lisp_list_nth(&form_arguments, 0);
                        lisp_form_get_f32(*arg0, &current_tile_entry->time_until_next_frame);
                    }
                }
            }
        }
    }
}
