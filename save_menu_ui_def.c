#ifndef SAVE_MENU_UI_DEF_C
#define SAVE_MENU_UI_DEF_C

#define GAME_MAX_SAVE_SLOTS (16)
#define SAVE_SLOT_WIDGET_SAVE_NAME_LENGTH  (32)
#define SAVE_SLOT_WIDGET_DESCRIPTOR_LENGTH (64)

enum save_menu_process_id {
    SAVE_MENU_PROCESS_ID_EXIT,
    SAVE_MENU_PROCESS_ID_LOADED_EXIT,
    SAVE_MENU_PROCESS_ID_SAVED_EXIT,
    SAVE_MENU_PROCESS_ID_OKAY,
};

void save_menu_open_for_loading(void);
void save_menu_open_for_saving(void);
void save_menu_close(void);
void save_menu_off(void);
s32  do_save_menu(struct software_framebuffer* framebuffer, f32 dt);

#endif
