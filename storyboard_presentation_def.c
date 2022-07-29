#ifndef STORYBOARD_PRESENTATION_DEF_C
#define STORYBOARD_PRESENTATION_DEF_C

local bool storyboard_active = false;
#define STORYBOARD_READ_SPEED (0.15f)

local void storyboard_next_page(void);
local void start_storyboard(void);
local s32 game_display_and_update_storyboard(struct software_framebuffer* framebuffer, f32 dt);

#endif
