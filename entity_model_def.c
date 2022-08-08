#ifndef ENTITY_MODEL_DEF_C
#define ENTITY_MODEL_DEF_C

struct entity_animation {
    string name;
    f32    time_until_next_frame;
    s32    frame_count;
};

struct entity_model {
    string name;
    image_id sprites[DIRECTION_COUNT];
};

struct entity_model_database {
    s32 capacity;
    s32 count;
    struct entity_model* models;
};

static struct entity_model_database global_entity_models = {};

struct entity_model_database entity_model_database_create(struct memory_arena* arena, s32 count);

s32 entity_model_database_add_model(struct memory_arena* arena, string name);

#endif
