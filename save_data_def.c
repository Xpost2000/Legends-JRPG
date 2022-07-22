#ifndef SAVE_DATA_DEF_C
#define SAVE_DATA_DEF_C

void initialize_save_data(void);
void apply_save_data(struct game_state* state);

void begin_save_entry(u32 hash);
void end_save_entry(u32 hash);

#endif
