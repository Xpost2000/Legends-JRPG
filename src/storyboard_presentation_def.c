#ifndef STORYBOARD_PRESENTATION_DEF_C
#define STORYBOARD_PRESENTATION_DEF_C

/*
  NOTE: story boards do not use rich text.

  It puts too many uncertainities in the format.

  Unfortunately this means this is highly specific but whatever.

  All storyboards must fade!.
 */

local bool storyboard_active = false;
#define STORYBOARD_READ_SPEED (0.04f)

void storyboard_next_page(void);
void start_storyboard(void); 
void storyboard_reserve_pages(s32 count);
void initialize_storyboard(struct memory_arena* arena);
void load_storyboard_page(struct lisp_form* form);
local s32 game_display_and_update_storyboard(struct software_framebuffer* framebuffer, f32 dt);

#endif
