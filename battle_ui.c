/*
  NOTE: This should probably all stop depending on the player as the main target since that's
  not good if we want multiple party members lol.

  TODO: Finish new after action report

  Would like the hurt command but that's okay if I can't do it.

  NOTE: last target is incorrect for certain directions, which makes sense. they should be sorted by distance,
  but chances are I know what order they'll be added so I can just reverse them in select cases.

  TODO: Round Start / Round End transition is broken and I didn't notice until now, but it's a really minor bug so I'll fix it later.
*/

enum battle_ui_animation_phase {
    BATTLE_UI_FADE_IN_DARK,
    BATTLE_UI_FADE_IN_DARK_END_TURN,

    BATTLE_UI_FLASH_IN_BATTLE_TEXT,
    BATTLE_UI_FADE_OUT,
    BATTLE_UI_MOVE_IN_DETAILS,
    BATTLE_UI_FLASH_IN_END_TURN_TEXT,
    BATTLE_UI_FADE_OUT_END_TURN_TEXT,
    BATTLE_UI_IDLE,

    BATTLE_UI_FADE_OUT_DETAILS_AFTER_BATTLE_COMPLETION,
    BATTLE_UI_FADE_OUT_DETAILS_AFTER_TURN_COMPLETION,
    /* 
       I would add more animation phases but I don't have infinite time or patience right now,
       I've mostly done nothing but UI code, and while some of it is kind of cool, it's also a bit
       soul retching sometimes.
       
       I need to do some crappy script writing to have a nice time I guess.
       
       TODO add remaining animated phases.
    */
    BATTLE_UI_AFTER_ACTION_REPORT_IDLE,
    /* BATTLE_UI_AFTER_ACTION_BYE, */
};

enum battle_ui_submodes {
    BATTLE_UI_SUBMODE_NONE,
    BATTLE_UI_SUBMODE_LOOKING,
    BATTLE_UI_SUBMODE_MOVING,
    BATTLE_UI_SUBMODE_ATTACKING,
    BATTLE_UI_SUBMODE_USING_ABILITY,
    BATTLE_UI_SUBMODE_EQUIPMENT_MODE,
    BATTLE_UI_SUBMODE_USING_ITEM,
    /* the rest are not submodes, and are just registered actions. */
};

/* This is a pretty generous number. */
#define MAX_KILLED_ENTITY_TRACKER           (MAX_SELECTED_ENTITIES_FOR_ABILITIES)
#define MAX_LOOT_ITEMS                      (MAX_SELECTED_ENTITIES_FOR_ABILITIES*16)

#define MAX_SELECTABLE_ITEMS (MAX_PARTY_ITEMS)
struct battle_ui_item_state {
    /* Item code */
    s32 selection;
    s32 selectable_item_count;
    s32 selectable_items[MAX_SELECTABLE_ITEMS];
};

struct battle_ui_action_message {
    string message;
    f32    timer;
};

struct battle_ui_state {
    f32 timer;
    u32 phase;

    s32 selection;

    struct battle_ui_action_message message;

    /* Can be animated? */
    s32 submode;

    bool remembered_original_camera_position;
    v2f32 prelooking_mode_camera_position;

    /* for ability's focus camera */
    /* or when watching enemy entities do stuff */
    bool stalk_entity_with_camera;
    struct entity* entity_to_stalk; /* pointer is stable */

    s32 movement_start_x;
    s32 movement_start_y;
    s32 movement_end_x;
    s32 movement_end_y;

    /* wasteful? Maybe. Easy? Yes. */
    s32   max_remembered_path_points_count;
    v2f32 max_remembered_path_points[256];

    bool selecting_ability_target;
    /* I need to copy this to allow myself to rotate the grid. */
    u8   selection_field[ENTITY_ABILITY_SELECTION_FIELD_MAX_Y][ENTITY_ABILITY_SELECTION_FIELD_MAX_X];

    /* NOTE: This coordinate means different things based on the mode of selection*/
    /*
      if it's FIELD it's an absolute world coordinate
      if it's a FIELD SHAPE it's relative to the grid itself.
     */
    s32 ability_target_x;
    s32 ability_target_y;

    entity_id currently_selected_entity_id;

    /* This should really be a part of the entities themselves, since entities will soon be able to also execute abilities */
    entity_id  selected_entities_for_abilities[MAX_SELECTED_ENTITIES_FOR_ABILITIES];
    s32        selected_entities_for_abilities_count;

    s32        usable_abilities[ENTITY_MAX_ABILITIES];
    s32        usable_ability_count;

    /* read their loot tables when combat finishes, and distribute */
    /* also I really mean by the player and their friends. */
    entity_id  killed_entities[MAX_KILLED_ENTITY_TRACKER];
    s32        killed_entity_count;

    bool                 populated_loot_table;
    struct item_instance loot_results[MAX_LOOT_ITEMS];
    s32                  loot_result_count;

    struct battle_ui_item_state item_use;

    entity_id targetable_entities[512];
    s32       targetable_entity_count;

    /*
      With multiple party members, I do want this to involve some tactics thinking ish,
      and I don't really want players to waste a turn placing units in obligatory decent positions.

      That is unless an encounter is explicitly supposed to be an "ambush", but if in most battles
      I'm sure you can get into a not totally disadvantageous position to start off.

      It just allows you to put your units in a formation,
      (Mainly it circumvents the weird problem of entity snapping)
    */
    bool      formations_placed;
} global_battle_ui_state;

local void announce_battle_action(struct entity_id who, string what) {
    global_battle_ui_state.message.message = what;
    global_battle_ui_state.message.timer   = 1.2f;
}

local void battle_ui_calculate_usable_abilities(void) {
    struct game_state_combat_state* combat_state            = &game_state->combat_state;
    struct entity*                  active_combatant_entity = game_dereference_entity(game_state, combat_state->participants[combat_state->active_combatant]);
    s32 wrote_count = entity_get_usable_ability_indices(active_combatant_entity, array_count(global_battle_ui_state.usable_abilities), global_battle_ui_state.usable_abilities);
    global_battle_ui_state.usable_ability_count = wrote_count;
}

local void setup_item_use_menu(void) {
    struct battle_ui_item_state* item_use = &global_battle_ui_state.item_use;
    item_use->selection = 0;

    struct entity_inventory* inventory = (struct entity_inventory*)(&game_state->inventory);

    {
        for (s32 item_index = 0; item_index < inventory->count; ++item_index) {
            struct item_instance* current_item_instance = inventory->items + item_index;
            struct item_def*      item_base             = item_database_find_by_id(current_item_instance->item);
            
            bool allow_item = false;

            {
                if (item_base->type == ITEM_TYPE_CONSUMABLE_ITEM) {
                    allow_item = true;
                }
            }

            if (allow_item) {
                item_use->selectable_items[item_use->selectable_item_count++] = item_index;
            }
        }
    }
    _debugprintf("Maximum usable items: %d", item_use->selectable_item_count);
}

void battle_ui_stalk_entity_with_camera(struct entity* to_stalk) {
    global_battle_ui_state.stalk_entity_with_camera = true; 
    global_battle_ui_state.entity_to_stalk          = to_stalk;

    if (!global_battle_ui_state.remembered_original_camera_position) {
        global_battle_ui_state.remembered_original_camera_position = true;
        global_battle_ui_state.prelooking_mode_camera_position     = game_state->camera.xy;
    } 
}
void battle_ui_stop_stalk_entity_with_camera(void) {
    if (global_battle_ui_state.stalk_entity_with_camera) {
        global_battle_ui_state.stalk_entity_with_camera = false;
        global_battle_ui_state.entity_to_stalk          = NULL;
        camera_set_point_to_interpolate(&game_state->camera, global_battle_ui_state.prelooking_mode_camera_position);
    }
}

void battle_clear_loot_results(void) {
    global_battle_ui_state.populated_loot_table = false;
    zero_array(global_battle_ui_state.loot_results);
    global_battle_ui_state.loot_result_count = 0;
}
void battle_clear_all_killed_entities(void) {
    zero_array(global_battle_ui_state.killed_entities);
    global_battle_ui_state.killed_entity_count = 0;
}

void battle_notify_killed_entity(entity_id killed) {
    assertion(global_battle_ui_state.killed_entity_count < MAX_KILLED_ENTITY_TRACKER);
    global_battle_ui_state.killed_entities[global_battle_ui_state.killed_entity_count++] = killed;
}

/* calculate modifiers maybe in the future? */
local s32 total_xp_gained_from_enemies(void) {
    s32 total = 0;

    for (s32 killed_entity_index = 0; killed_entity_index < global_battle_ui_state.killed_entity_count; ++killed_entity_index) {
        struct entity* killed_entity = game_dereference_entity(game_state, global_battle_ui_state.killed_entities[killed_entity_index]);
        total += killed_entity->stat_block.xp_value;
    }

    return total;
}

local void populate_post_battle_loot_table(void) {
    if (!global_battle_ui_state.populated_loot_table) {
        for (s32 killed_entity_index = 0; killed_entity_index < global_battle_ui_state.killed_entity_count; ++killed_entity_index) {
            struct entity* killed_entity = game_dereference_entity(game_state, global_battle_ui_state.killed_entities[killed_entity_index]);

            s32 loot_table_temporary_count = 0;

            struct entity_loot_table* loot_table = entity_lookup_loot_table(&game_state->entity_database, killed_entity);
            struct item_instance* loot_table_temporary = entity_loot_table_find_loot(&scratch_arena, loot_table, &loot_table_temporary_count);

            {
                for (s32 loot_table_temporary_index = 0; loot_table_temporary_index < loot_table_temporary_count; ++loot_table_temporary_index) {
                    assertion(global_battle_ui_state.loot_result_count <= MAX_LOOT_ITEMS && "Too much loot to store!");
                    item_id loot_table_current_item  = loot_table_temporary[loot_table_temporary_index].item;
                    s32     loot_table_current_count = loot_table_temporary[loot_table_temporary_index].count;

                    bool already_exists = false;

                    for (s32 loot_index = 0; loot_index < global_battle_ui_state.loot_result_count; ++loot_index) {
                        if (item_id_equal(global_battle_ui_state.loot_results[loot_index].item, loot_table_current_item)) {
                            global_battle_ui_state.loot_results[loot_index].count += loot_table_current_count;
                            already_exists = true;
                            break;
                        }
                    }

                    if (!already_exists) {
                        global_battle_ui_state.loot_results[global_battle_ui_state.loot_result_count++] = loot_table_temporary[loot_table_temporary_index];
                    }
                }
            }
        }

        global_battle_ui_state.populated_loot_table = true;
    }
}

enum battle_options{
    BATTLE_LOOK,
    BATTLE_MOVE,
    BATTLE_ATTACK,
    BATTLE_ABILITY,
    BATTLE_ITEM,
    BATTLE_DEFEND,
    BATTLE_EQUIPMENT,
    BATTLE_THROW_OR_PICKUP,
    BATTLE_WAIT,
};
local string battle_menu_main_options[] = {
    [BATTLE_LOOK]            = string_literal("LOOK"),
    [BATTLE_MOVE]            = string_literal("MOVE"),
    [BATTLE_ATTACK]          = string_literal("ATTACK"),
    [BATTLE_ABILITY]         = string_literal("ABILITY"),
    [BATTLE_ITEM]            = string_literal("ITEM"),
    [BATTLE_DEFEND]          = string_literal("DEFEND"),
    [BATTLE_EQUIPMENT]            = string_literal("EQUIPMENT"),
    [BATTLE_THROW_OR_PICKUP] = string_literal("PICKUP/THROW"),
    [BATTLE_WAIT]            = string_literal("END TURN"),
};
local string battle_menu_main_option_descriptions[] = {
    [BATTLE_LOOK]            = string_literal("scout the battle field."),
    [BATTLE_MOVE]            = string_literal("pick a space to move to."),
    [BATTLE_ATTACK]          = string_literal("use your weapons' basic attack."),
    [BATTLE_ABILITY]         = string_literal("use an ability from your list."),
    [BATTLE_ITEM]            = string_literal("use an item from your party inventory."),
    [BATTLE_DEFEND]          = string_literal("increase defense, and delay future turns."),
    [BATTLE_EQUIPMENT]       = string_literal("Change equipment to increase your advantage."),
    [BATTLE_THROW_OR_PICKUP] = string_literal("Pickup an object and toss it around for tactical advantage."),
    [BATTLE_WAIT]            = string_literal("finish turn"),
};
        /*
         * 
         5 * 16
         8 * 20
         */
        /*                                                                             tw  th  ar ac */

local void start_combat_ui(void) {
    zero_memory(&global_battle_ui_state, sizeof(global_battle_ui_state));
    battle_clear_all_killed_entities();
    battle_clear_loot_results();
}

/* TODO does not account for per entity objects! */
local bool is_player_combat_turn(struct game_state* state) {
    struct game_state_combat_state* combat_state            = &state->combat_state;
    struct entity*                  active_combatant_entity = game_dereference_entity(state, combat_state->participants[combat_state->active_combatant]);

    if (game_entity_is_party_member(active_combatant_entity)) {
        if (active_combatant_entity->ai.current_action == 0)
            return true;

        return false;
    }

    return false;
}

local void draw_turn_panel(struct game_state* state, struct software_framebuffer* framebuffer, f32 x, f32 y) {
    struct game_state_combat_state* combat_state = &state->combat_state;

    struct entity* active_combatant_entity = game_dereference_entity(state, combat_state->participants[combat_state->active_combatant]);
    bool player_turn = is_player_combat_turn(state);

    struct entity_database* entities_database        = &game_state->entity_database;
    for (s32 index = combat_state->active_combatant; index < combat_state->count; ++index) {
        union color32u8 color = color32u8(128, 128, 128, 255);

        struct entity* entity = game_dereference_entity(state, combat_state->participants[index]);

        struct entity_base_data* data                    = entity_database_find_by_index(entities_database, entity->base_id_index);
        string                   facing_direction_string = facing_direction_strings_normal[0];
        struct entity_animation* anim                    = find_animation_by_name(data->model_index, format_temp_s("idle_%.*s", facing_direction_string.length, facing_direction_string.data));
        image_id sprite_to_use = anim->sprites[0];
        {
            if (entity->flags & ENTITY_FLAGS_PLAYER_CONTROLLED) {
                color = color32u8(0, 255, 0, 255);
            } else {
                if (entity->ai.flags & ENTITY_AI_FLAGS_AGGRESSIVE_TO_PLAYER) {
                    color = color32u8(255, 0, 0, 255);
                }
            }
        }

        if (!player_turn) {
            color.r >>= 1;
            color.g >>= 1;
            color.b >>= 1;
        }

        const s32 square_size = 32;
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(x, y + (index - combat_state->active_combatant) * square_size * 1.3, square_size, square_size), color, BLEND_MODE_ALPHA);
        software_framebuffer_draw_image_ex(framebuffer,
                                           graphics_assets_get_image_by_id(&graphics_assets, sprite_to_use),
                                           rectangle_f32(x, y + (index - combat_state->active_combatant) * square_size * 1.3, square_size, square_size),
                                           rectangle_f32(0, 0, 16, 16), /* This should be a little more generic but whatever for now */
                                           color32f32(1,1,1,1), NO_FLAGS, BLEND_MODE_ALPHA);

    }
}

local void cancel_ability_selections(void) {
    {
        _debugprintf("%d selected entities", global_battle_ui_state.selected_entities_for_abilities_count);
        global_battle_ui_state.selected_entities_for_abilities_count = 0;
        zero_array(global_battle_ui_state.selected_entities_for_abilities);
    }
}

local v2s32 get_grid_position_for_ability(struct entity_ability* ability) {
    struct game_state_combat_state* combat_state            = &game_state->combat_state;
    struct entity*                  active_combatant_entity = game_dereference_entity(game_state, combat_state->participants[combat_state->active_combatant]);
    struct entity*                  user                    = active_combatant_entity;
    s32                             grid_x                  = 0;
    s32                             grid_y                  = 0;

    if (ability->selection_type == ABILITY_SELECTION_TYPE_FIELD_SHAPE) {
        grid_x = user->position.x / TILE_UNIT_SIZE;
        grid_y = user->position.y / TILE_UNIT_SIZE;
    } else if (ability->selection_type == ABILITY_SELECTION_TYPE_FIELD) {
        grid_x = global_battle_ui_state.ability_target_x;
        grid_y = global_battle_ui_state.ability_target_y;
    }

    /* Coordinate centering. */
    grid_x -= ENTITY_ABILITY_SELECTION_FIELD_CENTER_X;
    grid_y -= ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y;
    return v2s32(grid_x,grid_y);
}

local void filter_ability_selection_field(struct entity_ability* ability, u8* selection_field, struct game_state* state) {
    struct game_state_combat_state*                                          combat_state                                                                                  = &game_state->combat_state;
    struct entity*                                                           active_combatant_entity                                                                       = game_dereference_entity(game_state, combat_state->participants[combat_state->active_combatant]);
    struct entity*                                                           user                                                                                          = active_combatant_entity;
    struct level_area*                                                       area                                                                                          = &state->loaded_area;
    v2s32                                                                    grid_xy                                                                                       = get_grid_position_for_ability(ability);
    s32                                                                      grid_x                                                                                        = grid_xy.x;
    s32                                                                      grid_y                                                                                        = grid_xy.y;
    bool                                                                     selectable_blocks[ENTITY_ABILITY_SELECTION_FIELD_MAX_Y][ENTITY_ABILITY_SELECTION_FIELD_MAX_X] = {};
    bool                                                                     searched_blocks[ENTITY_ABILITY_SELECTION_FIELD_MAX_Y][ENTITY_ABILITY_SELECTION_FIELD_MAX_X]   = {};
    v2s32 selection_block_search_list[ENTITY_ABILITY_SELECTION_FIELD_MAX_Y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X]                                                         = {};
    s32                                                                      selection_block_search_list_size                                                              = 0;

    struct entity* does_not_count_as_obstacle[] = {
        user,
    };
    s32 does_not_count_as_obstacle_count = 1;

    /* NOTE: for now only allows the user as not an obstacle */
    if (!ability->requires_no_obstructions) {
        for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
            for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
                if (selection_field[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x]) {
                    selectable_blocks[y][x] = true;
                }
            }
        }
    } else {
        bool is_at_user_foot = false; /* if an ability is not at the user foot, we'll have a simpler selection field method, which is just copying the original selection field. */
        {
            s32 x = ENTITY_ABILITY_SELECTION_FIELD_CENTER_X;
            s32 y = ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y;

            if (!level_area_any_obstructions_at(area, grid_x+x,grid_y+y)) {
                selection_block_search_list[selection_block_search_list_size++] = v2s32(x, y);
                searched_blocks[y][x]                                           = true;
                is_at_user_foot                                                 = true;
            }
        }

        if (is_at_user_foot) {
            while (selection_block_search_list_size) {
                v2s32 current_position                                     = selection_block_search_list[selection_block_search_list_size-1];
                selection_block_search_list_size                          -= 1;
                searched_blocks[current_position.y][current_position.x]    = true;
                selectable_blocks[current_position.y][current_position.x]  = true;
                _debugprintf("current position: %d, %d", current_position.x, current_position.y);

                {
                    local v2s32 neighbor_offsets[] = {
                        v2s32(-1, 0),
                        v2s32(1, 0),
                        v2s32(0, -1),
                        v2s32(0, 1),
                    };

                    for (s32 neighbor_offset_index = 0; neighbor_offset_index < array_count(neighbor_offsets); ++neighbor_offset_index) {
                        {
                            v2s32 neighbor_offset = neighbor_offsets[neighbor_offset_index];
                            s32   x               = neighbor_offset.x + current_position.x;
                            s32   y               = neighbor_offset.y + current_position.y;
                            bool  do_not_search   = false;

                            _debugprintf("Considering neighbor: %d, %d", neighbor_offset.x, neighbor_offset.y);

                            if (!do_not_search &&
                                (x >= ENTITY_ABILITY_SELECTION_FIELD_MAX_X || x < 0 ||
                                 y >= ENTITY_ABILITY_SELECTION_FIELD_MAX_Y || y < 0)) {
                                do_not_search = true;
                                _debugprintf("Do not search : %d, %d", x, y);
                            }

                            _debugprintf("Ability field at %d, %d? : %d", x, y, ability->selection_field[y][x]);
                            if (!do_not_search && selection_field[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x] == 0) {
                                do_not_search = true;
                                _debugprintf("Field is empty. Do not search : %d, %d", x, y);
                            }

                            if (do_not_search) {
                                continue;
                            }

                            if (!searched_blocks[y][x]) {
                                if (!level_area_any_obstructions_at(area, grid_x+x,grid_y+y)) {
                                    searched_blocks[y][x] = true;
                                    selection_block_search_list[selection_block_search_list_size++] = v2s32(x, y);
                                    _debugprintf("adding %d, %d to search", x, y);
                                } else {
                                    _debugprintf("Do not search : %d, %d (hit something?)", x, y);
                                }
                            }
                        }
                    }
                }
            }
        } else {
            for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
                for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
                    if (selection_field[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x]) {
                        selectable_blocks[y][x] = true;
                    }
                }
            }
        }
    }

    _debugprintf("copy results");
    for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
        for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
            global_battle_ui_state.selection_field[y][x] = selectable_blocks[y][x];
        }
    }
}

/* TODO: this doesn't quite work right now! Need to sync this to what you can see! */
local void recalculate_targeted_entities_by_ability(struct entity_ability* ability, u8* selection_field, struct game_state* state) {
    struct game_state_combat_state* combat_state            = &game_state->combat_state;
    struct entity*                  active_combatant_entity = game_dereference_entity(game_state, combat_state->participants[combat_state->active_combatant]);
    struct level_area*              area                    = &state->loaded_area;
    struct entity*                  user                    = active_combatant_entity;
    const f32                       SMALL_ENOUGH_EPISILON   = 0.03;

    cancel_ability_selections();
    filter_ability_selection_field(ability, selection_field, state);
    struct entity_iterator entities = game_entity_iterator(state);

    bool encountered_any_collision  = false;
    if (ability->selection_type == ABILITY_SELECTION_TYPE_FIELD && !ability->moving_field) {
        for (struct entity* potential_target = entity_iterator_begin(&entities); !entity_iterator_finished(&entities); potential_target = entity_iterator_advance(&entities)) {
            if (!(potential_target->flags & ENTITY_FLAGS_ACTIVE)) {
                continue;
            }

            if (potential_target == user) {
                continue;
            }

            if (!(potential_target->flags & ENTITY_FLAGS_ALIVE)) {
                continue;
            }

            /* grid interesection */
            bool should_add_to_targets_list = false;

            {
                _debugprintf("Okay... Looking at %p", potential_target);

                for (s32 selection_grid_y = 0; selection_grid_y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y && !should_add_to_targets_list; ++selection_grid_y) {
                    for (s32 selection_grid_x = 0; selection_grid_x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X && !should_add_to_targets_list; ++selection_grid_x) {
                        if (global_battle_ui_state.selection_field[selection_grid_y][selection_grid_x]) {
                            f32 real_x = (global_battle_ui_state.ability_target_x + selection_grid_x - ENTITY_ABILITY_SELECTION_FIELD_CENTER_X) * TILE_UNIT_SIZE;
                            f32 real_y = (global_battle_ui_state.ability_target_y + selection_grid_y - ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y) * TILE_UNIT_SIZE;

                            /* I'm very certain I guarantee that entities should be "grid-locked", so I can just check. */
                            f32 delta_x = fabs(potential_target->position.x - real_x);
                            f32 delta_y = fabs(potential_target->position.y - real_y);

                            if (!encountered_any_collision && level_area_any_obstructions_at(area, floorf(real_x/TILE_UNIT_SIZE), floorf(real_y/TILE_UNIT_SIZE))) {
                                encountered_any_collision = true;
                            }

                            if (delta_x <= SMALL_ENOUGH_EPISILON && delta_y <= SMALL_ENOUGH_EPISILON) {
                                should_add_to_targets_list = true;
                            }
                        }
                    }
                }
            }

            if (should_add_to_targets_list) {
                global_battle_ui_state.selected_entities_for_abilities[global_battle_ui_state.selected_entities_for_abilities_count++] = entities.current_id;
            }

            /* sort them by distance to the active player, insertion sort again */
            {
                v2f32 attacker_position = user->position;

                for (s32 first_index = 1; first_index < global_battle_ui_state.selected_entities_for_abilities_count; ++first_index) {
                    s32 insertion_index = 0;
                    entity_id key_entity = global_battle_ui_state.selected_entities_for_abilities[first_index];
                    v2f32 key_position = game_dereference_entity(game_state, key_entity)->position;

                    f32   key_distance_sq = v2f32_distance_sq(key_position, attacker_position);
                    for (insertion_index = first_index; insertion_index > 0; --insertion_index) {
                        entity_id key_entity2 = global_battle_ui_state.selected_entities_for_abilities[insertion_index-1];
                        v2f32 key_position2 = game_dereference_entity(game_state, key_entity2)->position;
                        f32 distance_sq = v2f32_distance_sq(attacker_position, key_position2);

                        if (distance_sq < key_distance_sq) {
                            break;
                        } else {
                            global_battle_ui_state.selected_entities_for_abilities[insertion_index] = global_battle_ui_state.selected_entities_for_abilities[insertion_index-1];
                        }
                    }

                    global_battle_ui_state.selected_entities_for_abilities[insertion_index] = key_entity;
                }
            }
        }
    } else {
        for (struct entity* potential_target = entity_iterator_begin(&entities); !entity_iterator_finished(&entities); potential_target = entity_iterator_advance(&entities)) {
            if (!(potential_target->flags & ENTITY_FLAGS_ACTIVE)) {
                continue;
            }

            if (potential_target == user) {
                continue;
            }

            if (!(potential_target->flags & ENTITY_FLAGS_ALIVE)) {
                continue;
            }
            
            bool should_add_to_targets_list = false;

            {
                f32 real_x = ((s32)(user->position.x/TILE_UNIT_SIZE) + global_battle_ui_state.ability_target_x - ENTITY_ABILITY_SELECTION_FIELD_CENTER_X) * TILE_UNIT_SIZE;
                f32 real_y = ((s32)(user->position.y/TILE_UNIT_SIZE) + global_battle_ui_state.ability_target_y - ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y) * TILE_UNIT_SIZE;

                f32 delta_x = fabs(potential_target->position.x - real_x);
                f32 delta_y = fabs(potential_target->position.y - real_y);

                if (!encountered_any_collision && level_area_any_obstructions_at(area, floorf(real_x/TILE_UNIT_SIZE), floorf(real_y/TILE_UNIT_SIZE))) {
                    encountered_any_collision = true;
                }

                if (delta_x <= SMALL_ENOUGH_EPISILON && delta_y <= SMALL_ENOUGH_EPISILON) {
                    should_add_to_targets_list = true;
                }
            }

            if (should_add_to_targets_list) {
                global_battle_ui_state.selected_entities_for_abilities[global_battle_ui_state.selected_entities_for_abilities_count++] = entities.current_id;
            }
        }
    }

    if (encountered_any_collision) {
        if (ability->requires_no_obstructions) {
            cancel_ability_selections();
        }
    }
}

local void battle_ui_determine_disabled_actions(entity_id id, bool* disabled_actions) {
    struct game_state_combat_state* combat_state = &game_state->combat_state;
    struct entity*                  entity       = game_dereference_entity(game_state, id); 

    /* disable selecting attack if we don't have anyone within attack range, also cache it. */
    {
        f32 attack_radius = DEFAULT_ENTITY_ATTACK_RADIUS;
        struct entity_query_list nearby_potential_targets = find_entities_within_radius(&scratch_arena, game_state, entity->position, attack_radius * TILE_UNIT_SIZE, true);
        global_battle_ui_state.targetable_entity_count = 0;
        for (s32 index = 0; index < nearby_potential_targets.count; ++index) {
            struct entity* potential_target = game_dereference_entity(game_state, nearby_potential_targets.ids[index]);

            if (!(potential_target->flags & ENTITY_FLAGS_PLAYER_CONTROLLED)) {
                if ((potential_target->flags & ENTITY_FLAGS_ALIVE)) {
                    global_battle_ui_state.targetable_entities[global_battle_ui_state.targetable_entity_count++] = nearby_potential_targets.ids[index];
                } else {
                }
            } else {
            }
        }

        if (global_battle_ui_state.targetable_entity_count <= 0)  {
            disabled_actions[BATTLE_ATTACK] = true;
        }
    }
    {
        /* when the user is ability locked... TODO */
        s32 usable_ability_count = entity_usable_ability_count(entity);
        _debugprintf("usable ability count: %d", usable_ability_count);
        {
            if (usable_ability_count > 0) {
                disabled_actions[BATTLE_ABILITY] = false;
            } else  {
                disabled_actions[BATTLE_ABILITY] = true;   
            }
        }
    }
    {
        if (entity_already_used(entity, LAST_USED_ENTITY_ACTION_MOVEMENT)) {
            disabled_actions[BATTLE_MOVE]   = true;
        }

        if (entity_already_used(entity, LAST_USED_ENTITY_ACTION_DEFEND)) {
            disabled_actions[BATTLE_DEFEND]          = true;
            disabled_actions[BATTLE_ITEM]            = true;
            disabled_actions[BATTLE_THROW_OR_PICKUP] = true;
            disabled_actions[BATTLE_MOVE]            = true;
            disabled_actions[BATTLE_ABILITY]         = true;
            disabled_actions[BATTLE_ATTACK]          = true;
        }

        if (entity_already_used(entity, LAST_USED_ENTITY_ACTION_ITEM_USAGE)) {
            disabled_actions[BATTLE_ITEM]   = true;
        }
    }
}

local void do_battle_selection_menu(struct game_state* state, struct software_framebuffer* framebuffer, f32 x, f32 y, bool allow_input, f32 dt) {
    struct game_state_combat_state* combat_state            = &state->combat_state;
    struct entity*                  active_combatant_entity = game_dereference_entity(state, combat_state->participants[combat_state->active_combatant]);

    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    if (disable_game_input) allow_input = false;

    bool selection_down    = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
    bool selection_up      = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);
    bool selection_left    = is_action_down_with_repeat(INPUT_ACTION_MOVE_LEFT);
    bool selection_right   = is_action_down_with_repeat(INPUT_ACTION_MOVE_RIGHT);
    bool selection_confirm = is_action_pressed(INPUT_ACTION_CONFIRMATION);
    bool selection_cancel  = is_action_pressed(INPUT_ACTION_CANCEL);

    /* TODO: weirdo */
    if (!allow_input || game_command_console_enabled) {
        selection_down = selection_up = selection_left = selection_right =  selection_confirm = selection_cancel = false;
    }

    if (selection_cancel) {
        if (global_battle_ui_state.submode != BATTLE_UI_SUBMODE_NONE) {
            if (global_battle_ui_state.submode == BATTLE_UI_SUBMODE_EQUIPMENT_MODE) {
                /* the menu will handle that itself. */
            } else if (global_battle_ui_state.selecting_ability_target) {
                global_battle_ui_state.selecting_ability_target = false;
                cancel_ability_selections();
            } else {
            battle_menu_default_cleanup:
                global_battle_ui_state.submode                             = BATTLE_UI_SUBMODE_NONE;

                if (global_battle_ui_state.remembered_original_camera_position) {
                    global_battle_ui_state.remembered_original_camera_position = false;
                    state->camera.xy                                           = global_battle_ui_state.prelooking_mode_camera_position;
                }

                global_battle_ui_state.max_remembered_path_points_count = 0;
                level_area_clear_movement_visibility_map(&state->loaded_area);
                _debugprintf("restore to previous menu state");
            }
        } else {
            if (entity_action_stack_any(active_combatant_entity)) {
                entity_undo_last_used_battle_action(active_combatant_entity);
            }
        }
    }

    /* These all need to be animated and polished much more in the future. */
    switch (global_battle_ui_state.submode) {
        case BATTLE_UI_SUBMODE_NONE: { /* I want to also show this main menu during specific UIs but whatever. */
            union color32f32 modulation_color = color32f32_WHITE;
            union color32f32 ui_color         = UI_BATTLE_COLOR;

            if (!allow_input) {
                modulation_color = color32f32(0.5, 0.5, 0.5, 0.5);   
                ui_color.a = 0.5;
            }

            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(x, y), 8, 18, ui_color);

            bool disabled_actions[array_count(battle_menu_main_options)] = {};
            battle_ui_determine_disabled_actions(combat_state->participants[combat_state->active_combatant], disabled_actions);

            for (unsigned index = 0; index < array_count(battle_menu_main_options); ++index) {
                struct font_cache* painting_font = normal_font;

                if (index == global_battle_ui_state.selection) {
                    painting_font = highlighted_font;
                }

                if (allow_input) {
                    if (disabled_actions[index]) {
                        modulation_color = color32f32(0.5, 0.5, 0.5, 1.0);
                    } else {
                        modulation_color = color32f32_WHITE;
                    }
                }

                draw_ui_breathing_text(framebuffer, v2f32(x + 14, y + 15 + index * 32), painting_font,
                                       2, battle_menu_main_options[index], index, modulation_color);
            }

            if (allow_input) {
                s32 options_count = array_count(battle_menu_main_options);

                if (selection_down) {
                    play_sound(ui_blip);
                    do {
                        global_battle_ui_state.selection++;

                        if (global_battle_ui_state.selection >= options_count) {
                            global_battle_ui_state.selection = 0;
                        }
                    } while (disabled_actions[global_battle_ui_state.selection]);
                }
                else if (selection_up) {
                    play_sound(ui_blip);
                    do {
                        global_battle_ui_state.selection--;

                        if (global_battle_ui_state.selection < 0) {
                            global_battle_ui_state.selection = options_count-1;
                        }
                    } while (disabled_actions[global_battle_ui_state.selection]);
                }

                if (selection_confirm) {
                    play_sound(ui_blip);
                    if (!disabled_actions[global_battle_ui_state.selection]) {
                        switch (global_battle_ui_state.selection) {
                            /* NOTE: No ability is expected to really reach outside of your view... Hence the need for a separate view command */
                            case BATTLE_LOOK: {
                                global_battle_ui_state.submode = BATTLE_UI_SUBMODE_LOOKING;
                            } break;
                            case BATTLE_MOVE: {
                                global_battle_ui_state.submode = BATTLE_UI_SUBMODE_MOVING;
                            
                                struct entity* active_combatant_entity = entity_list_dereference_entity(&state->permenant_entities, combat_state->participants[combat_state->active_combatant]);
                                level_area_build_movement_visibility_map(&scratch_arena, &state->loaded_area, active_combatant_entity->position.x / TILE_UNIT_SIZE, active_combatant_entity->position.y / TILE_UNIT_SIZE, 6);

                                global_battle_ui_state.movement_start_x                 = floorf((active_combatant_entity->position.x + active_combatant_entity->scale.x/2) / TILE_UNIT_SIZE);
                                global_battle_ui_state.movement_start_y                 = floorf((active_combatant_entity->position.y + active_combatant_entity->scale.y/2) / TILE_UNIT_SIZE);
                                global_battle_ui_state.movement_end_x                   = global_battle_ui_state.movement_start_x;
                                global_battle_ui_state.movement_end_y                   = global_battle_ui_state.movement_start_y;
                                global_battle_ui_state.max_remembered_path_points_count = 0;
                            } break;
                            case BATTLE_ATTACK: {
                                global_battle_ui_state.submode = BATTLE_UI_SUBMODE_ATTACKING;
                            } break;
                            case BATTLE_ABILITY: {
                                global_battle_ui_state.submode = BATTLE_UI_SUBMODE_USING_ABILITY;
                                battle_ui_calculate_usable_abilities();
                            } break;
                            case BATTLE_ITEM: {
                                global_battle_ui_state.submode = BATTLE_UI_SUBMODE_USING_ITEM;
                                setup_item_use_menu();
                            } break;
                            case BATTLE_DEFEND: {
                                entity_combat_submit_defend_action(active_combatant_entity);
                            } break;
                            case BATTLE_EQUIPMENT: {
                                global_battle_ui_state.submode = BATTLE_UI_SUBMODE_EQUIPMENT_MODE;
                                open_equipment_screen(combat_state->participants[combat_state->active_combatant]);
                            } break;
                            case BATTLE_WAIT: {
                                struct entity* active_combatant_entity   = entity_list_dereference_entity(&state->permenant_entities, combat_state->participants[combat_state->active_combatant]);
                                active_combatant_entity->waiting_on_turn = false;
                            } break;
                        }

                        global_battle_ui_state.selection = 0;
                    }
                }
            }
        } break;

            /* This is a simple menu for now, I may want to expand on this a little but for now this is it. */
            /* Attacking will just instantly apply some damage, we don't care about animating right now. That can be like next week. */
            
            /* TODO make attacking highlight the target obviously! Or focus on the target would work too. */
        case BATTLE_UI_SUBMODE_ATTACKING: {
            f32 attack_radius = DEFAULT_ENTITY_ATTACK_RADIUS;
            /* I mean, there's no battle this large... Ever */
            s32 target_list_count                      = 0;
            entity_id target_display_list_indices[512] = {};

            for (s32 index = 0; index < global_battle_ui_state.targetable_entity_count; ++index) {
                entity_id current_id = global_battle_ui_state.targetable_entities[index];
                struct entity* current_entity = game_dereference_entity(state, current_id);

                if (game_entity_is_party_member(current_entity)) {
                    continue;
                }

                if (!(current_entity->flags & ENTITY_FLAGS_ALIVE)) {
                    continue;
                }

                target_display_list_indices[target_list_count++] = current_id;
            }

            s32 BOX_WIDTH  = 8;
            /* should be dynamically sized but okay. */
            s32 BOX_HEIGHT = 12;

            v2f32 ui_box_size     = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);
            v2f32 ui_box_position = v2f32(framebuffer->width*0.9-ui_box_size.x, 50);
            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, ui_box_position, BOX_WIDTH, BOX_HEIGHT, UI_BATTLE_COLOR);

            for (s32 index = 0; index < target_list_count; ++index) {
                struct entity* target_entity = game_dereference_entity(state, target_display_list_indices[index]);

                string entity_name = target_entity->name;
                v2f32 draw_point = ui_box_position;
                draw_point.x    += 8;
                draw_point.y    += 15 + index * 16 * 1.3;

                struct font_cache* painted_font = normal_font;

                if (global_battle_ui_state.selection == index) {
                    painted_font = highlighted_font;
                    target_entity->under_selection = false;
                }

                draw_ui_breathing_text(framebuffer, draw_point, painted_font, 2, entity_name, 0, color32f32_WHITE);
            }

            if (selection_up) {
                play_sound(ui_blip);
                global_battle_ui_state.selection--;
                if (global_battle_ui_state.selection < 0) {
                    global_battle_ui_state.selection = target_list_count-1;
                }
            } else if (selection_down) {
                play_sound(ui_blip);
                global_battle_ui_state.selection++;
                if (global_battle_ui_state.selection >= target_list_count) {
                    global_battle_ui_state.selection = 0;
                }
            }

            entity_id enemy_id = target_display_list_indices[global_battle_ui_state.selection];

            if (!entity_id_equal(enemy_id, global_battle_ui_state.currently_selected_entity_id)) {
                global_battle_ui_state.currently_selected_entity_id = enemy_id;

                {
                    struct camera* camera = &state->camera;
                    struct entity* target_entity = game_dereference_entity(state, enemy_id);

                    camera_set_point_to_interpolate(camera, target_entity->position);
                    target_entity->under_selection = true;
                }
            }

            /* ATTACK ENEMY! */
            if (selection_confirm) {
                global_battle_ui_state.submode = BATTLE_UI_SUBMODE_NONE;
                entity_combat_submit_attack_action(active_combatant_entity, enemy_id);
            }
        } break;

        case BATTLE_UI_SUBMODE_USING_ITEM: {
            struct entity*               user     = active_combatant_entity;
            struct battle_ui_item_state* item_use = &global_battle_ui_state.item_use;

            s32 BOX_WIDTH  = nine_patch_estimate_fitting_extents_width(ui_chunky, 1,
                                                                       font_cache_text_width(game_get_font(MENU_FONT_COLOR_GOLD), string_literal("(x 999) "), 2)
                                                                       + item_database_longest_string_name(game_get_font(MENU_FONT_COLOR_GOLD), 2));
            s32 BOX_HEIGHT = roundf(5 * 1.6);

            v2f32 ui_box_size     = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);
            v2f32 ui_box_position = v2f32(framebuffer->width*0.94-ui_box_size.x, 20);
            draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, ui_box_position, BOX_WIDTH, BOX_HEIGHT, UI_BATTLE_COLOR);

            if (selection_up) {
                play_sound(ui_blip);
                item_use->selection--;
                if (item_use->selection < 0) {
                    item_use->selection = item_use->selectable_item_count-1;
                }
            } else if (selection_down) {
                play_sound(ui_blip);
                item_use->selection++;
                if (item_use->selection >= item_use->selectable_item_count) {
                    item_use->selection = 0;
                }
            }

            s32 bottom_index;
            s32 top_index   ;
            set_scrollable_ui_bounds(item_use->selection, &bottom_index, &top_index, item_use->selectable_item_count, 3, 5);

            f32 x_cursor = ui_box_position.x + 15;
            f32 y_cursor = ui_box_position.y + 15;

            struct font_cache*       painting_font = normal_font;
            struct entity_inventory* inventory     = (struct entity_inventory*)(&game_state->inventory);

            for (s32 display_item_index = bottom_index; display_item_index < top_index; ++display_item_index) {
                struct item_instance* current_item_instance = inventory->items + item_use->selectable_items[display_item_index];
                struct item_def*      item_base             = item_database_find_by_id(current_item_instance->item);

                if (display_item_index == item_use->selection) {
                    painting_font = highlighted_font;
                } else {
                    painting_font = normal_font;
                }

                string tmp = format_temp_s("(x %d) %.*s", current_item_instance->count, item_base->name.length, item_base->name.data);
                draw_ui_breathing_text(framebuffer, v2f32(x_cursor, y_cursor), painting_font, 2, tmp, display_item_index, color32f32_WHITE);
                y_cursor += 16*1.2;
            }

            if (selection_confirm) {
                /* MOVE TO USAGE PHASE */
                global_battle_ui_state.submode = BATTLE_UI_SUBMODE_NONE;
                entity_combat_submit_item_use_action(user, item_use->selectable_items[item_use->selection], true);
            }
        } break;

        case BATTLE_UI_SUBMODE_EQUIPMENT_MODE: {
            s32 equipment_menu_result = do_equipment_menu(framebuffer, dt);

            switch (equipment_menu_result) {
                case EQUIPMENT_MENU_PROCESS_ID_EXIT: {
                    global_battle_ui_state.submode = BATTLE_UI_SUBMODE_NONE;
                } break;
                case EQUIPMENT_MENU_PROCESS_ID_OKAY:
                default: {
                    /* control is back in the hands of the equipment UI */
                } break;
            }
        } break;

        case BATTLE_UI_SUBMODE_MOVING: {
            struct level_area* area = &state->loaded_area;

            s32 proposed_y = global_battle_ui_state.movement_end_y;
            s32 proposed_x = global_battle_ui_state.movement_end_x;

            bool should_find_new_path = false;

            {
                if (selection_up) {
                    proposed_y--;
                } else if (selection_down) {
                    proposed_y++;
                }

                if (area->combat_movement_visibility_map[(proposed_y - area->navigation_data.min_y) * area->navigation_data.width + (proposed_x - area->navigation_data.min_x)]) {
                    if (proposed_y != global_battle_ui_state.movement_end_y) {
                        _debugprintf("okay, new y is fine");
                        should_find_new_path = true;
                    }

                    global_battle_ui_state.movement_end_y = proposed_y;
                }
            }

            {
                if (selection_left) {
                    proposed_x--;
                } else if (selection_right) {
                    proposed_x++;
                }

                if (area->combat_movement_visibility_map[(proposed_y - area->navigation_data.min_y) * area->navigation_data.width + (proposed_x - area->navigation_data.min_x)]) {
                    if (proposed_x != global_battle_ui_state.movement_end_x) {
                        _debugprintf("okay, new x is fine");
                        should_find_new_path = true;
                    }

                    global_battle_ui_state.movement_end_x = proposed_x;
                }
            }

            if (should_find_new_path) {
                struct navigation_path new_path = navigation_path_find(&scratch_arena, area,
                                                                       v2f32(global_battle_ui_state.movement_start_x, global_battle_ui_state.movement_start_y),
                                                                       v2f32(global_battle_ui_state.movement_end_x, global_battle_ui_state.movement_end_y));
                s32 minimum_count = 0;

                if (new_path.count < array_count(global_battle_ui_state.max_remembered_path_points)) {
                    minimum_count = new_path.count;
                } else {
                    minimum_count = array_count(global_battle_ui_state.max_remembered_path_points);
                }

                global_battle_ui_state.max_remembered_path_points_count = minimum_count;

                for (s32 index = 0; index < minimum_count; ++index) {
                    global_battle_ui_state.max_remembered_path_points[index] = new_path.points[index];
                }
            }

            /* would like to do action points but not right now. */

            if (selection_confirm) {
                /* submit movement */
                if (global_battle_ui_state.max_remembered_path_points_count != 0) {
                    global_battle_ui_state.submode = BATTLE_UI_SUBMODE_NONE;

                    if (global_battle_ui_state.movement_start_x != global_battle_ui_state.movement_end_x ||
                        global_battle_ui_state.movement_start_y != global_battle_ui_state.movement_end_y) {
                        play_sound(ui_blip);
                        entity_combat_submit_movement_action(active_combatant_entity,
                                                             global_battle_ui_state.max_remembered_path_points,
                                                             global_battle_ui_state.max_remembered_path_points_count);

                        global_battle_ui_state.max_remembered_path_points_count = 0;
                        level_area_clear_movement_visibility_map(&state->loaded_area);
                        /* register camera lerp */
                        /* NOTE: the camera is in a weird intermediary position
                           during this action, however when waiting for the path to finish,
                           there is no issue. Right now we're doing instant teleport. Either way I should wait for the camera to finish...
                        */
                        {
                            struct camera* camera = &state->camera;
                            camera_set_point_to_interpolate(camera, v2f32(global_battle_ui_state.movement_end_x * TILE_UNIT_SIZE, global_battle_ui_state.movement_end_y * TILE_UNIT_SIZE));
                        }
                    }
                } else {
                    play_sound(ui_blip_bad);
                }
            }
            
        } break;

        case BATTLE_UI_SUBMODE_USING_ABILITY: {
            struct entity* user = active_combatant_entity;
            s32 BOX_WIDTH  = 8;
            s32 BOX_HEIGHT = 1 + 2 * user->ability_count;

            if (!global_battle_ui_state.selecting_ability_target) {
                v2f32 ui_box_size     = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);
                v2f32 ui_box_position = v2f32(framebuffer->width*0.9-ui_box_size.x, 50);
                draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, ui_box_position, BOX_WIDTH, BOX_HEIGHT, UI_BATTLE_COLOR);

                if (selection_up) {
                    play_sound(ui_blip);
                    global_battle_ui_state.selection--;
                    if (global_battle_ui_state.selection < 0) {
                        global_battle_ui_state.selection = global_battle_ui_state.usable_ability_count-1;
                    }
                } else if (selection_down) {
                    play_sound(ui_blip);
                    global_battle_ui_state.selection++;
                    if (global_battle_ui_state.selection >= global_battle_ui_state.usable_ability_count) {
                        global_battle_ui_state.selection = 0;
                    }
                }

                if (selection_confirm) {
                    global_battle_ui_state.selecting_ability_target = true;

                    struct entity_ability_slot slot    = user->abilities[global_battle_ui_state.usable_abilities[global_battle_ui_state.selection]];
                    struct entity_ability*     ability = entity_database_ability_find_by_index(&game_state->entity_database, slot.ability);

                    if (ability->selection_type == ABILITY_SELECTION_TYPE_FIELD_SHAPE) {
                        global_battle_ui_state.ability_target_x = ENTITY_ABILITY_SELECTION_FIELD_CENTER_X;
                        global_battle_ui_state.ability_target_y = ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y;
                    } else {
                        global_battle_ui_state.ability_target_x = user->position.x/TILE_UNIT_SIZE;
                        global_battle_ui_state.ability_target_y = user->position.y/TILE_UNIT_SIZE;
                    }

                    {
                        copy_selection_field_rotated_as(ability, (u8*)global_battle_ui_state.selection_field, facing_direction_to_quadrant(user->facing_direction));
                        recalculate_targeted_entities_by_ability(ability, (u8*)global_battle_ui_state.selection_field, state);
                    }
                }

                f32 y_cursor = ui_box_position.y + 10;
                for (s32 ability_index = 0; ability_index < global_battle_ui_state.usable_ability_count; ++ability_index) {
                    struct font_cache* painting_font = normal_font;
                    struct entity_ability_slot slot = user->abilities[global_battle_ui_state.usable_abilities[ability_index]];
                    /* should keep track of ability count. */
                    struct entity_ability*     ability = entity_database_ability_find_by_index(&game_state->entity_database, slot.ability);

                    if (ability_index == global_battle_ui_state.selection) {
                        painting_font = highlighted_font;
                    }

                    draw_ui_breathing_text(framebuffer, v2f32(ui_box_position.x + 10, y_cursor), painting_font,
                                           2, ability->name, ability_index, color32f32_WHITE);

                    y_cursor += 32;
                }
                y_cursor = (ui_box_position.y + ui_box_size.y) + 20;
                {
                    /* NOTE: don't ask how any of the UI was calculated. It was all funged and happened to look good within a few tries. */
                    s32 BOX_SQUARE_SIZE = 8+1;
                    v2f32 ui_box_size = nine_patch_estimate_extents(ui_chunky, 1, BOX_SQUARE_SIZE, BOX_SQUARE_SIZE);
                    v2f32 ui_box_position = v2f32(framebuffer->width * 0.9 - ui_box_size.x, y_cursor);
                    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, ui_box_position, BOX_SQUARE_SIZE+1, BOX_SQUARE_SIZE+1, UI_BATTLE_COLOR);

                    /* draw target region */
                    f32 square_size            = ui_box_size.x / ENTITY_ABILITY_SELECTION_FIELD_MAX_X;
                    f32 nearest_perfect_square = (square_size + 2);

                    struct entity_ability_slot slot = user->abilities[global_battle_ui_state.usable_abilities[global_battle_ui_state.selection]];
                    struct entity_ability*     ability = entity_database_ability_find_by_index(&game_state->entity_database, slot.ability);

                    for (s32 y_index = 0; y_index < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y_index) {
                        for (s32 x_index = 0; x_index < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x_index) {
                            f32 x_cursor = ui_box_position.x + 8 + x_index * nearest_perfect_square;
                            f32 y_cursor = ui_box_position.y + 8 + y_index * nearest_perfect_square;;

                            union color32u8 grid_color = color32u8(0,0,0,255);

                            {
                                if (ability->selection_field[y_index][x_index]) {
                                    if (ability->selection_type == ABILITY_SELECTION_TYPE_FIELD) {
                                        grid_color = color32u8(132, 140, 207, 255);
                                    } else {
                                        grid_color = color32u8(225, 30, 30, 255);
                                    }

                                    if (y_index == ENTITY_ABILITY_SELECTION_FIELD_CENTER_Y && x_index == ENTITY_ABILITY_SELECTION_FIELD_CENTER_X)
                                        grid_color = color32u8(30, 225, 30, 255);
                                }
                            }

                            software_framebuffer_draw_quad(framebuffer, rectangle_f32(x_cursor, y_cursor, square_size, square_size), grid_color, BLEND_MODE_ALPHA);
                        }
                    }
                }
            } else {
                bool recalculate_selection_field   = false;
                bool recalculate_targetted_entities = false;

                struct entity_ability_slot slot = user->abilities[global_battle_ui_state.usable_abilities[global_battle_ui_state.selection]];
                struct entity_ability*     ability = entity_database_ability_find_by_index(&game_state->entity_database, slot.ability);

                if (ability->selection_type == ABILITY_SELECTION_TYPE_FIELD && !ability->moving_field) {
                    if (selection_up) {
                        user->facing_direction = DIRECTION_UP;
                        recalculate_selection_field = true;
                    } else if (selection_down) {
                        user->facing_direction = DIRECTION_DOWN;
                        recalculate_selection_field = true;
                    } else if (selection_right) {
                        user->facing_direction = DIRECTION_RIGHT;
                        recalculate_selection_field = true;
                    } else if (selection_left) {
                        user->facing_direction = DIRECTION_LEFT;
                        recalculate_selection_field = true;
                    }

                    recalculate_targetted_entities = recalculate_selection_field;
                } else {
                    if (selection_up) {
                        global_battle_ui_state.ability_target_y -= 1;
                        recalculate_targetted_entities = true;
                    } else if (selection_down) {
                        global_battle_ui_state.ability_target_y += 1;
                        recalculate_targetted_entities = true;
                    } else if (selection_right) {
                        global_battle_ui_state.ability_target_x += 1;
                        recalculate_targetted_entities = true;
                    } else if (selection_left) {
                        global_battle_ui_state.ability_target_x -= 1;
                        recalculate_targetted_entities = true;
                    }

                    if (global_battle_ui_state.ability_target_x < 0) {
                        global_battle_ui_state.ability_target_x = 0;
                    } else if (global_battle_ui_state.ability_target_x >= ENTITY_ABILITY_SELECTION_FIELD_MAX_X) {
                        global_battle_ui_state.ability_target_x = ENTITY_ABILITY_SELECTION_FIELD_MAX_X-1;
                    }

                    if (global_battle_ui_state.ability_target_y < 0) {
                        global_battle_ui_state.ability_target_y = 0;
                    } else if (global_battle_ui_state.ability_target_y >= ENTITY_ABILITY_SELECTION_FIELD_MAX_Y) {
                        global_battle_ui_state.ability_target_y = ENTITY_ABILITY_SELECTION_FIELD_MAX_Y-1;
                    }
                }

                if (recalculate_selection_field) {
                    copy_selection_field_rotated_as(ability, (u8*)global_battle_ui_state.selection_field, facing_direction_to_quadrant(user->facing_direction));
                }

                if (recalculate_targetted_entities) {
                    recalculate_targeted_entities_by_ability(ability, (u8*)global_battle_ui_state.selection_field, state);
                }

                if (selection_confirm) {
                    if (global_battle_ui_state.selected_entities_for_abilities_count > 0) {
                        global_battle_ui_state.submode = BATTLE_UI_SUBMODE_NONE;
                        entity_combat_submit_ability_action(active_combatant_entity, global_battle_ui_state.selected_entities_for_abilities, global_battle_ui_state.selected_entities_for_abilities_count, global_battle_ui_state.usable_abilities[global_battle_ui_state.selection]);
                        global_battle_ui_state.selecting_ability_target = false;
                        cancel_ability_selections();
                    }
                }
            }
        } break;

        case BATTLE_UI_SUBMODE_LOOKING: {
            /* This could be done more cleanly with a better input layer where I can change the layer of control but this is okay too... */
            {
                if (!global_battle_ui_state.remembered_original_camera_position) {
                    global_battle_ui_state.remembered_original_camera_position = true;
                    global_battle_ui_state.prelooking_mode_camera_position     = state->camera.xy;
                } 

                /* mode should allow some basic analysis of opponents. */
                /* NOTE: (I want this to darken everything except for the selected entities...) */
                /* this is going to cause lots of hackery */
                f32 cursor_square_size = 4;
                software_framebuffer_draw_quad(framebuffer, rectangle_f32(framebuffer->width/2-cursor_square_size/2, framebuffer->height/2-cursor_square_size/2, cursor_square_size, cursor_square_size), color32u8_WHITE, BLEND_MODE_NONE);
            }
        } break;
    }
}

local void award_loot_table_to_party(void) {
    assertion(global_battle_ui_state.populated_loot_table && "Should be impossible to fail");

    struct entity_inventory* inventory = (struct entity_inventory*)(&game_state->inventory);

    for (s32 loot_table_index = 0; loot_table_index < global_battle_ui_state.loot_result_count; ++loot_table_index) {
        struct item_instance* loot_item = global_battle_ui_state.loot_results + loot_table_index;
        entity_inventory_add_multiple(inventory, MAX_PARTY_ITEMS, loot_item->item, loot_item->count);
    }
}

local void do_after_action_report_screen(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt, f32 y, bool allow_input) {
    if (disable_game_input) allow_input = false;

    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);
    struct font_cache* header_font      = game_get_font(MENU_FONT_COLOR_STEEL);
    struct font_cache* highlighted_font = game_get_font(MENU_FONT_COLOR_GOLD);

    bool selection_down    = is_action_down_with_repeat(INPUT_ACTION_MOVE_DOWN);
    bool selection_up      = is_action_down_with_repeat(INPUT_ACTION_MOVE_UP);
    bool selection_confirm = is_action_pressed(INPUT_ACTION_CONFIRMATION);
    bool selection_cancel  = is_action_pressed(INPUT_ACTION_CANCEL);

    /* TODO: weirdo */
    if (!allow_input || game_command_console_enabled) {
        selection_down = selection_up = selection_confirm = selection_cancel = false;
    }

    union color32f32 modulation_color = color32f32_WHITE;
    union color32f32 ui_color         = UI_BATTLE_COLOR;

    if (!allow_input) {
        modulation_color = color32f32(0.5, 0.5, 0.5, 0.5);   
        ui_color.a = 0.5;
    }

    s32 BOX_WIDTH  = 20;
    s32 BOX_HEIGHT = 9 + global_battle_ui_state.loot_result_count*2;

    /* DISPLAY XP Gain */ 
    /* and party member stuff */
    v2f32 ui_box_size     = nine_patch_estimate_extents(ui_chunky, 1, BOX_WIDTH, BOX_HEIGHT);
    v2f32 ui_box_position = v2f32(framebuffer->width/2-ui_box_size.x/2, y);

    /* TODO */
    draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, ui_box_position, BOX_WIDTH, BOX_HEIGHT, UI_BATTLE_COLOR);
    draw_ui_breathing_text(framebuffer, v2f32(ui_box_position.x+15, ui_box_position.y+15), highlighted_font, 3, string_literal("Battle Complete!"), 0, modulation_color);
    draw_ui_breathing_text(framebuffer, v2f32(ui_box_position.x+15, ui_box_position.y+55), header_font, 3, string_literal("Loot Gained: "), 0, modulation_color);

    f32 y_cursor = ui_box_position.y + 90;
    f32 text_height = font_cache_text_height(normal_font) * 2;

    {
        for (s32 looted_item_index = 0; looted_item_index < global_battle_ui_state.loot_result_count; ++looted_item_index) {
            struct item_instance* current_loot_entry = global_battle_ui_state.loot_results + looted_item_index;
            struct item_def*      item_base          = item_database_find_by_id(current_loot_entry->item);

            string temp_string = format_temp_s("found %.*s (x%d)", item_base->name.length, item_base->name.data, current_loot_entry->count);
            software_framebuffer_draw_text(framebuffer, normal_font, 2, v2f32(ui_box_position.x+15, y_cursor), temp_string, modulation_color, BLEND_MODE_ALPHA);
            y_cursor += text_height*1.1; 
        }
        y_cursor += text_height;
    }

    draw_ui_breathing_text(framebuffer, v2f32(ui_box_position.x+15, y_cursor), header_font, 3, string_literal("XP Gained"), 0, modulation_color);
    y_cursor += text_height*1.6;

    {
        s32 total_xp_count = total_xp_gained_from_enemies();
        string temp_string = format_temp_s("%d", total_xp_count);
        /* award amongst party members if we had more than one */
        {
            struct entity* user = game_get_player(state);
            entity_award_experience(user, total_xp_count);
        }
        software_framebuffer_draw_text(framebuffer, normal_font, 2, v2f32(ui_box_position.x+15, y_cursor), temp_string, modulation_color, BLEND_MODE_ALPHA);
    }

    if (selection_confirm) {
        global_battle_ui_state.phase = 0;

        state->combat_state.active_combat = false;

        struct entity* player = game_get_player(state);
        award_loot_table_to_party();
        camera_set_point_to_interpolate(&state->camera, player->position);
    }
}

local void update_game_camera_combat(struct game_state* state, f32 dt) {
    const f32 CAMERA_VELOCITY = 260;

    if (global_battle_ui_state.stalk_entity_with_camera) {
        struct entity* to_stalk = global_battle_ui_state.entity_to_stalk;
        /* camera_set_point_to_interpolate(&game_state->camera, to_stalk->position); */

        {
            v2f32 direction_to_entity = v2f32_direction(game_state->camera.xy, to_stalk->position);
            f32   distance            = v2f32_distance(game_state->camera.xy, to_stalk->position);

            /* f32 effective_velocity = CAMERA_VELOCITY / (1 - (distance*distance)); */
            f32 effective_velocity = distance*distance;

            if (effective_velocity > CAMERA_VELOCITY) {
                effective_velocity = CAMERA_VELOCITY;
            }

            if (distance < 5) {
                game_state->camera.xy.x = to_stalk->position.x;
                game_state->camera.xy.y = to_stalk->position.y;
            } else {
                game_state->camera.xy.x += effective_velocity * direction_to_entity.x * dt;
                game_state->camera.xy.y += effective_velocity * direction_to_entity.y * dt;
            }
        }
    } else {
        switch (global_battle_ui_state.submode) {
            case BATTLE_UI_SUBMODE_LOOKING: {
                if (is_action_down(INPUT_ACTION_MOVE_UP)) {
                    state->camera.xy.y -= CAMERA_VELOCITY * dt;
                } else if (is_action_down(INPUT_ACTION_MOVE_DOWN)) {
                    state->camera.xy.y += CAMERA_VELOCITY * dt;
                }

                if (is_action_down(INPUT_ACTION_MOVE_LEFT)) {
                    state->camera.xy.x -= CAMERA_VELOCITY * dt;
                } else if (is_action_down(INPUT_ACTION_MOVE_RIGHT)) {
                    state->camera.xy.x += CAMERA_VELOCITY * dt;
                }
            } break;
            default: {
            } break;
        }
    }
}

local string analyze_entity_and_display_tooltip(struct memory_arena* arena, v2f32 cursor_at, struct game_state* state) {
    struct entity* target_entity_to_analyze = 0;

    struct rectangle_f32 cursor_rect = rectangle_f32(cursor_at.x, cursor_at.y, 8, 8);

    struct entity_iterator it = game_entity_iterator(state);
    for (struct entity* current_entity = entity_iterator_begin(&it); !entity_iterator_finished(&it) && !target_entity_to_analyze; current_entity = entity_iterator_advance(&it)) {
        if (current_entity->flags & ENTITY_FLAGS_ACTIVE) {
            struct rectangle_f32 entity_rectangle = entity_rectangle_collision_bounds(current_entity);

            if (rectangle_f32_intersect(entity_rectangle, cursor_rect)) {
                target_entity_to_analyze = current_entity;
            }
        }
    }

    if (!target_entity_to_analyze) {
        return string_null;
    }

    if (!(target_entity_to_analyze->flags & ENTITY_FLAGS_ALIVE)) {
        char* r = format_temp("%.*s : is dead.", target_entity_to_analyze->name.length, target_entity_to_analyze->name.data);
        return string_from_cstring(r);
    }
    else {
        char* r = format_temp("%.*s : HP : %d/%d, %d/%d/%d/%d/%d/%d",
                              target_entity_to_analyze->name.length, target_entity_to_analyze->name.data,
                              target_entity_to_analyze->health.value, target_entity_to_analyze->health.max,
                              target_entity_to_analyze->stat_block.vigor,
                              target_entity_to_analyze->stat_block.strength,
                              target_entity_to_analyze->stat_block.agility,
                              target_entity_to_analyze->stat_block.speed,
                              target_entity_to_analyze->stat_block.intelligence,
                              target_entity_to_analyze->stat_block.luck);

        return string_from_cstring(r);
    }
}

local void draw_battle_tooltips(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt, f32 bottom_y, bool player_turn) {
    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_WHITE);

    union color32f32 modulation_color = color32f32_WHITE;
    union color32f32 ui_color         = UI_BATTLE_COLOR;

    if (!player_turn) {
        modulation_color = color32f32(0.5, 0.5, 0.5, 0.5);   
        ui_color.a = 0.5;
    }

    string tip = {};

    /* do context dependent text... */
    switch (global_battle_ui_state.submode) {
        case BATTLE_UI_SUBMODE_LOOKING: {
            tip = analyze_entity_and_display_tooltip(&scratch_arena,
                                                     v2f32(state->camera.xy.x - 4, state->camera.xy.y - 4),
                                                     state);
        } break;
        case BATTLE_UI_SUBMODE_USING_ABILITY: {
            struct game_state_combat_state* combat_state            = &game_state->combat_state;
            struct entity*                  active_combatant_entity = game_dereference_entity(game_state, combat_state->participants[combat_state->active_combatant]);
            struct entity_ability* ability = entity_database_ability_find_by_index(&game_state->entity_database,
                                                                                   active_combatant_entity->abilities[global_battle_ui_state.selection].ability);
            tip = ability->description;
        } break;
        case BATTLE_UI_SUBMODE_USING_ITEM: {
            struct battle_ui_item_state* item_use              = &global_battle_ui_state.item_use;
            struct entity_inventory*     inventory             = (struct entity_inventory*)(&game_state->inventory);
            struct item_instance*        current_item_instance = inventory->items + item_use->selectable_items[item_use->selection];
            assertion(current_item_instance && "There should be a valid item");
            struct item_def*             item_base             = item_database_find_by_id(current_item_instance->item);
            assertion(item_base && "There should be a valid item");
            tip = item_base->description;
        } break;
        case BATTLE_UI_SUBMODE_NONE: {
            tip = battle_menu_main_option_descriptions[global_battle_ui_state.selection];
        } break;
    }

    if (tip.length) {
        draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, v2f32(15, bottom_y - (16*4)), 36, 2, ui_color);
        draw_ui_breathing_text(framebuffer, v2f32(30, bottom_y - (16*4) + 15), normal_font, 2, tip, 0, modulation_color);
    }

    if (!player_turn) return;
}

local void battle_ui_banner_fade_text(struct software_framebuffer* framebuffer, struct font_cache* font, f32 dt, string text, f32 text_scale, s32 next_phase, bool inverse) {
    const f32 max_t = 0.45;
    f32 t = global_battle_ui_state.timer / max_t;
    if (t > 1.0) t = 1.0;

    if (inverse) {
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0, 128 * (1.0 - t)), BLEND_MODE_ALPHA);
        software_framebuffer_draw_text_bounds_centered(framebuffer,
                                                       font,
                                                       4,
                                                       rectangle_f32(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
                                                       text,
                                                       color32f32(1,1,1, (1.0 - t)), BLEND_MODE_ALPHA);
    } else {
        software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0, 128), BLEND_MODE_ALPHA);

        f32 x_cursor = lerp_f32(-300, 0, t);
        software_framebuffer_draw_text_bounds_centered(framebuffer,
                                                       font,
                                                       4,
                                                       rectangle_f32(x_cursor, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
                                                       text,
                                                       color32f32(1,1,1,1),
                                                       BLEND_MODE_ALPHA);
    }
    /* while this happens we should also snap all entities to grid points */

    global_battle_ui_state.timer += dt;

    if (global_battle_ui_state.timer >= (max_t+0.85)) {
        global_battle_ui_state.timer = 0;
        global_battle_ui_state.phase = next_phase;
    }
}

local void battle_ui_banner_fade_text_in(struct software_framebuffer* framebuffer, struct font_cache* font, f32 dt, string text, f32 text_scale, s32 next_phase) {
    battle_ui_banner_fade_text(framebuffer, font, dt, text, text_scale, next_phase, false);
}

local void battle_ui_banner_fade_text_out(struct software_framebuffer* framebuffer, struct font_cache* font, f32 dt, string text, f32 text_scale, s32 next_phase) {
    battle_ui_banner_fade_text(framebuffer, font, dt, text, text_scale, next_phase, true);
}

local void battle_ui_trigger_end_turn(void) {
    global_battle_ui_state.phase = BATTLE_UI_FADE_OUT_DETAILS_AFTER_TURN_COMPLETION;
    global_battle_ui_state.timer = 0;
}

local void update_and_render_battle_ui(struct game_state* state, struct software_framebuffer* framebuffer, f32 dt) {
    struct font_cache*              font         = graphics_assets_get_font_by_id(&graphics_assets, menu_fonts[MENU_FONT_COLOR_STEEL]);
    struct game_state_combat_state* combat_state = &state->combat_state;

    /* pixels */
    const f32 BATTLE_SELECTIONS_WIDTH = 16 * 2 * 5;

    switch (global_battle_ui_state.phase) {
        case BATTLE_UI_FADE_IN_DARK_END_TURN:
        case BATTLE_UI_FADE_IN_DARK: {
            const f32 max_t = 0.75;
            f32 t = global_battle_ui_state.timer / max_t;
            if (t > 1.0) t = 1.0;

            software_framebuffer_draw_quad(framebuffer, rectangle_f32(0,0,SCREEN_WIDTH,SCREEN_HEIGHT), color32u8(0,0,0, 128 * t), BLEND_MODE_ALPHA);

            global_battle_ui_state.timer += dt;

            if (global_battle_ui_state.timer >= max_t) {
                global_battle_ui_state.timer = 0;

                if (global_battle_ui_state.phase == BATTLE_UI_FADE_IN_DARK) {
                    global_battle_ui_state.phase = BATTLE_UI_FLASH_IN_BATTLE_TEXT;
                } else if (global_battle_ui_state.phase == BATTLE_UI_FADE_IN_DARK_END_TURN) {
                    global_battle_ui_state.phase = BATTLE_UI_FLASH_IN_END_TURN_TEXT;
                }
            }
        } break;

        case BATTLE_UI_FLASH_IN_BATTLE_TEXT: {
            battle_ui_banner_fade_text_in(
                framebuffer,
                font,
                dt,
                string_literal("ROUND START!"),
                4,
                BATTLE_UI_FADE_OUT
            );
        } break;

        case BATTLE_UI_FADE_OUT: {
            battle_ui_banner_fade_text_out(
                framebuffer,
                font,
                dt,
                string_literal("ROUND START!"),
                4,
                BATTLE_UI_MOVE_IN_DETAILS
            );
        } break;

        case BATTLE_UI_MOVE_IN_DETAILS: {
            /* move in battle UI in a fancy way. */ 

            /* need more thought for this... */
            /* turn panel (left) */
            const f32 max_t = 0.4;
            f32 t = global_battle_ui_state.timer / max_t;

            {
                f32 x_cursor = lerp_f32(-80, 0, t);
                draw_turn_panel(state, framebuffer, 10 + x_cursor, 100);
            }
            {
                f32 x_cursor = lerp_f32(framebuffer->width + BATTLE_SELECTIONS_WIDTH, framebuffer->width - BATTLE_SELECTIONS_WIDTH + 15, t);
                do_battle_selection_menu(state, framebuffer, x_cursor, 100, false, dt);
            }

            if (global_battle_ui_state.timer >= max_t) {
                global_battle_ui_state.timer = 0;
                global_battle_ui_state.phase = BATTLE_UI_IDLE;
            }

            global_battle_ui_state.timer += dt;
        } break;

        case BATTLE_UI_IDLE: {
            bool is_player_turn = is_player_combat_turn(state);

            draw_turn_panel(state, framebuffer, 10, 100);
            do_battle_selection_menu(state, framebuffer, framebuffer->width - BATTLE_SELECTIONS_WIDTH + 15, 100, is_player_turn, dt);
            draw_battle_tooltips(state, framebuffer, dt, framebuffer->height, is_player_turn);

            {
                struct battle_ui_action_message* message = &global_battle_ui_state.message;
                if (message->timer > 0) {
                    struct font_cache* normal_font      = game_get_font(MENU_FONT_COLOR_GOLD);
                    union color32f32 modulation_color = color32f32_WHITE;
                    union color32f32 ui_color         = UI_BATTLE_COLOR;

                    v2f32 ui_box_size     = nine_patch_estimate_extents(ui_chunky, 1, 18, 1);
                    v2f32 ui_box_position = v2f32(framebuffer->width/2 - ui_box_size.x/2, 40);
                    {
                        draw_nine_patch_ui(&graphics_assets, framebuffer, ui_chunky, 1, ui_box_position, 18, 1, ui_color);
                        f32 text_width = font_cache_text_width(normal_font, message->message, 2);
                        draw_ui_breathing_text(framebuffer, v2f32(ui_box_position.x + (ui_box_size.x/2)-text_width/2, ui_box_position.y+10), normal_font, 2, message->message, 0, modulation_color);
                    }
                    message->timer -= dt;
                }
            }
        } break;

        case BATTLE_UI_FLASH_IN_END_TURN_TEXT: {
            battle_ui_banner_fade_text_in(
                framebuffer,
                font,
                dt,
                string_literal("ROUND END!"),
                4,
                BATTLE_UI_FADE_OUT_END_TURN_TEXT
            );
        } break;

        case BATTLE_UI_FADE_OUT_END_TURN_TEXT: {
            battle_ui_banner_fade_text_out(
                framebuffer,
                font,
                dt,
                string_literal("ROUND END!"),
                4,
                BATTLE_UI_FADE_IN_DARK
            );
        } break;

        case BATTLE_UI_FADE_OUT_DETAILS_AFTER_TURN_COMPLETION:
        case BATTLE_UI_FADE_OUT_DETAILS_AFTER_BATTLE_COMPLETION: {
            const f32 linger_t = 0.1;
            const f32 max_t = 0.4;
            f32 t = (global_battle_ui_state.timer-linger_t) / max_t;
            if (t <= 0.0) t = 0.0;
            if (t >= 1.0) t = 1.0;

            draw_turn_panel(state, framebuffer, lerp_f32(10, -300, t), 100);

            do_battle_selection_menu(state, framebuffer, lerp_f32(framebuffer->width - BATTLE_SELECTIONS_WIDTH + 15, framebuffer->width + 300, t), 100, false, dt);
            draw_battle_tooltips(state, framebuffer, dt, lerp_f32(framebuffer->height, framebuffer->height + 300, t), false);

            global_battle_ui_state.timer += dt;

            if (global_battle_ui_state.timer > max_t+linger_t) {
                /* TODO: More robust player death conditions */
                {
                    if (game_total_party_knockout()) {
                        game_setup_death_ui();
                    } else {
                        if (global_battle_ui_state.phase == BATTLE_UI_FADE_OUT_DETAILS_AFTER_TURN_COMPLETION) {
                            global_battle_ui_state.phase = BATTLE_UI_FADE_IN_DARK_END_TURN;
                        } else if (global_battle_ui_state.phase == BATTLE_UI_FADE_OUT_DETAILS_AFTER_BATTLE_COMPLETION) {
                            global_battle_ui_state.phase = BATTLE_UI_AFTER_ACTION_REPORT_IDLE;
                        }
                    }
                }
                global_battle_ui_state.timer = 0;
            }
        } break;

        case BATTLE_UI_AFTER_ACTION_REPORT_IDLE: {
            do_after_action_report_screen(state, framebuffer, dt, 50, true);
        } break;
    }
}

bool _comparison_predicate(s32 sign, s32 a, s32 b) {
    if (sign == 1) {
        return a < b;
    } else if (sign == -1) {
        return a >= b;
    }

    return false;
}

local void render_combat_area_information(struct game_state* state, struct render_commands* commands, struct level_area* area) {
    struct level_area_navigation_map* navigation_map = &area->navigation_data;
    s32                               map_width               = navigation_map->width;
    s32                               map_height              = navigation_map->height;

    for (s32 y_cursor = 0; y_cursor < map_height; ++y_cursor) {
        for (s32 x_cursor = 0; x_cursor < map_width; ++x_cursor) {
            if (area->combat_movement_visibility_map[y_cursor * map_width + x_cursor]) {
                render_commands_push_quad(commands, rectangle_f32((x_cursor + navigation_map->min_x) * TILE_UNIT_SIZE, (y_cursor + navigation_map->min_y) * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE), color32u8(0, 0, 255, 128), BLEND_MODE_ALPHA);
            }
        }
    }

    if (global_battle_ui_state.submode == BATTLE_UI_SUBMODE_MOVING) {
        render_commands_push_quad(commands, rectangle_f32(global_battle_ui_state.movement_end_x * TILE_UNIT_SIZE, global_battle_ui_state.movement_end_y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE), color32u8(255, 0, 0, 128), BLEND_MODE_ALPHA);
    }

    /* draw nav path */
    {
        for (s32 index = 0; index < global_battle_ui_state.max_remembered_path_points_count; ++index) {
            v2f32 point = global_battle_ui_state.max_remembered_path_points[index];
            render_commands_push_quad(commands, rectangle_f32(point.x * TILE_UNIT_SIZE, point.y * TILE_UNIT_SIZE, TILE_UNIT_SIZE, TILE_UNIT_SIZE), color32u8(255, 0, 255, 128), BLEND_MODE_ALPHA);
        }
    }

    if (global_battle_ui_state.submode == BATTLE_UI_SUBMODE_USING_ABILITY) {
        struct entity* user = game_get_player(state);
        struct level_area* area = &state->loaded_area;

        if (global_battle_ui_state.selecting_ability_target) {
            struct entity_ability_slot slot = user->abilities[global_battle_ui_state.usable_abilities[global_battle_ui_state.selection]];
            struct entity_ability*     ability = entity_database_ability_find_by_index(&game_state->entity_database, slot.ability);

            v2s32 grid_xy = get_grid_position_for_ability(ability);
            s32   grid_x  = grid_xy.x;
            s32   grid_y  = grid_xy.y;

            for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
                for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
                    union color32u8 color = color32u8(0, 0, 255, 128);

                    if (ability->selection_type == ABILITY_SELECTION_TYPE_FIELD_SHAPE) {
                        if (global_battle_ui_state.ability_target_x == x && global_battle_ui_state.ability_target_y == y) {
                            color = color32u8(255, 0, 0, 128);
                        }
                    }

                    if (global_battle_ui_state.selection_field[y][x]) {
                        render_commands_push_quad(commands,
                                                  rectangle_f32(grid_x * TILE_UNIT_SIZE + x * TILE_UNIT_SIZE,
                                                                grid_y * TILE_UNIT_SIZE + y * TILE_UNIT_SIZE,
                                                                TILE_UNIT_SIZE, TILE_UNIT_SIZE), color, BLEND_MODE_ALPHA);
                    }
                }
            }
        }
    }
}

local void end_combat_ui(void) {
    if (global_battle_ui_state.phase < BATTLE_UI_FADE_OUT_DETAILS_AFTER_BATTLE_COMPLETION) {
        global_battle_ui_state.timer = 0;
        global_battle_ui_state.phase = BATTLE_UI_FADE_OUT_DETAILS_AFTER_BATTLE_COMPLETION;
        populate_post_battle_loot_table();
    }
}

local bool is_entity_under_ability_selection(entity_id who) {
    for (s32 entity_under_ability_selection_index = 0; entity_under_ability_selection_index < global_battle_ui_state.selected_entities_for_abilities_count; ++entity_under_ability_selection_index) {
        entity_id current_id = global_battle_ui_state.selected_entities_for_abilities[entity_under_ability_selection_index];
        _debug_print_id(current_id);
        if (entity_id_equal(current_id, who)) {
            return true;
        }
    }

    return false;
}
