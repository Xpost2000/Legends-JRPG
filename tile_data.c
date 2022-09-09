/* I think we'll probably change this a lot more later... But anyways... */
image_id get_tile_image_id(struct tile_data_definition* tile_def) {
    return graphics_assets_get_image_by_filepath(&graphics_assets, tile_def->frames[tile_def->frame_index]);
}

/* memory is read at start up */
/* this is kind of expensive since the entire file is kept in memory just to keep the strings. */
/* I mean it's a small amount of memory relative to the whole thing so it's something to think about. */
/* Anyways this is not really possible to hotreload. Oh well. */
/* NOTE: This is because all gamescript strings are intended to be read-only and this happens to be easy to do. */
struct file_buffer tile_data_source_file = {};

static void initialize_static_table_data(void) {
#if 1
    tile_data_source_file       = read_entire_file(memory_arena_allocator(&game_arena), string_literal("./res/tile_data.txt"));
    struct lisp_list file_forms = lisp_read_string_into_forms(&game_arena, file_buffer_as_string(&tile_data_source_file));

    tile_table_data_count = file_forms.count;
    tile_table_data = memory_arena_push(&game_arena, sizeof(*tile_table_data) * tile_table_data_count);

    /* TODO error checking? */
    for (s32 index = 0; index < file_forms.count; ++index) {
        struct lisp_form*            file_list_form     = file_forms.forms + index;
        struct tile_data_definition* current_tile_entry = tile_table_data + index;

        {
            zero_struct(current_tile_entry);
        }

        {
            struct lisp_form* name_string = lisp_list_nth(file_list_form, 0);

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

#else
    /* a very generous amount of table data... */
    tile_table_data = memory_arena_push(&game_arena, sizeof(*tile_table_data) * 2048);
    auto_tile_info  = memory_arena_push(&game_arena, sizeof(*auto_tile_info)  * 1024);

    s32 i = 0;
    s32 j = 0;

    /* NOTE this table may be subject to change, and a lot of things may explode? */
#define insert(x)    tile_table_data[i++] = (x)
#define AT_insert(x) auto_tile_info[j++] = (x)
#define current_AT   &auto_tile_info[j] 
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("grass"),
            .frames[0] = string_literal("./res/img/land/grass.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("brick"),
            .frames[0] = string_literal("./res/img/brick.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("dirt"),
            .frames[0] = string_literal("./res/img/land/dirt.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                  = string_literal("water (solid)"),
            .frames[0]             = string_literal("./res/img/land/water2.png"),
            .frames[1]             = string_literal("./res/img/land/water2.png"),
            .frame_count           = 2,
            .time_until_next_frame = 0.15,
            .flags                 = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                  = string_literal("(water)"),
            .frames[0]  = string_literal("./res/img/land/water.png"),
            .frames[1]             = string_literal("./res/img/land/water2.png"),
            .frame_count           = 2,
            .time_until_next_frame = 0.15,
            .flags                 = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("cave wall"),
            .frames[0] = string_literal("./res/img/cave/cavewall.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("cave wall opening"),
            .frames[0] = string_literal("./res/img/cave/cavewall_opening.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("cave wall mossy"),
            .frames[0] = string_literal("./res/img/cave/cavewall1.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("cobble floor"),
            .frames[0] = string_literal("./res/img/cave/cobble_floor1.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("bush"),
            .frames[0] = string_literal("./res/img/land/bush.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("house block"),
            .frames[0] = string_literal("./res/img/building/home_block.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("house block top"),
            .frames[0] = string_literal("./res/img/building/home_top_block.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("house block window"),
            .frames[0] = string_literal("./res/img/building/home_block_window.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("carpet red fuzz"),
            .frames[0] = string_literal("./res/img/building/home_red_fuzz_pit.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("home wood floor"),
            .frames[0] = string_literal("./res/img/building/home_wood_floor.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("home wood wall"),
            .frames[0] = string_literal("./res/img/building/home_wood_wall.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("home wood wall top"),
            .frames[0] = string_literal("./res/img/building/home_wood_wall_top.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("door"),
            .frames[0] = string_literal("./res/img/building/door.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("wood log side"),
            .frames[0] = string_literal("./res/img/cave/wood_log_side.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("small tree"),
            .frames[0] = string_literal("./res/img/land/tree_small.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
#undef insert 
#undef AT_insert 
#undef current_AT 
    tile_table_data_count = i;
#endif
}
