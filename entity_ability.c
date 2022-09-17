/*
  Later we can implement our animation code here probably.

  An attack might count as an ability so who knows.
*/

s32 facing_direction_to_quadrant(s32 facing_dir) {
    switch (facing_dir) {
        case DIRECTION_LEFT : return 1;
        case DIRECTION_UP   : return 0;
        case DIRECTION_RIGHT: return 3;
        case DIRECTION_DOWN : return 2;
    }

    return 0;
}

void copy_selection_field_rotated_as(struct entity_ability* ability, u8* field_copy, s32 quadrant) {
    quadrant %= 4;

    for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
        for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
            field_copy[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x] = ability->selection_field[y][x];
        }
    }

    /* Only field shapes, and planted fields can rotate. This should be semi intuitive? */
    if (ability->selection_type == ABILITY_SELECTION_TYPE_FIELD && !ability->moving_field) {
        u8* temporary_copy = memory_arena_push(&scratch_arena, sizeof(u8) * ENTITY_ABILITY_SELECTION_FIELD_MAX_Y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X);

        for (s32 rotations = 0; rotations < quadrant; ++rotations) {
            /* copy into temporary */
            {
                for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
                    for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
                        temporary_copy[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x] = field_copy[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x];
                    }
                }
            }
            /* -y rotation */
            {
                for (s32 y = 0; y < ENTITY_ABILITY_SELECTION_FIELD_MAX_Y; ++y) {
                    for (s32 x = 0; x < ENTITY_ABILITY_SELECTION_FIELD_MAX_X; ++x) {
                        s32 read_x = -y + (ENTITY_ABILITY_SELECTION_FIELD_MAX_X-1);
                        s32 read_y = x;

                        field_copy[y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + x] = temporary_copy[read_y * ENTITY_ABILITY_SELECTION_FIELD_MAX_X + read_x];
                    }
                }
            }
        }
    }
}

local void decode_sequence_action_target_entity(struct lisp_form* focus_target_form, struct sequence_action_target_entity* entity_target) {
    bool error = false;
    if (lisp_form_symbol_matching(*focus_target_form, string_literal("user"))) {
        entity_target->entity_target_type = ENTITY_TARGET_ID_USER;
    } else if (lisp_form_symbol_matching(*focus_target_form, string_literal("target"))) {
        entity_target->entity_target_type  = ENTITY_TARGET_ID_TARGET;
        entity_target->entity_target_index = 0;
    } else if (focus_target_form->type == LISP_FORM_LIST) {
        struct lisp_form* first_arg = lisp_list_nth(focus_target_form, 0);
        if (lisp_form_symbol_matching(*first_arg, string_literal("target"))) {
            struct lisp_form* second_arg = lisp_list_nth(focus_target_form, 1);
            s32 target_index = 0;
            assertion(lisp_form_get_s32(*second_arg, &target_index) && "???? bad number??");
            entity_target->entity_target_index = target_index;
        }
    } else {
        error = true;
    }

    if (error) {
        _debugprintf("Hmmm, not sure what the target type is");
        _debug_print_out_lisp_code_(focus_target_form);
        _debugprintf("bye");
        assertion(!"Do not recognize this argument... Check the console");
    }
}

void entity_ability_compile_animation_sequence(struct memory_arena* arena, struct entity_ability* ability, void* animation_sequence_list) {
    struct lisp_form* animation_sequence_list_form = (struct lisp_form*)animation_sequence_list;

    if (animation_sequence_list_form->type == LISP_FORM_LIST) {
        struct entity_ability_sequence* sequence     = &ability->sequence;
        s32                             action_count = ((struct lisp_form*)(animation_sequence_list))->list.count;

        sequence->sequence_action_count = action_count;
        sequence->sequence_actions      = memory_arena_push(arena, action_count * sizeof(*sequence->sequence_actions));

        if (action_count == 0) {
            _debugprintf("No anim sequence... This might be a problem?");
        }

        for (s32 action_form_index = 0; action_form_index < action_count; ++action_form_index) {
            struct entity_ability_sequence_action* action_data         = sequence->sequence_actions + action_form_index;
            struct lisp_form*                      current_action_form = lisp_list_nth(animation_sequence_list, action_form_index);

            {
                struct lisp_form* action_form_header         = lisp_list_nth(current_action_form, 0); 
                struct lisp_form  action_form_rest_arguments = lisp_list_sliced(*current_action_form, 1, -1);

                assertion(action_form_header && "This is probably in the wrong form!");

                bool successfully_parsed = true;
                {
                    if (lisp_form_symbol_matching(*action_form_header, string_literal("focus-camera"))) {
                        action_data->type = SEQUENCE_ACTION_FOCUS_CAMERA;

                        struct lisp_form* focus_target = lisp_list_nth(&action_form_rest_arguments, 0);
                        decode_sequence_action_target_entity(focus_target, &action_data->focus_camera.target);
                    } else if (lisp_form_symbol_matching(*action_form_header, string_literal("move-to"))) {
                        struct sequence_action_move_to* move_to = &action_data->move_to;

                        action_data->type = SEQUENCE_ACTION_MOVE_TO;
                        struct lisp_form* to_move_target             = lisp_list_nth(&action_form_rest_arguments, 0);
                        struct lisp_form* move_target                = lisp_list_nth(&action_form_rest_arguments, 1);
                        struct lisp_form* interpolation_type         = lisp_list_nth(&action_form_rest_arguments, 2);
                        struct lisp_form* desired_velocity_magnitude = lisp_list_nth(&action_form_rest_arguments, 3);

                        decode_sequence_action_target_entity(to_move_target, &move_to->to_move);
                        if (move_target->type == LISP_FORM_LIST) {
                            struct lisp_form* first_arg = lisp_list_nth(move_target, 0);
                            move_to->move_target_type = MOVE_TARGET_ENTITY;
                            if (lisp_form_symbol_matching(*first_arg, string_literal("past"))) {
                                struct lisp_form* actual_move_target  = lisp_list_nth(move_target, 1);
                                struct lisp_form* by_symbol           = lisp_list_nth(move_target, 2);
                                struct lisp_form* amount_to_move_past = lisp_list_nth(move_target, 3);

                                decode_sequence_action_target_entity(actual_move_target, &move_to->move_target.entity.target);

                                s32 move_amount = 0;
                                lisp_form_get_s32(*amount_to_move_past, &move_amount);
                                move_to->move_target.entity.move_past = move_amount;
                            } else if (lisp_form_symbol_matching(*first_arg, string_literal("direction-relative-to"))) {
                                /* a bit weird to call. Need to think of more useful ones. */
                                move_to->move_target_type = MOVE_TARGET_RELATIVE_DIRECTION_TO;

                                struct lisp_form* actual_move_target  = lisp_list_nth(move_target, 1);
                                struct lisp_form* by_symbol           = lisp_list_nth(move_target, 2);
                                struct lisp_form* amount_to_move_past = lisp_list_nth(move_target, 3);

                                decode_sequence_action_target_entity(actual_move_target, &move_to->move_target.entity.target);

                                s32 move_amount = 0;
                                lisp_form_get_s32(*amount_to_move_past, &move_amount);
                                move_to->move_target.entity.move_past = move_amount;
                            } else {
                                decode_sequence_action_target_entity(move_target, &move_to->move_target.entity.target);
                            }
                        } else {
                            if (lisp_form_symbol_matching(*move_target, string_literal("start"))) {
                                move_to->move_target_type = MOVE_TARGET_START;
                            } else {
                                move_to->move_target_type = MOVE_TARGET_ENTITY;
                                decode_sequence_action_target_entity(move_target, &move_to->move_target.entity.target);
                            }
                        }

                        {
                            if (lisp_form_symbol_matching(*interpolation_type, string_literal("linear-ease"))) {
                                move_to->interpolation_type = SEQUENCE_LINEAR;
                            } else if (lisp_form_symbol_matching(*interpolation_type, string_literal("cubic-ease-in"))) {
                                move_to->interpolation_type = SEQUENCE_CUBIC_EASE_IN;
                            } else if (lisp_form_symbol_matching(*interpolation_type, string_literal("cubic-ease-out"))) {
                                move_to->interpolation_type = SEQUENCE_CUBIC_EASE_OUT;
                            } else if (lisp_form_symbol_matching(*interpolation_type, string_literal("cube-ease-in-out"))) {
                                move_to->interpolation_type = SEQUENCE_CUBIC_EASE_IN_OUT;
                            } else if (lisp_form_symbol_matching(*interpolation_type, string_literal("quadratic-ease-in"))) {
                                move_to->interpolation_type = SEQUENCE_QUADRATIC_EASE_IN;
                            } else if (lisp_form_symbol_matching(*interpolation_type, string_literal("quadratic-ease-out"))) {
                                move_to->interpolation_type = SEQUENCE_QUADRATIC_EASE_OUT;
                            } else if (lisp_form_symbol_matching(*interpolation_type, string_literal("quadratic-ease-in-out"))) {
                                move_to->interpolation_type = SEQUENCE_QUADRATIC_EASE_IN_OUT;
                            } else if (lisp_form_symbol_matching(*interpolation_type, string_literal("back-ease-in"))) {
                                move_to->interpolation_type = SEQUENCE_BACK_EASE_IN;
                            } else if (lisp_form_symbol_matching(*interpolation_type, string_literal("back-ease-out"))) {
                                move_to->interpolation_type = SEQUENCE_BACK_EASE_OUT;
                            } else if (lisp_form_symbol_matching(*interpolation_type, string_literal("back-ease-in-out"))) {
                                move_to->interpolation_type = SEQUENCE_BACK_EASE_IN_OUT;
                            }
                        }
                        {
                            f32 desired_velocity = 1; 
                            lisp_form_get_f32(*desired_velocity_magnitude, &desired_velocity);

                            move_to->desired_velocity_magnitude = desired_velocity;
                        }
                    } else if (lisp_form_symbol_matching(*action_form_header, string_literal("hurt"))) {
                        action_data->type = SEQUENCE_ACTION_HURT;
                        
                        struct lisp_form* focus_target = lisp_list_nth(&action_form_rest_arguments, 0);
                        decode_sequence_action_target_entity(focus_target, &action_data->hurt.target);
                    } else if (lisp_form_symbol_matching(*action_form_header, string_literal("start-special-effects"))) {
                        action_data->type = SEQUENCE_ACTION_START_SPECIAL_FX;
                        struct lisp_form* effect_id = lisp_list_nth(&action_form_rest_arguments, 0);
                        lisp_form_get_s32(*effect_id, &action_data->special_fx.effect_id);
                    } else if (lisp_form_symbol_matching(*action_form_header, string_literal("stop-special-effects"))) {
                        action_data->type = SEQUENCE_ACTION_STOP_SPECIAL_FX;
                    } else if (lisp_form_symbol_matching(*action_form_header, string_literal("wait-special-effects"))) {
                        action_data->type = SEQUENCE_ACTION_WAIT_SPECIAL_FX_TO_FINISH;
                    } else {
                        successfully_parsed = false;
                    } 
                }
                _debugprintf("successful parse flag: %d", successfully_parsed);

                if (!successfully_parsed) {
                    _debugprintf("Do not recognize header: \"%.*s\"", action_form_header->string.length, action_form_header->string.data);
                    assertion(successfully_parsed && "Could not parse argument in sequence! Crashing. Please fix!");
                }
            }
        }

        _debugprintf("finished reading for %.*s", ability->name.length, ability->name.data);
        _debugprintf("ptr: %p, (count %d)", sequence, sequence->sequence_action_count);
    } else {
        /* nil form likely */
    }
}
