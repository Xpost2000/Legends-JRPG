#ifndef GAME_SCRIPT_DEF_C
#define GAME_SCRIPT_DEF_C

/*
  The game script writes and reads from game_state,
  so.
  
  Yay!
  
  This thing should only really be evaluating functions
  and even then it's supposed to be declarative...

  So it's not a real lisp. It only looks like a lisp
  since it's a very easy and versatile format to read.
*/

struct game_script_typed_ptr {
    s32   type;
    union {
        void* ptr;
        /* entities cannot be references with a generic pointer technically */
        entity_id entity_id;
    };
};

bool game_script_object_handle_matches_object(struct lisp_form object_handle, s32 type, s32 id);
struct game_script_typed_ptr game_script_object_handle_decode(struct lisp_form object_handle);
struct lisp_form game_script_evaluate_form(struct memory_arena* arena, struct game_state* state, struct lisp_form* form);

void game_script_enqueue_form_to_execute(struct lisp_form f);
void game_script_execute_awaiting_scripts(struct memory_arena* arena, struct game_state* state, f32 dt);

/* 
   NOTE:
   The timers we have for the game otherwise, are done manually,
   the script timers are solely used for waiting purposes, and do
   not trigger anything.
*/
void game_script_run_all_timers(f32 dt);

void game_script_initialize(struct memory_arena* arena);
void game_script_deinitialize(void);

#endif
