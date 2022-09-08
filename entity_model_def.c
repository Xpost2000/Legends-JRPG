#ifndef ENTITY_MODEL_DEF_C
#define ENTITY_MODEL_DEF_C

/*
  Fixed sizes to help simplify things.
  
  I mean everything will have the same amount of animations,
  and then we'll have a few extra atop that.
*/

#define ENTITY_ANIMATION_MAX_FRAMES (16)
struct entity_animation {
    string   name;
    f32      time_until_next_frame;
    s32      frame_count;
    image_id sprites[ENTITY_ANIMATION_MAX_FRAMES];
};

#define ENTITY_ANIMATION_MAX (64)
struct entity_model {
    string                  name;
    s32                     animation_count;
    struct entity_animation animations[ENTITY_ANIMATION_MAX * 4];
};

struct entity_model_database {
    s32 capacity;
    s32 count;
    struct entity_model* models;
};

static struct entity_model_database global_entity_models = {};

struct entity_model_database entity_model_database_create(struct memory_arena* arena, s32 count);

s32 entity_model_database_add_model(struct memory_arena* arena, string name);

/* animation frames are loaded based on the name ./res/img/(model_name)/(animation_name)_(frame_index) */
s32 entity_model_add_animation(s32 entity_model_id, string name, s32 frames, f32 time_to_next);
struct entity_animation* find_animation_by_name(s32 model_index, string name);

v2f32 entity_animation_get_frame_dimensions(struct entity_animation* anim, s32 frame);

#endif
