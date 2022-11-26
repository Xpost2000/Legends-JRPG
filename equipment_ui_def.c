#ifndef EQUIPMENT_UI_DEF_C
#define EQUIPMENT_UI_DEF_C

enum equipment_ui_process_status {
    EQUIPMENT_MENU_PROCESS_ID_EXIT,
    EQUIPMENT_MENU_PROCESS_ID_OKAY,
};

local void open_equipment_screen(entity_id target_id);
s32        do_equipment_menu(struct software_framebuffer* framebuffer, f32 dt);

#endif
