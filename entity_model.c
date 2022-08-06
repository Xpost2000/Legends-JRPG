struct entity_model_database entity_model_database_create(struct memory_arena* arena, s32 count) {
    struct entity_model_database result = {};
    result.capacity = count;
    result.models = memory_arena_push(arena, sizeof(*result.models) * count);
    return result;
}

s32 entity_model_database_add_model(struct memory_arena* arena, string name) {
    struct entity_model* result = &global_entity_models.models[global_entity_models.count++];

    result->name = name;

    for (s32 index = 0; index < array_count(result->sprites); ++index) {
        string fullpath = string_concatenate(arena, string_literal("./res/img/"), name);
        fullpath = string_concatenate(arena, fullpath,
                                      string_concatenate(arena, string_concatenate(arena, string_literal("/"), facing_direction_strings_normal[index]),
                                                         string_literal(".png")));
        result->sprites[index] = graphics_assets_load_image(&graphics_assets, fullpath);
    }

    return (s32)(result - global_entity_models.models);
}
