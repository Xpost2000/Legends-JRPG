struct entity_model_database entity_model_database_create(struct memory_arena* arena, s32 count) {
    struct entity_model_database result = {};
    result.capacity = count;
    result.models = memory_arena_push(arena, sizeof(*result.models) * count);
    return result;
}

s32 entity_model_database_add_model(struct memory_arena* arena, string name) {
    struct entity_model* result = &global_entity_models.models[global_entity_models.count++];

    result->name            = name;
    result->animation_count = 0;
    result->width_units     = 1;

    return (s32)(result - global_entity_models.models);
}

s32 entity_model_add_animation(s32 entity_model_id, string name, s32 frames, f32 time_to_next) {
    struct entity_model*     model = global_entity_models.models + entity_model_id;
    struct entity_animation* anim  = &model->animations[model->animation_count++];

    assertion(model->animation_count <= ENTITY_ANIMATION_MAX && "Too many animations for this engine!");

    anim->name                  = name;
    anim->time_until_next_frame = time_to_next;
    anim->frame_count           = frames;

    assertion(frames < ENTITY_ANIMATION_MAX_FRAMES && "Too many frames for this engine!");

    for (s32 index = 0; index < anim->frame_count; ++index) {
        s32 index_plus_one = index + 1;
        string fullpath = string_from_cstring(format_temp("./res/img/%.*s/%.*s_%d.png", model->name.length, model->name.data, name.length, name.data, index_plus_one));
        anim->sprites[index] = graphics_assets_load_image(&graphics_assets, fullpath);
    }

    return (model->animation_count-1);
}

#define FALLBACK_MODEL_ID (0)
struct entity_animation* find_animation_by_name(s32 model_index, string name) {
    struct entity_model* model = &global_entity_models.models[model_index];

    for (s32 index = 0; index < model->animation_count; ++index) {
        if (string_equal(model->animations[index].name, name)) {
            return model->animations + index;
        }
    }

    if (model_index == FALLBACK_MODEL_ID) {
        return NULL;
    }

    /* The engine should always guarantee the base animation "guy" exists. */
    return find_animation_by_name(FALLBACK_MODEL_ID, name);
}

void entity_model_set_width(s32 entity_model_id, f32 width_units) {
    struct entity_model* model = &global_entity_models.models[entity_model_id];
    model->width_units = width_units;
}

v2f32 entity_animation_get_frame_dimensions(struct entity_animation* anim, s32 frame) {
    v2f32 result = {};

    if (frame >= anim->frame_count) {
        frame = 0;
    }

    image_id img = anim->sprites[frame];
    struct image_buffer* img_ptr = graphics_assets_get_image_by_id(&graphics_assets, img);
    result.x = img_ptr->width;
    result.y = img_ptr->height;
    return result;
}

image_id entity_animation_get_sprite_frame(struct entity_animation* anim, s32 index) {
    image_id result = anim->sprites[index];

    if (index >= anim->frame_count) {
        result = anim->sprites[0];
    }

    return result;
}
