/* TODO: Equipment animation! */
#define EQUIPMENT_SCREEN_SPIN_TIMER_LENGTH (0.2)

struct {
    entity_id focus_entity;

    s32 direction_index;
    f32 spin_timer;
} equipment_screen_state;

/* render the entity but spinning their directional animations */

local void open_equipment_screen(entity_id target_id) {
    equipment_screen_state.focus_entity = target_id;
}

local void update_and_render_character_equipment_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct entity* target_entity = entity_list_dereference_entity(&state->entities, equipment_screen_state.focus_entity);

    {
        struct entity_animation* anim = find_animation_by_name(target_entity->model_index, facing_direction_strings_normal[equipment_screen_state.direction_index]);
        image_id sprite_to_use = anim->sprites[0];

        software_framebuffer_draw_image_ex(framebuffer,
                                           graphics_assets_get_image_by_id(&graphics_assets, sprite_to_use),
                                           rectangle_f32(100, 100, TILE_UNIT_SIZE, TILE_UNIT_SIZE*2),
                                           RECTANGLE_F32_NULL,
                                           color32f32_WHITE, NO_FLAGS, BLEND_MODE_ALPHA);
    }

    equipment_screen_state.spin_timer += dt;

    if (equipment_screen_state.spin_timer >= EQUIPMENT_SCREEN_SPIN_TIMER_LENGTH) {
        equipment_screen_state.spin_timer       = 0;
        equipment_screen_state.direction_index += 1;

        if (equipment_screen_state.direction_index >= 4) {
            equipment_screen_state.direction_index = 0;
        }
    }
}
