struct file_buffer item_schema_file = {};
local void parse_stat_form(struct item_def* item_definition, struct lisp_form* stat_form, bool modifier_block) {
    struct lisp_form* stat_name  = lisp_list_nth(stat_form, 0);
    struct lisp_form* stat_param = lisp_list_nth(stat_form, 1);
#define Read_Into_Stat(stat)                    \
    do {                                        \
        bool error  = false;                    \
        if (modifier_block) {                   \
            error  |= !lisp_form_get_f32(*stat_param, &item_definition->modifiers.stat); \
        } else {                                \
            error  |= !lisp_form_get_s32(*stat_param, &item_definition->stats.stat); \
        }                                       \
        assertion(!error && "Problem reading a stat form! " # stat); \
    } while (0);

    if (lisp_form_symbol_matching(*stat_name, string_literal("health"))) {
        Read_Into_Stat(health);
    } else if (lisp_form_symbol_matching(*stat_name, string_literal("spell-points"))) {
        Read_Into_Stat(spell_points);
    } else if (lisp_form_symbol_matching(*stat_name, string_literal("vigor"))) {
        Read_Into_Stat(vigor);
    } else if (lisp_form_symbol_matching(*stat_name, string_literal("strength"))) {
        Read_Into_Stat(strength);
    } else if (lisp_form_symbol_matching(*stat_name, string_literal("constitution"))) {
        Read_Into_Stat(constitution);
    } else if (lisp_form_symbol_matching(*stat_name, string_literal("willpower"))) {
        Read_Into_Stat(willpower);
    } else if (lisp_form_symbol_matching(*stat_name, string_literal("agility"))) {
        Read_Into_Stat(agility);
    } else if (lisp_form_symbol_matching(*stat_name, string_literal("speed"))) {
        Read_Into_Stat(speed);
    } else if (lisp_form_symbol_matching(*stat_name, string_literal("intelligence"))) {
        Read_Into_Stat(intelligence);
    } else if (lisp_form_symbol_matching(*stat_name, string_literal("luck"))) {
        Read_Into_Stat(luck);
    }

#undef Read_Into_Stat
}

static void initialize_items_database(void) {
    item_schema_file                  = read_entire_file(memory_arena_allocator(&game_arena), string_literal("./res/item_data.txt"));
    struct lisp_list item_schema_list = lisp_read_string_into_forms(&game_arena, file_buffer_as_string(&item_schema_file));

    item_database_count = item_schema_list.count;
    item_database       = memory_arena_push(&game_arena, item_database_count * sizeof(*item_database));

    for (s32 item_form_index = 0; item_form_index < item_database_count; ++item_form_index) {
        struct lisp_form* item_form = item_schema_list.forms + item_form_index;

        struct lisp_form* id_name_form         = lisp_list_nth(item_form, 0);
        struct lisp_form* name_form            = lisp_list_nth(item_form, 1);
        struct lisp_form* description_form     = lisp_list_nth(item_form, 2);
        struct lisp_form  remaining_parameters = lisp_list_sliced(*item_form, 3, -1);

        struct item_def* current_item_definition = &item_database[item_form_index];

        /* initialize to unit */
        {
            current_item_definition->modifiers.health       = 1;
            current_item_definition->modifiers.spell_points = 1;
            for (s32 stat_index = 0; stat_index < STAT_COUNT; ++stat_index) {
                current_item_definition->modifiers.values[stat_index] = 1;
            }
        }

        assertion(lisp_form_get_string(*id_name_form,     &current_item_definition->id_name)     && "bad id name form!");
        assertion(lisp_form_get_string(*name_form,        &current_item_definition->name)        && "bad name form!");
        assertion(lisp_form_get_string(*description_form, &current_item_definition->description) && "bad description form!");

        for (s32 remaining_parameter_index = 0; remaining_parameter_index < remaining_parameters.list.count; ++remaining_parameter_index) {
            struct lisp_form* parameter_form = lisp_list_nth(&remaining_parameters, remaining_parameter_index);
            {
                struct lisp_form* parameter_name      = lisp_list_nth(parameter_form, 0);
                struct lisp_form  parameter_arguments = lisp_list_sliced(*parameter_form, 1, -1);

                if (lisp_form_symbol_matching(*parameter_name, string_literal("stat-block"))) {
                    for (s32 stat_index = 0; stat_index < parameter_arguments.list.count; ++stat_index) {
                        struct lisp_form* stat_form = lisp_list_nth(&parameter_arguments, stat_index);
                        parse_stat_form(current_item_definition, stat_form, false);
                    }
                } else if (lisp_form_symbol_matching(*parameter_name, string_literal("modifier-stat-block"))) {
                    for (s32 stat_index = 0; stat_index < parameter_arguments.list.count; ++stat_index) {
                        struct lisp_form* stat_form = lisp_list_nth(&parameter_arguments, stat_index);
                        parse_stat_form(current_item_definition, stat_form, true);
                    }
                } else if (lisp_form_symbol_matching(*parameter_name, string_literal("type"))) {
                    struct lisp_form* type_param = lisp_list_nth(&parameter_arguments, 0);

                    if (lisp_form_symbol_matching(*type_param, string_literal("misc"))) {
                        current_item_definition->type = ITEM_TYPE_MISC;
                    } else if (lisp_form_symbol_matching(*type_param, string_literal("consumable"))) {
                        current_item_definition->type = ITEM_TYPE_CONSUMABLE_ITEM;
                    } else if (lisp_form_symbol_matching(*type_param, string_literal("weapon"))) {
                        current_item_definition->type = ITEM_TYPE_WEAPON;
                    } else if (lisp_form_symbol_matching(*type_param, string_literal("equipment"))) {
                        current_item_definition->type = ITEM_TYPE_EQUIPMENT;
                    }
                } else if (lisp_form_symbol_matching(*parameter_name, string_literal("value"))) {
                    struct lisp_form* value = lisp_list_nth(&parameter_arguments, 0);
                    assertion(lisp_form_get_s32(*value, &current_item_definition->gold_value) && "bad gold value form");
                } else if (lisp_form_symbol_matching(*parameter_name, string_literal("max-stack"))) {
                    struct lisp_form* value = lisp_list_nth(&parameter_arguments, 0);
                    assertion(lisp_form_get_s32(*value, &current_item_definition->max_stack_value) && "bad stack form");
                } else if (lisp_form_symbol_matching(*parameter_name, string_literal("equip-slot"))) {
                    struct lisp_form* slot_param = lisp_list_nth(&parameter_arguments, 0);

                    if (lisp_form_symbol_matching(*slot_param, string_literal("head"))) {
                        current_item_definition->equipment_slot_flags = EQUIPMENT_SLOT_FLAG_HEAD;
                    } else if (lisp_form_symbol_matching(*slot_param, string_literal("body"))) {
                        current_item_definition->equipment_slot_flags = EQUIPMENT_SLOT_FLAG_BODY;
                    } else if (lisp_form_symbol_matching(*slot_param, string_literal("hands"))) {
                        current_item_definition->equipment_slot_flags = EQUIPMENT_SLOT_FLAG_HANDS;
                    } else if (lisp_form_symbol_matching(*slot_param, string_literal("legs"))) {
                        current_item_definition->equipment_slot_flags = EQUIPMENT_SLOT_FLAG_LEGS;
                    } else if (lisp_form_symbol_matching(*slot_param, string_literal("misc"))) {
                        current_item_definition->equipment_slot_flags = EQUIPMENT_SLOT_FLAG_MISC;
                    } else if (lisp_form_symbol_matching(*slot_param, string_literal("weapon"))) {
                        current_item_definition->equipment_slot_flags = EQUIPMENT_SLOT_FLAG_WEAPON;
                    }
                }
            }
        }
    }
}
