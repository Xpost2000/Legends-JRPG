void initialize_entity_model_database(struct memory_arena* arena) {
    struct file_buffer model_schema_file = read_entire_file(memory_arena_allocator(&game_arena), string_literal(GAME_DEFAULT_ENTITY_MODEL_DEF_FILE));
    struct lisp_list model_schema_list = lisp_read_string_into_forms(&game_arena, file_buffer_as_string(&model_schema_file));

    global_entity_models.capacity = model_schema_list.count;
    global_entity_models.arena    = arena;
    global_entity_models.models   = memory_arena_push(arena, sizeof(*global_entity_models.models) * global_entity_models.capacity);

    for (s32 form_index = 0; form_index < model_schema_list.count; ++form_index) {
        s32 new_model = entity_model_database_alloc_model();

        struct lisp_form* current_model_form = model_schema_list.forms + form_index;
        {
            for (s32 form_index = 0; form_index < current_model_form->list.count; ++form_index) {
                struct lisp_form* subform        = lisp_list_nth(current_model_form, form_index);
                struct lisp_form* subform_header = lisp_list_nth(subform, 0);

                if (lisp_form_symbol_matching(*subform_header, string_literal("animation"))) {
                    struct lisp_form* name_form           = lisp_list_nth(subform, 1);
                    struct lisp_form* frames_form         = lisp_list_nth(subform, 2);
                    struct lisp_form* time_per_frame_form = lisp_list_nth(subform, 3);

                    string name = {};
                    s32 frames = 0;
                    f32 time_to_next = 0;

                    lisp_form_get_string(*name_form, &name);
                    lisp_form_get_s32(*frames_form, &frames);
                    lisp_form_get_f32(*time_per_frame_form, &time_to_next);

                    entity_model_add_animation(
                        new_model,
                        name,
                        frames,
                        time_to_next
                    );
                } else if (lisp_form_symbol_matching(*subform_header, string_literal("name"))) {
                    struct lisp_form* name_form = lisp_list_nth(subform, 1);
                    string name = {};
                    lisp_form_get_string(*name_form, &name);
                    entity_model_database_set_model_name(new_model, name);
                }
            }
        }
    }
}

s32 entity_model_database_alloc_model(void) {
    struct entity_model* result = &global_entity_models.models[global_entity_models.count++];
    result->animation_count     = 0;
    result->width_units         = 1;
    return (s32)(result - global_entity_models.models);
}

s32 entity_model_database_add_model(string name) {
    struct entity_model* result = &global_entity_models.models[global_entity_models.count++];

    result->name            = string_clone(global_entity_models.arena, name);
    result->animation_count = 0;
    result->width_units     = 1;

    return (s32)(result - global_entity_models.models);
}

void entity_model_database_set_model_name(s32 entity_id, string name) {
    struct entity_model* model = global_entity_models.models + entity_id;
    model->name = string_clone(global_entity_models.arena, name);
}

s32 entity_model_add_animation(s32 entity_model_id, string name, s32 frames, f32 time_to_next) {
    struct entity_model*     model = global_entity_models.models + entity_model_id;
    struct entity_animation* anim  = &model->animations[model->animation_count++];

    assertion(model->animation_count <= ENTITY_ANIMATION_MAX && "Too many animations for this engine!");

    anim->name                  = string_clone(global_entity_models.arena, name);
    anim->time_until_next_frame = time_to_next;
    anim->frame_count           = frames;

    assertion(frames < ENTITY_ANIMATION_MAX_FRAMES && "Too many frames for this engine!");

    for (s32 index = 0; index < anim->frame_count; ++index) {
        s32 index_plus_one = index + 1;
        string fullpath = format_temp_s(GAME_DEFAULT_RESOURCE_PATH "img/%.*s/%.*s_%d.png", model->name.length, model->name.data, name.length, name.data, index_plus_one);
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

f32 entity_model_get_width_units(s32 entity_model_id) {
    struct entity_model* model = &global_entity_models.models[entity_model_id];
    return model->width_units;
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
