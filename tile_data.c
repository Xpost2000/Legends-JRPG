static void initialize_static_table_data(void) {
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
            .name                 = string_literal("(grass test(filled))"),
            .image_asset_location = string_literal("./res/img/land/grass.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(brick test(filled))"),
            .image_asset_location = string_literal("./res/img/brick.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(dirt)"),
            .image_asset_location = string_literal("./res/img/land/dirt.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(water (unpassable))"),
            .image_asset_location = string_literal("./res/img/land/water.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(water (passable))"),
            .image_asset_location = string_literal("./res/img/land/water.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(cave wall)"),
            .image_asset_location = string_literal("./res/img/cave/cavewall.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(cave wall opening)"),
            .image_asset_location = string_literal("./res/img/cave/cavewall_opening.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(cave wall mossy"),
            .image_asset_location = string_literal("./res/img/cave/cavewall1.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(cobble floor1)"),
            .image_asset_location = string_literal("./res/img/cave/cobble_floor1.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(bush)"),
            .image_asset_location = string_literal("./res/img/land/bush.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(house block)"),
            .image_asset_location = string_literal("./res/img/building/home_block.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(house block top)"),
            .image_asset_location = string_literal("./res/img/building/home_top_block.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(house block window)"),
            .image_asset_location = string_literal("./res/img/building/home_block_window.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(carpet red fuzz)"),
            .image_asset_location = string_literal("./res/img/building/home_red_fuzz_pit.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(home wood floor)"),
            .image_asset_location = string_literal("./res/img/building/home_wood_floor.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(home wood wall)"),
            .image_asset_location = string_literal("./res/img/building/home_wood_wall.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(home wood wall top)"),
            .image_asset_location = string_literal("./res/img/building/home_wood_wall_top.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(door)"),
            .image_asset_location = string_literal("./res/img/building/door.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(wood_log_side)"),
            .image_asset_location = string_literal("./res/img/cave/wood_log_side.png"),
            .flags                = TILE_DATA_FLAGS_NONE,
        })
    );
    insert(
        ((struct tile_data_definition){
            .name                 = string_literal("(small tree)"),
            .image_asset_location = string_literal("./res/img/land/tree_small.png"),
            .flags                = TILE_DATA_FLAGS_SOLID,
        })
    );
#undef insert 
#undef AT_insert 
#undef current_AT 
    tile_table_data_count = i;
}
