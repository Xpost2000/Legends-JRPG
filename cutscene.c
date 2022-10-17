#include "cutscene_def.c"

/*
  A real complicated situation is that cutscenes need to be able to continuously stack different level areas.

  Then transition between them through fadeouts and such. Which is pretty brutal to implement...
*/

struct {
    struct memory_arena arena;
    struct file_buffer  file_buffer;
    struct lisp_form    script_forms;
    /*
     * pre cutscene state here later, we need to copy most
     * entities here to see if we should restore them later?
     */

    /* NOTE: Separate copy! */
    struct level_area loaded_area;
    bool              level_area_on_enter_triggered;
    bool              viewing_loaded_area;

    bool running;
} cutscene_state;

void cutscene_load_area(string path) {
    if (cutscene_state.viewing_loaded_area) {
        /* reload with a different area. */
        unimplemented("reloading area is not done yet");
    } else {
        string fullpath = string_concatenate(&scratch_arena, string_literal("areas/"), path);
        struct binary_serializer serializer = open_read_file_serializer(fullpath);
        {
            cutscene_state.viewing_loaded_area = true;
            _serialize_level_area(&cutscene_state.arena, &serializer, &cutscene_state.loaded_area, ENTITY_LIST_STORAGE_TYPE_PER_LEVEL_CUTSCENE);
            load_area_script(&cutscene_state.arena, &cutscene_state.loaded_area, string_concatenate(&scratch_arena, string_slice(fullpath, 0, (fullpath.length+1)-sizeof("area")), string_literal("area_script")));
        }
        serializer_finish(&serializer);
    }
}

void cutscene_unload_area(void) {
    cutscene_state.viewing_loaded_area = false;
    memory_arena_clear_top(&cutscene_state.arena);
}

void cutscene_initialize(struct memory_arena* arena) {
    cutscene_state.arena = memory_arena_push_sub_arena(arena, Megabyte(4));
}

void cutscene_open(string filepath) {
    /* kill any movement, just in-case it was happening earlier. */
    struct entity* player            = game_get_player(game_state);
    player->velocity.x               = 0; player->velocity.y = 0;
    cutscene_state.file_buffer       = read_entire_file(memory_arena_allocator(&cutscene_state.arena), format_temp_s("./scenes/%.*s.txt", filepath.length, filepath.data));
    cutscene_state.script_forms.list = lisp_read_string_into_forms(&cutscene_state.arena, file_buffer_as_string(&cutscene_state.file_buffer));
    cutscene_state.script_forms.type = LISP_FORM_LIST;

    game_script_clear_all_awaited_scripts();
    game_script_enqueue_form_to_execute(cutscene_state.script_forms);
    cutscene_state.running = true;
}

/* same as awaiting scripts but for only one script instance */
void cutscene_update(f32 dt) {
    /* Most of the cutscene state is actually handled by game script so this is empty. */

    /*
      However this is to update cutscene specific constructs such as the new story board or any cutscene
      sprite objects.

      Most other stuff is just handled by the game alone.
    */
    return;
}

bool cutscene_active(void) {
    return cutscene_state.running;
}

void cutscene_stop(void) {
    _debugprintf("Bye bye cutscene");
    cutscene_state.running = false;
    memory_arena_clear_top(&cutscene_state.arena);
    memory_arena_clear_bottom(&cutscene_state.arena);
}
