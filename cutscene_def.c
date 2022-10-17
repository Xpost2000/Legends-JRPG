/*
  The question is really... How extensible do I need this to be?

  For the record it's just running game script sequentially with a few extensions such as a
  parallel block (to allow for moving multiple entities at the same time. However game-script even for a lisp
  dialect isn't particularly powerful as it does not support loops, and this is okay since all game scenes are meant to be carefully
  constructed.)

  So again there will be a slight variance on the eval system as cutscenes shall also support the "wait" command.

  This might encompass the storyboard presentation system.

  Also this is another "module" style file with it's own global state... Fun!

  Cutscenes also need to reserve the ability to create sprite objects on demand if it should want to.
*/
#ifndef CUTSCENE_DEF_C
#define CUTSCENE_DEF_C

void cutscene_initialize(struct memory_arena* arena);
void cutscene_open(string filepath);
void cutscene_update(f32 dt);
bool cutscene_active(void);
void cutscene_stop(void);
void cutscene_load_area(string path);
void cutscene_unload_area(void);

struct entity_iterator game_cutscene_entity_iterator(void);

#endif
