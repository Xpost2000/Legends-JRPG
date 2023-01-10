#include "xp_tables_def.c"

struct xp_table xp_table_empty(void) {
    struct xp_table result = {};
    return result;
}

struct stat_growth_table stat_table_for(string base_id) {
    struct stat_growth_table result = {};
    copy_string_into_cstring(base_id, result.entity_base_id, array_count(result.entity_base_id));
    return result;
}

struct stat_growth_table_list {
    struct memory_arena*      arena;
    s32                       count;
    struct stat_growth_table* tables;
};

struct stat_growth_table_list stat_growth_table_list(struct memory_arena* arena) {
    struct stat_growth_table_list result = {};
    result.arena  = arena;
    result.tables = memory_arena_push(arena, 0);
    return result;
}

struct stat_growth_table* stat_growth_table_list_find_table(struct stat_growth_table_list* list, string base_id) {
    for (s32 table_index = 0; table_index < list->count; ++table_index) {
        struct stat_growth_table* current_table = list->tables + table_index;

        if (string_equal(string_from_cstring(current_table->entity_base_id), base_id)) {
            return current_table;
        }
    }

    return NULL;
}

struct stat_growth_table* stat_growth_table_list_add_table(struct stat_growth_table_list* list, string base_id) {
    struct stat_growth_table* result = stat_growth_table_list_find_table(list, base_id);
    memory_arena_push(list->arena, sizeof(*list->tables));

    if (!result) {
        _debugprintf("Introduced stat growth table for: %.*s", base_id.length, base_id.data);
        result = &list->tables[list->count++];
        *result = stat_table_for(base_id);
    }

    return result;
}

local struct stat_growth_table_list* global_stat_growth_tables = 0;
local struct xp_table*               global_game_xp_table      = 0;

struct xp_table* game_experience_table(void) {
    return global_game_xp_table;
}

struct stat_growth_table* find_progression_table_for(string base_id) {
    struct stat_growth_table* result   = stat_growth_table_list_find_table(global_stat_growth_tables, base_id);
    return result;
}

local void parse_experience_table(struct lisp_form* form) {
    assertion(form->type == LISP_FORM_LIST && "This should be impossible to hit.");

    for (s32 experience_value_index = 1; experience_value_index < form->list.count; ++experience_value_index) {
        if (global_game_xp_table->count < MAX_POSSIBLE_LEVELS_IN_ENGINE) {
            struct lisp_form* experience_value_form = lisp_list_nth(form, experience_value_index);

            s32 experience_value = -1;
            assertion(lisp_form_get_s32(*experience_value_form, &experience_value) && "Need to experience value be a valid integer type!");
            global_game_xp_table->levels[global_game_xp_table->count++ + 1].xp = experience_value;
        }
    }

    _debugprintf("Max level for this XP table is: %d", global_game_xp_table->count);
}

local void parse_stat_growth_table(struct lisp_form* form) {
    assertion(form->type == LISP_FORM_LIST && "This should be impossible to hit.");
    struct lisp_form* entity_base_id_form = lisp_list_nth(form, 1);

    string entity_base_id = {};
    assertion(lisp_form_get_string(*entity_base_id_form, &entity_base_id) && "entity base id must be a string");

    struct stat_growth_table* table = stat_growth_table_list_add_table(global_stat_growth_tables, entity_base_id);

    for (s32 growth_table_index = 2; growth_table_index < form->list.count; ++growth_table_index) {
        struct lisp_form* growth_data_form = lisp_list_nth(form, growth_table_index);

        struct stat_growth_table_entry* current_level_stats = &table->growths[growth_table_index-1];

        {
            if (growth_data_form->type == LISP_FORM_LIST) {
                for (s32 stat_entry_index = 0; stat_entry_index < growth_data_form->list.count; ++stat_entry_index) {
                    struct lisp_form* stat_entry_form = lisp_list_nth(growth_data_form, stat_entry_index);

                    {
                        struct lisp_form* stat_entry_header = lisp_list_nth(stat_entry_form, 0);
                        struct lisp_form* stat_entry_value  = lisp_list_nth(stat_entry_form, 1);

                        s32 value = -1;
                        assertion(lisp_form_get_s32(*stat_entry_value, &value) && "stat entry form must have an integer second parameter");

                        if (lisp_form_symbol_matching(*stat_entry_header, string_literal("health"))) {
                            current_level_stats->health = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("magic"))) {
                            current_level_stats->magic = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("vigor"))) {
                            current_level_stats->vigor = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("strength"))) {
                            current_level_stats->strength = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("constitution"))) {
                            current_level_stats->constitution = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("willpower"))) {
                            current_level_stats->willpower = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("agility"))) {
                            current_level_stats->agility = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("speed"))) {
                            current_level_stats->speed = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("intelligence"))) {
                            current_level_stats->intelligence = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("luck"))) {
                            current_level_stats->luck = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("counter"))) {
                            current_level_stats->counter = value;
                        } else if (lisp_form_symbol_matching(*stat_entry_header, string_literal("ability"))) {
                            unimplemented("no support for new abilities yet.");
                        }
                    }
                }
            }
        }
    }
}

void initialize_progression_tables(struct memory_arena* arena) {
    global_game_xp_table               = memory_arena_push(arena, sizeof(*global_game_xp_table));
    global_stat_growth_tables          = memory_arena_push(arena, sizeof(*global_stat_growth_tables));
    
    struct lisp_list progression_forms = lisp_read_entire_file_into_forms(&scratch_arena, string_literal(GAME_DEFAULT_XP_PROGRESSION_FILE));

    bool already_found_experience_table = false;
    {
        for (s32 form_index = 0; form_index < progression_forms.count; ++form_index) {
            struct lisp_form* current_form = progression_forms.forms + form_index;
            assertion(current_form->type == LISP_FORM_LIST && "There should only be list forms in this file!");

            struct lisp_form* form_header = lisp_list_nth(current_form, 0);

            if (lisp_form_symbol_matching(*form_header, string_literal("xp-table"))) {
                if (!already_found_experience_table) {
                    _debugprintf("Found experience table to form...");
                    parse_experience_table(current_form);
                    already_found_experience_table = true;
                } else {
                    _debugprintf("warning: found multiple experience tables. Only using the first found one.");
                }
            } else if (lisp_form_symbol_matching(*form_header, string_literal("growth-table"))) {
                parse_stat_growth_table(current_form);
            }
        }
    }
}
