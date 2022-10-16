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

    bool running;
} cutscene_state;

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
