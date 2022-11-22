#ifndef ENTITY_MODEL_DEF_C
#define ENTITY_MODEL_DEF_C

/*
  Fixed sizes to help simplify things.
  
  I mean everything will have the same amount of animations,
  and then we'll have a few extra atop that.
*/

#define ENTITY_ANIMATION_MAX_FRAMES (8)
struct entity_animation {
    string   name;
    f32      time_until_next_frame;
    s32      frame_count;
    image_id sprites[ENTITY_ANIMATION_MAX_FRAMES];
};

#define ENTITY_ANIMATION_MAX (16)
struct entity_model {
    string                  name;
    s32                     animation_count;
    /*
      NOTE:
      We only collide based on the "foot" of our model.
      So we only need to care about width units.

      This width unit is in measures of the tile unit size. (divide by 16, which is the base measure.)
      (I know the engine has TILE_UNIT_SIZE == 64, that's a bit of a mess that I may never really correct
      since it doesn't really change anything. It's just semantic.)
     */
    f32                     width_units;
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

/* these apis are very inconsistent.... Oh well... */
s32  entity_model_add_animation(s32 entity_model_id, string name, s32 frames, f32 time_to_next);
void entity_model_set_width(s32 entity_model_id, f32 width_units);
f32  entity_model_get_width_units(s32 entity_model_id);
struct entity_animation* find_animation_by_name(s32 model_index, string name);

v2f32 entity_animation_get_frame_dimensions(struct entity_animation* anim, s32 frame);

/* returns frame 0 if the frame doesn't exist. */
image_id entity_animation_get_sprite_frame(struct entity_animation* anim, s32 index);

#endif
