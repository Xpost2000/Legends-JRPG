#ifndef SAVE_DATA_DEF_C
#define SAVE_DATA_DEF_C

void initialize_save_data(void);
void apply_save_data(struct game_state* state);

/* just a bunch of save entry types we can write down */

void finish_save_data(void);

string filename_from_saveslot_id(s32 id);
void   save_data_register_chest_looted(u32 chest_id);

#endif
