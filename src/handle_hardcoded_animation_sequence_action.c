/*
  Hoping to never really have to use this, but this is just an escape hatch
  just incase I need this.
 */
enum hardcoded_animation_id {
    HARDCODED_ANIM_UNKNOWN = -1,
    HARDCODED_ANIM_NONE    = 0,
};

local void handle_hardcoded_animation_sequence_action(s32 id, struct entity* entity) {
    struct entity_ai_data* ai_data          = &entity->ai;
    /* to store state */
    s32*                   animation_params = ai_data->anim_param;

    switch (id) {
        case HARDCODED_ANIM_NONE:
        case HARDCODED_ANIM_UNKNOWN: {
            zero_array(ai_data->anim_param);
            entity_advance_ability_sequence(entity);
        } break;
    }
}
