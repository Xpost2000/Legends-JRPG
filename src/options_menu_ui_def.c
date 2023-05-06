#ifndef OPTIONS_MENU_DEF_C
#define OPTIONS_MENU_DEF_C

enum options_menu_process_id {
    OPTIONS_MENU_PROCESS_ID_EXIT,
    OPTIONS_MENU_PROCESS_ID_OKAY,
};

void options_menu_open(void);
void options_menu_close(void);
s32  do_options_menu(struct software_framebuffer* framebuffer, f32 dt);

#endif
