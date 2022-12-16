#ifndef SAVE_DATA_DEF_C
#define SAVE_DATA_DEF_C

#define CURRENT_SAVE_RECORD_VERSION (1)

void initialize_save_data(void);
void apply_save_data(struct game_state* state);

/* just a bunch of save entry types we can write down */

void finish_save_data(void);

string filename_from_saveslot_id(s32 id);
void   save_data_register_chest(u32 chest_id);
void   save_data_register_entity(entity_id id);
void   save_data_register_light(u32 savepoint_id);
void   save_data_register_savepoint(u32 savepoint_id);

struct save_data_description {
    bool good;
    char name[32];
    char descriptor[64];
    s64  timestamp;
};

struct save_data_description get_save_data_description(s32 save_id);

#endif
