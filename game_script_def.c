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
  
  NOTE:
  How do I consider waiting?

  I could push all script actions on to a stack and basically
  have this be like an implied event system...
  
  Who knows?
  
  For now just evaluate on-line instead of deferring or anything.
*/

struct lisp_form game_script_evaluate_form(struct memory_arena* arena, struct game_state* state, struct lisp_form* form);

#endif
